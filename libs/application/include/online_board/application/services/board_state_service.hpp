#pragma once

#include "online_board/common/result.hpp"
#include "online_board/domain/operations/operation_command.hpp"

namespace online_board::application {

class BoardStateService {
public:
    [[nodiscard]] common::Result<domain::BoardSnapshot> apply(
        domain::BoardSnapshot snapshot,
        const domain::OperationCommand& command,
        const common::Timestamp applied_at) const;
};

}  // namespace online_board::application
