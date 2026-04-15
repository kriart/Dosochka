#include "online_board/persistence/postgres/postgres_session_repository.hpp"

#include <utility>

#include "postgres_repository_utils.hpp"

namespace online_board::persistence {

PostgresSessionRepository::PostgresSessionRepository(SharedPostgresConnectionProvider connection_provider)
    : connection_provider_(std::move(connection_provider)) {
}

std::optional<domain::AuthSession> PostgresSessionRepository::find_auth_session_by_token_hash(
    const std::string& token_hash) const {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "SELECT id, user_id, token_hash, created_at_ms, expires_at_ms "
        "FROM auth_sessions WHERE token_hash = $1",
        {token_hash});
    return map_optional_single_row<domain::AuthSession>(result.get(), [&](int row) {
        return auth_session_from_result(result.get(), row);
    });
}

void PostgresSessionRepository::save_auth_session(domain::AuthSession session) {
    auto connection = connection_provider_->open();
    exec_params(
        connection->get(),
        "INSERT INTO auth_sessions (id, user_id, token_hash, created_at_ms, expires_at_ms) "
        "VALUES ($1, $2, $3, $4, $5) "
        "ON CONFLICT (id) DO UPDATE SET "
        "user_id = EXCLUDED.user_id, "
        "token_hash = EXCLUDED.token_hash, "
        "created_at_ms = EXCLUDED.created_at_ms, "
        "expires_at_ms = EXCLUDED.expires_at_ms",
        {
            session.id.value,
            session.user_id.value,
            session.token_hash,
            std::to_string(postgres_json::to_unix_millis(session.created_at)),
            std::to_string(postgres_json::to_unix_millis(session.expires_at)),
        });
}

void PostgresSessionRepository::save_guest_session(domain::GuestSession session) {
    auto connection = connection_provider_->open();
    exec_params(
        connection->get(),
        "INSERT INTO guest_sessions (id, nickname, created_at_ms) "
        "VALUES ($1, $2, $3) "
        "ON CONFLICT (id) DO UPDATE SET "
        "nickname = EXCLUDED.nickname, "
        "created_at_ms = EXCLUDED.created_at_ms",
        {
            session.id.value,
            session.nickname,
            std::to_string(postgres_json::to_unix_millis(session.created_at)),
        });
}

domain::AuthSession PostgresSessionRepository::auth_session_from_result(const PGresult* result, int row) {
    return {
        common::SessionId {field(result, row, "id")},
        common::UserId {field(result, row, "user_id")},
        field(result, row, "token_hash"),
        postgres_json::from_unix_millis(int64_field(result, row, "created_at_ms")),
        postgres_json::from_unix_millis(int64_field(result, row, "expires_at_ms")),
    };
}

}  // namespace online_board::persistence
