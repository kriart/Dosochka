#pragma once

#include <map>
#include <memory>
#include <variant>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/postgres/postgres_connection.hpp"

namespace online_board::persistence {

class PostgresBoardRuntimePersistence final : public application::IBoardRuntimePersistence {
public:
    explicit PostgresBoardRuntimePersistence(SharedPostgresConnectionProvider connection_provider);

    common::Result<std::monostate> persist_applied_operation(
        const domain::Board& board,
        const std::map<common::ObjectId, domain::BoardObject>& objects,
        const domain::AppliedOperation& operation) override;

private:
    SharedPostgresConnectionProvider connection_provider_;
};

}  // namespace online_board::persistence
