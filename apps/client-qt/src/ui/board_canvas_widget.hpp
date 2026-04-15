#pragma once

#include <functional>
#include <optional>
#include <vector>

#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QPointF>
#include <QPoint>
#include <QRectF>
#include <QSet>

#include "online_board/domain/board/board_snapshot.hpp"
#include "board_canvas_scene_growth.hpp"

namespace online_board::client_qt {

class BoardCanvasWidget final : public QGraphicsView {
public:
    enum class Tool {
        pen,
        rectangle,
        text,
        eraser,
    };

    explicit BoardCanvasWidget(QWidget* parent = nullptr);

    void set_snapshot(domain::BoardSnapshot snapshot);
    void set_role(domain::BoardRole role);
    void set_tool(Tool tool);
    void set_primary_color(const QColor& color);
    void zoom_in();
    void zoom_out();
    void reset_zoom();
    [[nodiscard]] std::optional<common::ObjectId> object_at(const QPointF& scene_point) const;

    std::function<void(const std::vector<QPointF>&)> on_stroke_finished;
    std::function<void(const QRectF&)> on_rectangle_created;
    std::function<void(const QPointF&)> on_text_requested;
    std::function<void(const common::ObjectId&)> on_object_erase_requested;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;

private:
    void ensure_scene_capacity(const QPointF& scene_point);
    void ensure_scene_capacity(const QRectF& scene_rect);
    void erase_at(const QPointF& scene_point);
    void redraw();
    bool can_edit() const;
    void update_cursor();

    QGraphicsScene scene_;
    domain::BoardSnapshot snapshot_;
    domain::BoardRole role_ {domain::BoardRole::viewer};
    Tool tool_ {Tool::pen};
    QColor primary_color_ {"#14212a"};
    bool dragging_ {false};
    bool panning_ {false};
    QPointF drag_start_;
    QPointF current_drag_point_;
    QPoint last_pan_position_;
    std::vector<QPointF> stroke_points_;
    QSet<QString> erased_object_ids_;
    qreal zoom_level_ {1.0};
    BoardCanvasGrowthLocks growth_locks_;
};

}  // namespace online_board::client_qt
