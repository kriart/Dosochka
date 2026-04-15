#pragma once

#include <map>
#include <utility>
#include <vector>

#include "online_board/application/repository_interfaces.hpp"

namespace online_board::persistence {

class InMemoryBoardOperationRepository final : public application::IBoardOperationRepository {
public:
    std::vector<domain::AppliedOperation> list_by_board(
        const common::BoardId& board_id) const override {
        auto it = operations_by_board_.find(board_id);
        if (it == operations_by_board_.end()) {
            return {};
        }
        return it->second;
    }

    void append(domain::AppliedOperation operation) override {
        operations_by_board_[operation.board_id].push_back(std::move(operation));
    }

    void remove_board(const common::BoardId& board_id) override {
        operations_by_board_.erase(board_id);
    }

private:
    std::map<common::BoardId, std::vector<domain::AppliedOperation>> operations_by_board_;
};

}  // namespace online_board::persistence
