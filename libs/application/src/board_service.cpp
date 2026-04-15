#include "online_board/application/services/board_service.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace online_board::application {

namespace {

common::Result<std::monostate> validate_create_board_request(const CreateBoardRequest& request) {
    if (request.title.empty()) {
        return common::fail<std::monostate>(
            common::ErrorCode::invalid_argument,
            "Board title cannot be empty");
    }
    if (request.access_mode != domain::AccessMode::password_protected) {
        if (request.password.has_value()) {
            return common::fail<std::monostate>(
                common::ErrorCode::invalid_argument,
                "Password can only be set for password-protected boards");
        }
        if (request.guest_policy != domain::GuestPolicy::guest_disabled) {
            return common::fail<std::monostate>(
                common::ErrorCode::invalid_argument,
                "Guests can only be enabled for password-protected boards");
        }
    }
    if (request.access_mode == domain::AccessMode::password_protected) {
        if (!request.password.has_value() || request.password->empty()) {
            return common::fail<std::monostate>(
                common::ErrorCode::invalid_argument,
                "Password-protected board requires a non-empty password");
        }
    }
    return std::monostate{};
}

domain::BoardSnapshot build_snapshot(
    const domain::Board& board,
    IBoardObjectRepository& object_repository) {
    return domain::BoardSnapshot{
        .board_id = board.id,
        .revision = board.last_revision,
        .objects = object_repository.load_by_board(board.id),
        .active_participants = {},
    };
}

}  // namespace

BoardService::BoardService(
    IBoardRepository& board_repository,
    IBoardMemberRepository& member_repository,
    IBoardObjectRepository& object_repository,
    IBoardOperationRepository& operation_repository,
    const BoardAccessService& access_service,
    const IIdGenerator& id_generator,
    const IPasswordHasher& password_hasher,
    const common::IClock& clock)
    : board_repository_(board_repository),
      member_repository_(member_repository),
      object_repository_(object_repository),
      operation_repository_(operation_repository),
      access_service_(access_service),
      id_generator_(id_generator),
      password_hasher_(password_hasher),
      clock_(clock) {
}

common::Result<CreateBoardResponse> BoardService::create_board(const CreateBoardRequest& request) {
    if (!access_service_.can_create_board(request.principal)) {
        return common::fail<CreateBoardResponse>(
            common::ErrorCode::access_denied,
            "Only registered users may create boards");
    }
    const auto validation = validate_create_board_request(request);
    if (!common::is_ok(validation)) {
        return common::error(validation);
    }

    const auto& owner = std::get<domain::RegisteredUserPrincipal>(request.principal);
    const auto now = clock_.now();

    domain::Board board{
        .id = common::BoardId{id_generator_.next_id()},
        .owner_user_id = owner.user_id,
        .title = request.title,
        .access_mode = request.access_mode,
        .guest_policy = request.guest_policy,
        .password_hash = request.password.has_value()
            ? std::optional<std::string>{password_hasher_.hash(*request.password)}
            : std::nullopt,
        .last_revision = 0,
        .created_at = now,
        .updated_at = now,
    };
    board_repository_.save(board);

    member_repository_.save(domain::BoardMember{
        .board_id = board.id,
        .user_id = owner.user_id,
        .role = domain::BoardRole::owner,
        .created_at = now,
    });
    object_repository_.replace_for_board(board.id, {});

    return CreateBoardResponse{.board = std::move(board)};
}

common::Result<ListUserBoardsResponse> BoardService::list_user_boards(const ListUserBoardsRequest& request) const {
    const auto* user = std::get_if<domain::RegisteredUserPrincipal>(&request.principal);
    if (user == nullptr) {
        return ListUserBoardsResponse{.boards = {}};
    }

    std::vector<UserBoardSummary> boards;
    for (const auto& membership : member_repository_.list_memberships_for_user(user->user_id)) {
        const auto board = board_repository_.find_by_id(membership.board_id);
        if (!board.has_value()) {
            continue;
        }
        boards.push_back(UserBoardSummary{
            .board = *board,
            .role = access_service_.resolve_existing_role(
                *board,
                request.principal,
                std::optional<domain::BoardMember>{membership}).value_or(membership.role),
        });
    }

    std::sort(
        boards.begin(),
        boards.end(),
        [](const UserBoardSummary& lhs, const UserBoardSummary& rhs) {
            if (lhs.board.updated_at != rhs.board.updated_at) {
                return lhs.board.updated_at > rhs.board.updated_at;
            }
            return lhs.board.id.value < rhs.board.id.value;
        });

    return ListUserBoardsResponse{.boards = std::move(boards)};
}

