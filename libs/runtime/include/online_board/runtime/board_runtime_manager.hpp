#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "online_board/application/services/operation_service.hpp"
#include "online_board/application/services/presence_service.hpp"
#include "online_board/application/repository_interfaces.hpp"
#include "online_board/common/clock.hpp"
#include "online_board/domain/auth/principal.hpp"

namespace online_board::runtime {

class BoardRuntimeManager {
public:
    BoardRuntimeManager(
        domain::Board board,
        domain::BoardSnapshot snapshot,
        application::OperationService& operation_service,
        application::PresenceService& presence_service,
        const common::IClock& clock,
        application::IBoardRuntimePersistence& runtime_persistence);

    [[nodiscard]] domain::Board board() const;
    [[nodiscard]] domain::BoardSnapshot snapshot() const;
    [[nodiscard]] std::vector<domain::BoardParticipant> active_participants() const;
    void join_participant(const domain::Principal& principal, domain::BoardRole role, std::string nickname = {});
    void leave_participant(const domain::Principal& principal);
    void close();

    [[nodiscard]] common::Result<application::ApplyOperationResult> apply_operation(
        domain::BoardRole actor_role,
        const domain::OperationCommand& command);

private:
    mutable std::mutex mutex_;
    domain::Board board_;
    domain::BoardSnapshot snapshot_;
    application::OperationService& operation_service_;
    application::PresenceService& presence_service_;
    const common::IClock& clock_;
    application::IBoardRuntimePersistence& runtime_persistence_;
    bool closed_ {false};
};

}  // namespace online_board::runtime
