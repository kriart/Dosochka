#pragma once

#include <optional>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/postgres/postgres_connection.hpp"
#include "online_board/persistence/postgres/postgres_json.hpp"

namespace online_board::persistence {

class PostgresUserRepository final : public application::IUserRepository {
public:
    explicit PostgresUserRepository(SharedPostgresConnectionProvider connection_provider);

    std::optional<domain::User> find_by_id(const common::UserId& user_id) const override;

    std::optional<domain::User> find_by_login(const std::string& login) const override;

    void save(domain::User user) override;

private:
    static domain::User user_from_result(const PGresult* result, int row);

    SharedPostgresConnectionProvider connection_provider_;
};

}  // namespace online_board::persistence
