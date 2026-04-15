#include "client_session.hpp"

#include <exception>
#include <string>

namespace online_board::server {

void ClientSession::handle_message(const std::string& line) {
    auto message_result = json_helpers::parse_message(line);
    if (!common::is_ok(message_result)) {
        send_json(json_helpers::serialize_error(common::error(message_result).code, common::error(message_result).message));
        return;
    }

    const auto& message = common::value(message_result);
    try {
        if (message.type == protocol::message_type::register_request) {
            handle_register(message.payload);
            return;
        }
        if (message.type == protocol::message_type::login_request) {
            handle_login(message.payload);
            return;
        }
        if (message.type == protocol::message_type::guest_enter_request) {
            handle_guest_enter(message.payload);
            return;
        }
        if (message.type == protocol::message_type::session_restore_request) {
            handle_session_restore(message.payload);
            return;
        }
        if (message.type == protocol::message_type::create_board_request) {
            handle_create_board(message.payload);
            return;
        }
        if (message.type == protocol::message_type::list_user_boards_request) {
            handle_list_user_boards();
            return;
        }
        if (message.type == protocol::message_type::delete_board_request) {
            handle_delete_board(message.payload);
            return;
        }
        if (message.type == protocol::message_type::join_board_request) {
            handle_join_board(message.payload);
            return;
        }
        if (message.type == protocol::message_type::leave_board_request) {
            handle_leave_board();
            return;
        }
        if (message.type == protocol::message_type::operation_request) {
            handle_operation(message.payload);
            return;
        }
        if (message.type == protocol::message_type::ping) {
            handle_ping();
            return;
        }

        send_json(json_helpers::serialize_error(common::ErrorCode::invalid_argument, "Unknown message type"));
    } catch (const std::exception& exception) {
        send_json(json_helpers::serialize_error(common::ErrorCode::invalid_argument, exception.what()));
    }
}

void ClientSession::send_error(const common::Error& error) {
    send_json(json_helpers::serialize_error(error.code, error.message));
}

}  // namespace online_board::server
