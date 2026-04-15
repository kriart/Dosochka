#pragma once

#include <string>

#include "online_board/common/clock.hpp"
#include "online_board/common/ids.hpp"

namespace online_board::domain {

struct AuthSession {
    common::SessionId id;
    common::UserId user_id;
    std::string token_hash;
    common::Timestamp created_at;
    common::Timestamp expires_at;
};

struct GuestSession {
    common::GuestSessionId id;
    std::string nickname;
    common::Timestamp created_at;
};

}  // namespace online_board::domain
