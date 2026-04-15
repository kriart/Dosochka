#include "online_board/persistence/postgres/postgres_board_repository.hpp"

#include <utility>

#include "postgres_repository_utils.hpp"

namespace online_board::persistence {

PostgresBoardRepository::PostgresBoardRepository(SharedPostgresConnectionProvider connection_provider)
    : connection_provider_(std::move(connection_provider)) {
}

std::optional<domain::Board> PostgresBoardRepository::find_by_id(const common::BoardId& board_id) const {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "SELECT id, owner_user_id, title, access_mode, guest_policy, password_hash, "
        "last_revision, created_at_ms, updated_at_ms "
        "FROM boards WHERE id = $1",
        {board_id.value});
    return map_optional_single_row<domain::Board>(result.get(), [&](int row) {
        return board_from_result(result.get(), row);
    });
}

void PostgresBoardRepository::save(domain::Board board) {
    auto connection = connection_provider_->open();
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
}

bool PostgresBoardRepository::remove(const common::BoardId& board_id) {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "DELETE FROM boards WHERE id = $1",
        {board_id.value});
    return affected_rows(result.get()) > 0;
}

domain::Board PostgresBoardRepository::board_from_result(const PGresult* result, int row) {
    return {
        common::BoardId {field(result, row, "id")},
        common::UserId {field(result, row, "owner_user_id")},
        field(result, row, "title"),
        postgres_json::access_mode_from_string(field(result, row, "access_mode")),
        postgres_json::guest_policy_from_string(field(result, row, "guest_policy")),
        optional_field(result, row, "password_hash"),
        uint64_field(result, row, "last_revision"),
        postgres_json::from_unix_millis(int64_field(result, row, "created_at_ms")),
        postgres_json::from_unix_millis(int64_field(result, row, "updated_at_ms")),
    };
}

}  // namespace online_board::persistence
