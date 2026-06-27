#include "ui/ChatPanel.h"

#include <algorithm>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

namespace semanticad::ui {
namespace {

// Modern dark theme, tuned to sit next to VTK's #1a1a1f viewport background.
const char* kStyleSheet = R"qss(
QWidget#chatPanel { background: #16161c; }

QFrame#header {
    background: #1c1c24;
    border-bottom: 1px solid #2a2a33;
}
QLabel#title { color: #f5f5fa; font-size: 17px; font-weight: 700; }
QLabel#subtitle { color: #8b8b96; font-size: 11px; }
QLabel#statusText { color: #b8b8c2; font-size: 11px; }

QScrollArea#chatScroll { border: none; background: #16161c; }
QScrollArea#chatScroll > QWidget > QWidget { background: #16161c; }
QScrollBar:vertical { background: transparent; width: 8px; margin: 4px 2px; }
QScrollBar::handle:vertical { background: #33333f; border-radius: 4px; min-height: 28px; }
QScrollBar::handle:vertical:hover { background: #43434f; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }

QFrame#bubble[role="user"] { background: #3b82f6; border-radius: 14px; }
QFrame#bubble[role="user"] QLabel { color: #ffffff; font-size: 13px; }
QFrame#bubble[role="assistant"] { background: #262630; border-radius: 14px; }
QFrame#bubble[role="assistant"] QLabel { color: #e8e8ef; font-size: 13px; }
QFrame#bubble[role="system"] { background: transparent; border: 1px solid #2a2a33; border-radius: 12px; }
QFrame#bubble[role="system"] QLabel { color: #8b8b96; font-size: 12px; font-style: italic; }

QFrame#inputBar { background: #1c1c24; border-top: 1px solid #2a2a33; }
QLineEdit#messageInput {
    background: #262630;
    border: 1px solid #33333f;
    border-radius: 19px;
    padding: 9px 16px;
    color: #f5f5fa;
    font-size: 13px;
    selection-background-color: #3b82f6;
}
QLineEdit#messageInput:focus { border: 1px solid #3b82f6; }
QLineEdit#messageInput:disabled { color: #6a6a74; }

QPushButton#sendButton {
    background: #3b82f6; color: #ffffff; border: none;
    border-radius: 19px; padding: 9px 18px; font-size: 13px; font-weight: 600;
}
QPushButton#sendButton:hover { background: #4f8cff; }
QPushButton#sendButton:pressed { background: #2f6fe0; }
QPushButton#sendButton:disabled { background: #2a3140; color: #6a6a74; }

QPushButton#openButton {
    background: #262630; color: #e8e8ef; border: 1px solid #33333f;
    border-radius: 8px; padding: 7px 14px; font-size: 12px; font-weight: 600;
}
QPushButton#openButton:hover { background: #2f2f3a; border: 1px solid #43434f; }
QPushButton#openButton:pressed { background: #222229; }
)qss";

const char* dotColor(ChatPanel::Status level) {
    switch (level) {
        case ChatPanel::Status::Ready: return "#22c55e";
        case ChatPanel::Status::Error: return "#ef4444";
        case ChatPanel::Status::Loading:
        default: return "#f59e0b";
    }
}

}  // namespace

