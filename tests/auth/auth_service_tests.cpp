#include <chrono>

#include <gtest/gtest.h>

#include "online_board/application/services/auth_service.hpp"
#include "online_board/common/result.hpp"
#include "online_board/persistence/in_memory_repositories.hpp"
#include "support/test_support.hpp"

namespace {

TEST(AuthServiceTests, RegisterCreatesUserAndSession) {
    using namespace online_board;

    persistence::InMemoryUserRepository user_repository;
    persistence::InMemorySessionRepository session_repository;
    FakePasswordHasher hasher;
    SequentialTokenGenerator token_generator;
    SequentialIdGenerator id_generator;
    FakeClock clock;

    application::AuthService service(
        user_repository,
        session_repository,
        hasher,
        token_generator,
        id_generator,
        clock);

    auto result = service.register_user({
        .login = "alice",
        .display_name = "Alice",
        .password = "secret",
    });

    ASSERT_TRUE(common::is_ok(result));
    const auto& response = common::value(result);
    EXPECT_EQ(response.user.login, "alice");
    EXPECT_EQ(response.raw_token, "token-1");
    EXPECT_EQ(response.session.token_hash, "hash:token-1");
}

TEST(AuthServiceTests, LoginCreatesNewSessionForExistingUser) {
    using namespace online_board;

    persistence::InMemoryUserRepository user_repository;
    persistence::InMemorySessionRepository session_repository;
    FakePasswordHasher hasher;
    SequentialTokenGenerator token_generator;
    SequentialIdGenerator id_generator;
    FakeClock clock;

    user_repository.save(domain::User{
        .id = common::UserId{"user-1"},
        .login = "alice",
        .display_name = "Alice",
        .password_hash = hasher.hash("secret"),
        .created_at = clock.now(),
    });

    application::AuthService service(
        user_repository,
        session_repository,
        hasher,
        token_generator,
        id_generator,
        clock);

    auto result = service.login({
        .login = "alice",
        .password = "secret",
    });

    ASSERT_TRUE(common::is_ok(result));
    EXPECT_EQ(common::value(result).session.user_id.value, "user-1");
}

TEST(AuthServiceTests, RegisterRejectsDuplicateLogin) {
    using namespace online_board;

    persistence::InMemoryUserRepository user_repository;
    persistence::InMemorySessionRepository session_repository;
    FakePasswordHasher hasher;
    SequentialTokenGenerator token_generator;
    SequentialIdGenerator id_generator;
    FakeClock clock;

    application::AuthService service(
        user_repository,
        session_repository,
        hasher,
        token_generator,
        id_generator,
        clock);

    auto first = service.register_user({
        .login = "alice",
        .display_name = "Alice",
        .password = "secret",
    });
    ASSERT_TRUE(common::is_ok(first));

    auto second = service.register_user({
        .login = "alice",
        .display_name = "Alice Again",
        .password = "secret-2",
    });

    ASSERT_FALSE(common::is_ok(second));
    EXPECT_EQ(common::error(second).code, common::ErrorCode::already_exists);
}

TEST(AuthServiceTests, GuestEnterCreatesGuestSessionAndPrincipal) {
    using namespace online_board;

    persistence::InMemoryUserRepository user_repository;
    persistence::InMemorySessionRepository session_repository;
    FakePasswordHasher hasher;
    SequentialTokenGenerator token_generator;
    SequentialIdGenerator id_generator;
    FakeClock clock;

    application::AuthService service(
        user_repository,
        session_repository,
        hasher,
        token_generator,
        id_generator,
        clock);

    auto result = service.enter_guest({
        .nickname = "Guest One",
    });

    ASSERT_TRUE(common::is_ok(result));
    const auto& response = common::value(result);
    EXPECT_EQ(response.session.nickname, "Guest One");
    EXPECT_EQ(response.principal.nickname, "Guest One");
    EXPECT_EQ(response.principal.guest_id, response.session.id.value);
}

TEST(AuthServiceTests, RestoreSessionFindsValidSession) {
    using namespace online_board;

    persistence::InMemoryUserRepository user_repository;
    persistence::InMemorySessionRepository session_repository;
    FakePasswordHasher hasher;
    SequentialTokenGenerator token_generator;
    SequentialIdGenerator id_generator;
    FakeClock clock;

    application::AuthService service(
        user_repository,
        session_repository,
        hasher,
        token_generator,
        id_generator,
        clock);

    auto registered = service.register_user({
        .login = "alice",
        .display_name = "Alice",
        .password = "secret",
    });
    ASSERT_TRUE(common::is_ok(registered));

    auto restored = service.restore_session(common::value(registered).raw_token);
    ASSERT_TRUE(common::is_ok(restored));
    EXPECT_EQ(common::value(restored).user_id, common::value(registered).user.id);
}

TEST(AuthServiceTests, RestoreSessionRejectsExpiredSession) {
    using namespace online_board;

    persistence::InMemoryUserRepository user_repository;
    persistence::InMemorySessionRepository session_repository;
    FakePasswordHasher hasher;
    SequentialTokenGenerator token_generator;
    SequentialIdGenerator id_generator;
    FakeClock clock;

    application::AuthService service(
        user_repository,
        session_repository,
        hasher,
        token_generator,
        id_generator,
        clock);

    session_repository.save_auth_session(domain::AuthSession{
        .id = common::SessionId{"session-1"},
        .user_id = common::UserId{"user-1"},
        .token_hash = hasher.hash("token-1"),
        .created_at = clock.now() - std::chrono::hours(48),
        .expires_at = clock.now() - std::chrono::hours(1),
    });

    auto restored = service.restore_session("token-1");
    ASSERT_FALSE(common::is_ok(restored));
    EXPECT_EQ(common::error(restored).code, common::ErrorCode::access_denied);
}

}  // namespace
