#pragma once

#include <map>
#include <vector>

#include "online_board/domain/board/board_object.hpp"
#include "online_board/domain/board/board_participant.hpp"

namespace online_board::domain {

struct BoardSnapshot {
    common::BoardId board_id;
    std::uint64_t revision {0};
    std::map<common::ObjectId, BoardObject> objects;
    std::vector<BoardParticipant> active_participants;
};

}  // namespace online_board::domain
