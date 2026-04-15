#include "client_json.hpp"

#include <chrono>
#include <optional>
#include <vector>

#include <QJsonArray>

namespace online_board::client_qt::client_json {
namespace {

domain::AccessMode access_mode_from_string(const QString& value) {
    if (value == QStringLiteral("password_protected")) {
        return domain::AccessMode::password_protected;
    }
    return domain::AccessMode::private_board;
}

domain::GuestPolicy guest_policy_from_string(const QString& value) {
    if (value == QStringLiteral("guest_view_only")) {
        return domain::GuestPolicy::guest_view_only;
    }
    if (value == QStringLiteral("guest_edit_allowed")) {
        return domain::GuestPolicy::guest_edit_allowed;
    }
    return domain::GuestPolicy::guest_disabled;
}

domain::ShapeType shape_type_from_string(const QString& value) {
    if (value == QStringLiteral("ellipse")) {
        return domain::ShapeType::ellipse;
    }
    if (value == QStringLiteral("line")) {
        return domain::ShapeType::line;
    }
    return domain::ShapeType::rectangle;
}

domain::Point point_from_json(const QJsonObject& object) {
    return {.x = object.value("x").toDouble(), .y = object.value("y").toDouble()};
}

domain::Size size_from_json(const QJsonObject& object) {
    return {.width = object.value("width").toDouble(), .height = object.value("height").toDouble()};
}

domain::Rect rect_from_json(const QJsonObject& object) {
    return {
        .x = object.value("x").toDouble(),
        .y = object.value("y").toDouble(),
        .width = object.value("width").toDouble(),
        .height = object.value("height").toDouble(),
    };
}

domain::Principal principal_from_json(const QJsonObject& object) {
    if (object.value("type").toString() == QStringLiteral("guest")) {
        return domain::GuestPrincipal{
            .guest_id = object.value("guest_id").toString().toStdString(),
            .nickname = object.value("nickname").toString().toStdString(),
        };
    }
    return domain::RegisteredUserPrincipal{.user_id = common::UserId{object.value("user_id").toString().toStdString()}};
}

domain::BoardParticipant participant_from_json(const QJsonObject& object) {
    return domain::BoardParticipant{
        .board_id = common::BoardId{object.value("board_id").toString().toStdString()},
        .principal = principal_from_json(object.value("principal").toObject()),
        .nickname = object.value("nickname").toString().toStdString(),
        .role = board_role_from_string(object.value("role").toString()),
        .joined_at = common::Timestamp{},
    };
}

domain::BoardObject board_object_from_json(const QJsonObject& object) {
    const auto payload = object.value("payload").toObject();
    domain::BoardObject board_object{
        .object_id = common::ObjectId{object.value("object_id").toString().toStdString()},
        .board_id = common::BoardId{object.value("board_id").toString().toStdString()},
        .type = domain::ObjectType::shape,
        .created_by = principal_from_json(object.value("created_by").toObject()),
        .created_at = common::Timestamp{std::chrono::milliseconds{object.value("created_at").toVariant().toLongLong()}},
        .updated_at = common::Timestamp{std::chrono::milliseconds{object.value("updated_at").toVariant().toLongLong()}},
        .is_deleted = object.value("is_deleted").toBool(),
        .z_index = object.value("z_index").toInteger(),
        .payload = domain::ShapePayload{},
    };

    const auto type = payload.value("type").toString();
    if (type == QStringLiteral("stroke")) {
        std::vector<domain::Point> points;
        for (const auto& point_value : payload.value("points").toArray()) {
            points.push_back(point_from_json(point_value.toObject()));
        }
        board_object.type = domain::ObjectType::stroke;
        board_object.payload = domain::StrokePayload{
            .color = payload.value("color").toString().toStdString(),
            .thickness = payload.value("thickness").toDouble(),
            .points = std::move(points),
            .finished = payload.value("finished").toBool(),
        };
    } else if (type == QStringLiteral("text")) {
        board_object.type = domain::ObjectType::text;
        board_object.payload = domain::TextPayload{
            .position = point_from_json(payload.value("position").toObject()),
            .size = size_from_json(payload.value("size").toObject()),
            .text = payload.value("text").toString().toStdString(),
            .font_family = payload.value("font_family").toString().toStdString(),
            .font_size = payload.value("font_size").toDouble(),
            .color = payload.value("color").toString().toStdString(),
        };
    } else {
        board_object.type = domain::ObjectType::shape;
        board_object.payload = domain::ShapePayload{
            .shape_type = shape_type_from_string(payload.value("shape_type").toString()),
            .geometry = rect_from_json(payload.value("geometry").toObject()),
            .stroke_color = payload.value("stroke_color").toString().toStdString(),
            .fill_color = payload.value("fill_color").isNull() ? std::nullopt : std::optional<std::string>{payload.value("fill_color").toString().toStdString()},
            .stroke_width = payload.value("stroke_width").toDouble(),
        };
    }

    return board_object;
}

domain::OperationPayload operation_payload_from_json(const QJsonObject& object) {
    const auto type = object.value("type").toString();
    if (type == QStringLiteral("create_shape")) {
        return domain::CreateShapeCommand{
            .object_id = common::ObjectId{object.value("object_id").toString().toStdString()},
            .shape_type = shape_type_from_string(object.value("shape_type").toString()),
            .geometry = rect_from_json(object.value("geometry").toObject()),
            .stroke_color = object.value("stroke_color").toString().toStdString(),
            .fill_color = object.value("fill_color").isNull() ? std::nullopt : std::optional<std::string>{object.value("fill_color").toString().toStdString()},
            .stroke_width = object.value("stroke_width").toDouble(),
            .z_index = object.value("z_index").toInteger(),
        };
    }
    if (type == QStringLiteral("create_text")) {
        return domain::CreateTextCommand{
            .object_id = common::ObjectId{object.value("object_id").toString().toStdString()},
            .position = point_from_json(object.value("position").toObject()),
            .size = size_from_json(object.value("size").toObject()),
            .text = object.value("text").toString().toStdString(),
            .font_family = object.value("font_family").toString().toStdString(),
            .font_size = object.value("font_size").toDouble(),
            .color = object.value("color").toString().toStdString(),
            .z_index = object.value("z_index").toInteger(),
        };
    }
    if (type == QStringLiteral("begin_stroke")) {
        return domain::BeginStrokeCommand{
            .object_id = common::ObjectId{object.value("object_id").toString().toStdString()},
            .color = object.value("color").toString().toStdString(),
            .thickness = object.value("thickness").toDouble(),
            .start_point = point_from_json(object.value("start_point").toObject()),
            .z_index = object.value("z_index").toInteger(),
        };
    }
    if (type == QStringLiteral("append_stroke_points")) {
        std::vector<domain::Point> points;
        for (const auto& point_value : object.value("points").toArray()) {
            points.push_back(point_from_json(point_value.toObject()));
        }
        return domain::AppendStrokePointsCommand{
            .object_id = common::ObjectId{object.value("object_id").toString().toStdString()},
            .points = std::move(points),
        };
    }
    if (type == QStringLiteral("end_stroke")) {
        return domain::EndStrokeCommand{.object_id = common::ObjectId{object.value("object_id").toString().toStdString()}};
    }
    if (type == QStringLiteral("resize_object")) {
        return domain::ResizeObjectCommand{.object_id = common::ObjectId{object.value("object_id").toString().toStdString()}, .size = size_from_json(object.value("size").toObject())};
    }
    if (type == QStringLiteral("update_text_content")) {
        return domain::UpdateTextContentCommand{.object_id = common::ObjectId{object.value("object_id").toString().toStdString()}, .text = object.value("text").toString().toStdString()};
    }
    if (type == QStringLiteral("update_text_style")) {
        return domain::UpdateTextStyleCommand{.object_id = common::ObjectId{object.value("object_id").toString().toStdString()}, .font_family = object.value("font_family").toString().toStdString(), .font_size = object.value("font_size").toDouble(), .color = object.value("color").toString().toStdString()};
    }
    if (type == QStringLiteral("update_shape_geometry")) {
        return domain::UpdateShapeGeometryCommand{.object_id = common::ObjectId{object.value("object_id").toString().toStdString()}, .geometry = rect_from_json(object.value("geometry").toObject())};
    }
    if (type == QStringLiteral("update_shape_style")) {
        return domain::UpdateShapeStyleCommand{.object_id = common::ObjectId{object.value("object_id").toString().toStdString()}, .stroke_color = object.value("stroke_color").toString().toStdString(), .fill_color = object.value("fill_color").isNull() ? std::nullopt : std::optional<std::string>{object.value("fill_color").toString().toStdString()}, .stroke_width = object.value("stroke_width").toDouble()};
    }
    return domain::DeleteObjectCommand{.object_id = common::ObjectId{object.value("object_id").toString().toStdString()}};
}

}  // namespace

domain::BoardRole board_role_from_string(const QString& value) {
    if (value == QStringLiteral("owner")) {
        return domain::BoardRole::owner;
    }
    if (value == QStringLiteral("editor")) {
        return domain::BoardRole::editor;
    }
    return domain::BoardRole::viewer;
}

domain::Board board_from_json(const QJsonObject& object) {
    return domain::Board{
        .id = common::BoardId{object.value("id").toString().toStdString()},
        .owner_user_id = common::UserId{object.value("owner_user_id").toString().toStdString()},
        .title = object.value("title").toString().toStdString(),
        .access_mode = access_mode_from_string(object.value("access_mode").toString()),
        .guest_policy = guest_policy_from_string(object.value("guest_policy").toString()),
        .password_hash = std::nullopt,
        .last_revision = static_cast<std::uint64_t>(object.value("last_revision").toVariant().toULongLong()),
        .created_at = common::Timestamp{std::chrono::milliseconds{object.value("created_at").toVariant().toLongLong()}},
        .updated_at = common::Timestamp{std::chrono::milliseconds{object.value("updated_at").toVariant().toLongLong()}},
    };
}

std::vector<domain::BoardParticipant> participants_from_json(const QJsonArray& array) {
    std::vector<domain::BoardParticipant> participants;
    participants.reserve(array.size());
    for (const auto& participant_value : array) {
        participants.push_back(participant_from_json(participant_value.toObject()));
    }
    return participants;
}

domain::BoardSnapshot snapshot_from_json(const QJsonObject& object) {
    domain::BoardSnapshot snapshot{
        .board_id = common::BoardId{object.value("board_id").toString().toStdString()},
        .revision = static_cast<std::uint64_t>(object.value("revision").toVariant().toULongLong()),
        .objects = {},
        .active_participants = {},
    };

    for (const auto& object_value : object.value("objects").toArray()) {
        auto board_object = board_object_from_json(object_value.toObject());
        snapshot.objects.emplace(board_object.object_id, std::move(board_object));
    }
    snapshot.active_participants = participants_from_json(object.value("active_participants").toArray());
    return snapshot;
}

domain::AppliedOperation applied_operation_from_json(const QJsonObject& object) {
    return domain::AppliedOperation{
        .operation_id = common::OperationId{object.value("operation_id").toString().toStdString()},
        .board_id = common::BoardId{object.value("board_id").toString().toStdString()},
        .actor = principal_from_json(object.value("actor").toObject()),
        .revision = static_cast<std::uint64_t>(object.value("revision").toVariant().toULongLong()),
        .applied_at = common::Timestamp{std::chrono::milliseconds{object.value("applied_at").toVariant().toLongLong()}},
        .payload = operation_payload_from_json(object.value("payload").toObject()),
    };
}

QJsonObject make_point(double x, double y) {
    return {{"x", x}, {"y", y}};
}

QJsonObject make_size(double width, double height) {
    return {{"width", width}, {"height", height}};
}

QJsonObject make_rect(const QRectF& rect) {
    return {{"x", rect.x()}, {"y", rect.y()}, {"width", rect.width()}, {"height", rect.height()}};
}

}  // namespace online_board::client_qt::client_json
