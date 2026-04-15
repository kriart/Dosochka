#pragma once

#include <optional>
#include <string>

#include "online_board/common/clock.hpp"
#include "online_board/common/ids.hpp"
#include "online_board/domain/board/board_member.hpp"
#include "online_board/domain/board/board_participant.hpp"
#include "online_board/domain/core/enums.hpp"

namespace online_board::domain {

struct Board {
    common::BoardId id;
    common::UserId owner_user_id;
    std::string title;
    AccessMode access_mode {AccessMode::private_board};
    GuestPolicy guest_policy {GuestPolicy::guest_disabled};
    std::optional<std::string> password_hash;
    std::uint64_t last_revision {0};
    common::Timestamp created_at;
    common::Timestamp updated_at;
};

}  // namespace online_board::domain
