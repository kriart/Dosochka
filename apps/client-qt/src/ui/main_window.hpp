#pragma once

#include <QComboBox>
#include <QColor>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QToolButton>

#include "board_canvas_widget.hpp"
#include "board_tool_panel.hpp"
#include "board_top_bar.hpp"
#include "client_session_controller.hpp"

namespace online_board::client_qt {

class MainWindow final : public QMainWindow {
public:
    MainWindow();

private:
    void setup_ui();
    void apply_styles();
    void build_auth_page();
    void build_cabinet_page();
    void build_board_page();
    void bind_callbacks();
    void ensure_connected();
    void enqueue_operation(QJsonObject operation);
    void sync_create_board_controls();
    void sync_ui_state();
    void set_active_tool(BoardCanvasWidget::Tool tool);
    void set_status_text(const QString& text, const QString& state);
    void set_account_text(const QString& text);
    void set_board_context(const QString& title, const QString& subtitle);
    void update_color_button();
    void update_tool_panel_geometry();
    void update_participants_view();
    void update_user_boards(const std::vector<ClientSessionController::UserBoardEntry>& boards);
    void sync_user_board_actions();
    void show_auth_page();
    void show_cabinet_page();
    void show_board_page();
    void reset_board_state();
    void leave_board();
    void logout();

    ClientSessionController controller_;
    QString current_board_title_;
    QString current_board_subtitle_;
    QString current_board_id_;

    QStackedWidget* page_stack_ {nullptr};

    QLineEdit* host_edit_ {nullptr};
    QSpinBox* port_spin_ {nullptr};
    QPushButton* connect_button_ {nullptr};
    QLabel* auth_status_label_ {nullptr};
    QLineEdit* login_edit_ {nullptr};
    QLineEdit* display_name_edit_ {nullptr};
    QLineEdit* password_edit_ {nullptr};
    QLineEdit* guest_edit_ {nullptr};
    QPushButton* register_button_ {nullptr};
    QPushButton* login_button_ {nullptr};
    QPushButton* guest_button_ {nullptr};

    QLabel* cabinet_account_label_ {nullptr};
    QLabel* cabinet_status_label_ {nullptr};
    QPushButton* logout_button_ {nullptr};
    QLineEdit* create_title_edit_ {nullptr};
    QComboBox* create_access_mode_combo_ {nullptr};
    QComboBox* create_guest_policy_combo_ {nullptr};
    QLineEdit* create_password_edit_ {nullptr};
    QPushButton* create_board_button_ {nullptr};
    QLineEdit* join_board_id_edit_ {nullptr};
    QLineEdit* join_password_edit_ {nullptr};
    QPushButton* join_board_button_ {nullptr};
    QListWidget* user_boards_list_ {nullptr};
    QPushButton* open_selected_board_button_ {nullptr};
    QPushButton* delete_selected_board_button_ {nullptr};

    BoardTopBar* board_top_bar_ {nullptr};
    BoardToolPanel* tool_panel_ {nullptr};
    BoardCanvasWidget* canvas_ {nullptr};

    QColor current_color_ {"#14212a"};
};

}  // namespace online_board::client_qt
