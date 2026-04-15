#pragma once

#include <string_view>

namespace online_board::protocol::message_type {

inline constexpr std::string_view register_request = "register_request";
inline constexpr std::string_view register_response = "register_response";
inline constexpr std::string_view login_request = "login_request";
inline constexpr std::string_view login_response = "login_response";
inline constexpr std::string_view guest_enter_request = "guest_enter_request";
inline constexpr std::string_view guest_enter_response = "guest_enter_response";
inline constexpr std::string_view session_restore_request = "session_restore_request";
inline constexpr std::string_view session_restore_response = "session_restore_response";
inline constexpr std::string_view create_board_request = "create_board_request";
inline constexpr std::string_view create_board_response = "create_board_response";
inline constexpr std::string_view list_user_boards_request = "list_user_boards_request";
inline constexpr std::string_view list_user_boards_response = "list_user_boards_response";
inline constexpr std::string_view delete_board_request = "delete_board_request";
inline constexpr std::string_view delete_board_response = "delete_board_response";
inline constexpr std::string_view join_board_request = "join_board_request";
inline constexpr std::string_view join_board_response = "join_board_response";
inline constexpr std::string_view leave_board_request = "leave_board_request";
inline constexpr std::string_view leave_board_response = "leave_board_response";
inline constexpr std::string_view board_deleted = "board_deleted";
inline constexpr std::string_view presence_update = "presence_update";
inline constexpr std::string_view operation_request = "operation_request";
inline constexpr std::string_view operation_applied = "operation_applied";
inline constexpr std::string_view ping = "ping";
inline constexpr std::string_view pong = "pong";
inline constexpr std::string_view error = "error";

}  // namespace online_board::protocol::message_type
