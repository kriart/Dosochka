#pragma once

#include <string>

#include "online_board/application/services/board_state_service.hpp"

namespace online_board::application::detail {

common::Result<domain::BoardSnapshot> fail_snapshot(common::ErrorCode code, std::string message);

bool has_positive_size(const domain::Size& size);
bool has_positive_rect(const domain::Rect& rect);
bool has_non_empty_text(const std::string& value);

domain::BoardObject* find_active_object(
    domain::BoardSnapshot& snapshot,
    const common::ObjectId& object_id);

template <typename Payload>
struct ActivePayloadView {
    domain::BoardObject* object;
    Payload* payload;
};

template <typename Payload>
common::Result<ActivePayloadView<Payload>> find_active_payload(
    domain::BoardSnapshot& snapshot,
    const common::ObjectId& object_id,
    std::string not_found_message,
    std::string invalid_type_message) {
    auto* object = find_active_object(snapshot, object_id);
    if (object == nullptr) {
        return common::fail<ActivePayloadView<Payload>>(
            common::ErrorCode::not_found,
            std::move(not_found_message));
    }

    auto* payload = std::get_if<Payload>(&object->payload);
    if (payload == nullptr) {
        return common::fail<ActivePayloadView<Payload>>(
            common::ErrorCode::invalid_state,
            std::move(invalid_type_message));
    }

    return ActivePayloadView<Payload>{
        .object = object,
        .payload = payload,
    };
}

common::Result<domain::BoardSnapshot> add_object(
    domain::BoardSnapshot& snapshot,
    domain::BoardObject object);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::CreateShapeCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::UpdateShapeGeometryCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::UpdateShapeStyleCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::CreateTextCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::UpdateTextContentCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::UpdateTextStyleCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::BeginStrokeCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::AppendStrokePointsCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::EndStrokeCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::ResizeObjectCommand& payload);

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::DeleteObjectCommand& payload);

}  // namespace online_board::application::detail
