#pragma once

// The right-hand chat panel: a styled message history, a text input with a send
// button, a model-status indicator and an "open file" button. It is a pure view
// — it never touches the NLP or VTK layers. User intent leaves through two
// signals (messageSubmitted / openFileRequested); MainWindow wires those to the
// inference worker and the viewer and feeds replies back via the add*Message /
// status helpers.

#include <QFrame>
#include <QString>
#include <QWidget>
#include <vector>

class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

namespace semanticad::ui {

class ChatPanel : public QWidget {
    Q_OBJECT
public:
    enum class Status { Loading, Ready, Error };

    explicit ChatPanel(QWidget* parent = nullptr);

    void addUserMessage(const QString& text);
    void addAssistantMessage(const QString& text);
    void addSystemMessage(const QString& text);

    void setInputEnabled(bool enabled);
    void setStatus(const QString& text, Status level);

    // A transient "thinking…" bubble shown while the model is generating.
    void showThinking();
    void hideThinking();

signals:
    void messageSubmitted(const QString& text);
    void openFileRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onSend();

private:
    enum class Role { User, Assistant, System };
    QWidget* addRow(const QString& text, Role role);
    void scrollToBottom();

    QScrollArea* scroll_ = nullptr;
    QVBoxLayout* messagesLayout_ = nullptr;
    QLineEdit* input_ = nullptr;
    QPushButton* sendButton_ = nullptr;
    QPushButton* openButton_ = nullptr;
    QLabel* statusDot_ = nullptr;
    QLabel* statusText_ = nullptr;

    QWidget* thinkingRow_ = nullptr;
    std::vector<QFrame*> bubbles_;  // tracked so wrapping width follows resizes
};

}  // namespace semanticad::ui
