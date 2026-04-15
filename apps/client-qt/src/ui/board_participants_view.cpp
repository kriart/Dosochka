#include "board_participants_view.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

namespace online_board::client_qt {

BoardParticipantsView::BoardParticipantsView(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    caption_label_ = new QLabel(QStringLiteral("Participants"), this);
    caption_label_->setObjectName(QStringLiteral("participantsCaption"));
    layout->addWidget(caption_label_);

    summary_button_ = new QToolButton(this);
    summary_button_->setObjectName(QStringLiteral("participantsSummaryButton"));
    summary_button_->setPopupMode(QToolButton::InstantPopup);
    popup_menu_ = new QMenu(summary_button_);
    summary_button_->setMenu(popup_menu_);
    layout->addWidget(summary_button_);

    set_entries({});
}

void BoardParticipantsView::set_entries(const QStringList& entries) {
    entries_ = entries;
    const int count = entries_.size();
    summary_button_->setText(
        count == 0
            ? QStringLiteral("0 online")
            : count == 1 ? QStringLiteral("1 online") : QStringLiteral("%1 online").arg(count));
    summary_button_->setEnabled(count > 0);
    summary_button_->setToolTip(entries_.isEmpty() ? QStringLiteral("No active participants") : entries_.join(QStringLiteral("\n")));
    rebuild_menu();
}

void BoardParticipantsView::rebuild_menu() {
    popup_menu_->clear();
    if (entries_.isEmpty()) {
        auto* action = popup_menu_->addAction(QStringLiteral("No active participants"));
        action->setEnabled(false);
        return;
    }

    for (const auto& entry : entries_) {
        auto* action = popup_menu_->addAction(entry);
        action->setEnabled(false);
    }
}

}  // namespace online_board::client_qt
