#pragma once

#include <mutex>
#include <utility>
#include <variant>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/in_memory/in_memory_storage.hpp"

namespace online_board::persistence {

class InMemoryBoardRuntimePersistence final : public application::IBoardRuntimePersistence {
public:
    explicit InMemoryBoardRuntimePersistence(SharedInMemoryStorage storage)
        : storage_(std::move(storage)) {
    }

    InMemoryBoardRuntimePersistence(
        application::IBoardRepository& board_repository,
        application::IBoardObjectRepository& object_repository,
        application::IBoardOperationRepository& operation_repository)
        : board_repository_(&board_repository),
          object_repository_(&object_repository),
          operation_repository_(&operation_repository) {
    }

    common::Result<std::monostate> persist_applied_operation(
        const domain::Board& board,
        const std::map<common::ObjectId, domain::BoardObject>& objects,
        const domain::AppliedOperation& operation) override {
        if (storage_) {
            std::lock_guard<std::mutex> lock(storage_->mutex);
            storage_->boards[board.id] = board;
            storage_->objects_by_board[board.id] = objects;
            storage_->operations_by_board[operation.board_id].push_back(operation);
            return std::monostate{};
        }
        board_repository_->save(board);
        object_repository_->replace_for_board(board.id, objects);
        operation_repository_->append(operation);
        return std::monostate{};
    }

private:
    SharedInMemoryStorage storage_;
    application::IBoardRepository* board_repository_ {nullptr};
    application::IBoardObjectRepository* object_repository_ {nullptr};
    application::IBoardOperationRepository* operation_repository_ {nullptr};
};

}  // namespace online_board::persistence
