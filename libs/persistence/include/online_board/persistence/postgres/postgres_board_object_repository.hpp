#pragma once

#include <map>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/postgres/postgres_connection.hpp"
#include "online_board/persistence/postgres/postgres_json.hpp"

namespace online_board::persistence {

class PostgresBoardObjectRepository final : public application::IBoardObjectRepository {
public:
    explicit PostgresBoardObjectRepository(SharedPostgresConnectionProvider connection_provider);

    std::map<common::ObjectId, domain::BoardObject> load_by_board(
        const common::BoardId& board_id) const override;

    void replace_for_board(
        const common::BoardId& board_id,
        std::map<common::ObjectId, domain::BoardObject> objects) override;

    void remove_board(const common::BoardId& board_id) override;

private:
    static std::pair<common::ObjectId, domain::BoardObject> object_entry_from_result(const PGresult* result, int row);

    SharedPostgresConnectionProvider connection_provider_;
};

}  // namespace online_board::persistence
