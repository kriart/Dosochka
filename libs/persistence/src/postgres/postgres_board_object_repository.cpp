#include "online_board/persistence/postgres/postgres_board_object_repository.hpp"

#include <utility>

#include "postgres_repository_utils.hpp"

namespace online_board::persistence {

PostgresBoardObjectRepository::PostgresBoardObjectRepository(
    SharedPostgresConnectionProvider connection_provider)
    : connection_provider_(std::move(connection_provider)) {
}

std::map<common::ObjectId, domain::BoardObject> PostgresBoardObjectRepository::load_by_board(
    const common::BoardId& board_id) const {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "SELECT object_id, board_id, object_type, created_by_type, created_by_key, "
        "created_by_nickname, created_at_ms, updated_at_ms, is_deleted, z_index, payload_json "
        "FROM board_objects WHERE board_id = $1 ORDER BY z_index, object_id",
        {board_id.value});
    return map_rows_to_map<common::ObjectId, domain::BoardObject>(result.get(), [&](int row) {
        return object_entry_from_result(result.get(), row);
    });
}

void PostgresBoardObjectRepository::replace_for_board(
    const common::BoardId& board_id,
    std::map<common::ObjectId, domain::BoardObject> objects) {
    auto connection = connection_provider_->open();
    exec_params(connection->get(), "DELETE FROM board_objects WHERE board_id = $1", {board_id.value});
    for (const auto& [object_id, object] : objects) {
        exec_params(
            connection->get(),
            "INSERT INTO board_objects "
            "(object_id, board_id, object_type, created_by_type, created_by_key, "
            "created_by_nickname, created_at_ms, updated_at_ms, is_deleted, z_index, payload_json) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11::jsonb)",
            {
                object_id.value,
                object.board_id.value,
                postgres_json::to_string(object.type),
                postgres_json::to_string(domain::principal_type(object.created_by)),
                postgres_json::principal_key_value(object.created_by),
                postgres_json::principal_nickname_value(object.created_by),
                std::to_string(postgres_json::to_unix_millis(object.created_at)),
                std::to_string(postgres_json::to_unix_millis(object.updated_at)),
                object.is_deleted ? "true" : "false",
                std::to_string(object.z_index),
                postgres_json::stringify_json(
                    postgres_json::board_object_payload_to_json(object.payload)),
            });
    }
}

void PostgresBoardObjectRepository::remove_board(const common::BoardId& board_id) {
    auto connection = connection_provider_->open();
    exec_params(connection->get(), "DELETE FROM board_objects WHERE board_id = $1", {board_id.value});
}

std::pair<common::ObjectId, domain::BoardObject> PostgresBoardObjectRepository::object_entry_from_result(
    const PGresult* result,
    int row) {
    const auto object_id = common::ObjectId {field(result, row, "object_id")};
    const auto object_type = postgres_json::object_type_from_string(field(result, row, "object_type"));
    return {
        object_id,
        domain::BoardObject {
            object_id,
            common::BoardId {field(result, row, "board_id")},
            object_type,
            postgres_json::principal_from_columns(
                field(result, row, "created_by_type"),
                field(result, row, "created_by_key"),
                optional_field(result, row, "created_by_nickname")),
            postgres_json::from_unix_millis(int64_field(result, row, "created_at_ms")),
            postgres_json::from_unix_millis(int64_field(result, row, "updated_at_ms")),
            bool_field(result, row, "is_deleted"),
            int64_field(result, row, "z_index"),
            postgres_json::board_object_payload_from_json(
                object_type,
                postgres_json::parse_json_object(field(result, row, "payload_json"))),
        },
    };
}

}  // namespace online_board::persistence
