#pragma once

#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/postgres/postgres_connection.hpp"
#include "online_board/persistence/postgres/postgres_json.hpp"

namespace online_board::persistence {

class PostgresBoardMemberRepository final : public application::IBoardMemberRepository {
public:
    explicit PostgresBoardMemberRepository(SharedPostgresConnectionProvider connection_provider);

    std::optional<domain::BoardMember> find_member(
        const common::BoardId& board_id,
        const common::UserId& user_id) const override;

    std::vector<domain::BoardMember> list_members(const common::BoardId& board_id) const override;

    std::vector<domain::BoardMember> list_memberships_for_user(const common::UserId& user_id) const override;

    void save(domain::BoardMember member) override;

    bool remove_member(const common::BoardId& board_id, const common::UserId& user_id) override;

    void remove_board(const common::BoardId& board_id) override;

private:
    static domain::BoardMember member_from_result(const PGresult* result, int row);

    SharedPostgresConnectionProvider connection_provider_;
};

}  // namespace online_board::persistence
