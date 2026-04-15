#pragma once

#include <optional>
#include <string_view>
#include <variant>

#include "online_board/common/result.hpp"
#include "online_board/domain/board/board.hpp"

namespace online_board::application {

class BoardAccessService {
public:
    [[nodiscard]] bool can_create_board(const domain::Principal& principal) const {
        return std::holds_alternative<domain::RegisteredUserPrincipal>(principal);
    }

    [[nodiscard]] bool can_manage_board_settings(domain::BoardRole role) const {
        return role == domain::BoardRole::owner;
    }

    [[nodiscard]] bool can_apply_operation(domain::BoardRole role) const {
        return role == domain::BoardRole::owner || role == domain::BoardRole::editor;
    }

    [[nodiscard]] std::optional<domain::BoardRole> resolve_existing_role(
        const domain::Board& board,
        const domain::Principal& principal,
        const std::optional<domain::BoardMember>& existing_member) const {
        if (const auto* user = std::get_if<domain::RegisteredUserPrincipal>(&principal)) {
            if (user->user_id == board.owner_user_id) {
                return domain::BoardRole::owner;
            }
        }

        if (existing_member.has_value()) {
            return existing_member->role;
        }

        return std::nullopt;
    }

    [[nodiscard]] common::Result<domain::BoardRole> resolve_role_for_join(
        const domain::Board& board,
        const domain::Principal& principal,
        const std::optional<domain::BoardMember>& existing_member,
        std::optional<std::string_view> supplied_password = std::nullopt) const {
        if (const auto existing_role = resolve_existing_role(board, principal, existing_member); existing_role.has_value()) {
            return *existing_role;
        }

        const bool password_ok =
            !board.password_hash.has_value() ||
            (supplied_password.has_value() && board.password_hash.value() == supplied_password.value());

        switch (board.access_mode) {
        case domain::AccessMode::private_board:
            return common::fail<domain::BoardRole>(
                common::ErrorCode::access_denied,
                "Board is not open for new participants");
        case domain::AccessMode::password_protected:
            if (!password_ok) {
                return common::fail<domain::BoardRole>(
                    common::ErrorCode::access_denied,
                    "Board password is invalid");
            }
            break;
        }

        if (std::holds_alternative<domain::GuestPrincipal>(principal)) {
            switch (board.guest_policy) {
            case domain::GuestPolicy::guest_disabled:
                return common::fail<domain::BoardRole>(
                    common::ErrorCode::access_denied,
                    "Guests are disabled for this board");
            case domain::GuestPolicy::guest_view_only:
                return domain::BoardRole::viewer;
            case domain::GuestPolicy::guest_edit_allowed:
                return domain::BoardRole::editor;
            }
        }

        return domain::BoardRole::viewer;
    }
};

}  // namespace online_board::application
