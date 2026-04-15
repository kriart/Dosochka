#include <gtest/gtest.h>

#include "online_board/application/services/board_service.hpp"
#include "online_board/common/result.hpp"
#include "online_board/persistence/in_memory_repositories.hpp"
#include "support/test_support.hpp"

namespace {

TEST(BoardServiceTests, OwnerCanUpdateBoardSettings) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardMemberRepository member_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    application::BoardAccessService access_service;
    SequentialIdGenerator id_generator;
    FakePasswordHasher hasher;
    FakeClock clock;

    application::BoardService service(
        board_repository,
        member_repository,
        object_repository,
        operation_repository,
        access_service,
        id_generator,
        hasher,
        clock);

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Old title",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_disabled,
    });
    ASSERT_TRUE(common::is_ok(create_result));

    auto update_result = service.update_board_settings({
        .board_id = common::value(create_result).board.id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = std::string{"New title"},
        .access_mode = domain::AccessMode::password_protected,
        .guest_policy = domain::GuestPolicy::guest_edit_allowed,
        .change_password = true,
        .password = std::string{"new-pass"},
    });

    ASSERT_TRUE(common::is_ok(update_result));
    const auto& board = common::value(update_result).board;
    EXPECT_EQ(board.title, "New title");
    EXPECT_EQ(board.access_mode, domain::AccessMode::password_protected);
    EXPECT_EQ(board.guest_policy, domain::GuestPolicy::guest_edit_allowed);
    ASSERT_TRUE(board.password_hash.has_value());
    EXPECT_EQ(*board.password_hash, "hash:new-pass");
}

TEST(BoardServiceTests, EditorCannotUpdateBoardSettings) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardMemberRepository member_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    application::BoardAccessService access_service;
    SequentialIdGenerator id_generator;
    FakePasswordHasher hasher;
    FakeClock clock;

    application::BoardService service(
        board_repository,
        member_repository,
        object_repository,
        operation_repository,
        access_service,
        id_generator,
        hasher,
        clock);

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Restricted board",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_disabled,
    });
    ASSERT_TRUE(common::is_ok(create_result));
    const auto board_id = common::value(create_result).board.id;

    auto member_result = service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"editor-1"},
        .role = domain::BoardRole::editor,
    });
    ASSERT_TRUE(common::is_ok(member_result));

    auto update_result = service.update_board_settings({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"editor-1"}},
        .title = std::string{"Should fail"},
    });

    ASSERT_FALSE(common::is_ok(update_result));
    EXPECT_EQ(common::error(update_result).code, common::ErrorCode::access_denied);
}

TEST(BoardServiceTests, CreateBoardRejectsContradictoryAccessSettings) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardMemberRepository member_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    application::BoardAccessService access_service;
    SequentialIdGenerator id_generator;
    FakePasswordHasher hasher;
    FakeClock clock;

    application::BoardService service(
        board_repository,
        member_repository,
        object_repository,
        operation_repository,
        access_service,
        id_generator,
        hasher,
        clock);

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Private board",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_view_only,
    });

    ASSERT_FALSE(common::is_ok(create_result));
    EXPECT_EQ(common::error(create_result).code, common::ErrorCode::invalid_argument);
}

TEST(BoardServiceTests, CreateBoardRejectsPasswordProtectedBoardWithoutPassword) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardMemberRepository member_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    application::BoardAccessService access_service;
    SequentialIdGenerator id_generator;
    FakePasswordHasher hasher;
    FakeClock clock;

    application::BoardService service(
        board_repository,
        member_repository,
        object_repository,
        operation_repository,
        access_service,
        id_generator,
        hasher,
        clock);

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Protected board",
        .access_mode = domain::AccessMode::password_protected,
        .guest_policy = domain::GuestPolicy::guest_view_only,
    });

    ASSERT_FALSE(common::is_ok(create_result));
    EXPECT_EQ(common::error(create_result).code, common::ErrorCode::invalid_argument);
}

TEST(BoardServiceTests, UpdateBoardSettingsRejectsPasswordPayloadWithoutChangeFlag) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardMemberRepository member_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    application::BoardAccessService access_service;
    SequentialIdGenerator id_generator;
    FakePasswordHasher hasher;
    FakeClock clock;

    application::BoardService service(
        board_repository,
        member_repository,
        object_repository,
        operation_repository,
        access_service,
        id_generator,
        hasher,
        clock);

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Protected board",
        .access_mode = domain::AccessMode::password_protected,
        .guest_policy = domain::GuestPolicy::guest_view_only,
        .password = std::string{"initial-pass"},
    });
    ASSERT_TRUE(common::is_ok(create_result));

    auto update_result = service.update_board_settings({
        .board_id = common::value(create_result).board.id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .password = std::string{"new-pass"},
    });

    ASSERT_FALSE(common::is_ok(update_result));
    EXPECT_EQ(common::error(update_result).code, common::ErrorCode::invalid_argument);
}

TEST(BoardServiceTests, UpdateBoardSettingsClearsGuestPolicyAndPasswordWhenBoardStopsBeingProtected) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardMemberRepository member_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    application::BoardAccessService access_service;
    SequentialIdGenerator id_generator;
    FakePasswordHasher hasher;
    FakeClock clock;

    application::BoardService service(
        board_repository,
        member_repository,
        object_repository,
        operation_repository,
        access_service,
        id_generator,
        hasher,
        clock);

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Protected board",
        .access_mode = domain::AccessMode::password_protected,
        .guest_policy = domain::GuestPolicy::guest_edit_allowed,
        .password = std::string{"initial-pass"},
    });
    ASSERT_TRUE(common::is_ok(create_result));

    auto update_result = service.update_board_settings({
        .board_id = common::value(create_result).board.id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .access_mode = domain::AccessMode::private_board,
    });

    ASSERT_TRUE(common::is_ok(update_result));
    const auto& board = common::value(update_result).board;
    EXPECT_EQ(board.access_mode, domain::AccessMode::private_board);
    EXPECT_EQ(board.guest_policy, domain::GuestPolicy::guest_disabled);
    EXPECT_FALSE(board.password_hash.has_value());
}

TEST(BoardServiceTests, UpdateBoardSettingsRejectsGuestEnableOnPrivateBoard) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardMemberRepository member_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    application::BoardAccessService access_service;
    SequentialIdGenerator id_generator;
    FakePasswordHasher hasher;
    FakeClock clock;

    application::BoardService service(
        board_repository,
        member_repository,
        object_repository,
        operation_repository,
        access_service,
        id_generator,
        hasher,
        clock);

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Private board",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_disabled,
    });
    ASSERT_TRUE(common::is_ok(create_result));

    auto update_result = service.update_board_settings({
        .board_id = common::value(create_result).board.id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .guest_policy = domain::GuestPolicy::guest_view_only,
    });

    ASSERT_FALSE(common::is_ok(update_result));
    EXPECT_EQ(common::error(update_result).code, common::ErrorCode::invalid_argument);
}

}  // namespace
