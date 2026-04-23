#pragma once

#include <map>
#include <optional>
#include <variant>
#include <vector>

#include "online_board/common/ids.hpp"
#include "online_board/domain/operations/applied_operation.hpp"
#include "online_board/domain/board/board.hpp"
#include "online_board/domain/board/board_member.hpp"
#include "online_board/domain/board/board_object.hpp"
#include "online_board/common/result.hpp"

namespace online_board::application {

struct IBoardRepository {
    virtual ~IBoardRepository() = default;
    virtual std::optional<domain::Board> find_by_id(const common::BoardId& board_id) const = 0;
    virtual void save(domain::Board board) = 0;
    virtual bool remove(const common::BoardId& board_id) = 0;
};

struct IBoardMemberRepository {
    virtual ~IBoardMemberRepository() = default;
    virtual std::optional<domain::BoardMember> find_member(
        const common::BoardId& board_id,
        const common::UserId& user_id) const = 0;
    virtual std::vector<domain::BoardMember> list_members(const common::BoardId& board_id) const = 0;
    virtual std::vector<domain::BoardMember> list_memberships_for_user(const common::UserId& user_id) const = 0;
    virtual void save(domain::BoardMember member) = 0;
    virtual bool remove_member(const common::BoardId& board_id, const common::UserId& user_id) = 0;
    virtual void remove_board(const common::BoardId& board_id) = 0;
};

struct IBoardObjectRepository {
    virtual ~IBoardObjectRepository() = default;
    virtual std::map<common::ObjectId, domain::BoardObject> load_by_board(
        const common::BoardId& board_id) const = 0;
    virtual void replace_for_board(
        const common::BoardId& board_id,
        std::map<common::ObjectId, domain::BoardObject> objects) = 0;
    virtual void remove_board(const common::BoardId& board_id) = 0;
};

struct IBoardOperationRepository {
    virtual ~IBoardOperationRepository() = default;
    virtual std::vector<domain::AppliedOperation> list_by_board(
        const common::BoardId& board_id) const = 0;
    virtual void append(domain::AppliedOperation operation) = 0;
    virtual void remove_board(const common::BoardId& board_id) = 0;
};

struct IBoardLifecyclePersistence {
    virtual ~IBoardLifecyclePersistence() = default;

    virtual common::Result<std::monostate> create_board_with_owner(
        const domain::Board& board,
        const domain::BoardMember& owner_member) = 0;

    virtual common::Result<bool> upsert_member_and_touch_board(
        const domain::Board& board,
        const domain::BoardMember& member) = 0;

    virtual common::Result<bool> remove_member_and_touch_board(
        const domain::Board& board,
        const common::UserId& user_id) = 0;

    virtual common::Result<bool> delete_board_cascade(
        const common::BoardId& board_id) = 0;
};

struct IBoardRuntimePersistence {
    virtual ~IBoardRuntimePersistence() = default;
    virtual common::Result<std::monostate> persist_applied_operation(
        const domain::Board& board,
        const std::map<common::ObjectId, domain::BoardObject>& objects,
        const domain::AppliedOperation& operation) = 0;
};

}  // namespace online_board::application
