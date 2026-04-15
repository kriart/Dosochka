#pragma once

#include <optional>

#include "online_board/application/services/board_access_service.hpp"
#include "online_board/application/dto.hpp"
#include "online_board/application/repository_interfaces.hpp"
#include "online_board/application/security_interfaces.hpp"
#include "online_board/common/clock.hpp"
#include "online_board/common/result.hpp"

namespace online_board::application {

class BoardService {
public:
    BoardService(
        IBoardRepository& board_repository,
        IBoardMemberRepository& member_repository,
        IBoardObjectRepository& object_repository,
        IBoardOperationRepository& operation_repository,
        const BoardAccessService& access_service,
        const IIdGenerator& id_generator,
        const IPasswordHasher& password_hasher,
        const common::IClock& clock);

    [[nodiscard]] common::Result<CreateBoardResponse> create_board(const CreateBoardRequest& request);

    [[nodiscard]] common::Result<ListUserBoardsResponse> list_user_boards(
        const ListUserBoardsRequest& request) const;

    [[nodiscard]] common::Result<UpdateBoardSettingsResponse> update_board_settings(
        const UpdateBoardSettingsRequest& request);

    [[nodiscard]] common::Result<UpsertBoardMemberResponse> upsert_board_member(
        const UpsertBoardMemberRequest& request);

    [[nodiscard]] common::Result<ListBoardMembersResponse> list_board_members(
        const ListBoardMembersRequest& request) const;

    [[nodiscard]] common::Result<RemoveBoardMemberResponse> remove_board_member(
        const RemoveBoardMemberRequest& request);

    [[nodiscard]] common::Result<DeleteBoardResponse> delete_board(const DeleteBoardRequest& request);

    [[nodiscard]] common::Result<JoinBoardResponse> join_board(const JoinBoardRequest& request) const;

    [[nodiscard]] common::Result<domain::BoardSnapshot> load_snapshot(const common::BoardId& board_id) const;

private:
    [[nodiscard]] std::optional<domain::BoardMember> find_member_for_principal(
        const common::BoardId& board_id,
        const domain::Principal& principal) const;

    IBoardRepository& board_repository_;
    IBoardMemberRepository& member_repository_;
    IBoardObjectRepository& object_repository_;
    IBoardOperationRepository& operation_repository_;
    const BoardAccessService& access_service_;
    const IIdGenerator& id_generator_;
    const IPasswordHasher& password_hasher_;
    const common::IClock& clock_;
};

}  // namespace online_board::application
