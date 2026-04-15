#pragma once

#include <optional>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/postgres/postgres_connection.hpp"
#include "online_board/persistence/postgres/postgres_json.hpp"

namespace online_board::persistence {

class PostgresBoardRepository final : public application::IBoardRepository {
public:
    explicit PostgresBoardRepository(SharedPostgresConnectionProvider connection_provider);

    std::optional<domain::Board> find_by_id(const common::BoardId& board_id) const override;

    void save(domain::Board board) override;

    bool remove(const common::BoardId& board_id) override;

private:
    static domain::Board board_from_result(const PGresult* result, int row);

    SharedPostgresConnectionProvider connection_provider_;
};

}  // namespace online_board::persistence
