#pragma once

#include "online_board/common/clock.hpp"
#include "online_board/common/ids.hpp"
#include "online_board/domain/core/enums.hpp"

namespace online_board::domain {

struct BoardMember {
    common::BoardId board_id;
    common::UserId user_id;
    BoardRole role {BoardRole::viewer};
    common::Timestamp created_at;
};

}  // namespace online_board::domain
