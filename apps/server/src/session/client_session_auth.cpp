#include "client_session.hpp"

namespace online_board::server {

void ClientSession::handle_register(const boost::json::object& payload) {
    auto result = state_.auth_service().register_user(application::RegisterUserRequest{
        .login = json_helpers::expect_string(payload, "login"),
        .display_name = json_helpers::expect_string(payload, "display_name"),
        .password = json_helpers::expect_string(payload, "password"),
    });
    if (!common::is_ok(result)) {
        send_error(common::error(result));
        return;
    }

    const auto& response = common::value(result);
    set_registered_principal(response.user);
    send_json(json_helpers::serialize_register_response(response));
}

void ClientSession::handle_login(const boost::json::object& payload) {
    auto result = state_.auth_service().login(application::LoginRequest{
        .login = json_helpers::expect_string(payload, "login"),
        .password = json_helpers::expect_string(payload, "password"),
    });
    if (!common::is_ok(result)) {
        send_error(common::error(result));
        return;
    }

    const auto& response = common::value(result);
    set_registered_principal(response.user);
    send_json(json_helpers::serialize_login_response(response));
}

void ClientSession::handle_guest_enter(const boost::json::object& payload) {
    auto result = state_.auth_service().enter_guest(application::GuestEnterRequest{
        .nickname = json_helpers::expect_string(payload, "nickname"),
    });
    if (!common::is_ok(result)) {
        send_error(common::error(result));
        return;
    }

    const auto& response = common::value(result);
    set_guest_principal(response);
    send_json(json_helpers::serialize_guest_enter_response(response));
}

void ClientSession::handle_session_restore(const boost::json::object& payload) {
    auto session_result = state_.auth_service().restore_session(json_helpers::expect_string(payload, "token"));
    if (!common::is_ok(session_result)) {
        send_error(common::error(session_result));
        return;
    }

    const auto& session = common::value(session_result);
    const auto user = state_.user_repository().find_by_id(session.user_id);
    if (!user.has_value()) {
        send_json(json_helpers::serialize_error(common::ErrorCode::not_found, "User for session was not found"));
        return;
    }

    set_registered_principal(*user);
    send_json(json_helpers::serialize_session_restore_response(*user));
}

}  // namespace online_board::server
