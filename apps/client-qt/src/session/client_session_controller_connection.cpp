#include "client_session_controller.hpp"

namespace online_board::client_qt {

void ClientSessionController::bind_connection_callbacks() {
    connection_.on_connected = [this]() {
        emit_status(QStringLiteral("Server connected"), QStringLiteral("success"));
        emit_session_state_changed();
    };

    connection_.on_disconnected = [this]() {
        clear_board_session();
        identity_mode_ = IdentityMode::none;
        emit_user_boards_changed({});
        emit_account_changed(QStringLiteral("Anonymous"));
        show_auth_page();
        emit_status(
            logout_requested_ ? QStringLiteral("Logged out") : QStringLiteral("Disconnected"),
            QStringLiteral("neutral"));
        logout_requested_ = false;
        emit_session_state_changed();
    };

    connection_.on_error = [this](const QString& text) {
        emit_status(text, QStringLiteral("error"));
        emit_session_state_changed();
        if (on_warning_requested) {
            on_warning_requested(QStringLiteral("Network error"), text);
        }
    };

    connection_.on_message = [this](const QString& type, const QJsonObject& payload) {
        handle_message(type, payload);
    };
}

}  // namespace online_board::client_qt
