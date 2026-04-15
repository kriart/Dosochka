#pragma once

#include <map>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"

namespace online_board::persistence {

class InMemoryBoardRepository final : public application::IBoardRepository {
public:
    std::optional<domain::Board> find_by_id(const common::BoardId& board_id) const override {
        auto it = boards_.find(board_id);
        if (it == boards_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void save(domain::Board board) override {
        boards_[board.id] = std::move(board);
    }

    bool remove(const common::BoardId& board_id) override {
        return boards_.erase(board_id) > 0;
    }

private:
    std::map<common::BoardId, domain::Board> boards_;
};

}  // namespace online_board::persistence
