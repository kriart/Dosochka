#pragma once

#include <optional>
#include <string>
#include <vector>

#include "online_board/domain/board/board.hpp"
#include "online_board/domain/board/board_snapshot.hpp"

namespace online_board::application {

struct CreateBoardRequest {
    domain::Principal principal;
    std::string title;
    domain::AccessMode access_mode {domain::AccessMode::private_board};
    domain::GuestPolicy guest_policy {domain::GuestPolicy::guest_disabled};
    std::optional<std::string> password;
};

struct CreateBoardResponse {
    domain::Board board;
};

struct UserBoardSummary {
    domain::Board board;
    domain::BoardRole role {domain::BoardRole::viewer};
};

struct ListUserBoardsRequest {
    domain::Principal principal;
};

struct ListUserBoardsResponse {
    std::vector<UserBoardSummary> boards;
};

struct UpdateBoardSettingsRequest {
    common::BoardId board_id;
    domain::Principal principal;
    std::optional<std::string> title;
    std::optional<domain::AccessMode> access_mode;
    std::optional<domain::GuestPolicy> guest_policy;
    bool change_password {false};
    std::optional<std::string> password;
};

struct UpdateBoardSettingsResponse {
    domain::Board board;
};

struct UpsertBoardMemberRequest {
    common::BoardId board_id;
    domain::Principal principal;
    common::UserId target_user_id;
    domain::BoardRole role {domain::BoardRole::viewer};
};

struct UpsertBoardMemberResponse {
    domain::BoardMember member;
};

struct ListBoardMembersRequest {
    common::BoardId board_id;
    domain::Principal principal;
};

struct ListBoardMembersResponse {
    std::vector<domain::BoardMember> members;
};

struct RemoveBoardMemberRequest {
    common::BoardId board_id;
    domain::Principal principal;
    common::UserId target_user_id;
};

struct RemoveBoardMemberResponse {
    common::BoardId board_id;
    common::UserId removed_user_id;
};

struct DeleteBoardRequest {
    common::BoardId board_id;
    domain::Principal principal;
};

struct DeleteBoardResponse {
    common::BoardId board_id;
};

struct JoinBoardRequest {
    common::BoardId board_id;
    domain::Principal principal;
    std::optional<std::string> password;
};

struct JoinBoardResponse {
    domain::Board board;
    domain::BoardRole role {domain::BoardRole::viewer};
    domain::BoardSnapshot snapshot;
};

}  // namespace online_board::application
