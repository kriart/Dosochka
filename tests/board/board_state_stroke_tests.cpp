#include <gtest/gtest.h>

#include "online_board/application/services/board_state_service.hpp"
#include "online_board/common/result.hpp"
#include "support/test_support.hpp"

namespace {

TEST(BoardStateServiceTests, AppendStrokeRequiresBeginStroke) {
    using namespace online_board;

    application::BoardStateService service;
    FakeClock clock;

    auto result = service.apply(
        domain::BoardSnapshot{
            .board_id = common::BoardId{"board-1"},
            .revision = 0,
        },
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}},
            .base_revision = 0,
            .payload = domain::AppendStrokePointsCommand{
                .object_id = common::ObjectId{"stroke-1"},
                .points = {{1.0, 1.0}, {2.0, 2.0}},
            },
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(result));
    EXPECT_EQ(common::error(result).code, common::ErrorCode::not_found);
}

TEST(BoardStateServiceTests, BeginStrokeRejectsInvalidThickness) {
    using namespace online_board;

    application::BoardStateService service;
    FakeClock clock;

    auto result = service.apply(
        domain::BoardSnapshot{.board_id = common::BoardId{"board-1"}, .revision = 0},
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}},
            .base_revision = 0,
            .payload = domain::BeginStrokeCommand{
                .object_id = common::ObjectId{"stroke-1"},
                .color = "#000",
                .thickness = 0.0,
                .start_point = {0.0, 0.0},
                .z_index = 1,
            },
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(result));
    EXPECT_EQ(common::error(result).code, common::ErrorCode::invalid_argument);
}

TEST(BoardStateServiceTests, EndStrokeCannotBeAppliedTwice) {
    using namespace online_board;

    application::BoardStateService service;
    FakeClock clock;
    const auto actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}};

    auto started = service.apply(
        domain::BoardSnapshot{.board_id = common::BoardId{"board-1"}, .revision = 0},
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::BeginStrokeCommand{
                .object_id = common::ObjectId{"stroke-1"},
                .color = "#000",
                .thickness = 1.0,
                .start_point = {0.0, 0.0},
                .z_index = 1,
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(started));

    auto finished = service.apply(
        common::value(started),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::EndStrokeCommand{.object_id = common::ObjectId{"stroke-1"}},
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(finished));

    auto second_finish = service.apply(
        common::value(finished),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::EndStrokeCommand{.object_id = common::ObjectId{"stroke-1"}},
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(second_finish));
    EXPECT_EQ(common::error(second_finish).code, common::ErrorCode::invalid_state);
}

TEST(BoardStateServiceTests, ResizeStrokeIsRejected) {
    using namespace online_board;

    application::BoardStateService service;
    FakeClock clock;
    const auto actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}};

    auto started = service.apply(
        domain::BoardSnapshot{.board_id = common::BoardId{"board-1"}, .revision = 0},
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::BeginStrokeCommand{
                .object_id = common::ObjectId{"stroke-1"},
                .color = "#000",
                .thickness = 1.0,
                .start_point = {0.0, 0.0},
                .z_index = 1,
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(started));

    auto resize_result = service.apply(
        common::value(started),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::ResizeObjectCommand{
                .object_id = common::ObjectId{"stroke-1"},
                .size = {10.0, 10.0},
            },
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(resize_result));
    EXPECT_EQ(common::error(resize_result).code, common::ErrorCode::invalid_state);
}

}  // namespace
