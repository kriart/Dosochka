#include "json_common.hpp"

namespace online_board::server::json_helpers {

std::int64_t to_unix_millis(const common::Timestamp timestamp) {
    return duration_cast<milliseconds>(timestamp.time_since_epoch()).count();
}

const json::object& expect_object(const json::value& value, std::string_view name) {
    if (!value.is_object()) {
        throw std::runtime_error(std::string(name) + " must be an object");
    }
    return value.as_object();
}

const json::array& expect_array(const json::value& value, std::string_view name) {
    if (!value.is_array()) {
        throw std::runtime_error(std::string(name) + " must be an array");
    }
    return value.as_array();
}

const json::value& expect_field(const json::object& object, std::string_view key) {
    const auto it = object.find(key);
    if (it == object.end()) {
        throw std::runtime_error(std::string(key) + " is required");
    }
    return it->value();
}

std::string expect_string(const json::object& object, std::string_view key) {
    const auto it = object.find(key);
    if (it == object.end() || !it->value().is_string()) {
        throw std::runtime_error(std::string(key) + " must be a string");
    }
    return std::string(it->value().as_string().c_str());
}

std::optional<std::string> optional_string(const json::object& object, std::string_view key) {
    const auto it = object.find(key);
    if (it == object.end() || it->value().is_null()) {
        return std::nullopt;
    }
    if (!it->value().is_string()) {
        throw std::runtime_error(std::string(key) + " must be a string");
    }
    return std::string(it->value().as_string().c_str());
}

bool expect_bool(const json::object& object, std::string_view key) {
    const auto it = object.find(key);
    if (it == object.end() || !it->value().is_bool()) {
        throw std::runtime_error(std::string(key) + " must be a boolean");
    }
    return it->value().as_bool();
}

double expect_double(const json::object& object, std::string_view key) {
    const auto it = object.find(key);
    if (it == object.end() || (!it->value().is_double() && !it->value().is_int64() && !it->value().is_uint64())) {
        throw std::runtime_error(std::string(key) + " must be numeric");
    }
    return json::value_to<double>(it->value());
}

std::int64_t expect_int64(const json::object& object, std::string_view key) {
    const auto it = object.find(key);
    if (it == object.end() || (!it->value().is_int64() && !it->value().is_uint64())) {
        throw std::runtime_error(std::string(key) + " must be integer");
    }
    return json::value_to<std::int64_t>(it->value());
}

std::uint64_t expect_uint64(const json::object& object, std::string_view key) {
    const auto it = object.find(key);
    if (it == object.end() || (!it->value().is_int64() && !it->value().is_uint64())) {
        throw std::runtime_error(std::string(key) + " must be unsigned integer");
    }
    return json::value_to<std::uint64_t>(it->value());
}

std::string to_string(common::ErrorCode code) {
    switch (code) {
    case common::ErrorCode::access_denied:
        return "access_denied";
    case common::ErrorCode::already_exists:
        return "already_exists";
    case common::ErrorCode::conflict:
        return "conflict";
    case common::ErrorCode::internal_error:
        return "internal_error";
    case common::ErrorCode::invalid_argument:
        return "invalid_argument";
    case common::ErrorCode::invalid_state:
        return "invalid_state";
    case common::ErrorCode::not_found:
        return "not_found";
    }
    return "invalid_argument";
}

domain::AccessMode access_mode_from_string(const std::string& value) {
    if (value == "private_board") {
        return domain::AccessMode::private_board;
    }
    if (value == "password_protected") {
        return domain::AccessMode::password_protected;
    }
    throw std::runtime_error("Unknown access mode");
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
    throw std::runtime_error("Unknown guest policy");
}

std::string to_string(domain::BoardRole role) {
    switch (role) {
    case domain::BoardRole::owner:
        return "owner";
    case domain::BoardRole::editor:
        return "editor";
    case domain::BoardRole::viewer:
        return "viewer";
    }
    return "viewer";
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
    throw std::runtime_error("Unknown shape type");
}

std::string to_string(domain::ShapeType shape_type) {
    switch (shape_type) {
    case domain::ShapeType::rectangle:
        return "rectangle";
    case domain::ShapeType::ellipse:
        return "ellipse";
    case domain::ShapeType::line:
        return "line";
    }
    return "rectangle";
}

json::object to_json(const domain::Point& point) {
    return {{"x", point.x}, {"y", point.y}};
}

json::object to_json(const domain::Size& size) {
    return {{"width", size.width}, {"height", size.height}};
}

json::object to_json(const domain::Rect& rect) {
    return {{"x", rect.x}, {"y", rect.y}, {"width", rect.width}, {"height", rect.height}};
}

json::object to_json(const domain::Principal& principal) {
    if (const auto* user = std::get_if<domain::RegisteredUserPrincipal>(&principal)) {
        return {{"type", "registered_user"}, {"user_id", user->user_id.value}};
    }
    const auto& guest = std::get<domain::GuestPrincipal>(principal);
    return {{"type", "guest"}, {"guest_id", guest.guest_id}, {"nickname", guest.nickname}};
}

}  // namespace online_board::server::json_helpers
