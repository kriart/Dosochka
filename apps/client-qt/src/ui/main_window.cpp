#include "main_window.hpp"

namespace online_board::client_qt {

MainWindow::MainWindow() {
    setup_ui();
    bind_callbacks();
    apply_styles();
    update_tool_panel_geometry();
    update_color_button();
    set_active_tool(BoardCanvasWidget::Tool::pen);
    set_account_text(QStringLiteral("Anonymous"));
    reset_board_state();
    show_auth_page();
    set_status_text(QStringLiteral("Connect to the server to continue"), QStringLiteral("neutral"));
    sync_ui_state();
}

}  // namespace online_board::client_qt
