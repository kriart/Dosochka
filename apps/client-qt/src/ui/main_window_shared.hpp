#pragma once

#include <optional>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QPlainTextEdit>
#include <QStyle>
#include <QVBoxLayout>

#include "online_board/domain/core/enums.hpp"

namespace online_board::client_qt {

struct CardSection {
    QFrame* frame {nullptr};
    QVBoxLayout* content_layout {nullptr};
};

inline CardSection create_card(const QString& title, const QString& subtitle, QWidget* parent) {
    auto* frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("card"));

    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto* title_label = new QLabel(title, frame);
    title_label->setObjectName(QStringLiteral("cardTitle"));
    layout->addWidget(title_label);

    if (!subtitle.isEmpty()) {
        auto* subtitle_label = new QLabel(subtitle, frame);
        subtitle_label->setObjectName(QStringLiteral("cardSubtitle"));
        subtitle_label->setWordWrap(true);
        layout->addWidget(subtitle_label);
    }

    return CardSection{frame, layout};
}

inline QFrame* create_divider(QWidget* parent) {
    auto* divider = new QFrame(parent);
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setObjectName(QStringLiteral("divider"));
    return divider;
}

inline QString role_text(domain::BoardRole role) {
    switch (role) {
        case domain::BoardRole::owner:
            return QStringLiteral("owner");
        case domain::BoardRole::editor:
            return QStringLiteral("editor");
        case domain::BoardRole::viewer:
            return QStringLiteral("viewer");
    }
    return QStringLiteral("viewer");
}

inline void refresh_widget_style(QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

inline std::optional<QString> request_text_input(QWidget* parent) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Add text"));
    dialog.setModal(true);
    dialog.resize(420, 240);
    dialog.setMinimumSize(360, 220);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto* heading = new QLabel(QStringLiteral("Text object"), &dialog);
    heading->setObjectName(QStringLiteral("dialogTitle"));
    layout->addWidget(heading);

    auto* hint = new QLabel(QStringLiteral("Enter the text that should appear on the board."), &dialog);
    hint->setObjectName(QStringLiteral("dialogSubtitle"));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto* text_edit = new QPlainTextEdit(&dialog);
    text_edit->setPlaceholderText(QStringLiteral("Type something short and readable"));
    text_edit->setTabChangesFocus(true);
    layout->addWidget(text_edit, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Insert text"));
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    layout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(text_edit, &QPlainTextEdit::textChanged, &dialog, [text_edit, buttons]() {
        buttons->button(QDialogButtonBox::Ok)->setEnabled(!text_edit->toPlainText().trimmed().isEmpty());
    });

    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }

    const auto text = text_edit->toPlainText().trimmed();
    if (text.isEmpty()) {
        return std::nullopt;
    }

    return text;
}

}  // namespace online_board::client_qt
