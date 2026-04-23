#include "board_access_coordinator.hpp"

namespace online_board::server {

std::shared_ptr<std::shared_mutex> BoardAccessCoordinator::mutex_for(const common::BoardId& board_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& weak_mutex = board_mutexes_[board_id.value];
    auto shared_mutex = weak_mutex.lock();
    if (!shared_mutex) {
        shared_mutex = std::make_shared<std::shared_mutex>();
        weak_mutex = shared_mutex;
    }
    return shared_mutex;
}

BoardAccessCoordinator::SharedAccess BoardAccessCoordinator::lock_shared(const common::BoardId& board_id) {
    return SharedAccess{mutex_for(board_id)};
}

BoardAccessCoordinator::ExclusiveAccess BoardAccessCoordinator::lock_exclusive(const common::BoardId& board_id) {
    return ExclusiveAccess{mutex_for(board_id)};
}

void BoardAccessCoordinator::forget(const common::BoardId& board_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    board_mutexes_.erase(board_id.value);
}

}  // namespace online_board::server
