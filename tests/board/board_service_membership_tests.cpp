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

TEST(BoardServiceTests, OnlyOwnerCanManageMembers) {
    using namespace online_board;

    BoardServiceContext context;
    auto& service = context.service;

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Members board",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_disabled,
    });
    ASSERT_TRUE(common::is_ok(create_result));
    const auto board_id = common::value(create_result).board.id;

    auto editor_member = service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"editor-1"},
        .role = domain::BoardRole::editor,
    });
    ASSERT_TRUE(common::is_ok(editor_member));

    auto forbidden_result = service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"editor-1"}},
        .target_user_id = common::UserId{"member-3"},
        .role = domain::BoardRole::viewer,
    });

    ASSERT_FALSE(common::is_ok(forbidden_result));
    EXPECT_EQ(common::error(forbidden_result).code, common::ErrorCode::access_denied);
}

TEST(BoardServiceTests, ExistingMemberCanListBoardMembers) {
    using namespace online_board;

    BoardServiceContext context;
    auto& service = context.service;

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Members board",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_disabled,
    });
    ASSERT_TRUE(common::is_ok(create_result));
    const auto board_id = common::value(create_result).board.id;

    ASSERT_TRUE(common::is_ok(service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"viewer-1"},
        .role = domain::BoardRole::viewer,
    })));

    auto list_result = service.list_board_members({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"viewer-1"}},
    });

    ASSERT_TRUE(common::is_ok(list_result));
    const auto& members = common::value(list_result).members;
    ASSERT_EQ(members.size(), 2U);
    EXPECT_EQ(members[0].user_id, common::UserId{"owner-1"});
    EXPECT_EQ(members[1].user_id, common::UserId{"viewer-1"});
}

TEST(BoardServiceTests, GuestCannotListBoardMembers) {
    using namespace online_board;

    BoardServiceContext context;
    auto& service = context.service;

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Members board",
        .access_mode = domain::AccessMode::password_protected,
        .guest_policy = domain::GuestPolicy::guest_view_only,
        .password = std::string{"join-me"},
    });
    ASSERT_TRUE(common::is_ok(create_result));

    auto list_result = service.list_board_members({
        .board_id = common::value(create_result).board.id,
        .principal = domain::GuestPrincipal{.guest_id = "guest-1", .nickname = "Guest"},
    });

    ASSERT_FALSE(common::is_ok(list_result));
    EXPECT_EQ(common::error(list_result).code, common::ErrorCode::access_denied);
}

TEST(BoardServiceTests, OwnerCanRemoveMemberAndRevokePrivateBoardAccess) {
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

    ASSERT_TRUE(common::is_ok(service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"member-1"},
        .role = domain::BoardRole::editor,
    })));

    auto remove_result = service.remove_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"member-1"},
    });

    ASSERT_TRUE(common::is_ok(remove_result));
    EXPECT_EQ(common::value(remove_result).removed_user_id, common::UserId{"member-1"});

    auto join_result = service.join_board({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"member-1"}},
    });

    ASSERT_FALSE(common::is_ok(join_result));
    EXPECT_EQ(common::error(join_result).code, common::ErrorCode::access_denied);
}

TEST(BoardServiceTests, OwnerCannotBeRemovedFromMembership) {
    using namespace online_board;

    BoardServiceContext context;
    auto& service = context.service;

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Members board",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_disabled,
    });
    ASSERT_TRUE(common::is_ok(create_result));
    const auto board_id = common::value(create_result).board.id;

    auto remove_result = service.remove_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"owner-1"},
    });

    ASSERT_FALSE(common::is_ok(remove_result));
    EXPECT_EQ(common::error(remove_result).code, common::ErrorCode::invalid_argument);
}

TEST(BoardServiceTests, NonOwnerCannotRemoveMember) {
    using namespace online_board;

    BoardServiceContext context;
    auto& service = context.service;

    auto create_result = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .title = "Members board",
        .access_mode = domain::AccessMode::private_board,
        .guest_policy = domain::GuestPolicy::guest_disabled,
    });
    ASSERT_TRUE(common::is_ok(create_result));
    const auto board_id = common::value(create_result).board.id;

    ASSERT_TRUE(common::is_ok(service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"editor-1"},
        .role = domain::BoardRole::editor,
    })));
    ASSERT_TRUE(common::is_ok(service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"member-1"},
        .role = domain::BoardRole::viewer,
    })));

    auto remove_result = service.remove_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"editor-1"}},
        .target_user_id = common::UserId{"member-1"},
    });

    ASSERT_FALSE(common::is_ok(remove_result));
    EXPECT_EQ(common::error(remove_result).code, common::ErrorCode::access_denied);
}

}  // namespace
