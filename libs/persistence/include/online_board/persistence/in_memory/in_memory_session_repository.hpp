#pragma once

#include <string>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/in_memory/in_memory_storage.hpp"

namespace online_board::persistence {

class InMemorySessionRepository final : public application::ISessionRepository {
public:
    InMemorySessionRepository()
        : storage_(std::make_shared<InMemoryStorage>()) {
    }

    explicit InMemorySessionRepository(SharedInMemoryStorage storage)
        : storage_(std::move(storage)) {
    }

    std::optional<domain::AuthSession> find_auth_session_by_token_hash(
        const std::string& token_hash) const override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        auto it = storage_->sessions_by_token_hash.find(token_hash);
        if (it == storage_->sessions_by_token_hash.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void save_auth_session(domain::AuthSession session) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        storage_->sessions_by_token_hash[session.token_hash] = std::move(session);
    }

    void save_guest_session(domain::GuestSession session) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        storage_->guest_sessions[session.id] = std::move(session);
    }

private:
    SharedInMemoryStorage storage_;
};

}  // namespace online_board::persistence
