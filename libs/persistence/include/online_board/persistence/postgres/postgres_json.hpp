#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <boost/json.hpp>

#include "online_board/domain/auth/principal.hpp"
#include "online_board/domain/board/board.hpp"
#include "online_board/domain/board/board_member.hpp"
#include "online_board/domain/board/board_object.hpp"
#include "online_board/domain/operations/applied_operation.hpp"

namespace online_board::persistence::postgres_json {

namespace json = boost::json;

std::int64_t to_unix_millis(const common::Timestamp& timestamp);
common::Timestamp from_unix_millis(std::int64_t millis);

std::string to_string(domain::PrincipalType value);
std::string to_string(domain::BoardRole value);
std::string to_string(domain::AccessMode value);
std::string to_string(domain::GuestPolicy value);
std::string to_string(domain::ObjectType value);
std::string to_string(domain::ShapeType value);

domain::PrincipalType principal_type_from_string(const std::string& value);
domain::BoardRole board_role_from_string(const std::string& value);
domain::AccessMode access_mode_from_string(const std::string& value);
domain::GuestPolicy guest_policy_from_string(const std::string& value);
domain::ObjectType object_type_from_string(const std::string& value);
domain::ShapeType shape_type_from_string(const std::string& value);

json::object point_to_json(const domain::Point& point);
domain::Point point_from_json(const json::object& object);

json::object size_to_json(const domain::Size& size);
domain::Size size_from_json(const json::object& object);

json::object rect_to_json(const domain::Rect& rect);
domain::Rect rect_from_json(const json::object& object);

std::string principal_key_value(const domain::Principal& principal);
std::optional<std::string> principal_nickname_value(const domain::Principal& principal);
domain::Principal principal_from_columns(
    const std::string& type,
    const std::string& key,
    const std::optional<std::string>& nickname);

json::object stroke_payload_to_json(const domain::StrokePayload& payload);
domain::StrokePayload stroke_payload_from_json(const json::object& object);

json::object shape_payload_to_json(const domain::ShapePayload& payload);
domain::ShapePayload shape_payload_from_json(const json::object& object);

json::object text_payload_to_json(const domain::TextPayload& payload);
domain::TextPayload text_payload_from_json(const json::object& object);

json::object board_object_payload_to_json(const domain::BoardObjectPayload& payload);
domain::BoardObjectPayload board_object_payload_from_json(
    domain::ObjectType type,
    const json::object& object);

json::object command_to_json(const domain::CreateShapeCommand& command);
json::object command_to_json(const domain::CreateTextCommand& command);
json::object command_to_json(const domain::BeginStrokeCommand& command);
json::object command_to_json(const domain::AppendStrokePointsCommand& command);
json::object command_to_json(const domain::EndStrokeCommand& command);
json::object command_to_json(const domain::UpdateShapeGeometryCommand& command);
json::object command_to_json(const domain::UpdateShapeStyleCommand& command);
json::object command_to_json(const domain::UpdateTextContentCommand& command);
json::object command_to_json(const domain::UpdateTextStyleCommand& command);
json::object command_to_json(const domain::ResizeObjectCommand& command);
json::object command_to_json(const domain::DeleteObjectCommand& command);

json::object operation_payload_to_json(const domain::OperationPayload& payload);
domain::OperationPayload operation_payload_from_json(const json::object& object);

std::string stringify_json(const json::object& object);
json::object parse_json_object(const std::string& payload);

}  // namespace online_board::persistence::postgres_json
