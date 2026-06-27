#include "ui/InferenceWorker.h"

#include "core/NLP/process/SemanticIO.h"

#include <exception>

namespace semanticad::ui {
namespace {

// Friendly Turkish label for a command name (for the chat confirmation line).
QString commandLabel(const std::string& name) {
    if (name == "resize") return QStringLiteral("Boyutlandırma");
    if (name == "translate") return QStringLiteral("Taşıma");
    if (name == "rotate") return QStringLiteral("Döndürme");
    if (name == "mirror") return QStringLiteral("Aynalama");
    return QString::fromStdString(name);
}

// Turns a pipeline result into the line shown in the chat.
QString displayMessageFor(const semanticad::nlp::CommandResult& r,
                          const std::string& command,
                          const semanticad::nlp::json& params) {
    if (!r.ok) {
        if (r.kind == "validation_error")
            return QStringLiteral("Komutu anladım ama değerler geçersiz. (%1)")
                .arg(QString::fromStdString(r.message));
        if (r.kind == "unknown_command")
            return QStringLiteral("Bu komutu tanımıyorum.");
        if (r.kind == "empty")
            return QStringLiteral("Lütfen bir komut yazın.");
        return QStringLiteral("Bir hata oluştu: %1").arg(QString::fromStdString(r.message));
    }
    if (command == "none")
        return QStringLiteral(
            "Bunu bir işlem olarak anlayamadım. Örnekler:\n"
            "• \"objeyi 2 kat büyült\"\n"
            "• \"x ekseninde 10 mm taşı\"\n"
            "• \"z ekseni etrafında 90 derece döndür\"");
    return QStringLiteral("✓ %1 uygulandı  %2")
        .arg(commandLabel(command), QString::fromStdString(params.dump()));
}

}  // namespace

InferenceWorker::InferenceWorker(QString modelPath, QString commandsDir, QObject* parent)
    : QObject(parent),
      modelPath_(std::move(modelPath)),
      commandsDir_(std::move(commandsDir)) {}

InferenceWorker::~InferenceWorker() = default;

void InferenceWorker::initialize() {
    emit statusChanged(QStringLiteral("Model yükleniyor…"), /*loading=*/0);
    try {
        io_ = std::make_unique<nlp::SemanticIO>(modelPath_.toStdString(),
                                                commandsDir_.toStdString());
    } catch (const std::exception& e) {
        emit statusChanged(
            QStringLiteral("Model yüklenemedi: %1").arg(QString::fromUtf8(e.what())), /*error=*/2);
        return;
    }
    if (!io_->ready()) {
        emit statusChanged(
            QStringLiteral("Model hazır değil (dosya veya komut tanımı eksik)."), /*error=*/2);
        return;
    }
    emit statusChanged(QStringLiteral("Hazır"), /*ready=*/1);
}

void InferenceWorker::process(const QString& text) {
    if (!io_ || !io_->ready()) {
        emit resultReady(false, QStringLiteral("error"), QStringLiteral("{}"),
                         QStringLiteral("Model hazır değil."));
        return;
    }

    const nlp::CommandResult result = io_->process(text.toStdString());

    const std::string command = result.envelope.value("command", std::string{});
    const nlp::json params = result.envelope.contains("params")
                                 ? result.envelope.at("params")
                                 : nlp::json::object();

    emit resultReady(result.ok, QString::fromStdString(command),
                     QString::fromStdString(params.dump()),
                     displayMessageFor(result, command, params));
}

}  // namespace semanticad::ui
