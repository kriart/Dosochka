#include <gtest/gtest.h>

#include "online_board/application/services/board_service.hpp"
#include "online_board/common/result.hpp"
#include "online_board/persistence/in_memory_repositories.hpp"
#include "support/test_support.hpp"

namespace {

using namespace online_board;

struct BoardServiceContext {
    persistence::SharedInMemoryStorage storage {std::make_shared<persistence::InMemoryStorage>()};
    persistence::InMemoryBoardRepository board_repository {storage};
    persistence::InMemoryBoardMemberRepository member_repository {storage};
    persistence::InMemoryBoardObjectRepository object_repository {storage};
    persistence::InMemoryBoardOperationRepository operation_repository {storage};
    persistence::InMemoryBoardLifecyclePersistence lifecycle_persistence {storage};
    application::BoardAccessService access_service;
    SequentialIdGenerator id_generator;
    FakePasswordHasher hasher;
    FakeClock clock;
    application::BoardService service;

    BoardServiceContext()
        : service(
              board_repository,
              member_repository,
              object_repository,
              operation_repository,
              lifecycle_persistence,
              access_service,
              id_generator,
              hasher,
              clock) {
    }
};

TEST(BoardServiceTests, RegisteredUserCanCreateBoardAndJoinAsOwner) {
    using namespace online_board;

    BoardServiceContext context;
    auto& service = context.service;

    domain::Principal owner = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}};

    auto create_result = service.create_board({
        .principal = owner,
        .title = "Architecture board",
        .access_mode = domain::AccessMode::password_protected,
        .guest_policy = domain::GuestPolicy::guest_view_only,
        .password = std::string{"join-me"},
    });

    ASSERT_TRUE(common::is_ok(create_result));
    const auto board_id = common::value(create_result).board.id;

    auto join_result = service.join_board({
        .board_id = board_id,
        .principal = owner,
        .password = std::string{"join-me"},
    });

    ASSERT_TRUE(common::is_ok(join_result));
    EXPECT_EQ(common::value(join_result).role, domain::BoardRole::owner);
    EXPECT_EQ(common::value(join_result).snapshot.board_id, board_id);
}

TEST(BoardServiceTests, GuestCanJoinPasswordProtectedBoardWhenAllowed) {
    using namespace online_board;

    BoardServiceContext context;
    auto& service = context.service;

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Open board",
        .access_mode = domain::AccessMode::password_protected,
        .guest_policy = domain::GuestPolicy::guest_view_only,
        .password = std::string{"1234"},
    });
    ASSERT_TRUE(common::is_ok(create_result));

    auto join_result = service.join_board({
        .board_id = common::value(create_result).board.id,
        .principal = domain::GuestPrincipal{.guest_id = "guest-1", .nickname = "Spectator"},
        .password = std::string{"1234"},
    });

    ASSERT_TRUE(common::is_ok(join_result));
    EXPECT_EQ(common::value(join_result).role, domain::BoardRole::viewer);
}

TEST(BoardServiceTests, PrivateBoardRejectsUnlistedRegisteredUser) {
    using namespace online_board;

    BoardServiceContext context;
    auto& service = context.service;

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Private board",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_disabled,
    });
    ASSERT_TRUE(common::is_ok(create_result));

    auto join_result = service.join_board({
        .board_id = common::value(create_result).board.id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-2"}},
    });

    ASSERT_FALSE(common::is_ok(join_result));
    EXPECT_EQ(common::error(join_result).code, common::ErrorCode::access_denied);
}

TEST(BoardServiceTests, PrivateBoardAllowsExistingMember) {
    using namespace online_board;

    BoardServiceContext context;
    auto& service = context.service;

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Private board",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_disabled,
    });
    ASSERT_TRUE(common::is_ok(create_result));
    const auto board_id = common::value(create_result).board.id;

    auto member_result = service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"member-1"},
        .role = domain::BoardRole::editor,
    });
    ASSERT_TRUE(common::is_ok(member_result));

    auto join_result = service.join_board({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"member-1"}},
    });

    ASSERT_TRUE(common::is_ok(join_result));
    EXPECT_EQ(common::value(join_result).role, domain::BoardRole::editor);
}

}  // namespace
