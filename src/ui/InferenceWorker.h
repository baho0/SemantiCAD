#pragma once

// Runs the (slow, blocking) NLP pipeline off the GUI thread.
//
// SemanticIO loads a multi-GB LLM at construction and each process() call runs
// grammar-constrained generation that can take seconds — doing either on the
// GUI thread would freeze the window. This QObject is moved onto a worker
// thread; the UI talks to it only through queued signals/slots, so nothing VTK-
// or Qt-widget-related ever happens here. Results cross back as plain strings
// (the canonical command name + its params JSON), which the GUI thread turns
// into a CommandEvent — keeping the EventBus/VTK work on the GUI thread.

#include <memory>

#include <QObject>
#include <QString>

namespace semanticad::nlp {
class SemanticIO;
}

namespace semanticad::ui {

class InferenceWorker : public QObject {
    Q_OBJECT
public:
    InferenceWorker(QString modelPath, QString commandsDir, QObject* parent = nullptr);
    ~InferenceWorker() override;

public slots:
    // Construct SemanticIO and load the model. Run once, after the worker has
    // been moved to its thread (emits statusChanged with the outcome).
    void initialize();

    // Run one natural-language instruction through the pipeline.
    void process(const QString& text);

signals:
    // level: 0 = loading, 1 = ready, 2 = error (matches ChatPanel::Status).
    // The message is user-facing (Turkish).
    void statusChanged(const QString& message, int level);

    // ok=true means a valid command was produced. command/paramsJson carry the
    // canonical envelope; displayMessage is a friendly TR line for the chat.
    void resultReady(bool ok, const QString& command, const QString& paramsJson,
                     const QString& displayMessage);

private:
    QString modelPath_;
    QString commandsDir_;
    std::unique_ptr<nlp::SemanticIO> io_;
};

}  // namespace semanticad::ui
