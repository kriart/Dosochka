#pragma once

#include <optional>
#include <string>

#include "online_board/domain/auth/session.hpp"

namespace online_board::application {

struct ISessionRepository {
    virtual ~ISessionRepository() = default;
    virtual std::optional<domain::AuthSession> find_auth_session_by_token_hash(
        const std::string& token_hash) const = 0;
    virtual void save_auth_session(domain::AuthSession session) = 0;
    virtual void save_guest_session(domain::GuestSession session) = 0;
};

}  // namespace online_board::application
