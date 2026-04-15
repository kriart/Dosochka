#include "online_board/runtime/board_runtime_manager.hpp"

#include <utility>

namespace online_board::runtime {

BoardRuntimeManager::BoardRuntimeManager(
    domain::Board board,
    domain::BoardSnapshot snapshot,
    application::OperationService& operation_service,
    application::PresenceService& presence_service,
    const common::IClock& clock,
    application::IBoardRuntimePersistence& runtime_persistence)
    : board_(std::move(board)),
      snapshot_(std::move(snapshot)),
      operation_service_(operation_service),
      presence_service_(presence_service),
      clock_(clock),
      runtime_persistence_(runtime_persistence) {
}

domain::Board BoardRuntimeManager::board() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return board_;
}

domain::BoardSnapshot BoardRuntimeManager::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

std::vector<domain::BoardParticipant> BoardRuntimeManager::active_participants() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_.active_participants;
}

void BoardRuntimeManager::join_participant(const domain::Principal& principal, domain::BoardRole role, std::string nickname) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return;
    }
    snapshot_.active_participants = presence_service_.join(
        std::move(snapshot_.active_participants),
        domain::BoardParticipant{
            .board_id = board_.id,
            .principal = principal,
            .nickname = nickname.empty() ? domain::display_name_for(principal) : std::move(nickname),
            .role = role,
            .joined_at = clock_.now(),
        });
}

void BoardRuntimeManager::leave_participant(const domain::Principal& principal) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.active_participants = presence_service_.leave(
        std::move(snapshot_.active_participants),
        principal);
}

void BoardRuntimeManager::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    snapshot_.active_participants.clear();
}

common::Result<application::ApplyOperationResult> BoardRuntimeManager::apply_operation(
    domain::BoardRole actor_role,
    const domain::OperationCommand& command) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (closed_) {
        return common::fail<application::ApplyOperationResult>(
            common::ErrorCode::not_found,
            "Board was deleted");
    }

    auto result = operation_service_.apply(board_, snapshot_, actor_role, command);
    if (!common::is_ok(result)) {
        return common::error(result);
    }

    auto apply_result = common::value(result);
    auto persisted_snapshot = apply_result.snapshot;
    persisted_snapshot.active_participants = snapshot_.active_participants;

    auto persisted_board = board_;
    persisted_board.last_revision = persisted_snapshot.revision;
    persisted_board.updated_at = apply_result.applied_operation.applied_at;

    auto persist_result = runtime_persistence_.persist_applied_operation(
        persisted_board,
        persisted_snapshot.objects,
        apply_result.applied_operation);
    if (!common::is_ok(persist_result)) {
        return common::error(persist_result);
    }

    snapshot_ = std::move(persisted_snapshot);
    board_ = std::move(persisted_board);
    apply_result.snapshot = snapshot_;

    return apply_result;
}

}  // namespace online_board::runtime
