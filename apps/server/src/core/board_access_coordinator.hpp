#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "online_board/common/ids.hpp"

namespace online_board::server {

class BoardAccessCoordinator {
public:
    struct SharedAccess {
        std::shared_ptr<std::shared_mutex> mutex;
        std::shared_lock<std::shared_mutex> lock;

        explicit SharedAccess(std::shared_ptr<std::shared_mutex> target)
            : mutex(std::move(target)),
              lock(*mutex) {
        }

        SharedAccess(SharedAccess&&) noexcept = default;
        SharedAccess& operator=(SharedAccess&&) noexcept = default;

        SharedAccess(const SharedAccess&) = delete;
        SharedAccess& operator=(const SharedAccess&) = delete;
    };

    struct ExclusiveAccess {
        std::shared_ptr<std::shared_mutex> mutex;
        std::unique_lock<std::shared_mutex> lock;

        explicit ExclusiveAccess(std::shared_ptr<std::shared_mutex> target)
            : mutex(std::move(target)),
              lock(*mutex) {
        }

        ExclusiveAccess(ExclusiveAccess&&) noexcept = default;
        ExclusiveAccess& operator=(ExclusiveAccess&&) noexcept = default;

        ExclusiveAccess(const ExclusiveAccess&) = delete;
        ExclusiveAccess& operator=(const ExclusiveAccess&) = delete;
    };

    [[nodiscard]] SharedAccess lock_shared(const common::BoardId& board_id);
    [[nodiscard]] ExclusiveAccess lock_exclusive(const common::BoardId& board_id);
    void forget(const common::BoardId& board_id);

private:
    [[nodiscard]] std::shared_ptr<std::shared_mutex> mutex_for(const common::BoardId& board_id);

    std::mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<std::shared_mutex>> board_mutexes_;
};

}  // namespace online_board::server
