#pragma once

#include <algorithm>
#include <vector>

#include "online_board/domain/board/board_snapshot.hpp"

namespace online_board::application {

class PresenceService {
public:
    [[nodiscard]] std::vector<domain::BoardParticipant> join(
        std::vector<domain::BoardParticipant> participants,
        domain::BoardParticipant participant) const {
        participants.erase(
            std::remove_if(
                participants.begin(),
                participants.end(),
                [&](const domain::BoardParticipant& current) {
                    return domain::principal_key(current.principal) ==
                        domain::principal_key(participant.principal);
                }),
            participants.end());
        participants.push_back(std::move(participant));
        return participants;
    }

    [[nodiscard]] std::vector<domain::BoardParticipant> leave(
        std::vector<domain::BoardParticipant> participants,
        const domain::Principal& principal) const {
        participants.erase(
            std::remove_if(
                participants.begin(),
                participants.end(),
                [&](const domain::BoardParticipant& current) {
                    return domain::principal_key(current.principal) ==
                        domain::principal_key(principal);
                }),
            participants.end());
        return participants;
    }
};

}  // namespace online_board::application
