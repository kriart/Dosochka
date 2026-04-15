#include <gtest/gtest.h>

#include "online_board/application/services/board_state_service.hpp"
#include "online_board/common/result.hpp"
#include "support/test_support.hpp"

namespace {

TEST(BoardStateServiceTests, DeletedObjectCannotBeUpdated) {
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
            .payload = domain::CreateTextCommand{
                .object_id = common::ObjectId{"text-1"},
                .position = {0.0, 0.0},
                .size = {50.0, 20.0},
                .text = "hello",
                .font_family = "Sans",
                .font_size = 12.0,
                .color = "#111",
                .z_index = 1,
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(created));

    auto deleted = service.apply(
        common::value(created),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::DeleteObjectCommand{.object_id = common::ObjectId{"text-1"}},
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(deleted));

    auto update_result = service.apply(
        common::value(deleted),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::UpdateTextContentCommand{
                .object_id = common::ObjectId{"text-1"},
                .text = "changed",
            },
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(update_result));
    EXPECT_EQ(common::error(update_result).code, common::ErrorCode::not_found);
}

TEST(BoardStateServiceTests, TextObjectCanBeResizedAndRestyled) {
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
            .payload = domain::CreateTextCommand{
                .object_id = common::ObjectId{"text-1"},
                .position = {0.0, 0.0},
                .size = {50.0, 20.0},
                .text = "hello",
                .font_family = "Sans",
                .font_size = 12.0,
                .color = "#111",
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
                .object_id = common::ObjectId{"text-1"},
                .size = {120.0, 40.0},
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
            .payload = domain::UpdateTextStyleCommand{
                .object_id = common::ObjectId{"text-1"},
                .font_family = "Mono",
                .font_size = 14.0,
                .color = "#222",
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(restyled));

    const auto& object = common::value(restyled).objects.at(common::ObjectId{"text-1"});
    const auto* text_payload = std::get_if<domain::TextPayload>(&object.payload);
    ASSERT_NE(text_payload, nullptr);
    EXPECT_EQ(text_payload->position.x, 0.0);
    EXPECT_EQ(text_payload->position.y, 0.0);
    EXPECT_EQ(text_payload->size.width, 120.0);
    EXPECT_EQ(text_payload->size.height, 40.0);
    EXPECT_EQ(text_payload->font_family, "Mono");
    EXPECT_EQ(text_payload->font_size, 14.0);
    EXPECT_EQ(text_payload->color, "#222");
}

TEST(BoardStateServiceTests, UpdateTextContentRejectsEmptyText) {
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
            .payload = domain::CreateTextCommand{
                .object_id = common::ObjectId{"text-1"},
                .position = {0.0, 0.0},
                .size = {50.0, 20.0},
                .text = "hello",
                .font_family = "Sans",
                .font_size = 12.0,
                .color = "#111",
                .z_index = 1,
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(created));

    auto update_result = service.apply(
        common::value(created),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::UpdateTextContentCommand{
                .object_id = common::ObjectId{"text-1"},
                .text = "",
            },
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(update_result));
    EXPECT_EQ(common::error(update_result).code, common::ErrorCode::invalid_argument);
}

TEST(BoardStateServiceTests, CreateTextRejectsEmptyText) {
    using namespace online_board;

    application::BoardStateService service;
    FakeClock clock;

    auto result = service.apply(
        domain::BoardSnapshot{.board_id = common::BoardId{"board-1"}, .revision = 0},
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = domain::RegisteredUserPrincipal{.user_id = common::UserId{"user-1"}},
            .base_revision = 0,
            .payload = domain::CreateTextCommand{
                .object_id = common::ObjectId{"text-1"},
                .position = {0.0, 0.0},
                .size = {50.0, 20.0},
                .text = "",
                .font_family = "Sans",
                .font_size = 12.0,
                .color = "#111",
                .z_index = 1,
            },
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(result));
    EXPECT_EQ(common::error(result).code, common::ErrorCode::invalid_argument);
}

TEST(BoardStateServiceTests, UpdateTextStyleRejectsInvalidFontSize) {
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
            .payload = domain::CreateTextCommand{
                .object_id = common::ObjectId{"text-1"},
                .position = {0.0, 0.0},
                .size = {50.0, 20.0},
                .text = "hello",
                .font_family = "Sans",
                .font_size = 12.0,
                .color = "#111",
                .z_index = 1,
            },
        },
        clock.now());
    ASSERT_TRUE(common::is_ok(created));

    auto update_result = service.apply(
        common::value(created),
        domain::OperationCommand{
            .board_id = common::BoardId{"board-1"},
            .actor = actor,
            .base_revision = 0,
            .payload = domain::UpdateTextStyleCommand{
                .object_id = common::ObjectId{"text-1"},
                .font_family = "Sans",
                .font_size = 0.0,
                .color = "#111",
            },
        },
        clock.now());

    ASSERT_FALSE(common::is_ok(update_result));
    EXPECT_EQ(common::error(update_result).code, common::ErrorCode::invalid_argument);
}

}  // namespace
