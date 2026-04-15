#include "online_board/runtime/board_registry.hpp"

namespace online_board::runtime {

std::shared_ptr<BoardRuntimeManager> BoardRegistry::get_or_create(
    const domain::Board& board,
    const domain::BoardSnapshot& snapshot,
    application::OperationService& operation_service,
    application::PresenceService& presence_service,
    const common::IClock& clock,
    application::IBoardRuntimePersistence& runtime_persistence) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = runtimes_.find(board.id);
    if (it != runtimes_.end()) {
        return it->second;
    }

    auto runtime = std::make_shared<BoardRuntimeManager>(
        board,
        snapshot,
        operation_service,
        presence_service,
        clock,
        runtime_persistence);
    runtimes_.emplace(board.id, runtime);
    return runtime;
}

std::shared_ptr<BoardRuntimeManager> BoardRegistry::remove(const common::BoardId& board_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = runtimes_.find(board_id);
    if (it == runtimes_.end()) {
        return {};
    }

    auto runtime = it->second;
    runtimes_.erase(it);
    runtime->close();
    return runtime;
}

}  // namespace online_board::runtime
