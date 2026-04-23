#include "board_state_detail.hpp"

namespace online_board::application::detail {

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp& applied_at,
    const domain::BeginStrokeCommand& payload) {
    if (!has_non_empty_text(payload.color) || payload.thickness <= 0.0) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Stroke style is invalid");
    }

    domain::BoardObject object{
        .object_id = payload.object_id,
        .board_id = snapshot.board_id,
        .type = domain::ObjectType::stroke,
        .created_by = command.actor,
        .created_at = applied_at,
        .updated_at = applied_at,
        .is_deleted = false,
        .z_index = payload.z_index,
        .payload = domain::StrokePayload{
            .color = payload.color,
            .thickness = payload.thickness,
            .points = {payload.start_point},
            .finished = false,
        },
    };
    return add_object(snapshot, std::move(object));
}

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand&,
    const common::Timestamp& applied_at,
    const domain::AppendStrokePointsCommand& payload) {
    auto stroke_result = find_active_payload<domain::StrokePayload>(
        snapshot,
        payload.object_id,
        "Stroke object was not found",
        "Target object is not a stroke");
    if (!common::is_ok(stroke_result)) {
        return common::error(stroke_result);
    }

    auto stroke_view = common::value(stroke_result);
    if (stroke_view.payload->finished) {
        return fail_snapshot(common::ErrorCode::invalid_state, "Stroke is already finished");
    }
    if (payload.points.empty()) {
        return fail_snapshot(common::ErrorCode::invalid_argument, "Stroke append requires points");
    }

    stroke_view.payload->points.insert(
        stroke_view.payload->points.end(),
        payload.points.begin(),
        payload.points.end());
    stroke_view.object->updated_at = applied_at;
    return snapshot;
}

common::Result<domain::BoardSnapshot> apply_payload(
    domain::BoardSnapshot& snapshot,
    const domain::OperationCommand&,
    const common::Timestamp& applied_at,
    const domain::EndStrokeCommand& payload) {
    auto stroke_result = find_active_payload<domain::StrokePayload>(
        snapshot,
        payload.object_id,
        "Stroke object was not found",
        "Target object is not a stroke");
    if (!common::is_ok(stroke_result)) {
        return common::error(stroke_result);
    }

    auto stroke_view = common::value(stroke_result);
    if (stroke_view.payload->finished) {
        return fail_snapshot(common::ErrorCode::invalid_state, "Stroke cannot be finished");
    }

    stroke_view.payload->finished = true;
    stroke_view.object->updated_at = applied_at;
    return snapshot;
}

}  // namespace online_board::application::detail
