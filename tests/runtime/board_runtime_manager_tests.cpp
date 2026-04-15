#include <future>
#include <vector>

#include <gtest/gtest.h>

#include "online_board/application/services/operation_service.hpp"
#include "online_board/application/services/presence_service.hpp"
#include "online_board/common/result.hpp"
#include "online_board/persistence/in_memory_repositories.hpp"
#include "online_board/runtime/board_runtime_manager.hpp"
#include "support/test_support.hpp"

namespace {

struct FailingBoardRuntimePersistence final : online_board::application::IBoardRuntimePersistence {
    online_board::common::Result<std::monostate> persist_applied_operation(
        const online_board::domain::Board&,
        const std::map<online_board::common::ObjectId, online_board::domain::BoardObject>&,
        const online_board::domain::AppliedOperation&) override {
        return online_board::common::fail<std::monostate>(
            online_board::common::ErrorCode::internal_error,
            "Persistence failed");
    }
};

TEST(BoardRuntimeManagerTests, ApplyOperationPersistsObjectsAndRevision) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    persistence::InMemoryBoardRuntimePersistence runtime_persistence(
        board_repository,
        object_repository,
        operation_repository);
    application::BoardAccessService access_service;
    application::BoardStateService state_service;
    application::PresenceService presence_service;
    FakeClock clock;
    application::OperationService operation_service(access_service, state_service, clock);

    domain::Board board{
        .id = common::BoardId{"board-1"},
        .owner_user_id = common::UserId{"owner-1"},
        .title = "Runtime board",
        .created_at = clock.now(),
        .updated_at = clock.now(),
    };
    board_repository.save(board);

    runtime::BoardRuntimeManager manager(
        board,
        domain::BoardSnapshot{
            .board_id = board.id,
            .revision = 0,
        },
        operation_service,
        presence_service,
        clock,
        runtime_persistence);

    auto result = manager.apply_operation(
        domain::BoardRole::editor,
        domain::OperationCommand{
            .board_id = board.id,
            .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"editor-1"}},
            .base_revision = 0,
            .payload = domain::CreateShapeCommand{
                .object_id = common::ObjectId{"shape-1"},
                .shape_type = domain::ShapeType::rectangle,
                .geometry = {0.0, 0.0, 50.0, 50.0},
                .stroke_color = "#000",
                .fill_color = std::string{"#fff"},
                .stroke_width = 2.0,
                .z_index = 1,
            },
        });

    ASSERT_TRUE(common::is_ok(result));
    EXPECT_EQ(manager.snapshot().revision, 1U);
    EXPECT_EQ(object_repository.load_by_board(board.id).size(), 1U);
    EXPECT_EQ(operation_repository.list_by_board(board.id).size(), 1U);
}

TEST(BoardRuntimeManagerTests, JoinAndLeaveParticipantUpdatesActiveParticipants) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    persistence::InMemoryBoardRuntimePersistence runtime_persistence(
        board_repository,
        object_repository,
        operation_repository);
    application::BoardAccessService access_service;
    application::BoardStateService state_service;
    application::PresenceService presence_service;
    FakeClock clock;
    application::OperationService operation_service(access_service, state_service, clock);

    domain::Board board{
        .id = common::BoardId{"board-1"},
        .owner_user_id = common::UserId{"owner-1"},
        .title = "Runtime board",
        .created_at = clock.now(),
        .updated_at = clock.now(),
    };

    runtime::BoardRuntimeManager manager(
        board,
        domain::BoardSnapshot{.board_id = board.id, .revision = 0},
        operation_service,
        presence_service,
        clock,
        runtime_persistence);

    domain::Principal guest = domain::GuestPrincipal{
        .guest_id = "guest-1",
        .nickname = "Guest One",
    };

    manager.join_participant(guest, domain::BoardRole::viewer);
    const auto joined = manager.active_participants();
    ASSERT_EQ(joined.size(), 1U);
    EXPECT_EQ(joined.front().nickname, "Guest One");
    EXPECT_EQ(joined.front().role, domain::BoardRole::viewer);

    manager.leave_participant(guest);
    EXPECT_TRUE(manager.active_participants().empty());
}

TEST(BoardRuntimeManagerTests, RejoinParticipantReplacesExistingEntry) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    persistence::InMemoryBoardRuntimePersistence runtime_persistence(
        board_repository,
        object_repository,
        operation_repository);
    application::BoardAccessService access_service;
    application::BoardStateService state_service;
    application::PresenceService presence_service;
    FakeClock clock;
    application::OperationService operation_service(access_service, state_service, clock);

    domain::Board board{
        .id = common::BoardId{"board-1"},
        .owner_user_id = common::UserId{"owner-1"},
        .title = "Runtime board",
        .created_at = clock.now(),
        .updated_at = clock.now(),
    };

    runtime::BoardRuntimeManager manager(
        board,
        domain::BoardSnapshot{.board_id = board.id, .revision = 0},
        operation_service,
        presence_service,
        clock,
        runtime_persistence);

    domain::Principal user = domain::RegisteredUserPrincipal{
        .user_id = common::UserId{"user-1"},
    };

    manager.join_participant(user, domain::BoardRole::viewer);
    manager.join_participant(user, domain::BoardRole::editor);

    const auto participants = manager.active_participants();
    ASSERT_EQ(participants.size(), 1U);
    EXPECT_EQ(participants.front().role, domain::BoardRole::editor);
}

