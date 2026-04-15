#pragma once

#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/postgres/postgres_connection.hpp"
#include "online_board/persistence/postgres/postgres_json.hpp"

namespace online_board::persistence {

class PostgresBoardOperationRepository final : public application::IBoardOperationRepository {
public:
    explicit PostgresBoardOperationRepository(SharedPostgresConnectionProvider connection_provider);

    std::vector<domain::AppliedOperation> list_by_board(
        const common::BoardId& board_id) const override;

    void append(domain::AppliedOperation operation) override;

    void remove_board(const common::BoardId& board_id) override;

private:
    static domain::AppliedOperation operation_from_result(const PGresult* result, int row);

    SharedPostgresConnectionProvider connection_provider_;
};

}  // namespace online_board::persistence
