#include "online_board/persistence/postgres/postgres_board_runtime_persistence.hpp"

#include <exception>
#include <utility>

#include "online_board/persistence/postgres/postgres_json.hpp"

namespace online_board::persistence {

namespace {

void rollback_noexcept(PGconn* connection) {
    if (connection == nullptr) {
        return;
    }
    auto rollback_result = make_result(PQexec(connection, "ROLLBACK"));
    (void)rollback_result;
}

}  // namespace

PostgresBoardRuntimePersistence::PostgresBoardRuntimePersistence(
    SharedPostgresConnectionProvider connection_provider)
    : connection_provider_(std::move(connection_provider)) {
}

common::Result<std::monostate> PostgresBoardRuntimePersistence::persist_applied_operation(
    const domain::Board& board,
    const std::map<common::ObjectId, domain::BoardObject>& objects,
    const domain::AppliedOperation& operation) {
    try {
        auto connection = connection_provider_->open();
        exec_params(connection->get(), "BEGIN", {});

        try {
            exec_params(
                connection->get(),
                "INSERT INTO boards (id, owner_user_id, title, access_mode, guest_policy, "
                "password_hash, last_revision, created_at_ms, updated_at_ms) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) "
                "ON CONFLICT (id) DO UPDATE SET "
                "owner_user_id = EXCLUDED.owner_user_id, "
                "title = EXCLUDED.title, "
                "access_mode = EXCLUDED.access_mode, "
                "guest_policy = EXCLUDED.guest_policy, "
                "password_hash = EXCLUDED.password_hash, "
                "last_revision = EXCLUDED.last_revision, "
                "created_at_ms = EXCLUDED.created_at_ms, "
                "updated_at_ms = EXCLUDED.updated_at_ms",
                {
                    board.id.value,
                    board.owner_user_id.value,
                    board.title,
                    postgres_json::to_string(board.access_mode),
                    postgres_json::to_string(board.guest_policy),
                    board.password_hash,
                    std::to_string(board.last_revision),
                    std::to_string(postgres_json::to_unix_millis(board.created_at)),
                    std::to_string(postgres_json::to_unix_millis(board.updated_at)),
                });

            exec_params(
                connection->get(),
                "DELETE FROM board_objects WHERE board_id = $1",
                {board.id.value});

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

            exec_params(connection->get(), "COMMIT", {});
        } catch (...) {
            rollback_noexcept(connection->get());
            throw;
        }

        return std::monostate{};
    } catch (const std::exception& exception) {
        return common::fail<std::monostate>(
            common::ErrorCode::internal_error,
            exception.what());
    }
}

}  // namespace online_board::persistence
