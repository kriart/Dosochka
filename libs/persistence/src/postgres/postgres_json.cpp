#include "online_board/persistence/postgres/postgres_json.hpp"

#include <chrono>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace online_board::persistence::postgres_json {

namespace {

double json_number_to_double(const json::value& value) {
    return value.is_double() ? value.as_double() : json::value_to<double>(value);
}

}  // namespace

std::int64_t to_unix_millis(const common::Timestamp& timestamp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               timestamp.time_since_epoch())
        .count();
}

common::Timestamp from_unix_millis(std::int64_t millis) {
    return common::Timestamp {std::chrono::milliseconds {millis}};
}

std::string to_string(domain::PrincipalType value) {
    switch (value) {
    case domain::PrincipalType::registered_user:
        return "registered_user";
    case domain::PrincipalType::guest:
        return "guest";
    }
    throw std::runtime_error("Unsupported principal type");
}

std::string to_string(domain::BoardRole value) {
    switch (value) {
    case domain::BoardRole::owner:
        return "owner";
    case domain::BoardRole::editor:
        return "editor";
    case domain::BoardRole::viewer:
        return "viewer";
    }
    throw std::runtime_error("Unsupported board role");
}

std::string to_string(domain::AccessMode value) {
    switch (value) {
    case domain::AccessMode::private_board:
        return "private_board";
    case domain::AccessMode::password_protected:
        return "password_protected";
    }
    throw std::runtime_error("Unsupported access mode");
}

std::string to_string(domain::GuestPolicy value) {
    switch (value) {
    case domain::GuestPolicy::guest_disabled:
        return "guest_disabled";
    case domain::GuestPolicy::guest_view_only:
        return "guest_view_only";
    case domain::GuestPolicy::guest_edit_allowed:
        return "guest_edit_allowed";
    }
    throw std::runtime_error("Unsupported guest policy");
}

std::string to_string(domain::ObjectType value) {
    switch (value) {
    case domain::ObjectType::stroke:
        return "stroke";
    case domain::ObjectType::shape:
        return "shape";
    case domain::ObjectType::text:
        return "text";
    }
    throw std::runtime_error("Unsupported object type");
}

std::string to_string(domain::ShapeType value) {
    switch (value) {
    case domain::ShapeType::rectangle:
        return "rectangle";
    case domain::ShapeType::ellipse:
        return "ellipse";
    case domain::ShapeType::line:
        return "line";
    }
    throw std::runtime_error("Unsupported shape type");
}

domain::PrincipalType principal_type_from_string(const std::string& value) {
    if (value == "registered_user") {
        return domain::PrincipalType::registered_user;
    }
    if (value == "guest") {
        return domain::PrincipalType::guest;
    }
    throw std::runtime_error("Unsupported principal type string: " + value);
}

domain::BoardRole board_role_from_string(const std::string& value) {
    if (value == "owner") {
        return domain::BoardRole::owner;
    }
    if (value == "editor") {
        return domain::BoardRole::editor;
    }
    if (value == "viewer") {
        return domain::BoardRole::viewer;
    }
    throw std::runtime_error("Unsupported board role string: " + value);
}

domain::AccessMode access_mode_from_string(const std::string& value) {
    if (value == "private_board") {
        return domain::AccessMode::private_board;
    }
    if (value == "password_protected") {
        return domain::AccessMode::password_protected;
    }
    throw std::runtime_error("Unsupported access mode string: " + value);
}

domain::GuestPolicy guest_policy_from_string(const std::string& value) {
    if (value == "guest_disabled") {
        return domain::GuestPolicy::guest_disabled;
    }
    if (value == "guest_view_only") {
        return domain::GuestPolicy::guest_view_only;
    }
    if (value == "guest_edit_allowed") {
        return domain::GuestPolicy::guest_edit_allowed;
    }
    throw std::runtime_error("Unsupported guest policy string: " + value);
}

domain::ObjectType object_type_from_string(const std::string& value) {
    if (value == "stroke") {
        return domain::ObjectType::stroke;
    }
    if (value == "shape") {
        return domain::ObjectType::shape;
    }
    if (value == "text") {
        return domain::ObjectType::text;
    }
    throw std::runtime_error("Unsupported object type string: " + value);
}

