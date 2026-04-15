#pragma once

#include <string>
#include <variant>

#include "online_board/common/ids.hpp"
#include "online_board/domain/core/enums.hpp"

namespace online_board::domain {

struct RegisteredUserPrincipal {
    common::UserId user_id;
};

struct GuestPrincipal {
    std::string guest_id;
    std::string nickname;
};

using Principal = std::variant<RegisteredUserPrincipal, GuestPrincipal>;

inline PrincipalType principal_type(const Principal& principal) {
    if (std::holds_alternative<RegisteredUserPrincipal>(principal)) {
        return PrincipalType::registered_user;
    }
    return PrincipalType::guest;
}

inline std::string principal_key(const Principal& principal) {
    if (const auto* user = std::get_if<RegisteredUserPrincipal>(&principal)) {
        return "user:" + user->user_id.value;
    }

    const auto& guest = std::get<GuestPrincipal>(principal);
    return "guest:" + guest.guest_id;
}

inline std::string display_name_for(const Principal& principal) {
    if (const auto* user = std::get_if<RegisteredUserPrincipal>(&principal)) {
        return user->user_id.value;
    }

    return std::get<GuestPrincipal>(principal).nickname;
}

}  // namespace online_board::domain
