#pragma once

#include <cstdint>
#include <deque>

#include <QJsonObject>
#include <QString>
#include <QtGlobal>

#include "online_board/application/services/board_state_service.hpp"
#include "online_board/domain/operations/applied_operation.hpp"

namespace online_board::client_qt {

class BoardSessionState {
public:
    void attach(const common::BoardId& board_id, domain::BoardRole role, domain::BoardSnapshot snapshot);
    void clear();

    [[nodiscard]] bool has_board() const;
    [[nodiscard]] const common::BoardId& board_id() const;
    [[nodiscard]] domain::BoardRole role() const;
    [[nodiscard]] const domain::BoardSnapshot& snapshot() const;
    [[nodiscard]] std::uint64_t revision() const;
    void set_active_participants(std::vector<domain::BoardParticipant> participants);

    QString next_object_id();
    [[nodiscard]] qint64 next_z_index() const;

    void enqueue_operation(QJsonObject operation);
    [[nodiscard]] bool has_operation_ready() const;
    [[nodiscard]] const QJsonObject& front_operation() const;
    void mark_operation_dispatched();
    void handle_operation_conflict();
    void discard_front_operation_group();

    common::Result<domain::BoardSnapshot> apply_remote_operation(
        const domain::AppliedOperation& applied_operation,
        const application::BoardStateService& board_state_service);

private:
    [[nodiscard]] bool operation_matches_front(const domain::OperationPayload& payload) const;

    common::BoardId current_board_id_;
    domain::BoardRole current_role_ {domain::BoardRole::viewer};
    domain::BoardSnapshot snapshot_;
    QString client_instance_id_;
    std::uint64_t local_object_counter_ {1};
    std::deque<QJsonObject> pending_operations_;
    bool awaiting_operation_ack_ {false};
};

}  // namespace online_board::client_qt
