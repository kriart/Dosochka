#include "online_board/application/services/board_state_service.hpp"

#include <string>
#include <type_traits>
#include <utility>

namespace online_board::application {

namespace {

common::Result<domain::BoardSnapshot> fail_snapshot(common::ErrorCode code, std::string message) {
    return common::fail<domain::BoardSnapshot>(code, std::move(message));
}

bool has_positive_size(const domain::Size& size) {
    return size.width > 0.0 && size.height > 0.0;
}

bool has_positive_rect(const domain::Rect& rect) {
    return rect.width > 0.0 && rect.height > 0.0;
}

bool has_non_empty_text(const std::string& value) {
    return !value.empty();
}

domain::BoardObject* find_active_object(
    domain::BoardSnapshot& snapshot,
    const common::ObjectId& object_id) {
    auto it = snapshot.objects.find(object_id);
    if (it == snapshot.objects.end() || it->second.is_deleted) {
        return nullptr;
    }
    return &it->second;
}

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
    domain::BoardObject object) {
    if (snapshot.objects.contains(object.object_id)) {
        return fail_snapshot(common::ErrorCode::already_exists, "Object already exists");
    }
    snapshot.objects.emplace(object.object_id, std::move(object));
    return snapshot;
}

}  // namespace

common::Result<domain::BoardSnapshot> BoardStateService::apply(
    domain::BoardSnapshot snapshot,
    const domain::OperationCommand& command,
    const common::Timestamp applied_at) const {
    if (snapshot.board_id != command.board_id) {
        return fail_snapshot(
            common::ErrorCode::invalid_argument,
            "Command board_id does not match snapshot board_id");
    }

    return std::visit(
        [&](const auto& payload) -> common::Result<domain::BoardSnapshot> {
            using Payload = std::decay_t<decltype(payload)>;

            if constexpr (std::is_same_v<Payload, domain::CreateShapeCommand>) {
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
            } else if constexpr (std::is_same_v<Payload, domain::CreateTextCommand>) {
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
            } else if constexpr (std::is_same_v<Payload, domain::BeginStrokeCommand>) {
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
            } else if constexpr (std::is_same_v<Payload, domain::AppendStrokePointsCommand>) {
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
            } else if constexpr (std::is_same_v<Payload, domain::EndStrokeCommand>) {
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
            } else if constexpr (std::is_same_v<Payload, domain::UpdateShapeGeometryCommand>) {
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
            } else if constexpr (std::is_same_v<Payload, domain::UpdateShapeStyleCommand>) {
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
            } else if constexpr (std::is_same_v<Payload, domain::UpdateTextContentCommand>) {
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
            } else if constexpr (std::is_same_v<Payload, domain::UpdateTextStyleCommand>) {
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
            } else if constexpr (std::is_same_v<Payload, domain::ResizeObjectCommand>) {
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
            } else {
                auto* object = find_active_object(snapshot, payload.object_id);
                if (object == nullptr) {
                    return fail_snapshot(common::ErrorCode::not_found, "Object was not found");
                }
                object->is_deleted = true;
                object->updated_at = applied_at;
                return snapshot;
            }
        },
        command.payload);
}

}  // namespace online_board::application
