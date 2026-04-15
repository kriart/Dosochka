#include "board_canvas_snapshot_renderer.hpp"

#include <algorithm>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QGraphicsItem>
#include <QPen>

namespace online_board::client_qt {

QRectF render_board_snapshot(
    QGraphicsScene& scene,
    const domain::BoardSnapshot& snapshot,
    const QRectF& initial_bounds) {
    scene.clear();

    std::vector<const domain::BoardObject*> objects;
    objects.reserve(snapshot.objects.size());
    for (const auto& [_, object] : snapshot.objects) {
        if (!object.is_deleted) {
            objects.push_back(&object);
        }
    }

    std::sort(
        objects.begin(),
        objects.end(),
        [](const domain::BoardObject* lhs, const domain::BoardObject* rhs) {
            if (lhs->z_index != rhs->z_index) {
                return lhs->z_index < rhs->z_index;
            }
            if (lhs->created_at != rhs->created_at) {
                return lhs->created_at < rhs->created_at;
            }
            return lhs->object_id.value < rhs->object_id.value;
        });

    QRectF content_bounds = initial_bounds;

    for (const auto* object : objects) {
        if (const auto* stroke = std::get_if<domain::StrokePayload>(&object->payload)) {
            if (stroke->points.empty()) {
                continue;
            }
            QPainterPath path(QPointF(stroke->points.front().x, stroke->points.front().y));
            for (std::size_t index = 1; index < stroke->points.size(); ++index) {
                path.lineTo(stroke->points[index].x, stroke->points[index].y);
            }
            auto* item = scene.addPath(path, QPen(QColor(QString::fromStdString(stroke->color)), stroke->thickness));
            item->setZValue(static_cast<qreal>(object->z_index));
            item->setData(0, QString::fromStdString(object->object_id.value));
            content_bounds = content_bounds.united(path.boundingRect());
            continue;
        }

        if (const auto* shape = std::get_if<domain::ShapePayload>(&object->payload)) {
            const QRectF rect(shape->geometry.x, shape->geometry.y, shape->geometry.width, shape->geometry.height);
            QPen pen(QColor(QString::fromStdString(shape->stroke_color)), shape->stroke_width);
            QBrush brush(shape->fill_color.has_value() ? QColor(QString::fromStdString(*shape->fill_color)) : Qt::transparent);
            QGraphicsItem* item = nullptr;
            if (shape->shape_type == domain::ShapeType::rectangle) {
                item = scene.addRect(rect, pen, brush);
            } else if (shape->shape_type == domain::ShapeType::ellipse) {
                item = scene.addEllipse(rect, pen, brush);
            } else {
                item = scene.addLine(rect.left(), rect.top(), rect.right(), rect.bottom(), pen);
            }
            item->setZValue(static_cast<qreal>(object->z_index));
            item->setData(0, QString::fromStdString(object->object_id.value));
            content_bounds = content_bounds.united(rect);
            continue;
        }

        const auto& text = std::get<domain::TextPayload>(object->payload);
        auto* item = scene.addText(QString::fromStdString(text.text));
        QFont font(QString::fromStdString(text.font_family), static_cast<int>(text.font_size));
        item->setFont(font);
        item->setDefaultTextColor(QColor(QString::fromStdString(text.color)));
        item->setPos(text.position.x, text.position.y);
        item->setTextWidth(text.size.width);
        item->setZValue(static_cast<qreal>(object->z_index));
        item->setData(0, QString::fromStdString(object->object_id.value));
        content_bounds = content_bounds.united(QRectF(text.position.x, text.position.y, text.size.width, text.size.height));
    }

    return content_bounds;
}

}  // namespace online_board::client_qt
