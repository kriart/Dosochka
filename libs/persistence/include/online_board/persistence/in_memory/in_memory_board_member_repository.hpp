#pragma once

#include <map>
#include <utility>
#include <vector>

#include "online_board/application/repository_interfaces.hpp"

namespace online_board::persistence {

class InMemoryBoardMemberRepository final : public application::IBoardMemberRepository {
public:
    std::optional<domain::BoardMember> find_member(
        const common::BoardId& board_id,
        const common::UserId& user_id) const override {
        auto range = members_.equal_range(board_id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.user_id == user_id) {
                return it->second;
            }
        }
        return std::nullopt;
    }

    std::vector<domain::BoardMember> list_members(const common::BoardId& board_id) const override {
        std::vector<domain::BoardMember> result;
        auto range = members_.equal_range(board_id);
        for (auto it = range.first; it != range.second; ++it) {
            result.push_back(it->second);
        }
        return result;
    }

    std::vector<domain::BoardMember> list_memberships_for_user(const common::UserId& user_id) const override {
        std::vector<domain::BoardMember> result;
        for (const auto& [_, member] : members_) {
            if (member.user_id == user_id) {
                result.push_back(member);
            }
        }
        return result;
    }

    void save(domain::BoardMember member) override {
        auto range = members_.equal_range(member.board_id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.user_id == member.user_id) {
                it->second = std::move(member);
                return;
            }
        }
        members_.emplace(member.board_id, std::move(member));
    }

    bool remove_member(const common::BoardId& board_id, const common::UserId& user_id) override {
        auto range = members_.equal_range(board_id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.user_id == user_id) {
                members_.erase(it);
                return true;
            }
        }
        return false;
    }

    void remove_board(const common::BoardId& board_id) override {
        members_.erase(board_id);
    }

private:
    std::multimap<common::BoardId, domain::BoardMember> members_;
};

}  // namespace online_board::persistence
