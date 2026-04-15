#include "board_top_bar.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "board_participants_view.hpp"

namespace online_board::client_qt {
namespace {

void refresh_widget_style(QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

}  // namespace

BoardTopBar::BoardTopBar(QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("topBar"));

    auto* board_top_layout = new QHBoxLayout(this);
    board_top_layout->setContentsMargins(14, 10, 14, 10);
    board_top_layout->setSpacing(12);

    auto* board_left = new QVBoxLayout();
    board_left->setSpacing(0);
    board_label_ = new QLabel(QStringLiteral("No board open"), this);
    board_label_->setObjectName(QStringLiteral("boardTitleLight"));
    board_role_label_ = new QLabel(QStringLiteral("Open a board from the cabinet"), this);
    board_role_label_->setObjectName(QStringLiteral("boardMetaLight"));
    board_left->addWidget(board_label_);
    board_left->addWidget(board_role_label_);
    board_top_layout->addLayout(board_left, 1);

    board_status_label_ = new QLabel(QStringLiteral("Awaiting board"), this);
    board_status_label_->setObjectName(QStringLiteral("inlineStatus"));
    board_status_label_->setProperty("state", QStringLiteral("neutral"));
    board_top_layout->addWidget(board_status_label_, 0, Qt::AlignVCenter);

    participants_view_ = new BoardParticipantsView(this);
    board_top_layout->addWidget(participants_view_, 0, Qt::AlignVCenter);

    leave_button_ = new QToolButton(this);
    leave_button_->setText(QStringLiteral("X"));
    leave_button_->setObjectName(QStringLiteral("closeBoardButton"));
    leave_button_->setToolTip(QStringLiteral("Return to cabinet"));
    board_top_layout->addWidget(leave_button_, 0, Qt::AlignVCenter);
}

void BoardTopBar::set_board_context(const QString& title, const QString& subtitle, const QString& board_id) {
    current_title_ = title;
    current_subtitle_ = subtitle;

    if (!board_id.isEmpty()) {
        board_label_->setText(
            QStringLiteral(
                "%1 <span style=\"font-size:15px; color:#7d8c95; font-family:'Segoe UI'; font-weight:500;\">(%2)</span>")
                .arg(title.toHtmlEscaped(), board_id.toHtmlEscaped()));
    } else {
        board_label_->setText(title);
    }

    board_role_label_->setText(subtitle);
}

void BoardTopBar::set_status(const QString& text, const QString& state) {
    board_status_label_->setText(text);
    board_status_label_->setProperty("state", state);
    refresh_widget_style(board_status_label_);
}

void BoardTopBar::set_participants(const QStringList& entries) {
    participants_view_->set_entries(entries);
}

}  // namespace online_board::client_qt
