#include "ui/MainWindow.h"

#include "core/eventbus/Events.h"
#include "infrastructure/MeshLoader.h"
#include "ui/ChatPanel.h"
#include "ui/InferenceWorker.h"

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPolyData.h>
#include <vtkRenderWindow.h>

#include <nlohmann/json.hpp>

#include <QFileDialog>
#include <QFileInfo>
#include <QSplitter>
#include <QThread>

namespace semanticad::ui {

MainWindow::MainWindow(QString modelPath, QString commandsDir, QWidget* parent)
    : QMainWindow(parent), bus_(), viewer_(bus_) {
    setWindowTitle(QStringLiteral("SemantiCAD"));
    resize(1280, 800);

    // --- 70 / 30 horizontal split: VTK viewport | chat panel ---------------
    vtkWidget_ = new QVTKOpenGLNativeWidget;
    vtkWidget_->setMinimumWidth(420);
    chat_ = new ChatPanel;
    chat_->setMinimumWidth(340);

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(vtkWidget_);
    splitter->addWidget(chat_);
    splitter->setStretchFactor(0, 7);  // viewport grows 70%
    splitter->setStretchFactor(1, 3);  // chat grows 30%
    splitter->setSizes({static_cast<int>(width() * 0.7), static_cast<int>(width() * 0.3)});
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(1);
    setCentralWidget(splitter);

    // --- Scene: embed the viewer's renderer in the widget's render window ---
    viewer_.loadDefaultCube();
    viewer_.setRenderWindow(vtkWidget_->renderWindow());
    style_ = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    if (vtkWidget_->interactor())
        vtkWidget_->interactor()->SetInteractorStyle(style_);

    // --- NLP worker on its own thread (model load + generation are slow) ----
    worker_ = new InferenceWorker(modelPath, commandsDir);
    workerThread_ = new QThread(this);
    worker_->moveToThread(workerThread_);
    connect(workerThread_, &QThread::started, worker_, &InferenceWorker::initialize);
    connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(worker_, &InferenceWorker::statusChanged, this, &MainWindow::onWorkerStatus);
    connect(worker_, &InferenceWorker::resultReady, this, &MainWindow::onWorkerResult);
    connect(this, &MainWindow::requestProcess, worker_, &InferenceWorker::process);

    connect(chat_, &ChatPanel::messageSubmitted, this, &MainWindow::onUserMessage);
    connect(chat_, &ChatPanel::openFileRequested, this, &MainWindow::onOpenFileRequested);

    chat_->setInputEnabled(false);
    chat_->setStatus(QStringLiteral("Başlatılıyor…"), ChatPanel::Status::Loading);

    workerThread_->start();
}

MainWindow::~MainWindow() {
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait();
    }
}

void MainWindow::openFile(const QString& path) {
    if (path.isEmpty()) return;
    std::string err;
    auto mesh = infra::loadMesh(path.toStdString(), err);
    if (!mesh) {
        chat_->addSystemMessage(
            QStringLiteral("Dosya açılamadı: %1").arg(QString::fromStdString(err)));
        return;
    }
    viewer_.loadPolyData(mesh);  // re-frames the camera and renders
    currentFile_ = path;
    const QString name = QFileInfo(path).fileName();
    setWindowTitle(QStringLiteral("SemantiCAD — %1").arg(name));
    chat_->addSystemMessage(QStringLiteral("Yüklendi: %1").arg(name));
}

void MainWindow::onOpenFileRequested() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("3B model aç"), currentFile_,
        QStringLiteral("3B Modeller (*.stl *.obj *.ply *.vtp);;Tüm dosyalar (*)"));
    openFile(path);
}

void MainWindow::onUserMessage(const QString& text) {
    chat_->addUserMessage(text);
    if (!modelReady_) {
        chat_->addAssistantMessage(QStringLiteral("Model henüz hazır değil, lütfen bekleyin."));
        return;
    }
    chat_->setInputEnabled(false);
    chat_->showThinking();
    chat_->setStatus(QStringLiteral("Düşünüyor…"), ChatPanel::Status::Loading);
    emit requestProcess(text);
}

void MainWindow::onWorkerStatus(const QString& message, int level) {
    const auto status = static_cast<ChatPanel::Status>(level);
    chat_->setStatus(message, status);
    modelReady_ = (status == ChatPanel::Status::Ready);
    chat_->setInputEnabled(modelReady_);
    if (status == ChatPanel::Status::Error)
        chat_->addSystemMessage(message);
}

void MainWindow::onWorkerResult(bool ok, const QString& command, const QString& paramsJson,
                                const QString& displayMessage) {
    chat_->hideThinking();
    chat_->addAssistantMessage(displayMessage);
    chat_->setInputEnabled(true);
    chat_->setStatus(QStringLiteral("Hazır"), ChatPanel::Status::Ready);

    // Only a real, applicable command reaches the scene. "none" (no action) and
    // "error" stay in the chat. The EventBus publish + VTK update run here, on
    // the GUI thread — the worker never touches VTK.
    if (!ok || command == QLatin1String("none") || command == QLatin1String("error"))
        return;

    auto params = nlohmann::ordered_json::parse(paramsJson.toStdString(), nullptr,
                                                /*allow_exceptions=*/false);
    if (params.is_discarded()) params = nlohmann::ordered_json::object();
    bus_.publish(core::CommandEvent{command.toStdString(), std::move(params)});
}

}  // namespace semanticad::ui
