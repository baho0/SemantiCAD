#pragma once

// The application window: a horizontal 70/30 split with the VTK viewport on the
// left and the chat panel on the right. It is the composition root for the GUI —
// the GUI analogue of main.cpp. It owns the EventBus + VtkViewer (GUI thread)
// and an InferenceWorker living on its own thread, and brokers between the chat
// panel, the model and the scene. All VTK/EventBus work stays on the GUI thread;
// only the worker's process()/initialize() run off-thread.

#include "core/eventbus/EventBus.h"
#include "core/vtk/VtkViewer.h"

#include <vtkSmartPointer.h>

#include <QMainWindow>
#include <QString>

class QThread;
class QVTKOpenGLNativeWidget;
class vtkInteractorStyleTrackballCamera;

namespace semanticad::ui {

class ChatPanel;
class InferenceWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QString modelPath = QStringLiteral("models/Qwen2.5-3B-Instruct-Q4_K_M.gguf"),
                        QString commandsDir = QStringLiteral("commands"),
                        QWidget* parent = nullptr);
    ~MainWindow() override;

    // Load a mesh into the viewport (used at startup for an optional CLI object).
    void openFile(const QString& path);

signals:
    void requestProcess(const QString& text);  // GUI thread -> worker thread

private slots:
    void onUserMessage(const QString& text);
    void onOpenFileRequested();
    void onWorkerStatus(const QString& message, int level);
    void onWorkerResult(bool ok, const QString& command, const QString& paramsJson,
                        const QString& displayMessage);

private:
    // EventBus must outlive (be constructed before) the viewer that subscribes.
    core::EventBus bus_;
    viz::VtkViewer viewer_;

    QVTKOpenGLNativeWidget* vtkWidget_ = nullptr;
    ChatPanel* chat_ = nullptr;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> style_;

    QThread* workerThread_ = nullptr;
    InferenceWorker* worker_ = nullptr;

    bool modelReady_ = false;
    QString currentFile_;
};

}  // namespace semanticad::ui
