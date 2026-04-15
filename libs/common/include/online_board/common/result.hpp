#pragma once

#include <utility>
#include <variant>

#include "online_board/common/error.hpp"

namespace online_board::common {

template <typename T>
using Result = std::variant<T, Error>;

template <typename T>
inline bool is_ok(const Result<T>& result) {
    return std::holds_alternative<T>(result);
}

template <typename T>
inline const T& value(const Result<T>& result) {
    return std::get<T>(result);
}

template <typename T>
inline T& value(Result<T>& result) {
    return std::get<T>(result);
}

template <typename T>
inline const Error& error(const Result<T>& result) {
    return std::get<Error>(result);
}

template <typename T>
inline Result<T> fail(ErrorCode code, std::string message) {
    return Error{code, std::move(message)};
}

}  // namespace online_board::common
