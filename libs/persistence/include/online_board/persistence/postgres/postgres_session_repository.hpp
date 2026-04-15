#pragma once

#include <optional>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/postgres/postgres_connection.hpp"
#include "online_board/persistence/postgres/postgres_json.hpp"

namespace online_board::persistence {

class PostgresSessionRepository final : public application::ISessionRepository {
public:
    explicit PostgresSessionRepository(SharedPostgresConnectionProvider connection_provider);

    std::optional<domain::AuthSession> find_auth_session_by_token_hash(
        const std::string& token_hash) const override;

    void save_auth_session(domain::AuthSession session) override;

    void save_guest_session(domain::GuestSession session) override;

private:
    static domain::AuthSession auth_session_from_result(const PGresult* result, int row);

    SharedPostgresConnectionProvider connection_provider_;
};

}  // namespace online_board::persistence
