#pragma once

#include <chrono>
#include <shared_mutex>

#include "online_board/application/dto.hpp"
#include "online_board/application/repository_interfaces.hpp"
#include "online_board/application/security_interfaces.hpp"
#include "online_board/common/result.hpp"

namespace online_board::application {

class AuthService {
public:
    AuthService(
        IUserRepository& user_repository,
        ISessionRepository& session_repository,
        const IPasswordHasher& password_hasher,
        ITokenGenerator& token_generator,
        const IIdGenerator& id_generator,
        const common::IClock& clock);

    [[nodiscard]] common::Result<RegisterUserResponse> register_user(const RegisterUserRequest& request);
    [[nodiscard]] common::Result<LoginResponse> login(const LoginRequest& request);
    [[nodiscard]] common::Result<GuestEnterResponse> enter_guest(const GuestEnterRequest& request);
    [[nodiscard]] common::Result<domain::AuthSession> restore_session(const std::string& raw_token) const;

private:
    IUserRepository& user_repository_;
    ISessionRepository& session_repository_;
    const IPasswordHasher& password_hasher_;
    ITokenGenerator& token_generator_;
    const IIdGenerator& id_generator_;
    const common::IClock& clock_;
    mutable std::shared_mutex mutex_;
};

}  // namespace online_board::application
