#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "online_board/application/services/presence_service.hpp"
#include "online_board/common/clock.hpp"
#include "online_board/runtime/board_runtime_manager.hpp"

namespace online_board::runtime {

class BoardRegistry {
public:
    std::shared_ptr<BoardRuntimeManager> get_or_create(
        const domain::Board& board,
        const domain::BoardSnapshot& snapshot,
        application::OperationService& operation_service,
        application::PresenceService& presence_service,
        const common::IClock& clock,
        application::IBoardRuntimePersistence& runtime_persistence);

    std::shared_ptr<BoardRuntimeManager> remove(const common::BoardId& board_id);

private:
    std::mutex mutex_;
    std::map<common::BoardId, std::shared_ptr<BoardRuntimeManager>> runtimes_;
};

}  // namespace online_board::runtime
