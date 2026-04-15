#include "json_parsing.hpp"

#include <utility>

namespace online_board::server::json_helpers {

domain::Point point_from_json(const json::value& value) {
    const auto& object = expect_object(value, "point");
    return domain::Point{.x = expect_double(object, "x"), .y = expect_double(object, "y")};
}

domain::Size size_from_json(const json::value& value) {
    const auto& object = expect_object(value, "size");
    return domain::Size{.width = expect_double(object, "width"), .height = expect_double(object, "height")};
}

domain::Rect rect_from_json(const json::value& value) {
    const auto& object = expect_object(value, "rect");
    return domain::Rect{
        .x = expect_double(object, "x"),
        .y = expect_double(object, "y"),
        .width = expect_double(object, "width"),
        .height = expect_double(object, "height"),
    };
}

domain::OperationPayload operation_payload_from_json(const json::value& value) {
    const auto& object = expect_object(value, "operation");
    const auto type = expect_string(object, "type");

    if (type == "create_shape") {
        return domain::CreateShapeCommand{.object_id = parse_id<common::ObjectId>(object, "object_id"), .shape_type = shape_type_from_string(expect_string(object, "shape_type")), .geometry = rect_from_json(expect_field(object, "geometry")), .stroke_color = expect_string(object, "stroke_color"), .fill_color = optional_string(object, "fill_color"), .stroke_width = expect_double(object, "stroke_width"), .z_index = expect_int64(object, "z_index")};
    }
    if (type == "create_text") {
        return domain::CreateTextCommand{.object_id = parse_id<common::ObjectId>(object, "object_id"), .position = point_from_json(expect_field(object, "position")), .size = size_from_json(expect_field(object, "size")), .text = expect_string(object, "text"), .font_family = expect_string(object, "font_family"), .font_size = expect_double(object, "font_size"), .color = expect_string(object, "color"), .z_index = expect_int64(object, "z_index")};
    }
    if (type == "begin_stroke") {
        return domain::BeginStrokeCommand{.object_id = parse_id<common::ObjectId>(object, "object_id"), .color = expect_string(object, "color"), .thickness = expect_double(object, "thickness"), .start_point = point_from_json(expect_field(object, "start_point")), .z_index = expect_int64(object, "z_index")};
    }
    if (type == "append_stroke_points") {
        std::vector<domain::Point> points;
        for (const auto& point : expect_array(expect_field(object, "points"), "points")) {
            points.push_back(point_from_json(point));
        }
        return domain::AppendStrokePointsCommand{.object_id = parse_id<common::ObjectId>(object, "object_id"), .points = std::move(points)};
    }
    if (type == "end_stroke") {
        return domain::EndStrokeCommand{.object_id = parse_id<common::ObjectId>(object, "object_id")};
    }
    if (type == "update_shape_geometry") {
        return domain::UpdateShapeGeometryCommand{.object_id = parse_id<common::ObjectId>(object, "object_id"), .geometry = rect_from_json(expect_field(object, "geometry"))};
    }
    if (type == "update_shape_style") {
        return domain::UpdateShapeStyleCommand{.object_id = parse_id<common::ObjectId>(object, "object_id"), .stroke_color = expect_string(object, "stroke_color"), .fill_color = optional_string(object, "fill_color"), .stroke_width = expect_double(object, "stroke_width")};
    }
    if (type == "update_text_content") {
        return domain::UpdateTextContentCommand{.object_id = parse_id<common::ObjectId>(object, "object_id"), .text = expect_string(object, "text")};
    }
    if (type == "update_text_style") {
        return domain::UpdateTextStyleCommand{.object_id = parse_id<common::ObjectId>(object, "object_id"), .font_family = expect_string(object, "font_family"), .font_size = expect_double(object, "font_size"), .color = expect_string(object, "color")};
    }
    if (type == "resize_object") {
        return domain::ResizeObjectCommand{.object_id = parse_id<common::ObjectId>(object, "object_id"), .size = size_from_json(expect_field(object, "size"))};
    }
    if (type == "delete_object") {
        return domain::DeleteObjectCommand{.object_id = parse_id<common::ObjectId>(object, "object_id")};
    }

    throw std::runtime_error("Unknown operation type");
}

common::Result<IncomingMessage> parse_message(std::string_view text) {
    try {
        const auto parsed = json::parse(text);
        const auto& envelope = expect_object(parsed, "message");
        return IncomingMessage{
            .type = expect_string(envelope, "type"),
            .payload = json::object(expect_object(expect_field(envelope, "payload"), "payload")),
        };
    } catch (const std::exception& exception) {
        return common::fail<IncomingMessage>(common::ErrorCode::invalid_argument, exception.what());
    }
}

}  // namespace online_board::server::json_helpers