domain::ShapeType shape_type_from_string(const std::string& value) {
    if (value == "rectangle") {
        return domain::ShapeType::rectangle;
    }
    if (value == "ellipse") {
        return domain::ShapeType::ellipse;
    }
    if (value == "line") {
        return domain::ShapeType::line;
    }
    throw std::runtime_error("Unsupported shape type string: " + value);
}

json::object point_to_json(const domain::Point& point) {
    return {{"x", point.x}, {"y", point.y}};
}

domain::Point point_from_json(const json::object& object) {
    const auto& x = object.at("x");
    const auto& y = object.at("y");
    return {json_number_to_double(x), json_number_to_double(y)};
}

json::object size_to_json(const domain::Size& size) {
    return {{"width", size.width}, {"height", size.height}};
}

domain::Size size_from_json(const json::object& object) {
    const auto& width = object.at("width");
    const auto& height = object.at("height");
    return {json_number_to_double(width), json_number_to_double(height)};
}

json::object rect_to_json(const domain::Rect& rect) {
    return {
        {"x", rect.x},
        {"y", rect.y},
        {"width", rect.width},
        {"height", rect.height},
    };
}

domain::Rect rect_from_json(const json::object& object) {
    const auto& x = object.at("x");
    const auto& y = object.at("y");
    const auto& width = object.at("width");
    const auto& height = object.at("height");
    return {
        json_number_to_double(x),
        json_number_to_double(y),
        json_number_to_double(width),
        json_number_to_double(height),
    };
}

std::string principal_key_value(const domain::Principal& principal) {
    if (const auto* user = std::get_if<domain::RegisteredUserPrincipal>(&principal)) {
        return user->user_id.value;
    }
    return std::get<domain::GuestPrincipal>(principal).guest_id;
}

std::optional<std::string> principal_nickname_value(const domain::Principal& principal) {
    if (const auto* guest = std::get_if<domain::GuestPrincipal>(&principal)) {
        return guest->nickname;
    }
    return std::nullopt;
}

domain::Principal principal_from_columns(
    const std::string& type,
    const std::string& key,
    const std::optional<std::string>& nickname) {
    if (principal_type_from_string(type) == domain::PrincipalType::registered_user) {
        return domain::RegisteredUserPrincipal {common::UserId {key}};
    }
    return domain::GuestPrincipal {key, nickname.value_or("")};
}

json::object stroke_payload_to_json(const domain::StrokePayload& payload) {
    json::array points;
    for (const auto& point : payload.points) {
        points.push_back(point_to_json(point));
    }
    return {
        {"color", payload.color},
        {"thickness", payload.thickness},
        {"points", std::move(points)},
        {"finished", payload.finished},
    };
}

domain::StrokePayload stroke_payload_from_json(const json::object& object) {
    std::vector<domain::Point> points;
    for (const auto& point_value : object.at("points").as_array()) {
        points.push_back(point_from_json(point_value.as_object()));
    }
    return {
        std::string(object.at("color").as_string().c_str()),
        json_number_to_double(object.at("thickness")),
        std::move(points),
        object.at("finished").as_bool(),
    };
}

json::object shape_payload_to_json(const domain::ShapePayload& payload) {
    json::object object {
        {"shape_type", to_string(payload.shape_type)},
        {"geometry", rect_to_json(payload.geometry)},
        {"stroke_color", payload.stroke_color},
        {"stroke_width", payload.stroke_width},
    };
    object["fill_color"] = payload.fill_color.has_value() ? json::value(*payload.fill_color) : json::value(nullptr);
    return object;
}

domain::ShapePayload shape_payload_from_json(const json::object& object) {
    std::optional<std::string> fill_color;
    if (!object.at("fill_color").is_null()) {
        fill_color = std::string(object.at("fill_color").as_string().c_str());
    }
    return {
        shape_type_from_string(std::string(object.at("shape_type").as_string().c_str())),
        rect_from_json(object.at("geometry").as_object()),
        std::string(object.at("stroke_color").as_string().c_str()),
        std::move(fill_color),
        json_number_to_double(object.at("stroke_width")),
    };
}

