#pragma once

#include <string>

namespace online_board::common {

enum class ErrorCode {
    access_denied,
    already_exists,
    conflict,
    internal_error,
    invalid_argument,
    invalid_state,
    not_found
};

struct Error {
    ErrorCode code;
    std::string message;
};

}  // namespace online_board::common
