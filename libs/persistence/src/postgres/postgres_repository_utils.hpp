#pragma once

#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "online_board/persistence/postgres/postgres_connection.hpp"

namespace online_board::persistence {

inline int row_count(const PGresult* result) {
    return PQntuples(result);
}

template <typename T, typename Mapper>
std::optional<T> map_optional_single_row(const PGresult* result, Mapper&& mapper) {
    if (row_count(result) == 0) {
        return std::nullopt;
    }
    return std::forward<Mapper>(mapper)(0);
}

template <typename T, typename Mapper>
std::vector<T> map_rows_to_vector(const PGresult* result, Mapper&& mapper) {
    std::vector<T> values;
    values.reserve(row_count(result));
    for (int row = 0; row < row_count(result); ++row) {
        values.push_back(std::forward<Mapper>(mapper)(row));
    }
    return values;
}

template <typename Key, typename Value, typename Mapper>
std::map<Key, Value> map_rows_to_map(const PGresult* result, Mapper&& mapper) {
    std::map<Key, Value> values;
    for (int row = 0; row < row_count(result); ++row) {
        values.emplace(std::forward<Mapper>(mapper)(row));
    }
    return values;
}

}  // namespace online_board::persistence
