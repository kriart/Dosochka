#pragma once

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/postgres/postgres_connection.hpp"

namespace online_board::persistence {

class PostgresBoardLifecyclePersistence final : public application::IBoardLifecyclePersistence {
public:
    explicit PostgresBoardLifecyclePersistence(SharedPostgresConnectionProvider connection_provider);

    common::Result<std::monostate> create_board_with_owner(
        const domain::Board& board,
        const domain::BoardMember& owner_member) override;

    common::Result<bool> upsert_member_and_touch_board(
        const domain::Board& board,
        const domain::BoardMember& member) override;

    common::Result<bool> remove_member_and_touch_board(
        const domain::Board& board,
        const common::UserId& user_id) override;

    common::Result<bool> delete_board_cascade(const common::BoardId& board_id) override;

private:
    SharedPostgresConnectionProvider connection_provider_;
};

}  // namespace online_board::persistence
