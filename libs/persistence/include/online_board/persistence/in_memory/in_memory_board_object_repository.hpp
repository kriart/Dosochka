#pragma once

#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/in_memory/in_memory_storage.hpp"

namespace online_board::persistence {

class InMemoryBoardObjectRepository final : public application::IBoardObjectRepository {
public:
    InMemoryBoardObjectRepository()
        : storage_(std::make_shared<InMemoryStorage>()) {
    }

    explicit InMemoryBoardObjectRepository(SharedInMemoryStorage storage)
        : storage_(std::move(storage)) {
    }

    std::map<common::ObjectId, domain::BoardObject> load_by_board(
        const common::BoardId& board_id) const override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        auto it = storage_->objects_by_board.find(board_id);
        if (it == storage_->objects_by_board.end()) {
            return {};
        }
        return it->second;
    }

    void replace_for_board(
        const common::BoardId& board_id,
        std::map<common::ObjectId, domain::BoardObject> objects) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        storage_->objects_by_board[board_id] = std::move(objects);
    }

    void remove_board(const common::BoardId& board_id) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        storage_->objects_by_board.erase(board_id);
    }

private:
    SharedInMemoryStorage storage_;
};

}  // namespace online_board::persistence
