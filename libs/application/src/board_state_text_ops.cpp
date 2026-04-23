#include "board_state_detail.hpp"

namespace online_board::application::detail {

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::CreateTextCommand& payload) {
    if (!has_positive_size(payload.size)) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Text size must be positive");
    }
    if (!has_non_empty_text(payload.text)) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Text content cannot be empty");
    }
    if (!has_non_empty_text(payload.font_family) || !has_non_empty_text(payload.color)
        || payload.font_size <= 0.0) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Text style is invalid");
    }

    domain::BoardObject object{
        .object_id = payload.object_id,
        .board_id = snapshot.board_id,
        .type = domain::ObjectType::text,
        .created_by = command.actor,
        .created_at = applied_at,
        .updated_at = applied_at,
        .is_deleted = false,
        .z_index = payload.z_index,
        .payload = domain::TextPayload{
            .position = payload.position,
            .size = payload.size,
            .text = payload.text,
            .font_family = payload.font_family,
            .font_size = payload.font_size,
            .color = payload.color,
        },
    };
    return add_object(snapshot, std::move(object));
}

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand&,
    const common::Timestamp& applied_at,
    const domain::UpdateTextContentCommand& payload) {
    auto text_result = find_active_payload<domain::TextPayload>(
        snapshot,
        payload.object_id,
        "Text object was not found",
        "Target object is not text");
    if (!common::is_ok(text_result)) {
        return common::error(text_result);
    }

    auto text_view = common::value(text_result);
    if (!has_non_empty_text(payload.text)) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Text content cannot be empty");
    }

    text_view.payload->text = payload.text;
    text_view.object->updated_at = applied_at;
    return snapshot;
}

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand&,
    const common::Timestamp& applied_at,
    const domain::UpdateTextStyleCommand& payload) {
    auto text_result = find_active_payload<domain::TextPayload>(
        snapshot,
        payload.object_id,
        "Text object was not found",
        "Target object is not text");
    if (!common::is_ok(text_result)) {
        return common::error(text_result);
    }

    auto text_view = common::value(text_result);
    if (!has_non_empty_text(payload.font_family) || !has_non_empty_text(payload.color)
        || payload.font_size <= 0.0) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Text style is invalid");
    }

    text_view.payload->font_family = payload.font_family;
    text_view.payload->font_size = payload.font_size;
    text_view.payload->color = payload.color;
    text_view.object->updated_at = applied_at;
    return snapshot;
}

}  // namespace online_board::application::detail
