#pragma once

#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

#include "json_common.hpp"
#include "online_board/domain/operations/applied_operation.hpp"
#include "online_board/protocol/message_types.hpp"

namespace online_board::server::json_helpers {

json::object to_json(const domain::Board& board);
json::object to_json(const domain::BoardParticipant& participant);
json::object to_json(const domain::BoardObject& object);
json::array to_json(const std::vector<domain::BoardParticipant>& participants);
json::object to_json(const domain::BoardSnapshot& snapshot);
json::value to_json(const domain::OperationPayload& payload);
json::object to_json(const domain::AppliedOperation& operation);
std::string serialize_message(std::string_view type, json::object payload);
std::string serialize_error(common::ErrorCode code, std::string message);
std::string serialize_register_response(const application::RegisterUserResponse& response);
std::string serialize_login_response(const application::LoginResponse& response);
std::string serialize_guest_enter_response(const application::GuestEnterResponse& response);
std::string serialize_session_restore_response(const domain::User& user);
std::string serialize_create_board_response(const domain::Board& board);
std::string serialize_list_user_boards_response(const std::vector<application::UserBoardSummary>& boards);
std::string serialize_delete_board_response(const common::BoardId& board_id);
std::string serialize_join_board_response(const domain::Board& board, domain::BoardRole role, const domain::BoardSnapshot& snapshot);
std::string serialize_leave_board_response();
std::string serialize_board_deleted(const common::BoardId& board_id);
std::string serialize_presence_update(const std::vector<domain::BoardParticipant>& participants);
std::string serialize_operation_applied(const domain::AppliedOperation& operation);
std::string serialize_pong();

}  // namespace online_board::server::json_helpers
