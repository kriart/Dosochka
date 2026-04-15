#pragma once

#include <map>
#include <string>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"

namespace online_board::persistence {

class InMemorySessionRepository final : public application::ISessionRepository {
public:
    std::optional<domain::AuthSession> find_auth_session_by_token_hash(
        const std::string& token_hash) const override {
        auto it = sessions_by_token_hash_.find(token_hash);
        if (it == sessions_by_token_hash_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void save_auth_session(domain::AuthSession session) override {
        sessions_by_token_hash_[session.token_hash] = std::move(session);
    }

    void save_guest_session(domain::GuestSession session) override {
        guest_sessions_[session.id] = std::move(session);
    }

private:
    std::map<std::string, domain::AuthSession> sessions_by_token_hash_;
    std::map<common::GuestSessionId, domain::GuestSession> guest_sessions_;
};

}  // namespace online_board::persistence
