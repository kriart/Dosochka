#pragma once

#include <utility>
#include <variant>

#include "online_board/application/repository_interfaces.hpp"

namespace online_board::persistence {

class InMemoryBoardRuntimePersistence final : public application::IBoardRuntimePersistence {
public:
    InMemoryBoardRuntimePersistence(
        application::IBoardRepository& board_repository,
        application::IBoardObjectRepository& object_repository,
        application::IBoardOperationRepository& operation_repository)
        : board_repository_(board_repository),
          object_repository_(object_repository),
          operation_repository_(operation_repository) {
    }

    common::Result<std::monostate> persist_applied_operation(
        const domain::Board& board,
        const std::map<common::ObjectId, domain::BoardObject>& objects,
        const domain::AppliedOperation& operation) override {
        board_repository_.save(board);
        object_repository_.replace_for_board(board.id, objects);
        operation_repository_.append(operation);
        return std::monostate{};
    }

private:
    application::IBoardRepository& board_repository_;
    application::IBoardObjectRepository& object_repository_;
    application::IBoardOperationRepository& operation_repository_;
};

}  // namespace online_board::persistence