common::Result<UpdateBoardSettingsResponse> BoardService::update_board_settings(
    const UpdateBoardSettingsRequest& request) {
    const auto board = board_repository_.find_by_id(request.board_id);
    if (!board.has_value()) {
        return common::fail<UpdateBoardSettingsResponse>(
            common::ErrorCode::not_found,
            "Board was not found");
    }

    auto updated_board = *board;
    const auto member = find_member_for_principal(updated_board.id, request.principal);
    const auto role = access_service_.resolve_existing_role(updated_board, request.principal, member);
    if (!role.has_value() || !access_service_.can_manage_board_settings(*role)) {
        return common::fail<UpdateBoardSettingsResponse>(
            common::ErrorCode::access_denied,
            "Current principal cannot update board settings");
    }
    if (request.title.has_value() && request.title->empty()) {
        return common::fail<UpdateBoardSettingsResponse>(
            common::ErrorCode::invalid_argument,
            "Board title cannot be empty");
    }
    if (request.password.has_value() && !request.change_password) {
        return common::fail<UpdateBoardSettingsResponse>(
            common::ErrorCode::invalid_argument,
            "Password payload requires change_password=true");
    }
    if (request.password.has_value() && request.password->empty()) {
        return common::fail<UpdateBoardSettingsResponse>(
            common::ErrorCode::invalid_argument,
            "Board password cannot be empty");
    }

    const auto next_access_mode = request.access_mode.value_or(updated_board.access_mode);
    auto next_guest_policy = request.guest_policy.value_or(updated_board.guest_policy);
    auto next_password_hash = updated_board.password_hash;

    if (request.change_password) {
        next_password_hash = request.password.has_value()
            ? std::optional<std::string>{password_hasher_.hash(*request.password)}
            : std::nullopt;
    }

    if (next_access_mode != domain::AccessMode::password_protected) {
        if (request.guest_policy.has_value() && *request.guest_policy != domain::GuestPolicy::guest_disabled) {
            return common::fail<UpdateBoardSettingsResponse>(
                common::ErrorCode::invalid_argument,
                "Guests can only be enabled for password-protected boards");
        }
        if (request.change_password && request.password.has_value()) {
            return common::fail<UpdateBoardSettingsResponse>(
                common::ErrorCode::invalid_argument,
                "Password can only be changed for password-protected boards");
        }
        next_guest_policy = domain::GuestPolicy::guest_disabled;
        next_password_hash = std::nullopt;
    } else if (!next_password_hash.has_value()) {
        return common::fail<UpdateBoardSettingsResponse>(
            common::ErrorCode::invalid_argument,
            "Password-protected board requires a password");
    }

    if (request.title.has_value()) {
        updated_board.title = *request.title;
    }
    updated_board.access_mode = next_access_mode;
    updated_board.guest_policy = next_guest_policy;
    updated_board.password_hash = std::move(next_password_hash);
    updated_board.updated_at = clock_.now();
    board_repository_.save(updated_board);
    return UpdateBoardSettingsResponse{.board = std::move(updated_board)};
}

common::Result<UpsertBoardMemberResponse> BoardService::upsert_board_member(
    const UpsertBoardMemberRequest& request) {
    const auto board = board_repository_.find_by_id(request.board_id);
    if (!board.has_value()) {
        return common::fail<UpsertBoardMemberResponse>(
            common::ErrorCode::not_found,
            "Board was not found");
    }

    const auto actor_member = find_member_for_principal(request.board_id, request.principal);
    const auto actor_role = access_service_.resolve_existing_role(*board, request.principal, actor_member);
    if (!actor_role.has_value() || !access_service_.can_manage_board_settings(*actor_role)) {
        return common::fail<UpsertBoardMemberResponse>(
            common::ErrorCode::access_denied,
            "Current principal cannot manage board members");
    }
    if (request.role == domain::BoardRole::owner) {
        return common::fail<UpsertBoardMemberResponse>(
            common::ErrorCode::invalid_argument,
            "Owner role cannot be assigned through member management");
    }
    if (request.target_user_id == board->owner_user_id) {
        return common::fail<UpsertBoardMemberResponse>(
            common::ErrorCode::invalid_argument,
            "Board owner cannot be changed through member management");
    }

    const auto now = clock_.now();
    domain::BoardMember member{
        .board_id = request.board_id,
        .user_id = request.target_user_id,
        .role = request.role,
        .created_at = now,
    };
    member_repository_.save(member);

    auto updated_board = *board;
    updated_board.updated_at = now;
    board_repository_.save(updated_board);

    return UpsertBoardMemberResponse{.member = std::move(member)};
}

