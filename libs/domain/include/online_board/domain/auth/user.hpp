#pragma once

#include <string>

#include "online_board/common/clock.hpp"
#include "online_board/common/ids.hpp"

namespace online_board::domain {

struct User {
    common::UserId id;
    std::string login;
    std::string display_name;
    std::string password_hash;
    common::Timestamp created_at;
};

}  // namespace online_board::domain
