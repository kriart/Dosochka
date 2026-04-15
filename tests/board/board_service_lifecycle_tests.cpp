#include <gtest/gtest.h>

#include <map>

#include "online_board/application/services/board_service.hpp"
#include "online_board/common/result.hpp"
#include "online_board/persistence/in_memory_repositories.hpp"
#include "support/test_support.hpp"

namespace {

TEST(BoardServiceLifecycleTests, RegisteredUserCanListOwnedAndMemberBoards) {
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

    auto owned_board = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}},
        .title = "Owned board",
    });
    ASSERT_TRUE(common::is_ok(owned_board));

    clock.current += std::chrono::seconds{1};
    auto shared_board = service.create_board({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-2"}},
        .title = "Shared board",
    });
    ASSERT_TRUE(common::is_ok(shared_board));
    ASSERT_TRUE(common::is_ok(service.upsert_board_member({
        .board_id = common::value(shared_board).board.id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-2"}},
        .target_user_id = common::UserId{"user-1"},
        .role = domain::BoardRole::editor,
    })));

    auto list_result = service.list_user_boards({
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}},
    });

    ASSERT_TRUE(common::is_ok(list_result));
    const auto& boards = common::value(list_result).boards;
    ASSERT_EQ(boards.size(), 2U);
    EXPECT_EQ(boards[0].board.title, "Shared board");
    EXPECT_EQ(boards[0].role, domain::BoardRole::editor);
    EXPECT_EQ(boards[1].board.title, "Owned board");
    EXPECT_EQ(boards[1].role, domain::BoardRole::owner);
}

TEST(BoardServiceLifecycleTests, GuestGetsEmptyBoardList) {
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

    auto list_result = service.list_user_boards({
        .principal = domain::GuestPrincipal{.guest_id = "guest-1", .nickname = "Guest"},
    });

    ASSERT_TRUE(common::is_ok(list_result));
    EXPECT_TRUE(common::value(list_result).boards.empty());
}

TEST(BoardServiceLifecycleTests, OwnerCanDeleteBoardAndAssociatedState) {
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
        .title = "Disposable board",
    });
    ASSERT_TRUE(common::is_ok(create_result));
    const auto board_id = common::value(create_result).board.id;

    ASSERT_TRUE(common::is_ok(service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"editor-1"},
        .role = domain::BoardRole::editor,
    })));

    object_repository.replace_for_board(board_id, {
        {
            common::ObjectId{"object-1"},
            domain::BoardObject{
                .object_id = common::ObjectId{"object-1"},
                .board_id = board_id,
                .type = domain::ObjectType::shape,
                .created_by = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
                .created_at = clock.current,
                .updated_at = clock.current,
                .is_deleted = false,
                .z_index = 1,
                .payload = domain::ShapePayload{
                    .shape_type = domain::ShapeType::rectangle,
                    .geometry = domain::Rect{.x = 1.0, .y = 2.0, .width = 50.0, .height = 40.0},
                    .stroke_color = "#000000",
                    .fill_color = std::nullopt,
                    .stroke_width = 2.0,
                },
            },
        },
    });
    operation_repository.append(domain::AppliedOperation{
        .operation_id = common::OperationId{"operation-1"},
        .board_id = board_id,
        .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .revision = 1,
        .applied_at = clock.current,
        .payload = domain::DeleteObjectCommand{.object_id = common::ObjectId{"object-1"}},
    });

    auto delete_result = service.delete_board({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
    });

    ASSERT_TRUE(common::is_ok(delete_result));
    EXPECT_EQ(common::value(delete_result).board_id, board_id);
    EXPECT_FALSE(board_repository.find_by_id(board_id).has_value());
    EXPECT_TRUE(member_repository.list_members(board_id).empty());
    EXPECT_TRUE(object_repository.load_by_board(board_id).empty());
    EXPECT_TRUE(operation_repository.list_by_board(board_id).empty());
}

TEST(BoardServiceLifecycleTests, NonOwnerCannotDeleteBoard) {
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
    });
    ASSERT_TRUE(common::is_ok(create_result));
    const auto board_id = common::value(create_result).board.id;

    ASSERT_TRUE(common::is_ok(service.upsert_board_member({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"owner-1"}},
        .target_user_id = common::UserId{"editor-1"},
        .role = domain::BoardRole::editor,
    })));

    auto delete_result = service.delete_board({
        .board_id = board_id,
        .principal = domain::RegisteredUserPrincipal{.user_id = common::UserId{"editor-1"}},
    });

    ASSERT_FALSE(common::is_ok(delete_result));
    EXPECT_EQ(common::error(delete_result).code, common::ErrorCode::access_denied);
    EXPECT_TRUE(board_repository.find_by_id(board_id).has_value());
}

}  // namespace
