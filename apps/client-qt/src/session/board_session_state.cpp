#include "board_session_state.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>
#include <variant>

#include <QUuid>

namespace online_board::client_qt {
namespace {

QString operation_type(const QJsonObject& operation) {
    return operation.value("type").toString();
}

QString operation_object_id(const QJsonObject& operation) {
    return operation.value("object_id").toString();
}

QString operation_type(const domain::OperationPayload& payload) {
    return std::visit(
        [](const auto& command) -> QString {
            using Payload = std::decay_t<decltype(command)>;
            if constexpr (std::is_same_v<Payload, domain::CreateShapeCommand>) {
                return QStringLiteral("create_shape");
            } else if constexpr (std::is_same_v<Payload, domain::CreateTextCommand>) {
                return QStringLiteral("create_text");
            } else if constexpr (std::is_same_v<Payload, domain::BeginStrokeCommand>) {
                return QStringLiteral("begin_stroke");
            } else if constexpr (std::is_same_v<Payload, domain::AppendStrokePointsCommand>) {
                return QStringLiteral("append_stroke_points");
            } else if constexpr (std::is_same_v<Payload, domain::EndStrokeCommand>) {
                return QStringLiteral("end_stroke");
            } else if constexpr (std::is_same_v<Payload, domain::UpdateShapeGeometryCommand>) {
                return QStringLiteral("update_shape_geometry");
            } else if constexpr (std::is_same_v<Payload, domain::UpdateShapeStyleCommand>) {
                return QStringLiteral("update_shape_style");
            } else if constexpr (std::is_same_v<Payload, domain::UpdateTextContentCommand>) {
                return QStringLiteral("update_text_content");
            } else if constexpr (std::is_same_v<Payload, domain::UpdateTextStyleCommand>) {
                return QStringLiteral("update_text_style");
            } else if constexpr (std::is_same_v<Payload, domain::ResizeObjectCommand>) {
                return QStringLiteral("resize_object");
            }
            return QStringLiteral("delete_object");
        },
        payload);
}

QString operation_object_id(const domain::OperationPayload& payload) {
    return std::visit(
        [](const auto& command) -> QString {
            return QString::fromStdString(command.object_id.value);
        },
        payload);
}

}  // namespace

void BoardSessionState::attach(const common::BoardId& board_id, domain::BoardRole role, domain::BoardSnapshot snapshot) {
    current_board_id_ = board_id;
    current_role_ = role;
    snapshot_ = std::move(snapshot);
    client_instance_id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    local_object_counter_ = 1;
    pending_operations_.clear();
    awaiting_operation_ack_ = false;
}

void BoardSessionState::clear() {
    current_board_id_ = {};
    current_role_ = domain::BoardRole::viewer;
    snapshot_ = {};
    client_instance_id_.clear();
    local_object_counter_ = 1;
    pending_operations_.clear();
    awaiting_operation_ack_ = false;
}

bool BoardSessionState::has_board() const {
    return !current_board_id_.empty();
}

const common::BoardId& BoardSessionState::board_id() const {
    return current_board_id_;
}

domain::BoardRole BoardSessionState::role() const {
    return current_role_;
}

const domain::BoardSnapshot& BoardSessionState::snapshot() const {
    return snapshot_;
}

std::uint64_t BoardSessionState::revision() const {
    return snapshot_.revision;
}

void BoardSessionState::set_active_participants(std::vector<domain::BoardParticipant> participants) {
    snapshot_.active_participants = std::move(participants);
}

QString BoardSessionState::next_object_id() {
    return QStringLiteral("client-%1-%2-%3")
        .arg(QString::fromStdString(current_board_id_.empty() ? std::string("board") : current_board_id_.value))
        .arg(client_instance_id_.isEmpty() ? QStringLiteral("local") : client_instance_id_)
        .arg(local_object_counter_++);
}

qint64 BoardSessionState::next_z_index() const {
    qint64 value = 0;
    for (const auto& [_, object] : snapshot_.objects) {
        value = std::max<qint64>(value, object.z_index + 1);
    }
    return value;
}

void BoardSessionState::enqueue_operation(QJsonObject operation) {
    if (!has_board()) {
        return;
    }
    pending_operations_.push_back(std::move(operation));
}

bool BoardSessionState::has_operation_ready() const {
    return has_board() && !awaiting_operation_ack_ && !pending_operations_.empty();
}

const QJsonObject& BoardSessionState::front_operation() const {
    return pending_operations_.front();
}

void BoardSessionState::mark_operation_dispatched() {
    awaiting_operation_ack_ = true;
}

void BoardSessionState::handle_operation_conflict() {
    awaiting_operation_ack_ = false;
}

void BoardSessionState::discard_front_operation_group() {
    if (pending_operations_.empty()) {
        awaiting_operation_ack_ = false;
        return;
    }

    const auto failed_object_id = operation_object_id(pending_operations_.front());
    pending_operations_.pop_front();
    while (!pending_operations_.empty() && operation_object_id(pending_operations_.front()) == failed_object_id) {
        pending_operations_.pop_front();
    }
    awaiting_operation_ack_ = false;
}

common::Result<domain::BoardSnapshot> BoardSessionState::apply_remote_operation(
    const domain::AppliedOperation& applied_operation,
    const application::BoardStateService& board_state_service) {
    auto result = board_state_service.apply(
        snapshot_,
        domain::OperationCommand{
            .board_id = applied_operation.board_id,
            .actor = applied_operation.actor,
            .base_revision = snapshot_.revision,
            .payload = applied_operation.payload,
        },
        applied_operation.applied_at);
    if (!common::is_ok(result)) {
        return common::error(result);
    }

    snapshot_ = common::value(result);
    snapshot_.revision = applied_operation.revision;

    if (awaiting_operation_ack_ && operation_matches_front(applied_operation.payload)) {
        pending_operations_.pop_front();
        awaiting_operation_ack_ = false;
    }

    return snapshot_;
}

bool BoardSessionState::operation_matches_front(const domain::OperationPayload& payload) const {
    if (pending_operations_.empty()) {
        return false;
    }
    return operation_type(pending_operations_.front()) == operation_type(payload)
        && operation_object_id(pending_operations_.front()) == operation_object_id(payload);
}

}  // namespace online_board::client_qt
