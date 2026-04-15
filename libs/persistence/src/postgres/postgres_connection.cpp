#include "online_board/persistence/postgres/postgres_connection.hpp"

#include <stdexcept>
#include <utility>

namespace online_board::persistence {

PostgresConnection::PostgresConnection(const std::string& connection_string)
    : connection_(PQconnectdb(connection_string.c_str())) {
    if (!connection_ || PQstatus(connection_) != CONNECTION_OK) {
        const std::string message = connection_ ? PQerrorMessage(connection_) : "Unable to allocate PGconn";
        if (connection_) {
            PQfinish(connection_);
            connection_ = nullptr;
        }
        throw std::runtime_error("PostgreSQL connection failed: " + message);
    }
}

PostgresConnection::~PostgresConnection() {
    if (connection_) {
        PQfinish(connection_);
    }
}

PostgresConnection::PostgresConnection(PostgresConnection&& other) noexcept
    : connection_(std::exchange(other.connection_, nullptr)) {
}

PostgresConnection& PostgresConnection::operator=(PostgresConnection&& other) noexcept {
    if (this != &other) {
        if (connection_) {
            PQfinish(connection_);
        }
        connection_ = std::exchange(other.connection_, nullptr);
    }
    return *this;
}

PGconn* PostgresConnection::get() const noexcept {
    return connection_;
}

PostgresConnectionProvider::PostgresConnectionProvider(std::string connection_string)
    : connection_string_(std::move(connection_string)) {
}

std::unique_ptr<PostgresConnection> PostgresConnectionProvider::open() const {
    return std::make_unique<PostgresConnection>(connection_string_);
}

PostgresResult make_result(PGresult* result) {
    return PostgresResult(result, &PQclear);
}

std::vector<const char*> param_values(const std::vector<std::optional<std::string>>& params) {
    std::vector<const char*> values;
    values.reserve(params.size());
    for (const auto& param : params) {
        values.push_back(param ? param->c_str() : nullptr);
    }
    return values;
}

PostgresResult exec_params(
    PGconn* connection,
    const char* sql,
    const std::vector<std::optional<std::string>>& params) {
    const auto values = param_values(params);
    auto result = make_result(PQexecParams(
        connection,
        sql,
        static_cast<int>(values.size()),
        nullptr,
        values.data(),
        nullptr,
        nullptr,
        0));

    const auto status = PQresultStatus(result.get());
    if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
        throw std::runtime_error(PQerrorMessage(connection));
    }
    return result;
}

int column_index(const PGresult* result, const char* column_name) {
    const auto index = PQfnumber(result, column_name);
    if (index < 0) {
        throw std::runtime_error(std::string("Column not found: ") + column_name);
    }
    return index;
}

bool is_null(const PGresult* result, int row, const char* column_name) {
    return PQgetisnull(result, row, column_index(result, column_name)) != 0;
}

std::string field(const PGresult* result, int row, const char* column_name) {
    const auto index = column_index(result, column_name);
    return PQgetvalue(result, row, index);
}

std::optional<std::string> optional_field(const PGresult* result, int row, const char* column_name) {
    if (is_null(result, row, column_name)) {
        return std::nullopt;
    }
    return field(result, row, column_name);
}

std::int64_t int64_field(const PGresult* result, int row, const char* column_name) {
    return std::stoll(field(result, row, column_name));
}

std::uint64_t uint64_field(const PGresult* result, int row, const char* column_name) {
    return static_cast<std::uint64_t>(std::stoull(field(result, row, column_name)));
}

bool bool_field(const PGresult* result, int row, const char* column_name) {
    const auto value = field(result, row, column_name);
    return value == "t" || value == "true" || value == "1";
}

long long affected_rows(PGresult* result) {
    return std::atoll(PQcmdTuples(result));
}

}  // namespace online_board::persistence
