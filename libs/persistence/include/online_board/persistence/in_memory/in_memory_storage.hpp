#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "online_board/domain/auth/session.hpp"
#include "online_board/domain/auth/user.hpp"
#include "online_board/domain/board/board.hpp"
#include "online_board/domain/board/board_member.hpp"
#include "online_board/domain/board/board_object.hpp"
#include "online_board/domain/operations/applied_operation.hpp"

namespace online_board::persistence {

struct InMemoryStorage {
    mutable std::mutex mutex;
    std::map<common::UserId, domain::User> users_by_id;
    std::map<std::string, common::UserId> user_id_by_login;
    std::map<std::string, domain::AuthSession> sessions_by_token_hash;
    std::map<common::GuestSessionId, domain::GuestSession> guest_sessions;
    std::map<common::BoardId, domain::Board> boards;
    std::multimap<common::BoardId, domain::BoardMember> members;
    std::map<common::BoardId, std::map<common::ObjectId, domain::BoardObject>> objects_by_board;
    std::map<common::BoardId, std::vector<domain::AppliedOperation>> operations_by_board;
};

using SharedInMemoryStorage = std::shared_ptr<InMemoryStorage>;

}  // namespace online_board::persistence
