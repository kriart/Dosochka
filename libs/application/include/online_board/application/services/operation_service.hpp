#pragma once

#include <string>

#include "online_board/application/services/board_access_service.hpp"
#include "online_board/application/services/board_state_service.hpp"
#include "online_board/application/dto.hpp"
#include "online_board/common/clock.hpp"

namespace online_board::application {

class OperationService {
public:
    OperationService(
        const BoardAccessService& access_service,
        const BoardStateService& state_service,
        const common::IClock& clock)
        : access_service_(access_service),
          state_service_(state_service),
          clock_(clock) {
    }

    [[nodiscard]] common::Result<ApplyOperationResult> apply(
        const domain::Board& board,
        const domain::BoardSnapshot& snapshot,
        domain::BoardRole actor_role,
        const domain::OperationCommand& command) const {
        if (!access_service_.can_apply_operation(actor_role)) {
            return common::fail<ApplyOperationResult>(
                common::ErrorCode::access_denied,
                "Current role cannot mutate board state");
        }
        if (snapshot.revision != command.base_revision) {
            return common::fail<ApplyOperationResult>(
                common::ErrorCode::conflict,
                "Operation base_revision does not match current board revision");
        }
        if (board.id != snapshot.board_id || board.id != command.board_id) {
            return common::fail<ApplyOperationResult>(
                common::ErrorCode::invalid_argument,
                "Board and snapshot mismatch");
        }

        const auto applied_at = clock_.now();
        auto next_snapshot_result = state_service_.apply(snapshot, command, applied_at);
        if (!common::is_ok(next_snapshot_result)) {
            return common::error(next_snapshot_result);
        }

        auto next_snapshot = common::value(next_snapshot_result);
        next_snapshot.revision = snapshot.revision + 1;

        domain::AppliedOperation operation{
            .operation_id = common::OperationId{board.id.value + ":" + std::to_string(next_snapshot.revision)},
            .board_id = board.id,
            .actor = command.actor,
            .revision = next_snapshot.revision,
            .applied_at = applied_at,
            .payload = command.payload,
        };

        return ApplyOperationResult{
            .snapshot = std::move(next_snapshot),
            .applied_operation = std::move(operation),
        };
    }

private:
    const BoardAccessService& access_service_;
    const BoardStateService& state_service_;
    const common::IClock& clock_;
};

}  // namespace online_board::application
