#include <gtest/gtest.h>

#include "online_board/application/services/board_access_service.hpp"
#include "online_board/common/result.hpp"

namespace {

TEST(BoardAccessServiceTests, GuestCannotCreateBoard) {
    using namespace online_board;

    application::BoardAccessService service;
    domain::Principal guest = domain::GuestPrincipal{
        .guest_id = "guest-1",
        .nickname = "Guest",
    };

    EXPECT_FALSE(service.can_create_board(guest));
}

TEST(BoardAccessServiceTests, GuestGetsEditorRoleWhenPolicyAllowsIt) {
    using namespace online_board;

    application::BoardAccessService service;
    domain::Board board{
        .id = common::BoardId{"board-1"},
        .owner_user_id = common::UserId{"owner-1"},
        .title = "Demo",
        .access_mode = domain::AccessMode::password_protected,
        .guest_policy = domain::GuestPolicy::guest_edit_allowed,
        .password_hash = std::string{"hash:1234"},
    };
    domain::Principal guest = domain::GuestPrincipal{
        .guest_id = "guest-1",
        .nickname = "Guest",
    };

    auto result = service.resolve_role_for_join(
        board,
        guest,
        std::nullopt,
        std::string_view{"hash:1234"});

    ASSERT_TRUE(common::is_ok(result));
    EXPECT_EQ(common::value(result), domain::BoardRole::editor);
}

TEST(BoardAccessServiceTests, ViewerCannotApplyOperations) {
    using namespace online_board;

    application::BoardAccessService service;
    EXPECT_FALSE(service.can_apply_operation(domain::BoardRole::viewer));
    EXPECT_TRUE(service.can_apply_operation(domain::BoardRole::editor));
}

}  // namespace
