#pragma once

#include <QStringList>
#include <QFrame>

class QLabel;
class QToolButton;

namespace online_board::client_qt {

class BoardParticipantsView;

class BoardTopBar final : public QFrame {
public:
    explicit BoardTopBar(QWidget* parent = nullptr);

    [[nodiscard]] QToolButton* leave_button() const { return leave_button_; }
    [[nodiscard]] QString title_text() const { return current_title_; }
    [[nodiscard]] QString subtitle_text() const { return current_subtitle_; }

    void set_board_context(const QString& title, const QString& subtitle, const QString& board_id = {});
    void set_status(const QString& text, const QString& state);
    void set_participants(const QStringList& entries);

private:
    QString current_title_;
    QString current_subtitle_;
    QLabel* board_label_ {nullptr};
    QLabel* board_role_label_ {nullptr};
    QLabel* board_status_label_ {nullptr};
    BoardParticipantsView* participants_view_ {nullptr};
    QToolButton* leave_button_ {nullptr};
};

}  // namespace online_board::client_qt
