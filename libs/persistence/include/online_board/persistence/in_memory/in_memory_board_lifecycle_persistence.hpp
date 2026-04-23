#pragma once

#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/in_memory/in_memory_storage.hpp"

namespace online_board::persistence {

class InMemoryBoardLifecyclePersistence final : public application::IBoardLifecyclePersistence {
public:
    InMemoryBoardLifecyclePersistence()
        : storage_(std::make_shared<InMemoryStorage>()) {
    }

    explicit InMemoryBoardLifecyclePersistence(SharedInMemoryStorage storage)
        : storage_(std::move(storage)) {
    }

    common::Result<std::monostate> create_board_with_owner(
        const domain::Board& board,
        const domain::BoardMember& owner_member) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        if (storage_->boards.contains(board.id)) {
            return common::fail<std::monostate>(
                common::ErrorCode::already_exists,
                "Board already exists");
        }

        storage_->boards[board.id] = board;
        storage_->members.emplace(board.id, owner_member);
        storage_->objects_by_board[board.id] = {};
        return std::monostate{};
    }

    common::Result<bool> upsert_member_and_touch_board(
        const domain::Board& board,
        const domain::BoardMember& member) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        if (!storage_->boards.contains(board.id)) {
            return false;
        }

        storage_->boards[board.id] = board;
        auto range = storage_->members.equal_range(member.board_id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.user_id == member.user_id) {
                it->second = member;
                return true;
            }
        }

        storage_->members.emplace(member.board_id, member);
        return true;
    }

    common::Result<bool> remove_member_and_touch_board(
        const domain::Board& board,
        const common::UserId& user_id) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        if (!storage_->boards.contains(board.id)) {
            return false;
        }

        auto range = storage_->members.equal_range(board.id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.user_id == user_id) {
                storage_->members.erase(it);
                storage_->boards[board.id] = board;
                return true;
            }
        }
        return false;
    }

    common::Result<bool> delete_board_cascade(const common::BoardId& board_id) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        const auto removed = storage_->boards.erase(board_id) > 0;
        if (!removed) {
            return false;
        }

        storage_->members.erase(board_id);
        storage_->objects_by_board.erase(board_id);
        storage_->operations_by_board.erase(board_id);
        return true;
    }

private:
    SharedInMemoryStorage storage_;
};

}  // namespace online_board::persistence
