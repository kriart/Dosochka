#pragma once

#include <string>

#include "online_board/domain/auth/principal.hpp"
#include "online_board/domain/auth/session.hpp"
#include "online_board/domain/auth/user.hpp"

namespace online_board::application {

struct RegisterUserRequest {
    std::string login;
    std::string display_name;
    std::string password;
};

struct RegisterUserResponse {
    domain::User user;
    domain::AuthSession session;
    std::string raw_token;
};

struct LoginRequest {
    std::string login;
    std::string password;
};

struct LoginResponse {
    domain::User user;
    domain::AuthSession session;
    std::string raw_token;
};

struct GuestEnterRequest {
    std::string nickname;
};

struct GuestEnterResponse {
    domain::GuestSession session;
    domain::GuestPrincipal principal;
};

}  // namespace online_board::application
