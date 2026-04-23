#pragma once

#include <utility>
#include <vector>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/in_memory/in_memory_storage.hpp"

namespace online_board::persistence {

class InMemoryBoardOperationRepository final : public application::IBoardOperationRepository {
public:
    InMemoryBoardOperationRepository()
        : storage_(std::make_shared<InMemoryStorage>()) {
    }

    explicit InMemoryBoardOperationRepository(SharedInMemoryStorage storage)
        : storage_(std::move(storage)) {
    }

    std::vector<domain::AppliedOperation> list_by_board(
        const common::BoardId& board_id) const override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        auto it = storage_->operations_by_board.find(board_id);
        if (it == storage_->operations_by_board.end()) {
            return {};
        }
        return it->second;
    }

    void append(domain::AppliedOperation operation) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        storage_->operations_by_board[operation.board_id].push_back(std::move(operation));
    }

    void remove_board(const common::BoardId& board_id) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        storage_->operations_by_board.erase(board_id);
    }

private:
    SharedInMemoryStorage storage_;
};

}  // namespace online_board::persistence
