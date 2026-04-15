#pragma once

#include <optional>
#include <variant>

#include "online_board/domain/board/board_snapshot.hpp"

namespace online_board::domain {

struct CreateShapeCommand {
    common::ObjectId object_id;
    ShapeType shape_type {ShapeType::rectangle};
    Rect geometry;
    std::string stroke_color;
    std::optional<std::string> fill_color;
    double stroke_width {1.0};
    std::int64_t z_index {0};
};

struct CreateTextCommand {
    common::ObjectId object_id;
    Point position;
    Size size;
    std::string text;
    std::string font_family;
    double font_size {12.0};
    std::string color;
    std::int64_t z_index {0};
};

struct BeginStrokeCommand {
    common::ObjectId object_id;
    std::string color;
    double thickness {1.0};
    Point start_point;
    std::int64_t z_index {0};
};

struct AppendStrokePointsCommand {
    common::ObjectId object_id;
    std::vector<Point> points;
};

struct EndStrokeCommand {
    common::ObjectId object_id;
};

struct UpdateShapeGeometryCommand {
    common::ObjectId object_id;
    Rect geometry;
};

struct UpdateShapeStyleCommand {
    common::ObjectId object_id;
    std::string stroke_color;
    std::optional<std::string> fill_color;
    double stroke_width {1.0};
};

struct UpdateTextContentCommand {
    common::ObjectId object_id;
    std::string text;
};

struct UpdateTextStyleCommand {
    common::ObjectId object_id;
    std::string font_family;
    double font_size {12.0};
    std::string color;
};

struct ResizeObjectCommand {
    common::ObjectId object_id;
    Size size;
};

struct DeleteObjectCommand {
    common::ObjectId object_id;
};

using OperationPayload = std::variant<
    CreateShapeCommand,
    CreateTextCommand,
    BeginStrokeCommand,
    AppendStrokePointsCommand,
    EndStrokeCommand,
    UpdateShapeGeometryCommand,
    UpdateShapeStyleCommand,
    UpdateTextContentCommand,
    UpdateTextStyleCommand,
    ResizeObjectCommand,
    DeleteObjectCommand>;

struct OperationCommand {
    common::BoardId board_id;
    Principal actor;
    std::uint64_t base_revision {0};
    OperationPayload payload;
};

}  // namespace online_board::domain