common::Result<ListBoardMembersResponse> BoardService::list_board_members(
    const ListBoardMembersRequest& request) const {
    const auto board = board_repository_.find_by_id(request.board_id);
    if (!board.has_value()) {
        return common::fail<ListBoardMembersResponse>(
            common::ErrorCode::not_found,
            "Board was not found");
    }

    const auto member = find_member_for_principal(request.board_id, request.principal);
    const auto role = access_service_.resolve_existing_role(*board, request.principal, member);
    if (!role.has_value()) {
        return common::fail<ListBoardMembersResponse>(
            common::ErrorCode::access_denied,
            "Current principal cannot view board members");
    }

    auto members = member_repository_.list_members(request.board_id);
    std::sort(
        members.begin(),
        members.end(),
        [](const domain::BoardMember& lhs, const domain::BoardMember& rhs) {
            return lhs.user_id.value < rhs.user_id.value;
        });

    return ListBoardMembersResponse{.members = std::move(members)};
}

common::Result<RemoveBoardMemberResponse> BoardService::remove_board_member(
    const RemoveBoardMemberRequest& request) {
    const auto board = board_repository_.find_by_id(request.board_id);
    if (!board.has_value()) {
        return common::fail<RemoveBoardMemberResponse>(
            common::ErrorCode::not_found,
            "Board was not found");
    }

    const auto actor_member = find_member_for_principal(request.board_id, request.principal);
    const auto actor_role = access_service_.resolve_existing_role(*board, request.principal, actor_member);
    if (!actor_role.has_value() || !access_service_.can_manage_board_settings(*actor_role)) {
        return common::fail<RemoveBoardMemberResponse>(
            common::ErrorCode::access_denied,
            "Current principal cannot manage board members");
    }
    if (request.target_user_id == board->owner_user_id) {
        return common::fail<RemoveBoardMemberResponse>(
            common::ErrorCode::invalid_argument,
            "Board owner cannot be removed from membership");
    }
    if (!member_repository_.remove_member(request.board_id, request.target_user_id)) {
        return common::fail<RemoveBoardMemberResponse>(
            common::ErrorCode::not_found,
            "Board member was not found");
    }

    auto updated_board = *board;
    updated_board.updated_at = clock_.now();
    board_repository_.save(updated_board);

    return RemoveBoardMemberResponse{
        .board_id = request.board_id,
        .removed_user_id = request.target_user_id,
    };
}

common::Result<DeleteBoardResponse> BoardService::delete_board(const DeleteBoardRequest& request) {
    const auto board = board_repository_.find_by_id(request.board_id);
    if (!board.has_value()) {
        return common::fail<DeleteBoardResponse>(
            common::ErrorCode::not_found,
            "Board was not found");
    }

    const auto member = find_member_for_principal(request.board_id, request.principal);
    const auto role = access_service_.resolve_existing_role(*board, request.principal, member);
    if (!role.has_value() || *role != domain::BoardRole::owner) {
        return common::fail<DeleteBoardResponse>(
            common::ErrorCode::access_denied,
            "Only board owner can delete the board");
    }

    member_repository_.remove_board(request.board_id);
    object_repository_.remove_board(request.board_id);
    operation_repository_.remove_board(request.board_id);
    if (!board_repository_.remove(request.board_id)) {
        return common::fail<DeleteBoardResponse>(
            common::ErrorCode::not_found,
            "Board was not found");
    }

    return DeleteBoardResponse{.board_id = request.board_id};
}

common::Result<JoinBoardResponse> BoardService::join_board(const JoinBoardRequest& request) const {
    const auto board = board_repository_.find_by_id(request.board_id);
    if (!board.has_value()) {
        return common::fail<JoinBoardResponse>(
            common::ErrorCode::not_found,
            "Board was not found");
    }

    const auto member = find_member_for_principal(request.board_id, request.principal);

    std::optional<std::string_view> password_hash;
    std::string computed_password_hash;
    if (request.password.has_value()) {
        computed_password_hash = password_hasher_.hash(*request.password);
        password_hash = computed_password_hash;
    }

    auto role_result = access_service_.resolve_role_for_join(
        *board,
        request.principal,
        member,
        password_hash);
    if (!common::is_ok(role_result)) {
        return common::error(role_result);
    }

    return JoinBoardResponse{
        .board = *board,
        .role = common::value(role_result),
        .snapshot = build_snapshot(*board, object_repository_),
    };
}

common::Result<domain::BoardSnapshot> BoardService::load_snapshot(const common::BoardId& board_id) const {
    const auto board = board_repository_.find_by_id(board_id);
    if (!board.has_value()) {
        return common::fail<domain::BoardSnapshot>(
            common::ErrorCode::not_found,
            "Board was not found");
    }

    return build_snapshot(*board, object_repository_);
}

std::optional<domain::BoardMember> BoardService::find_member_for_principal(
    const common::BoardId& board_id,
    const domain::Principal& principal) const {
    if (const auto* user = std::get_if<domain::RegisteredUserPrincipal>(&principal)) {
        return member_repository_.find_member(board_id, user->user_id);
    }
    return std::nullopt;
}

}  // namespace online_board::application