TEST(BoardRuntimeManagerTests, ConcurrentOperationsAreSerializedByRuntimeManager) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    persistence::InMemoryBoardRuntimePersistence runtime_persistence(
        board_repository,
        object_repository,
        operation_repository);
    application::BoardAccessService access_service;
    application::BoardStateService state_service;
    application::PresenceService presence_service;
    FakeClock clock;
    application::OperationService operation_service(access_service, state_service, clock);

    domain::Board board{
        .id = common::BoardId{"board-1"},
        .owner_user_id = common::UserId{"owner-1"},
        .title = "Runtime board",
        .created_at = clock.now(),
        .updated_at = clock.now(),
    };
    board_repository.save(board);

    runtime::BoardRuntimeManager manager(
        board,
        domain::BoardSnapshot{.board_id = board.id, .revision = 0},
        operation_service,
        presence_service,
        clock,
        runtime_persistence);

    std::promise<void> start_promise;
    auto start_signal = start_promise.get_future().share();

    auto make_task = [&](std::string object_id) {
        return std::async(std::launch::async, [&, object_id]() {
            start_signal.wait();
            return manager.apply_operation(
                domain::BoardRole::editor,
                domain::OperationCommand{
                    .board_id = board.id,
                    .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"editor-1"}},
                    .base_revision = 0,
                    .payload = domain::CreateShapeCommand{
                        .object_id = common::ObjectId{object_id},
                        .shape_type = domain::ShapeType::rectangle,
                        .geometry = {0.0, 0.0, 50.0, 50.0},
                        .stroke_color = "#000",
                        .fill_color = std::string{"#fff"},
                        .stroke_width = 2.0,
                        .z_index = 1,
                    },
                });
        });
    };

    auto first = make_task("shape-1");
    auto second = make_task("shape-2");
    start_promise.set_value();

    std::vector<common::Result<application::ApplyOperationResult>> results;
    results.push_back(first.get());
    results.push_back(second.get());

    const auto success_count = static_cast<int>(std::count_if(
        results.begin(),
        results.end(),
        [](const auto& result) {
            return common::is_ok(result);
        }));

    const auto conflict_count = static_cast<int>(std::count_if(
        results.begin(),
        results.end(),
        [](const auto& result) {
            return !common::is_ok(result) && common::error(result).code == common::ErrorCode::conflict;
        }));

    EXPECT_EQ(success_count, 1);
    EXPECT_EQ(conflict_count, 1);
    EXPECT_EQ(manager.snapshot().revision, 1U);
    EXPECT_EQ(object_repository.load_by_board(board.id).size(), 1U);
    EXPECT_EQ(operation_repository.list_by_board(board.id).size(), 1U);
}

TEST(BoardRuntimeManagerTests, FailedPersistenceDoesNotAdvanceInMemoryState) {
    using namespace online_board;

    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    FailingBoardRuntimePersistence runtime_persistence;
    application::BoardAccessService access_service;
    application::BoardStateService state_service;
    application::PresenceService presence_service;
    FakeClock clock;
    application::OperationService operation_service(access_service, state_service, clock);

    domain::Board board{
        .id = common::BoardId{"board-1"},
        .owner_user_id = common::UserId{"owner-1"},
        .title = "Runtime board",
        .created_at = clock.now(),
        .updated_at = clock.now(),
    };
    board_repository.save(board);

    runtime::BoardRuntimeManager manager(
        board,
        domain::BoardSnapshot{.board_id = board.id, .revision = 0},
        operation_service,
        presence_service,
        clock,
        runtime_persistence);

    auto result = manager.apply_operation(
        domain::BoardRole::editor,
        domain::OperationCommand{
            .board_id = board.id,
            .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"editor-1"}},
            .base_revision = 0,
            .payload = domain::CreateShapeCommand{
                .object_id = common::ObjectId{"shape-1"},
                .shape_type = domain::ShapeType::rectangle,
                .geometry = {0.0, 0.0, 50.0, 50.0},
                .stroke_color = "#000",
                .fill_color = std::string{"#fff"},
                .stroke_width = 2.0,
                .z_index = 1,
            },
        });

    ASSERT_FALSE(common::is_ok(result));
    EXPECT_EQ(common::error(result).code, common::ErrorCode::internal_error);
    EXPECT_EQ(manager.snapshot().revision, 0U);
    EXPECT_TRUE(manager.snapshot().objects.empty());
    EXPECT_TRUE(object_repository.load_by_board(board.id).empty());
    EXPECT_TRUE(operation_repository.list_by_board(board.id).empty());
}

}  // namespace
