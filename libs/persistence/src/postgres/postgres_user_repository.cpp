#include "online_board/persistence/postgres/postgres_user_repository.hpp"

#include <utility>

#include "postgres_repository_utils.hpp"

namespace online_board::persistence {

PostgresUserRepository::PostgresUserRepository(SharedPostgresConnectionProvider connection_provider)
    : connection_provider_(std::move(connection_provider)) {
}

std::optional<domain::User> PostgresUserRepository::find_by_id(const common::UserId& user_id) const {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "SELECT id, login, display_name, password_hash, created_at_ms "
        "FROM users WHERE id = $1",
        {user_id.value});
    return map_optional_single_row<domain::User>(result.get(), [&](int row) {
        return user_from_result(result.get(), row);
    });
}

std::optional<domain::User> PostgresUserRepository::find_by_login(const std::string& login) const {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "SELECT id, login, display_name, password_hash, created_at_ms "
        "FROM users WHERE login = $1",
        {login});
    return map_optional_single_row<domain::User>(result.get(), [&](int row) {
        return user_from_result(result.get(), row);
    });
}

void PostgresUserRepository::save(domain::User user) {
    auto connection = connection_provider_->open();
    exec_params(
        connection->get(),
        "INSERT INTO users (id, login, display_name, password_hash, created_at_ms) "
        "VALUES ($1, $2, $3, $4, $5) "
        "ON CONFLICT (id) DO UPDATE SET "
        "login = EXCLUDED.login, "
        "display_name = EXCLUDED.display_name, "
        "password_hash = EXCLUDED.password_hash, "
        "created_at_ms = EXCLUDED.created_at_ms",
        {
            user.id.value,
            user.login,
            user.display_name,
            user.password_hash,
            std::to_string(postgres_json::to_unix_millis(user.created_at)),
        });
}

domain::User PostgresUserRepository::user_from_result(const PGresult* result, int row) {
    return {
        common::UserId {field(result, row, "id")},
        field(result, row, "login"),
        field(result, row, "display_name"),
        field(result, row, "password_hash"),
        postgres_json::from_unix_millis(int64_field(result, row, "created_at_ms")),
    };
}

}  // namespace online_board::persistence
