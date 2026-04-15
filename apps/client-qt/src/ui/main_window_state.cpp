#include "main_window.hpp"

#include "main_window_shared.hpp"

namespace online_board::client_qt {

void MainWindow::sync_ui_state() {
    const bool connected = controller_.is_connected();
    const bool authenticated = controller_.is_authenticated();
    const bool registered = controller_.can_list_user_boards();
    const bool has_board = controller_.has_board();

    connect_button_->setEnabled(!connected);
    connect_button_->setText(connected ? QStringLiteral("Connected") : QStringLiteral("Connect"));
    host_edit_->setEnabled(!connected);
    port_spin_->setEnabled(!connected);

    register_button_->setEnabled(connected);
    login_button_->setEnabled(connected);
    guest_button_->setEnabled(connected);

    create_title_edit_->setEnabled(registered);
    create_access_mode_combo_->setEnabled(registered);
    create_guest_policy_combo_->setEnabled(registered && create_access_mode_combo_->currentData().toString() == QStringLiteral("password_protected"));
    create_password_edit_->setEnabled(registered && create_access_mode_combo_->currentData().toString() == QStringLiteral("password_protected"));
    create_board_button_->setEnabled(registered);
    join_board_id_edit_->setEnabled(authenticated);
    join_password_edit_->setEnabled(authenticated);
    join_board_button_->setEnabled(authenticated);
    user_boards_list_->setEnabled(registered);

    logout_button_->setVisible(authenticated);
    logout_button_->setEnabled(authenticated);
    logout_button_->setText(controller_.logout_action_text());
    sync_user_board_actions();

    board_top_bar_->leave_button()->setEnabled(has_board);
    tool_panel_->set_controls_enabled(has_board);
    canvas_->setEnabled(has_board);
}

void MainWindow::set_active_tool(BoardCanvasWidget::Tool tool) {
    canvas_->set_tool(tool);
    tool_panel_->set_active_tool(tool);
}

void MainWindow::set_status_text(const QString& text, const QString& state) {
    auth_status_label_->setText(text);
    auth_status_label_->setProperty("state", state);
    refresh_widget_style(auth_status_label_);

    cabinet_status_label_->setText(text);
    cabinet_status_label_->setProperty("state", state);
    refresh_widget_style(cabinet_status_label_);

    board_top_bar_->set_status(text, state);
}

void MainWindow::set_account_text(const QString& text) {
    cabinet_account_label_->setText(text);
}

void MainWindow::set_board_context(const QString& title, const QString& subtitle) {
    current_board_title_ = title;
    current_board_subtitle_ = subtitle;

    if (controller_.has_board() && !current_board_id_.isEmpty()) {
        board_top_bar_->set_board_context(title, subtitle, current_board_id_);
    } else {
        board_top_bar_->set_board_context(title, subtitle);
    }
}

void MainWindow::update_color_button() {
    tool_panel_->set_selected_color(current_color_);
    canvas_->set_primary_color(current_color_);
}

void MainWindow::update_tool_panel_geometry() {
    if (tool_panel_ == nullptr) {
        return;
    }

    tool_panel_->place_default();
}

void MainWindow::update_participants_view() {
    board_top_bar_->set_participants({});
}

void MainWindow::update_user_boards(const std::vector<ClientSessionController::UserBoardEntry>& boards) {
    user_boards_list_->clear();
    for (const auto& board : boards) {
        auto* item = new QListWidgetItem(
            QStringLiteral("%1 [%2]  %3").arg(board.title, board.role, board.board_id),
            user_boards_list_);
        item->setData(Qt::UserRole, board.board_id);
        item->setData(Qt::UserRole + 1, board.title);
        item->setData(Qt::UserRole + 2, board.can_delete);
        item->setToolTip(QStringLiteral("Access: %1").arg(board.access_mode));
    }
    sync_user_board_actions();
}

void MainWindow::sync_user_board_actions() {
    const bool can_list = controller_.can_list_user_boards();
    const auto* item = user_boards_list_ == nullptr ? nullptr : user_boards_list_->currentItem();
    const bool has_selection = can_list && item != nullptr;
    open_selected_board_button_->setEnabled(has_selection);
    delete_selected_board_button_->setEnabled(
        has_selection && item->data(Qt::UserRole + 2).toBool());
}

void MainWindow::show_auth_page() {
    if (isMaximized()) {
        showNormal();
        resize(1120, 820);
    }
    page_stack_->setCurrentIndex(0);
}

void MainWindow::show_cabinet_page() {
    if (isMaximized()) {
        showNormal();
        resize(1240, 860);
    }
    page_stack_->setCurrentIndex(1);
    controller_.list_user_boards();
}

void MainWindow::show_board_page() {
    page_stack_->setCurrentIndex(2);
    if (!isMaximized()) {
        showMaximized();
    }
}

void MainWindow::reset_board_state() {
    canvas_->set_role(domain::BoardRole::viewer);
    canvas_->set_snapshot(domain::BoardSnapshot{});
    canvas_->reset_zoom();
    set_active_tool(BoardCanvasWidget::Tool::pen);
    board_top_bar_->set_participants({});
    current_board_title_.clear();
    current_board_subtitle_.clear();
    current_board_id_.clear();
    set_board_context(QStringLiteral("No board open"), QStringLiteral("Return to the cabinet to create or join another board"));
}

void MainWindow::leave_board() {
    controller_.leave_board();
}

void MainWindow::logout() {
    controller_.logout();
}

}  // namespace online_board::client_qt
