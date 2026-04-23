#include "client_session.hpp"

#include <vector>

namespace online_board::server {

void ClientSession::handle_create_board(const boost::json::object& payload) {
    if (!ensure_authenticated()) {
        return;
    }

    auto result = state_.board_service().create_board(application::CreateBoardRequest{
        .principal = *principal_,
        .title = json_helpers::expect_string(payload, "title"),
        .access_mode = json_helpers::access_mode_from_string(json_helpers::expect_string(payload, "access_mode")),
        .guest_policy = json_helpers::guest_policy_from_string(json_helpers::expect_string(payload, "guest_policy")),
        .password = json_helpers::optional_string(payload, "password"),
    });
    if (!common::is_ok(result)) {
        send_error(common::error(result));
        return;
    }

    send_json(json_helpers::serialize_create_board_response(common::value(result).board));
}

void ClientSession::handle_list_user_boards() {
    if (!ensure_authenticated()) {
        return;
    }

    auto result = state_.board_service().list_user_boards(application::ListUserBoardsRequest{
        .principal = *principal_,
    });
    if (!common::is_ok(result)) {
        send_error(common::error(result));
        return;
    }

    send_json(json_helpers::serialize_list_user_boards_response(common::value(result).boards));
}

void ClientSession::handle_delete_board(const boost::json::object& payload) {
    if (!ensure_authenticated()) {
        return;
    }

    const auto board_id = json_helpers::parse_id<common::BoardId>(payload, "board_id");
    auto board_guard = state_.board_access_coordinator().lock_exclusive(board_id);
    auto result = state_.board_service().delete_board(application::DeleteBoardRequest{
        .board_id = board_id,
        .principal = *principal_,
    });
    if (!common::is_ok(result)) {
        send_error(common::error(result));
        return;
    }

    state_.close_board(board_id, this);
    send_json(json_helpers::serialize_delete_board_response(board_id));
}

void ClientSession::handle_join_board(const boost::json::object& payload) {
    if (!ensure_authenticated()) {
        return;
    }

    leave_current_board();
    const auto board_id = json_helpers::parse_id<common::BoardId>(payload, "board_id");
    auto board_guard = state_.board_access_coordinator().lock_shared(board_id);
    auto result = state_.board_service().join_board(application::JoinBoardRequest{
        .board_id = board_id,
        .principal = *principal_,
        .password = json_helpers::optional_string(payload, "password"),
    });
    if (!common::is_ok(result)) {
        send_error(common::error(result));
        return;
    }

    const auto& join_response = common::value(result);
    runtime_ = state_.board_registry().get_or_create(
        join_response.board,
        join_response.snapshot,
        state_.operation_service(),
        state_.presence_service(),
        state_.clock(),
        state_.board_runtime_persistence());
    runtime_->join_participant(*principal_, join_response.role, resolved_principal_name());
    joined_board_id_ = join_response.board.id;
    joined_role_ = join_response.role;
    state_.subscribe(join_response.board.id, shared_from_this());
    send_json(json_helpers::serialize_join_board_response(join_response.board, join_response.role, runtime_->snapshot()));
    state_.broadcast_presence(join_response.board.id, runtime_->active_participants());
}

void ClientSession::handle_operation(const boost::json::object& payload) {
    if (!principal_.has_value() || !runtime_ || !joined_board_id_.has_value() || !joined_role_.has_value()) {
        send_json(json_helpers::serialize_error(common::ErrorCode::access_denied, "Join a board before sending operations"));
        return;
    }

    const auto board_id = json_helpers::parse_id<common::BoardId>(payload, "board_id");
    if (board_id != *joined_board_id_) {
        send_json(json_helpers::serialize_error(common::ErrorCode::invalid_argument, "Operation board_id does not match joined board"));
        return;
    }

    auto board_guard = state_.board_access_coordinator().lock_shared(board_id);
    auto result = runtime_->apply_operation(
        *joined_role_,
        domain::OperationCommand{
            .board_id = board_id,
            .actor = *principal_,
            .base_revision = json_helpers::expect_uint64(payload, "base_revision"),
            .payload = json_helpers::operation_payload_from_json(json_helpers::expect_field(payload, "operation")),
        });
    if (!common::is_ok(result)) {
        send_error(common::error(result));
        return;
    }

    state_.broadcast(board_id, json_helpers::serialize_operation_applied(common::value(result).applied_operation));
}

void ClientSession::handle_leave_board() {
    leave_current_board();
    send_json(json_helpers::serialize_leave_board_response());
}

void ClientSession::handle_ping() {
    send_json(json_helpers::serialize_pong());
}

bool ClientSession::ensure_authenticated() {
    if (principal_.has_value()) {
        return true;
    }
    send_json(json_helpers::serialize_error(common::ErrorCode::access_denied, "Authentication required"));
    return false;
}

void ClientSession::set_registered_principal(const domain::User& user) {
    principal_ = domain::RegisteredUserPrincipal{.user_id = user.id};
    principal_display_name_ = user.display_name.empty() ? user.login : user.display_name;
}

void ClientSession::set_guest_principal(const application::GuestEnterResponse& response) {
    principal_ = response.principal;
    principal_display_name_ = response.session.nickname;
}

std::string ClientSession::resolved_principal_name() const {
    if (!principal_.has_value()) {
        return {};
    }

    if (const auto* user_principal = std::get_if<domain::RegisteredUserPrincipal>(&*principal_)) {
        const auto user = state_.user_repository().find_by_id(user_principal->user_id);
        if (user.has_value()) {
            if (!user->display_name.empty()) {
                return user->display_name;
            }
            if (!user->login.empty()) {
                return user->login;
            }
        }
    }

    return principal_display_name_;
}

void ClientSession::leave_current_board() {
    if (!joined_board_id_.has_value()) {
        return;
    }

    std::vector<domain::BoardParticipant> remaining_participants;
    if (runtime_ && principal_.has_value()) {
        runtime_->leave_participant(*principal_);
        remaining_participants = runtime_->active_participants();
    }
    state_.unsubscribe(*joined_board_id_, this);
    if (!remaining_participants.empty() || runtime_) {
        state_.broadcast_presence(*joined_board_id_, remaining_participants);
    }
    joined_board_id_.reset();
    joined_role_.reset();
    runtime_.reset();
}

void ClientSession::handle_board_deleted(const common::BoardId& board_id) {
    if (!joined_board_id_.has_value() || *joined_board_id_ != board_id) {
        return;
    }

    joined_board_id_.reset();
    joined_role_.reset();
    runtime_.reset();
}

}  // namespace online_board::server
