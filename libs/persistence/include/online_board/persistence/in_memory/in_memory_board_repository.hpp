#pragma once

#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/in_memory/in_memory_storage.hpp"

namespace online_board::persistence {

class InMemoryBoardRepository final : public application::IBoardRepository {
public:
    InMemoryBoardRepository()
        : storage_(std::make_shared<InMemoryStorage>()) {
    }

    explicit InMemoryBoardRepository(SharedInMemoryStorage storage)
        : storage_(std::move(storage)) {
    }

    std::optional<domain::Board> find_by_id(const common::BoardId& board_id) const override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        auto it = storage_->boards.find(board_id);
        if (it == storage_->boards.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void save(domain::Board board) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        storage_->boards[board.id] = std::move(board);
    }

    bool remove(const common::BoardId& board_id) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        return storage_->boards.erase(board_id) > 0;
    }

private:
    SharedInMemoryStorage storage_;
};

}  // namespace online_board::persistence