json::object text_payload_to_json(const domain::TextPayload& payload) {
    return {
        {"position", point_to_json(payload.position)},
        {"size", size_to_json(payload.size)},
        {"text", payload.text},
        {"font_family", payload.font_family},
        {"font_size", payload.font_size},
        {"color", payload.color},
    };
}

domain::TextPayload text_payload_from_json(const json::object& object) {
    return {
        point_from_json(object.at("position").as_object()),
        size_from_json(object.at("size").as_object()),
        std::string(object.at("text").as_string().c_str()),
        std::string(object.at("font_family").as_string().c_str()),
        json_number_to_double(object.at("font_size")),
        std::string(object.at("color").as_string().c_str()),
    };
}

json::object board_object_payload_to_json(const domain::BoardObjectPayload& payload) {
    return std::visit(
        []<typename T>(const T& value) -> json::object {
            if constexpr (std::is_same_v<T, domain::StrokePayload>) {
                json::object object = stroke_payload_to_json(value);
                object["type"] = to_string(domain::ObjectType::stroke);
                return object;
            } else if constexpr (std::is_same_v<T, domain::ShapePayload>) {
                json::object object = shape_payload_to_json(value);
                object["type"] = to_string(domain::ObjectType::shape);
                return object;
            } else {
                json::object object = text_payload_to_json(value);
                object["type"] = to_string(domain::ObjectType::text);
                return object;
            }
        },
        payload);
}

domain::BoardObjectPayload board_object_payload_from_json(
    domain::ObjectType type,
    const json::object& object) {
    switch (type) {
    case domain::ObjectType::stroke:
        return stroke_payload_from_json(object);
    case domain::ObjectType::shape:
        return shape_payload_from_json(object);
    case domain::ObjectType::text:
        return text_payload_from_json(object);
    }
    throw std::runtime_error("Unsupported board object payload type");
}

json::object command_to_json(const domain::CreateShapeCommand& command) {
    json::object object {
        {"type", "create_shape"},
        {"object_id", command.object_id.value},
        {"shape_type", to_string(command.shape_type)},
        {"geometry", rect_to_json(command.geometry)},
        {"stroke_color", command.stroke_color},
        {"stroke_width", command.stroke_width},
        {"z_index", command.z_index},
    };
    object["fill_color"] = command.fill_color.has_value() ? json::value(*command.fill_color) : json::value(nullptr);
    return object;
}

json::object command_to_json(const domain::CreateTextCommand& command) {
    return {
        {"type", "create_text"},
        {"object_id", command.object_id.value},
        {"position", point_to_json(command.position)},
        {"size", size_to_json(command.size)},
        {"text", command.text},
        {"font_family", command.font_family},
        {"font_size", command.font_size},
        {"color", command.color},
        {"z_index", command.z_index},
    };
}

json::object command_to_json(const domain::BeginStrokeCommand& command) {
    return {
        {"type", "begin_stroke"},
        {"object_id", command.object_id.value},
        {"color", command.color},
        {"thickness", command.thickness},
        {"start_point", point_to_json(command.start_point)},
        {"z_index", command.z_index},
    };
}

json::object command_to_json(const domain::AppendStrokePointsCommand& command) {
    json::array points;
    for (const auto& point : command.points) {
        points.push_back(point_to_json(point));
    }
    return {
        {"type", "append_stroke_points"},
        {"object_id", command.object_id.value},
        {"points", std::move(points)},
    };
}

json::object command_to_json(const domain::EndStrokeCommand& command) {
    return {{"type", "end_stroke"}, {"object_id", command.object_id.value}};
}

json::object command_to_json(const domain::UpdateShapeGeometryCommand& command) {
    return {
        {"type", "update_shape_geometry"},
        {"object_id", command.object_id.value},
        {"geometry", rect_to_json(command.geometry)},
    };
}

json::object command_to_json(const domain::UpdateShapeStyleCommand& command) {
    json::object object {
        {"type", "update_shape_style"},
        {"object_id", command.object_id.value},
        {"stroke_color", command.stroke_color},
        {"stroke_width", command.stroke_width},
    };
    object["fill_color"] = command.fill_color.has_value() ? json::value(*command.fill_color) : json::value(nullptr);
    return object;
}

json::object command_to_json(const domain::UpdateTextContentCommand& command) {
    return {
        {"type", "update_text_content"},
        {"object_id", command.object_id.value},
        {"text", command.text},
    };
}

