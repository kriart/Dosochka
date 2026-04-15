#pragma once

#include <QStringList>
#include <QWidget>

class QLabel;
class QMenu;
class QToolButton;

namespace online_board::client_qt {

class BoardParticipantsView final : public QWidget {
public:
    explicit BoardParticipantsView(QWidget* parent = nullptr);

    void set_entries(const QStringList& entries);

private:
    void rebuild_menu();

    QStringList entries_;
    QLabel* caption_label_ {nullptr};
    QToolButton* summary_button_ {nullptr};
    QMenu* popup_menu_ {nullptr};
};

}  // namespace online_board::client_qt
