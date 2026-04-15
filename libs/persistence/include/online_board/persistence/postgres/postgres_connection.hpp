#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <libpq-fe.h>

namespace online_board::persistence {

class PostgresConnection {
public:
    explicit PostgresConnection(const std::string& connection_string);
    ~PostgresConnection();

    PostgresConnection(const PostgresConnection&) = delete;
    PostgresConnection& operator=(const PostgresConnection&) = delete;

    PostgresConnection(PostgresConnection&& other) noexcept;
    PostgresConnection& operator=(PostgresConnection&& other) noexcept;

    [[nodiscard]] PGconn* get() const noexcept;

private:
    PGconn* connection_ {nullptr};
};

class PostgresConnectionProvider {
public:
    explicit PostgresConnectionProvider(std::string connection_string);

    [[nodiscard]] std::unique_ptr<PostgresConnection> open() const;

private:
    std::string connection_string_;
};

using SharedPostgresConnectionProvider = std::shared_ptr<PostgresConnectionProvider>;

using PostgresResult = std::unique_ptr<PGresult, decltype(&PQclear)>;

PostgresResult make_result(PGresult* result);

std::vector<const char*> param_values(const std::vector<std::optional<std::string>>& params);

PostgresResult exec_params(
    PGconn* connection,
    const char* sql,
    const std::vector<std::optional<std::string>>& params);

int column_index(const PGresult* result, const char* column_name);
bool is_null(const PGresult* result, int row, const char* column_name);
std::string field(const PGresult* result, int row, const char* column_name);
std::optional<std::string> optional_field(const PGresult* result, int row, const char* column_name);
std::int64_t int64_field(const PGresult* result, int row, const char* column_name);
std::uint64_t uint64_field(const PGresult* result, int row, const char* column_name);
bool bool_field(const PGresult* result, int row, const char* column_name);
long long affected_rows(PGresult* result);

}  // namespace online_board::persistence
