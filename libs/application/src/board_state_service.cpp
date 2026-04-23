#include "online_board/application/services/board_state_service.hpp"

#include <variant>

#include "board_state_detail.hpp"

namespace online_board::application {

common::Result<domain::BoardSnapshot> BoardStateService::apply(
    domain::BoardSnapshot snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp applied_at) const {
    if (snapshot.board_id != command.board_id) {
        return detail::fail_snapshot(
            common::ErrorCode::invalid_argument,
            "Command board_id does not match snapshot board_id");
    }

    return std::visit(
        [&](const auto& payload) -> common::Result<domain::BoardSnapshot> {
            return detail::apply_payload(snapshot, command, applied_at, payload);
        },
        command.payload);
}

}  // namespace online_board::application