json::object command_to_json(const domain::UpdateTextStyleCommand& command) {
    return {
        {"type", "update_text_style"},
        {"object_id", command.object_id.value},
        {"font_family", command.font_family},
        {"font_size", command.font_size},
        {"color", command.color},
    };
}

json::object command_to_json(const domain::ResizeObjectCommand& command) {
    return {
        {"type", "resize_object"},
        {"object_id", command.object_id.value},
        {"size", size_to_json(command.size)},
    };
}

json::object command_to_json(const domain::DeleteObjectCommand& command) {
    return {{"type", "delete_object"}, {"object_id", command.object_id.value}};
}

json::object operation_payload_to_json(const domain::OperationPayload& payload) {
    return std::visit([](const auto& command) { return command_to_json(command); }, payload);
}

domain::OperationPayload operation_payload_from_json(const json::object& object) {
    const auto type = std::string(object.at("type").as_string().c_str());
    if (type == "create_shape") {
        std::optional<std::string> fill_color;
        if (!object.at("fill_color").is_null()) {
            fill_color = std::string(object.at("fill_color").as_string().c_str());
        }
        return domain::CreateShapeCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
            shape_type_from_string(std::string(object.at("shape_type").as_string().c_str())),
            rect_from_json(object.at("geometry").as_object()),
            std::string(object.at("stroke_color").as_string().c_str()),
            std::move(fill_color),
            json_number_to_double(object.at("stroke_width")),
            object.at("z_index").as_int64(),
        };
    }
    if (type == "create_text") {
        return domain::CreateTextCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
            point_from_json(object.at("position").as_object()),
            size_from_json(object.at("size").as_object()),
            std::string(object.at("text").as_string().c_str()),
            std::string(object.at("font_family").as_string().c_str()),
            json_number_to_double(object.at("font_size")),
            std::string(object.at("color").as_string().c_str()),
            object.at("z_index").as_int64(),
        };
    }
    if (type == "begin_stroke") {
        return domain::BeginStrokeCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
            std::string(object.at("color").as_string().c_str()),
            json_number_to_double(object.at("thickness")),
            point_from_json(object.at("start_point").as_object()),
            object.at("z_index").as_int64(),
        };
    }
    if (type == "append_stroke_points") {
        std::vector<domain::Point> points;
        for (const auto& point : object.at("points").as_array()) {
            points.push_back(point_from_json(point.as_object()));
        }
        return domain::AppendStrokePointsCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
            std::move(points),
        };
    }
    if (type == "end_stroke") {
        return domain::EndStrokeCommand {common::ObjectId {std::string(object.at("object_id").as_string().c_str())}};
    }
    if (type == "update_shape_geometry") {
        return domain::UpdateShapeGeometryCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
            rect_from_json(object.at("geometry").as_object()),
        };
    }
    if (type == "update_shape_style") {
        std::optional<std::string> fill_color;
        if (!object.at("fill_color").is_null()) {
            fill_color = std::string(object.at("fill_color").as_string().c_str());
        }
        return domain::UpdateShapeStyleCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
            std::string(object.at("stroke_color").as_string().c_str()),
            std::move(fill_color),
            json_number_to_double(object.at("stroke_width")),
        };
    }
    if (type == "update_text_content") {
        return domain::UpdateTextContentCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
            std::string(object.at("text").as_string().c_str()),
        };
    }
    if (type == "update_text_style") {
        return domain::UpdateTextStyleCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
            std::string(object.at("font_family").as_string().c_str()),
            json_number_to_double(object.at("font_size")),
            std::string(object.at("color").as_string().c_str()),
        };
    }
    if (type == "resize_object") {
        return domain::ResizeObjectCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
            size_from_json(object.at("size").as_object()),
        };
    }
    if (type == "delete_object") {
        return domain::DeleteObjectCommand {
            common::ObjectId {std::string(object.at("object_id").as_string().c_str())},
        };
    }
    throw std::runtime_error("Unsupported operation payload type: " + type);
}

std::string stringify_json(const json::object& object) {
    return json::serialize(object);
}

json::object parse_json_object(const std::string& payload) {
    return json::parse(payload).as_object();
}

}  // namespace online_board::persistence::postgres_json
