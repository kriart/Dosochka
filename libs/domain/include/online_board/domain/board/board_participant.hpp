#pragma once

#include <string>

#include "online_board/common/clock.hpp"
#include "online_board/common/ids.hpp"
#include "online_board/domain/core/enums.hpp"
#include "online_board/domain/auth/principal.hpp"

namespace online_board::domain {

struct BoardParticipant {
    common::BoardId board_id;
    Principal principal;
    std::string nickname;
    BoardRole role {BoardRole::viewer};
    common::Timestamp joined_at;
};

}  // namespace online_board::domain
