#include "board_canvas_widget.hpp"

#include <cmath>

#include <QGraphicsItem>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QScrollBar>
#include <QWheelEvent>

#include "board_canvas_scene_growth.hpp"
#include "board_canvas_snapshot_renderer.hpp"

namespace online_board::client_qt {
namespace {

constexpr qreal zoom_step = 1.16;
constexpr qreal min_zoom_level = 0.35;
constexpr qreal max_zoom_level = 4.5;

}

BoardCanvasWidget::BoardCanvasWidget(QWidget* parent)
    : QGraphicsView(parent) {
    setScene(&scene_);
    setRenderHint(QPainter::Antialiasing, true);
    setSceneRect(initial_board_scene_rect());
    setDragMode(QGraphicsView::NoDrag);
    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(Qt::transparent);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    update_cursor();
}

void BoardCanvasWidget::set_snapshot(domain::BoardSnapshot snapshot) {
    snapshot_ = std::move(snapshot);
    redraw();
}

void BoardCanvasWidget::set_role(domain::BoardRole role) {
    role_ = role;
    update_cursor();
}

void BoardCanvasWidget::set_tool(Tool tool) {
    tool_ = tool;
    update_cursor();
}

void BoardCanvasWidget::set_primary_color(const QColor& color) {
    primary_color_ = color;
    viewport()->update();
}

void BoardCanvasWidget::zoom_in() {
    if (zoom_level_ >= max_zoom_level) {
        return;
    }

    const qreal factor = std::min(zoom_step, max_zoom_level / zoom_level_);
    scale(factor, factor);
    zoom_level_ *= factor;
}

void BoardCanvasWidget::zoom_out() {
    if (zoom_level_ <= min_zoom_level) {
        return;
    }

    const qreal factor = std::max(1.0 / zoom_step, min_zoom_level / zoom_level_);
    scale(factor, factor);
    zoom_level_ *= factor;
}

void BoardCanvasWidget::reset_zoom() {
    if (qFuzzyCompare(zoom_level_, 1.0)) {
        return;
    }

    const qreal factor = 1.0 / zoom_level_;
    scale(factor, factor);
    zoom_level_ = 1.0;
}

std::optional<common::ObjectId> BoardCanvasWidget::object_at(const QPointF& scene_point) const {
    const auto items = scene_.items(scene_point, Qt::IntersectsItemShape, Qt::DescendingOrder);
    for (const auto* item : items) {
        const auto object_id = item->data(0).toString();
        if (!object_id.isEmpty()) {
            return common::ObjectId{object_id.toStdString()};
        }
    }
    return std::nullopt;
}

void BoardCanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        panning_ = true;
        last_pan_position_ = event->pos();
        viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (!can_edit() || event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    const auto scene_point = mapToScene(event->pos());
    ensure_scene_capacity(scene_point);
    if (tool_ == Tool::eraser) {
        dragging_ = true;
        erased_object_ids_.clear();
        erase_at(scene_point);
        return;
    }

    if (tool_ == Tool::text) {
        if (on_text_requested) {
            on_text_requested(scene_point);
        }
        return;
    }

    dragging_ = true;
    drag_start_ = scene_point;
    current_drag_point_ = scene_point;
    stroke_points_.clear();
    if (tool_ == Tool::pen) {
        stroke_points_.push_back(scene_point);
    }
    viewport()->update();
}

void BoardCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        const QPoint delta = event->pos() - last_pan_position_;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        last_pan_position_ = event->pos();
        event->accept();
        return;
    }

    if (!dragging_ || !can_edit()) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    const auto scene_point = mapToScene(event->pos());
    ensure_scene_capacity(scene_point);

    if (tool_ == Tool::eraser) {
        erase_at(scene_point);
        return;
    }

    if (tool_ == Tool::pen) {
        stroke_points_.push_back(scene_point);
    }
    current_drag_point_ = scene_point;
    viewport()->update();
}

void BoardCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (panning_ && event->button() == Qt::RightButton) {
        panning_ = false;
        update_cursor();
        event->accept();
        return;
    }

    if (!dragging_ || !can_edit() || event->button() != Qt::LeftButton) {
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }

    dragging_ = false;
    const auto scene_point = mapToScene(event->pos());
    ensure_scene_capacity(scene_point);
    current_drag_point_ = scene_point;

    if (tool_ == Tool::eraser) {
        erase_at(scene_point);
        erased_object_ids_.clear();
        viewport()->update();
        return;
    }

    if (tool_ == Tool::pen) {
        stroke_points_.push_back(scene_point);
        if (stroke_points_.size() > 1 && on_stroke_finished) {
            on_stroke_finished(stroke_points_);
        }
        stroke_points_.clear();
        viewport()->update();
        return;
    }

    const auto rect = QRectF(drag_start_, scene_point).normalized();
    ensure_scene_capacity(rect);
    if (rect.width() > 1.0 && rect.height() > 1.0 && on_rectangle_created) {
        on_rectangle_created(rect);
    }
    viewport()->update();
}

void BoardCanvasWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->angleDelta().y() > 0) {
            zoom_in();
        } else if (event->angleDelta().y() < 0) {
            zoom_out();
        }
        event->accept();
        return;
    }

    QGraphicsView::wheelEvent(event);
}

void BoardCanvasWidget::drawBackground(QPainter* painter, const QRectF& rect) {
    painter->fillRect(rect, QColor("#fffaf2"));

    const qreal minor_step = 40.0;
    const qreal major_step = 200.0;
    const auto left = std::floor(rect.left() / minor_step) * minor_step;
    const auto top = std::floor(rect.top() / minor_step) * minor_step;

    QPen minor_pen(QColor("#efe5d7"));
    minor_pen.setWidthF(1.0);
    QPen major_pen(QColor("#e2d2bc"));
    major_pen.setWidthF(1.0);

    for (qreal x = left; x < rect.right(); x += minor_step) {
        painter->setPen(std::fmod(x, major_step) == 0.0 ? major_pen : minor_pen);
        painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
    }

    for (qreal y = top; y < rect.bottom(); y += minor_step) {
        painter->setPen(std::fmod(y, major_step) == 0.0 ? major_pen : minor_pen);
        painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
    }
}

void BoardCanvasWidget::ensure_scene_capacity(const QPointF& scene_point) {
    ensure_scene_capacity(QRectF(scene_point, QSizeF(1.0, 1.0)));
}

void BoardCanvasWidget::ensure_scene_capacity(const QRectF& scene_rect) {
    const auto expanded = expand_scene_rect_for_target(sceneRect(), scene_rect, growth_locks_);
    if (!expanded.has_value()) {
        return;
    }
    scene_.setSceneRect(*expanded);
    setSceneRect(*expanded);
}

void BoardCanvasWidget::erase_at(const QPointF& scene_point) {
    if (!on_object_erase_requested) {
        return;
    }

    const auto target = object_at(scene_point);
    if (!target.has_value()) {
        return;
    }

    const auto object_id = QString::fromStdString(target->value);
    if (erased_object_ids_.contains(object_id)) {
        return;
    }

    erased_object_ids_.insert(object_id);
    on_object_erase_requested(*target);
}

void BoardCanvasWidget::drawForeground(QPainter* painter, const QRectF&) {
    if (!dragging_ || !can_edit()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (tool_ == Tool::pen && stroke_points_.size() > 1) {
        QPainterPath path(stroke_points_.front());
        for (std::size_t index = 1; index < stroke_points_.size(); ++index) {
            path.lineTo(stroke_points_[index]);
        }
        QPen preview_pen(primary_color_);
        preview_pen.setWidthF(2.5);
        preview_pen.setCapStyle(Qt::RoundCap);
        preview_pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(preview_pen);
        painter->drawPath(path);
        return;
    }

    if (tool_ == Tool::rectangle) {
        QPen preview_pen(primary_color_);
        preview_pen.setWidthF(2.0);
        preview_pen.setStyle(Qt::DashLine);
        painter->setPen(preview_pen);
        auto fill = primary_color_;
        fill.setAlpha(34);
        painter->setBrush(fill);
        painter->drawRect(QRectF(drag_start_, current_drag_point_).normalized());
    }
}

void BoardCanvasWidget::redraw() {
    const auto content_bounds = render_board_snapshot(scene_, snapshot_, sceneRect());
    const auto expanded = expand_scene_rect_to_fit_content(sceneRect(), content_bounds);
    if (!expanded.has_value()) {
        return;
    }
    scene_.setSceneRect(*expanded);
    setSceneRect(*expanded);
}

bool BoardCanvasWidget::can_edit() const {
    return role_ == domain::BoardRole::owner || role_ == domain::BoardRole::editor;
}

void BoardCanvasWidget::update_cursor() {
    if (panning_) {
        viewport()->setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (!can_edit()) {
        viewport()->setCursor(Qt::OpenHandCursor);
        return;
    }

    switch (tool_) {
        case Tool::pen:
        case Tool::rectangle:
            viewport()->setCursor(Qt::CrossCursor);
            break;
        case Tool::text:
            viewport()->setCursor(Qt::IBeamCursor);
            break;
        case Tool::eraser:
            viewport()->setCursor(Qt::PointingHandCursor);
            break;
    }
}

}  // namespace online_board::client_qt
