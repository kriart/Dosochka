#include "json_serialization.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace online_board::server::json_helpers {

json::object to_json(const domain::Board& board) {
    return {
        {"id", board.id.value},
        {"owner_user_id", board.owner_user_id.value},
        {"title", board.title},
        {"access_mode", board.access_mode == domain::AccessMode::private_board ? "private_board" : "password_protected"},
        {"guest_policy", board.guest_policy == domain::GuestPolicy::guest_disabled ? "guest_disabled" : board.guest_policy == domain::GuestPolicy::guest_view_only ? "guest_view_only" : "guest_edit_allowed"},
        {"last_revision", board.last_revision},
        {"created_at", to_unix_millis(board.created_at)},
        {"updated_at", to_unix_millis(board.updated_at)},
    };
}

json::object to_json(const domain::BoardParticipant& participant) {
    return {
        {"board_id", participant.board_id.value},
        {"principal", to_json(participant.principal)},
        {"nickname", participant.nickname},
        {"role", to_string(participant.role)},
        {"joined_at", to_unix_millis(participant.joined_at)},
    };
}

json::object to_json(const domain::BoardObject& object) {
    json::object payload;
    if (const auto* stroke = std::get_if<domain::StrokePayload>(&object.payload)) {
        json::array points;
        for (const auto& point : stroke->points) {
            points.emplace_back(to_json(point));
        }
        payload = {{"type", "stroke"}, {"color", stroke->color}, {"thickness", stroke->thickness}, {"points", std::move(points)}, {"finished", stroke->finished}};
    } else if (const auto* shape = std::get_if<domain::ShapePayload>(&object.payload)) {
        payload = {{"type", "shape"}, {"shape_type", to_string(shape->shape_type)}, {"geometry", to_json(shape->geometry)}, {"stroke_color", shape->stroke_color}, {"fill_color", shape->fill_color.has_value() ? json::value(*shape->fill_color) : json::value(nullptr)}, {"stroke_width", shape->stroke_width}};
    } else {
        const auto& text = std::get<domain::TextPayload>(object.payload);
        payload = {{"type", "text"}, {"position", to_json(text.position)}, {"size", to_json(text.size)}, {"text", text.text}, {"font_family", text.font_family}, {"font_size", text.font_size}, {"color", text.color}};
    }

    return {{"object_id", object.object_id.value}, {"board_id", object.board_id.value}, {"created_by", to_json(object.created_by)}, {"created_at", to_unix_millis(object.created_at)}, {"updated_at", to_unix_millis(object.updated_at)}, {"is_deleted", object.is_deleted}, {"z_index", object.z_index}, {"payload", std::move(payload)}};
}

json::array to_json(const std::vector<domain::BoardParticipant>& participants) {
    json::array array;
    for (const auto& participant : participants) {
        array.emplace_back(to_json(participant));
    }
    return array;
}

json::object to_json(const domain::BoardSnapshot& snapshot) {
    std::vector<const domain::BoardObject*> ordered_objects;
    ordered_objects.reserve(snapshot.objects.size());
    for (const auto& [_, object] : snapshot.objects) {
        ordered_objects.push_back(&object);
    }
    std::sort(ordered_objects.begin(), ordered_objects.end(), [](const domain::BoardObject* lhs, const domain::BoardObject* rhs) {
        if (lhs->z_index != rhs->z_index) {
            return lhs->z_index < rhs->z_index;
        }
        if (lhs->created_at != rhs->created_at) {
            return lhs->created_at < rhs->created_at;
        }
        return lhs->object_id.value < rhs->object_id.value;
    });

    json::array objects;
    for (const auto* object : ordered_objects) {
        objects.emplace_back(to_json(*object));
    }

    return {{"board_id", snapshot.board_id.value}, {"revision", snapshot.revision}, {"objects", std::move(objects)}, {"active_participants", to_json(snapshot.active_participants)}};
}

