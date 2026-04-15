#pragma once

#include "online_board/domain/operations/operation_command.hpp"

namespace online_board::domain {

struct AppliedOperation {
    common::OperationId operation_id;
    common::BoardId board_id;
    Principal actor;
    std::uint64_t revision {0};
    common::Timestamp applied_at;
    OperationPayload payload;
};

}  // namespace online_board::domain
