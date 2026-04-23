#include "board_state_detail.hpp"

namespace online_board::application::detail {

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::CreateShapeCommand& payload) {
    if (!has_positive_rect(payload.geometry)) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Shape size must be positive");
    }
    if (!has_non_empty_text(payload.stroke_color) || payload.stroke_width <= 0.0) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Shape style is invalid");
    }

    domain::BoardObject object{
        .object_id = payload.object_id,
        .board_id = snapshot.board_id,
        .type = domain::ObjectType::shape,
        .created_by = command.actor,
        .created_at = applied_at,
        .updated_at = applied_at,
        .is_deleted = false,
        .z_index = payload.z_index,
        .payload = domain::ShapePayload{
            .shape_type = payload.shape_type,
            .geometry = payload.geometry,
            .stroke_color = payload.stroke_color,
            .fill_color = payload.fill_color,
            .stroke_width = payload.stroke_width,
        },
    };
    return add_object(snapshot, std::move(object));
}

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand&,
    const common::Timestamp& applied_at,
    const domain::UpdateShapeGeometryCommand& payload) {
    auto shape_result = find_active_payload<domain::ShapePayload>(
        snapshot,
        payload.object_id,
        "Shape object was not found",
        "Target object is not a shape");
    if (!common::is_ok(shape_result)) {
        return common::error(shape_result);
    }

    auto shape_view = common::value(shape_result);
    if (!has_positive_rect(payload.geometry)) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Shape size must be positive");
    }

    shape_view.payload->geometry = payload.geometry;
    shape_view.object->updated_at = applied_at;
    return snapshot;
}

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand&,
    const common::Timestamp& applied_at,
    const domain::UpdateShapeStyleCommand& payload) {
    auto shape_result = find_active_payload<domain::ShapePayload>(
        snapshot,
        payload.object_id,
        "Shape object was not found",
        "Target object is not a shape");
    if (!common::is_ok(shape_result)) {
        return common::error(shape_result);
    }

    auto shape_view = common::value(shape_result);
    if (!has_non_empty_text(payload.stroke_color) || payload.stroke_width <= 0.0) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Shape style is invalid");
    }

    shape_view.payload->stroke_color = payload.stroke_color;
    shape_view.payload->fill_color = payload.fill_color;
    shape_view.payload->stroke_width = payload.stroke_width;
    shape_view.object->updated_at = applied_at;
    return snapshot;
}

}  // namespace online_board::application::detail
