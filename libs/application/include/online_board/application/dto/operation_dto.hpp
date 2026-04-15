#pragma once

#include "online_board/domain/operations/applied_operation.hpp"
#include "online_board/domain/board/board_snapshot.hpp"

namespace online_board::application {

struct ApplyOperationResult {
    domain::BoardSnapshot snapshot;
    domain::AppliedOperation applied_operation;
};

}  // namespace online_board::application
