#include "board_operation_builder.hpp"

#include <QJsonArray>
#include <QJsonValue>

#include "client_json.hpp"

namespace online_board::client_qt {

QJsonObject BoardOperationBuilder::make_rectangle(
    const QString& object_id,
    qint64 z_index,
    const QRectF& rect,
    const QColor& color,
    int stroke_width) {
    return QJsonObject{
        {"type", QStringLiteral("create_shape")},
        {"object_id", object_id},
        {"shape_type", QStringLiteral("rectangle")},
        {"geometry", client_json::make_rect(rect)},
        {"stroke_color", color.name(QColor::HexRgb)},
        {"fill_color", QJsonValue::Null},
        {"stroke_width", static_cast<double>(stroke_width)},
        {"z_index", z_index},
    };
}

QJsonObject BoardOperationBuilder::make_text(
    const QString& object_id,
    qint64 z_index,
    const QPointF& point,
    const QString& text,
    const QColor& color) {
    return QJsonObject{
        {"type", QStringLiteral("create_text")},
        {"object_id", object_id},
        {"position", client_json::make_point(point.x(), point.y())},
        {"size", client_json::make_size(260.0, 96.0)},
        {"text", text},
        {"font_family", QStringLiteral("Segoe UI")},
        {"font_size", 18.0},
        {"color", color.name(QColor::HexRgb)},
        {"z_index", z_index},
    };
}

QJsonObject BoardOperationBuilder::make_delete_object(const common::ObjectId& object_id) {
    return QJsonObject{
        {"type", QStringLiteral("delete_object")},
        {"object_id", QString::fromStdString(object_id.value)},
    };
}

std::vector<QJsonObject> BoardOperationBuilder::make_stroke_sequence(
    const QString& object_id,
    qint64 z_index,
    const std::vector<QPointF>& points,
    const QColor& color,
    int stroke_width) {
    std::vector<QJsonObject> operations;
    if (points.empty()) {
        return operations;
    }

    operations.push_back(QJsonObject{
        {"type", QStringLiteral("begin_stroke")},
        {"object_id", object_id},
        {"color", color.name(QColor::HexRgb)},
        {"thickness", static_cast<double>(stroke_width)},
        {"start_point", client_json::make_point(points.front().x(), points.front().y())},
        {"z_index", z_index},
    });

    if (points.size() > 1) {
        QJsonArray append_points;
        for (std::size_t index = 1; index < points.size(); ++index) {
            append_points.push_back(client_json::make_point(points[index].x(), points[index].y()));
        }
        operations.push_back(QJsonObject{
            {"type", QStringLiteral("append_stroke_points")},
            {"object_id", object_id},
            {"points", append_points},
        });
    }

    operations.push_back(QJsonObject{
        {"type", QStringLiteral("end_stroke")},
        {"object_id", object_id},
    });

    return operations;
}

}  // namespace online_board::client_qt
