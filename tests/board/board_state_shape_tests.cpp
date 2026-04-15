#include <gtest/gtest.h>

#include "online_board/application/services/board_state_service.hpp"
#include "online_board/common/result.hpp"
#include "support/test_support.hpp"

namespace {

TEST(BoardStateServiceTests, CreateShapeAndDeleteObjectUpdatesSnapshot) {
    using namespace online_board;

    application::BoardStateService service;
    FakeClock clock;
    domain::BoardSnapshot snapshot{
        .board_id = common::BoardId{"board-1"},
        .revision = 0,
    };
    domain::Principal actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}};

    auto create_result = service.apply(
        snapshot,
        domain::OperationCommand{
            .board_id = snapshot.board_id,
            .actor = actor,
            .base_revision = 0,
            .payload = domain::CreateShapeCommand{
                .object_id = common::ObjectId{"shape-1"},
                .shape_type = domain::ShapeType::rectangle,
                .geometry = domain::Rect{10.0, 20.0, 100.0, 80.0},
                .stroke_color = "#000",
                .fill_color = std::string{"#fff"},
                .stroke_width = 2.0,
                .z_index = 1,
            },
        },
        clock.now());

    ASSERT_TRUE(common::is_ok(create_result));
    auto next_snapshot = common::value(create_result);
    ASSERT_EQ(next_snapshot.objects.size(), 1U);

    auto delete_result = service.apply(
        next_snapshot,
        domain::OperationCommand{
            .board_id = snapshot.board_id,
            .actor = actor,
            .base_revision = 0,
            .payload = domain::DeleteObjectCommand{
                .object_id = common::ObjectId{"shape-1"},
            },
        },
        clock.now());

    ASSERT_TRUE(common::is_ok(delete_result));
    EXPECT_TRUE(common::value(delete_result).objects.at(common::ObjectId{"shape-1"}).is_deleted);
}

TEST(BoardStateServiceTests, CreateShapeRejectsInvalidGeometry) {
    using namespace online_board;

    application::BoardStateService service;
    FakeClock clock;

    auto result = service.apply(
        domain::BoardSnapshot{.board_id = common::BoardId{"board-1"}, .revision = 0},
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}},
            .base_revision = 0,
            .payload = domain::CreateShapeCommand{
                .object_id = common::ObjectId{"shape-1"},
                .shape_type = domain::ShapeType::rectangle,
                .geometry = domain::Rect{0.0, 0.0, -10.0, 50.0},
                .stroke_color = "#000",
                .fill_color = std::string{"#fff"},
                .stroke_width = 2.0,
                .z_index = 1,
            },
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(result));
    EXPECT_EQ(common::error(result).code, common::ErrorCode::invalid_argument);
}

TEST(BoardStateServiceTests, UpdateShapeGeometryRejectsNegativeSize) {
    using namespace online_board;

    application::BoardStateService service;
    FakeClock clock;
    const auto actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}};

    auto create_result = service.apply(
        domain::BoardSnapshot{.board_id = common::BoardId{"board-1"}, .revision = 0},
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::CreateShapeCommand{
                .object_id = common::ObjectId{"shape-1"},
                .shape_type = domain::ShapeType::rectangle,
                .geometry = domain::Rect{0.0, 0.0, 10.0, 10.0},
                .stroke_color = "#000",
                .fill_color = std::string{"#fff"},
                .stroke_width = 1.0,
                .z_index = 1,
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(create_result));

    auto update_result = service.apply(
        common::value(create_result),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::UpdateShapeGeometryCommand{
                .object_id = common::ObjectId{"shape-1"},
                .geometry = domain::Rect{0.0, 0.0, 0.0, 10.0},
            },
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(update_result));
    EXPECT_EQ(common::error(update_result).code, common::ErrorCode::invalid_argument);
}

TEST(BoardStateServiceTests, ShapeObjectCanBeResizedAndRestyled) {
    using namespace online_board;

    application::BoardStateService service;
    FakeClock clock;
    const auto actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}};

    auto created = service.apply(
        domain::BoardSnapshot{.board_id = common::BoardId{"board-1"}, .revision = 0},
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::CreateShapeCommand{
                .object_id = common::ObjectId{"shape-1"},
                .shape_type = domain::ShapeType::rectangle,
                .geometry = {0.0, 0.0, 50.0, 30.0},
                .stroke_color = "#000",
                .fill_color = std::string{"#fff"},
                .stroke_width = 2.0,
                .z_index = 1,
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(created));

    auto resized = service.apply(
        common::value(created),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::ResizeObjectCommand{
                .object_id = common::ObjectId{"shape-1"},
                .size = {90.0, 40.0},
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(resized));

    auto restyled = service.apply(
        common::value(resized),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::UpdateShapeStyleCommand{
                .object_id = common::ObjectId{"shape-1"},
                .stroke_color = "#111",
                .fill_color = std::string{"#222"},
                .stroke_width = 3.0,
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(restyled));

    const auto& object = common::value(restyled).objects.at(common::ObjectId{"shape-1"});
    const auto* shape_payload = std::get_if<domain::ShapePayload>(&object.payload);
    ASSERT_NE(shape_payload, nullptr);
    EXPECT_EQ(shape_payload->geometry.x, 0.0);
    EXPECT_EQ(shape_payload->geometry.y, 0.0);
    EXPECT_EQ(shape_payload->geometry.width, 90.0);
    EXPECT_EQ(shape_payload->geometry.height, 40.0);
    EXPECT_EQ(shape_payload->stroke_color, "#111");
    ASSERT_TRUE(shape_payload->fill_color.has_value());
    EXPECT_EQ(*shape_payload->fill_color, "#222");
    EXPECT_EQ(shape_payload->stroke_width, 3.0);
}

}  // namespace