ChatPanel::ChatPanel(QWidget* parent) : QWidget(parent) {
    setObjectName("chatPanel");
    setStyleSheet(QString::fromUtf8(kStyleSheet));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // --- Header: title, status, open-file button ---------------------------
    auto* header = new QFrame;
    header->setObjectName("header");
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(16, 14, 16, 12);
    headerLayout->setSpacing(6);

    auto* titleRow = new QHBoxLayout;
    titleRow->setSpacing(8);
    auto* title = new QLabel(QStringLiteral("SemantiCAD"));
    title->setObjectName("title");
    openButton_ = new QPushButton(QStringLiteral("📂  Dosya Aç"));
    openButton_->setObjectName("openButton");
    openButton_->setCursor(Qt::PointingHandCursor);
    titleRow->addWidget(title);
    titleRow->addStretch(1);
    titleRow->addWidget(openButton_);

    auto* statusRow = new QHBoxLayout;
    statusRow->setSpacing(7);
    auto* subtitle = new QLabel(QStringLiteral("Doğal dil ile 3B tasarım"));
    subtitle->setObjectName("subtitle");
    statusDot_ = new QLabel;
    statusDot_->setFixedSize(9, 9);
    statusText_ = new QLabel(QStringLiteral("Başlatılıyor…"));
    statusText_->setObjectName("statusText");
    statusRow->addWidget(subtitle);
    statusRow->addStretch(1);
    statusRow->addWidget(statusDot_);
    statusRow->addWidget(statusText_);

    headerLayout->addLayout(titleRow);
    headerLayout->addLayout(statusRow);
    root->addWidget(header);

    // --- Scrollable message history ----------------------------------------
    scroll_ = new QScrollArea;
    scroll_->setObjectName("chatScroll");
    scroll_->setWidgetResizable(true);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* container = new QWidget;
    messagesLayout_ = new QVBoxLayout(container);
    messagesLayout_->setContentsMargins(14, 16, 14, 16);
    messagesLayout_->setSpacing(10);
    messagesLayout_->addStretch(1);  // keeps messages pinned to the top
    scroll_->setWidget(container);
    root->addWidget(scroll_, 1);

    // --- Input bar ---------------------------------------------------------
    auto* inputBar = new QFrame;
    inputBar->setObjectName("inputBar");
    auto* inputLayout = new QHBoxLayout(inputBar);
    inputLayout->setContentsMargins(12, 12, 12, 14);
    inputLayout->setSpacing(8);
    input_ = new QLineEdit;
    input_->setObjectName("messageInput");
    input_->setPlaceholderText(QStringLiteral("Bir komut yazın…  örn: \"objeyi 2 kat büyült\""));
    input_->setClearButtonEnabled(true);
    sendButton_ = new QPushButton(QStringLiteral("Gönder"));
    sendButton_->setObjectName("sendButton");
    sendButton_->setCursor(Qt::PointingHandCursor);
    inputLayout->addWidget(input_, 1);
    inputLayout->addWidget(sendButton_);
    root->addWidget(inputBar);

    setStatus(QStringLiteral("Başlatılıyor…"), Status::Loading);

    connect(sendButton_, &QPushButton::clicked, this, &ChatPanel::onSend);
    connect(input_, &QLineEdit::returnPressed, this, &ChatPanel::onSend);
    connect(openButton_, &QPushButton::clicked, this, &ChatPanel::openFileRequested);
    // Auto-scroll to the newest message whenever the content grows.
    connect(scroll_->verticalScrollBar(), &QScrollBar::rangeChanged, this,
            [this](int, int max) { scroll_->verticalScrollBar()->setValue(max); });

    addSystemMessage(QStringLiteral(
        "Merhaba! Sahnedeki nesneyi doğal dille yönetebilirsiniz. "
        "Bir dosya açmak için sağ üstteki düğmeyi kullanın."));
}

QWidget* ChatPanel::addRow(const QString& text, Role role) {
    auto* bubble = new QFrame;
    bubble->setObjectName("bubble");
    const char* roleName = role == Role::User        ? "user"
                           : role == Role::Assistant ? "assistant"
                                                     : "system";
    bubble->setProperty("role", roleName);
    auto* bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(14, 10, 14, 10);
    auto* label = new QLabel(text);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubbleLayout->addWidget(label);

    auto* row = new QWidget;
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    if (role == Role::User) {
        rowLayout->addStretch(1);
        rowLayout->addWidget(bubble);
    } else {
        rowLayout->addWidget(bubble);
        rowLayout->addStretch(1);
    }

    // Insert just before the trailing stretch so messages stack top-to-bottom.
    messagesLayout_->insertWidget(messagesLayout_->count() - 1, row);
    bubbles_.push_back(bubble);
    const int w = std::max(140, int(scroll_->viewport()->width() * 0.82));
    bubble->setMaximumWidth(w);
    return row;
}

void ChatPanel::addUserMessage(const QString& text) { addRow(text, Role::User); }
void ChatPanel::addAssistantMessage(const QString& text) { addRow(text, Role::Assistant); }
void ChatPanel::addSystemMessage(const QString& text) { addRow(text, Role::System); }

void ChatPanel::showThinking() {
    if (thinkingRow_) return;
    thinkingRow_ = addRow(QStringLiteral("düşünüyor…"), Role::Assistant);
}

void ChatPanel::hideThinking() {
    if (!thinkingRow_) return;
    messagesLayout_->removeWidget(thinkingRow_);
    if (auto* b = thinkingRow_->findChild<QFrame*>("bubble"))
        bubbles_.erase(std::remove(bubbles_.begin(), bubbles_.end(), b), bubbles_.end());
    thinkingRow_->deleteLater();
    thinkingRow_ = nullptr;
}

void ChatPanel::setInputEnabled(bool enabled) {
    input_->setEnabled(enabled);
    sendButton_->setEnabled(enabled);
    if (enabled) input_->setFocus();
}

void ChatPanel::setStatus(const QString& text, Status level) {
    statusText_->setText(text);
    statusDot_->setStyleSheet(
        QStringLiteral("background: %1; border-radius: 4px;").arg(QString::fromUtf8(dotColor(level))));
}

void ChatPanel::onSend() {
    const QString text = input_->text().trimmed();
    if (text.isEmpty() || !input_->isEnabled()) return;
    input_->clear();
    emit messageSubmitted(text);
}

void ChatPanel::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    const int w = std::max(140, int(scroll_->viewport()->width() * 0.82));
    for (QFrame* bubble : bubbles_)
        if (bubble) bubble->setMaximumWidth(w);
}

void ChatPanel::scrollToBottom() {
    scroll_->verticalScrollBar()->setValue(scroll_->verticalScrollBar()->maximum());
}

}  // namespace semanticad::ui
