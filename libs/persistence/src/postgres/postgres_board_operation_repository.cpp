#include "online_board/persistence/postgres/postgres_board_operation_repository.hpp"

#include <utility>

#include "postgres_repository_utils.hpp"

namespace online_board::persistence {

PostgresBoardOperationRepository::PostgresBoardOperationRepository(
    SharedPostgresConnectionProvider connection_provider)
    : connection_provider_(std::move(connection_provider)) {
}

std::vector<domain::AppliedOperation> PostgresBoardOperationRepository::list_by_board(
    const common::BoardId& board_id) const {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "SELECT operation_id, board_id, actor_type, actor_key, actor_nickname, revision, "
        "applied_at_ms, payload_json "
        "FROM board_operations WHERE board_id = $1 ORDER BY revision, applied_at_ms",
        {board_id.value});
    return map_rows_to_vector<domain::AppliedOperation>(result.get(), [&](int row) {
        return operation_from_result(result.get(), row);
    });
}

void PostgresBoardOperationRepository::append(domain::AppliedOperation operation) {
    auto connection = connection_provider_->open();
    exec_params(
        connection->get(),
        "INSERT INTO board_operations "
        "(operation_id, board_id, actor_type, actor_key, actor_nickname, revision, applied_at_ms, payload_json) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8::jsonb)",
        {
            operation.operation_id.value,
            operation.board_id.value,
            postgres_json::to_string(domain::principal_type(operation.actor)),
            postgres_json::principal_key_value(operation.actor),
            postgres_json::principal_nickname_value(operation.actor),
            std::to_string(operation.revision),
            std::to_string(postgres_json::to_unix_millis(operation.applied_at)),
            postgres_json::stringify_json(
                postgres_json::operation_payload_to_json(operation.payload)),
            });
}

void PostgresBoardOperationRepository::remove_board(const common::BoardId& board_id) {
    auto connection = connection_provider_->open();
    exec_params(
        connection->get(),
        "DELETE FROM board_operations WHERE board_id = $1",
        {board_id.value});
}

domain::AppliedOperation PostgresBoardOperationRepository::operation_from_result(
    const PGresult* result,
    int row) {
    return {
        common::OperationId {field(result, row, "operation_id")},
        common::BoardId {field(result, row, "board_id")},
        postgres_json::principal_from_columns(
            field(result, row, "actor_type"),
            field(result, row, "actor_key"),
            optional_field(result, row, "actor_nickname")),
        uint64_field(result, row, "revision"),
        postgres_json::from_unix_millis(int64_field(result, row, "applied_at_ms")),
        postgres_json::operation_payload_from_json(
            postgres_json::parse_json_object(field(result, row, "payload_json"))),
    };
}

}  // namespace online_board::persistence
