#include <gtest/gtest.h>

#include "online_board/application/services/operation_service.hpp"
#include "online_board/common/result.hpp"
#include "support/test_support.hpp"

namespace {

TEST(OperationServiceTests, SuccessfulOperationIncrementsRevisionAndCreatesHistoryRecord) {
    using namespace online_board;

    application::BoardAccessService access_service;
    application::BoardStateService state_service;
    FakeClock clock;
    application::OperationService service(access_service, state_service, clock);

    domain::Board board{
        .id = common::BoardId{"board-1"},
        .owner_user_id = common::UserId{"owner-1"},
        .title = "Demo",
    };
    domain::BoardSnapshot snapshot{
        .board_id = board.id,
        .revision = 0,
    };

    auto result = service.apply(
        board,
        snapshot,
        domain::BoardRole::editor,
        domain::OperationCommand{
            .board_id = board.id,
            .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}},
            .base_revision = 0,
            .payload = domain::CreateTextCommand{
                .object_id = common::ObjectId{"text-1"},
                .position = {5.0, 6.0},
                .size = {100.0, 40.0},
                .text = "Hello",
                .font_family = "Sans",
                .font_size = 14.0,
                .color = "#333",
                .z_index = 1,
            },
        });

    ASSERT_TRUE(common::is_ok(result));
    EXPECT_EQ(common::value(result).snapshot.revision, 1U);
    EXPECT_EQ(common::value(result).applied_operation.revision, 1U);
}

TEST(OperationServiceTests, ViewerOperationIsRejected) {
    using namespace online_board;

    application::BoardAccessService access_service;
    application::BoardStateService state_service;
    FakeClock clock;
    application::OperationService service(access_service, state_service, clock);

    auto result = service.apply(
        domain::Board{
            .id = common::BoardId{"board-1"},
            .owner_user_id = common::UserId{"owner-1"},
            .title = "Demo",
        },
        domain::BoardSnapshot{
            .board_id = common::BoardId{"board-1"},
            .revision = 0,
        },
        domain::BoardRole::viewer,
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = domain::GuestPrincipal{.guest_id = "g-1", .nickname = "Viewer"},
            .base_revision = 0,
            .payload = domain::DeleteObjectCommand{
                .object_id = common::ObjectId{"obj-1"},
            },
        });

    ASSERT_FALSE(common::is_ok(result));
    EXPECT_EQ(common::error(result).code, common::ErrorCode::access_denied);
}

TEST(OperationServiceTests, StaleRevisionIsRejected) {
    using namespace online_board;

    application::BoardAccessService access_service;
    application::BoardStateService state_service;
    FakeClock clock;
    application::OperationService service(access_service, state_service, clock);

    auto result = service.apply(
        domain::Board{
            .id = common::BoardId{"board-1"},
            .owner_user_id = common::UserId{"owner-1"},
            .title = "Demo",
            .last_revision = 2,
        },
        domain::BoardSnapshot{
            .board_id = common::BoardId{"board-1"},
            .revision = 2,
        },
        domain::BoardRole::editor,
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}},
            .base_revision = 1,
            .payload = domain::CreateShapeCommand{
                .object_id = common::ObjectId{"shape-1"},
                .shape_type = domain::ShapeType::rectangle,
                .geometry = {0.0, 0.0, 10.0, 10.0},
                .stroke_color = "#000",
                .fill_color = std::string{"#fff"},
                .stroke_width = 1.0,
                .z_index = 1,
            },
        });

    ASSERT_FALSE(common::is_ok(result));
    EXPECT_EQ(common::error(result).code, common::ErrorCode::conflict);
}

}  // namespace
