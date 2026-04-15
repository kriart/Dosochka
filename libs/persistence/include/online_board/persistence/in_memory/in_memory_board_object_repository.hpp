#pragma once

#include <map>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"

namespace online_board::persistence {

class InMemoryBoardObjectRepository final : public application::IBoardObjectRepository {
public:
    std::map<common::ObjectId, domain::BoardObject> load_by_board(
        const common::BoardId& board_id) const override {
        auto it = objects_by_board_.find(board_id);
        if (it == objects_by_board_.end()) {
            return {};
        }
        return it->second;
    }

    void replace_for_board(
        const common::BoardId& board_id,
        std::map<common::ObjectId, domain::BoardObject> objects) override {
        objects_by_board_[board_id] = std::move(objects);
    }

    void remove_board(const common::BoardId& board_id) override {
        objects_by_board_.erase(board_id);
    }

private:
    std::map<common::BoardId, std::map<common::ObjectId, domain::BoardObject>> objects_by_board_;
};

}  // namespace online_board::persistence
