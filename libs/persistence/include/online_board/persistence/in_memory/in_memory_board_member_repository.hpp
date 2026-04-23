#pragma once

#include <utility>
#include <vector>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/in_memory/in_memory_storage.hpp"

namespace online_board::persistence {

class InMemoryBoardMemberRepository final : public application::IBoardMemberRepository {
public:
    InMemoryBoardMemberRepository()
        : storage_(std::make_shared<InMemoryStorage>()) {
    }

    explicit InMemoryBoardMemberRepository(SharedInMemoryStorage storage)
        : storage_(std::move(storage)) {
    }

    std::optional<domain::BoardMember> find_member(
        const common::BoardId& board_id,
        const common::UserId& user_id) const override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        auto range = storage_->members.equal_range(board_id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.user_id == user_id) {
                return it->second;
            }
        }
        return std::nullopt;
    }

    std::vector<domain::BoardMember> list_members(const common::BoardId& board_id) const override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        std::vector<domain::BoardMember> result;
        auto range = storage_->members.equal_range(board_id);
        for (auto it = range.first; it != range.second; ++it) {
            result.push_back(it->second);
        }
        return result;
    }

    std::vector<domain::BoardMember> list_memberships_for_user(const common::UserId& user_id) const override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        std::vector<domain::BoardMember> result;
        for (const auto& [_, member] : storage_->members) {
            if (member.user_id == user_id) {
                result.push_back(member);
            }
        }
        return result;
    }

    void save(domain::BoardMember member) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        auto range = storage_->members.equal_range(member.board_id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.user_id == member.user_id) {
                it->second = std::move(member);
                return;
            }
        }
        storage_->members.emplace(member.board_id, std::move(member));
    }

    bool remove_member(const common::BoardId& board_id, const common::UserId& user_id) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        auto range = storage_->members.equal_range(board_id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.user_id == user_id) {
                storage_->members.erase(it);
                return true;
            }
        }
        return false;
    }

    void remove_board(const common::BoardId& board_id) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        storage_->members.erase(board_id);
    }

private:
    SharedInMemoryStorage storage_;
};

}  // namespace online_board::persistence
