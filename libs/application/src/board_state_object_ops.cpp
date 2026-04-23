#include "board_state_detail.hpp"

namespace online_board::application::detail {

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand&,
    const common::Timestamp& applied_at,
    const domain::ResizeObjectCommand& payload) {
    auto* object = find_active_object(snapshot, payload.object_id);
    if (object == nullptr) {
        return fail_snapshot(common::ErrorCode::not_found, "Object was not found");
    }
    if (!has_positive_size(payload.size)) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Resize requires positive size");
    }

    if (auto* shape = std::get_if<domain::ShapePayload>(&object->payload)) {
        shape->geometry.width = payload.size.width;
        shape->geometry.height = payload.size.height;
    } else if (auto* text = std::get_if<domain::TextPayload>(&object->payload)) {
        text->size = payload.size;
    } else {
        return fail_snapshot(common::ErrorCode::invalid_state, "Resize is unsupported for stroke");
    }

    object->updated_at = applied_at;
    return snapshot;
}

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand&,
    const common::Timestamp& applied_at,
    const domain::DeleteObjectCommand& payload) {
    auto* object = find_active_object(snapshot, payload.object_id);
    if (object == nullptr) {
        return fail_snapshot(common::ErrorCode::not_found, "Object was not found");
    }

    object->is_deleted = true;
    object->updated_at = applied_at;
    return snapshot;
}

}  // namespace online_board::application::detail
