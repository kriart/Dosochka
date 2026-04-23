#include "online_board/application/services/auth_service.hpp"

#include <chrono>
#include <utility>

namespace online_board::application {

AuthService::AuthService(
    IUserRepository& user_repository,
    ISessionRepository& session_repository,
    const IPasswordHasher& password_hasher,
    ITokenGenerator& token_generator,
    const IIdGenerator& id_generator,
    const common::IClock& clock)
    : user_repository_(user_repository),
      session_repository_(session_repository),
      password_hasher_(password_hasher),
      token_generator_(token_generator),
      id_generator_(id_generator),
      clock_(clock) {
}

common::Result<RegisterUserResponse> AuthService::register_user(const RegisterUserRequest& request) {
    std::unique_lock lock(mutex_);
    const auto now = clock_.now();
    domain::User user{
        .id = common::UserId{id_generator_.next_id()},
        .login = request.login,
        .display_name = request.display_name,
        .password_hash = password_hasher_.hash(request.password),
        .created_at = now,
    };
    const auto create_result = user_repository_.create(user);
    if (!common::is_ok(create_result)) {
        return common::error(create_result);
    }

    const auto raw_token = token_generator_.next_token();
    domain::AuthSession session{
        .id = common::SessionId{id_generator_.next_id()},
        .user_id = user.id,
        .token_hash = password_hasher_.hash(raw_token),
        .created_at = now,
        .expires_at = now + std::chrono::hours(24),
    };
    session_repository_.save_auth_session(session);

    return RegisterUserResponse{
        .user = std::move(user),
        .session = std::move(session),
        .raw_token = raw_token,
    };
}

common::Result<LoginResponse> AuthService::login(const LoginRequest& request) {
    std::unique_lock lock(mutex_);
    const auto user = user_repository_.find_by_login(request.login);
    if (!user.has_value()) {
        return common::fail<LoginResponse>(
            common::ErrorCode::not_found,
            "User was not found");
    }
    if (!password_hasher_.verify(request.password, user->password_hash)) {
        return common::fail<LoginResponse>(
            common::ErrorCode::access_denied,
            "Invalid login or password");
    }

    const auto now = clock_.now();
    const auto raw_token = token_generator_.next_token();
    domain::AuthSession session{
        .id = common::SessionId{id_generator_.next_id()},
        .user_id = user->id,
        .token_hash = password_hasher_.hash(raw_token),
        .created_at = now,
        .expires_at = now + std::chrono::hours(24),
    };
    session_repository_.save_auth_session(session);

    return LoginResponse{
        .user = *user,
        .session = std::move(session),
        .raw_token = raw_token,
    };
}

common::Result<GuestEnterResponse> AuthService::enter_guest(const GuestEnterRequest& request) {
    std::unique_lock lock(mutex_);
    if (request.nickname.empty()) {
        return common::fail<GuestEnterResponse>(
            common::ErrorCode::invalid_argument,
            "Guest nickname must not be empty");
    }

    const auto now = clock_.now();
    domain::GuestSession session{
        .id = common::GuestSessionId{id_generator_.next_id()},
        .nickname = request.nickname,
        .created_at = now,
    };
    session_repository_.save_guest_session(session);

    return GuestEnterResponse{
        .session = session,
        .principal = domain::GuestPrincipal{
            .guest_id = session.id.value,
            .nickname = session.nickname,
        },
    };
}

common::Result<domain::AuthSession> AuthService::restore_session(const std::string& raw_token) const {
    std::shared_lock lock(mutex_);
    const auto session = session_repository_.find_auth_session_by_token_hash(password_hasher_.hash(raw_token));
    if (!session.has_value()) {
        return common::fail<domain::AuthSession>(
            common::ErrorCode::not_found,
            "Session was not found");
    }
    if (session->expires_at < clock_.now()) {
        return common::fail<domain::AuthSession>(
            common::ErrorCode::access_denied,
            "Session expired");
    }
    return *session;
}

}  // namespace online_board::application