json::value to_json(const domain::OperationPayload& payload) {
    return std::visit(
        [](const auto& command) -> json::value {
            using Payload = std::decay_t<decltype(command)>;
            if constexpr (std::is_same_v<Payload, domain::CreateShapeCommand>) {
                return json::object{{"type", "create_shape"}, {"object_id", command.object_id.value}, {"shape_type", to_string(command.shape_type)}, {"geometry", to_json(command.geometry)}, {"stroke_color", command.stroke_color}, {"fill_color", command.fill_color.has_value() ? json::value(*command.fill_color) : json::value(nullptr)}, {"stroke_width", command.stroke_width}, {"z_index", command.z_index}};
            } else if constexpr (std::is_same_v<Payload, domain::CreateTextCommand>) {
                return json::object{{"type", "create_text"}, {"object_id", command.object_id.value}, {"position", to_json(command.position)}, {"size", to_json(command.size)}, {"text", command.text}, {"font_family", command.font_family}, {"font_size", command.font_size}, {"color", command.color}, {"z_index", command.z_index}};
            } else if constexpr (std::is_same_v<Payload, domain::BeginStrokeCommand>) {
                return json::object{{"type", "begin_stroke"}, {"object_id", command.object_id.value}, {"color", command.color}, {"thickness", command.thickness}, {"start_point", to_json(command.start_point)}, {"z_index", command.z_index}};
            } else if constexpr (std::is_same_v<Payload, domain::AppendStrokePointsCommand>) {
                json::array points;
                for (const auto& point : command.points) {
                    points.emplace_back(to_json(point));
                }
                return json::object{{"type", "append_stroke_points"}, {"object_id", command.object_id.value}, {"points", std::move(points)}};
            } else if constexpr (std::is_same_v<Payload, domain::EndStrokeCommand>) {
                return json::object{{"type", "end_stroke"}, {"object_id", command.object_id.value}};
            } else if constexpr (std::is_same_v<Payload, domain::UpdateShapeGeometryCommand>) {
                return json::object{{"type", "update_shape_geometry"}, {"object_id", command.object_id.value}, {"geometry", to_json(command.geometry)}};
            } else if constexpr (std::is_same_v<Payload, domain::UpdateShapeStyleCommand>) {
                return json::object{{"type", "update_shape_style"}, {"object_id", command.object_id.value}, {"stroke_color", command.stroke_color}, {"fill_color", command.fill_color.has_value() ? json::value(*command.fill_color) : json::value(nullptr)}, {"stroke_width", command.stroke_width}};
            } else if constexpr (std::is_same_v<Payload, domain::UpdateTextContentCommand>) {
                return json::object{{"type", "update_text_content"}, {"object_id", command.object_id.value}, {"text", command.text}};
            } else if constexpr (std::is_same_v<Payload, domain::UpdateTextStyleCommand>) {
                return json::object{{"type", "update_text_style"}, {"object_id", command.object_id.value}, {"font_family", command.font_family}, {"font_size", command.font_size}, {"color", command.color}};
            } else if constexpr (std::is_same_v<Payload, domain::ResizeObjectCommand>) {
                return json::object{{"type", "resize_object"}, {"object_id", command.object_id.value}, {"size", to_json(command.size)}};
            } else {
                return json::object{{"type", "delete_object"}, {"object_id", command.object_id.value}};
            }
        },
        payload);
}

json::object to_json(const domain::AppliedOperation& operation) {
    return {{"operation_id", operation.operation_id.value}, {"board_id", operation.board_id.value}, {"actor", to_json(operation.actor)}, {"revision", operation.revision}, {"applied_at", to_unix_millis(operation.applied_at)}, {"payload", to_json(operation.payload)}};
}

std::string serialize_message(std::string_view type, json::object payload) {
    return json::serialize(json::object{{"type", std::string(type)}, {"payload", std::move(payload)}});
}

std::string serialize_error(common::ErrorCode code, std::string message) {
    return serialize_message(protocol::message_type::error, {{"code", to_string(code)}, {"message", std::move(message)}});
}

std::string serialize_register_response(const application::RegisterUserResponse& response) {
    return serialize_message(protocol::message_type::register_response, {{"user_id", response.user.id.value}, {"display_name", response.user.display_name}, {"token", response.raw_token}});
}

std::string serialize_login_response(const application::LoginResponse& response) {
    return serialize_message(protocol::message_type::login_response, {{"user_id", response.user.id.value}, {"display_name", response.user.display_name}, {"token", response.raw_token}});
}

std::string serialize_guest_enter_response(const application::GuestEnterResponse& response) {
    return serialize_message(protocol::message_type::guest_enter_response, {{"guest_id", response.session.id.value}, {"nickname", response.session.nickname}});
}

std::string serialize_session_restore_response(const domain::User& user) {
    return serialize_message(protocol::message_type::session_restore_response, {{"user_id", user.id.value}, {"display_name", user.display_name}});
}

std::string serialize_create_board_response(const domain::Board& board) {
    return serialize_message(protocol::message_type::create_board_response, {{"board", to_json(board)}});
}

std::string serialize_list_user_boards_response(const std::vector<application::UserBoardSummary>& boards) {
    json::array board_entries;
    for (const auto& summary : boards) {
        board_entries.emplace_back(json::object{
            {"board", to_json(summary.board)},
            {"role", to_string(summary.role)},
        });
    }
    return serialize_message(protocol::message_type::list_user_boards_response, {{"boards", std::move(board_entries)}});
}

std::string serialize_delete_board_response(const common::BoardId& board_id) {
    return serialize_message(protocol::message_type::delete_board_response, {{"board_id", board_id.value}});
}

std::string serialize_join_board_response(const domain::Board& board, domain::BoardRole role, const domain::BoardSnapshot& snapshot) {
    return serialize_message(protocol::message_type::join_board_response, {{"board", to_json(board)}, {"role", to_string(role)}, {"snapshot", to_json(snapshot)}});
}

std::string serialize_leave_board_response() {
    return serialize_message(protocol::message_type::leave_board_response, {});
}

std::string serialize_board_deleted(const common::BoardId& board_id) {
    return serialize_message(protocol::message_type::board_deleted, {{"board_id", board_id.value}});
}

std::string serialize_presence_update(const std::vector<domain::BoardParticipant>& participants) {
    return serialize_message(protocol::message_type::presence_update, {{"active_participants", to_json(participants)}});
}

std::string serialize_operation_applied(const domain::AppliedOperation& operation) {
    return serialize_message(protocol::message_type::operation_applied, {{"operation", to_json(operation)}});
}

std::string serialize_pong() {
    return serialize_message(protocol::message_type::pong, {});
}

}  // namespace online_board::server::json_helpers
