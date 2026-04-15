#include "board_canvas_scene_growth.hpp"

#include <algorithm>

namespace online_board::client_qt {
namespace {

constexpr qreal initial_scene_x = -3200.0;
constexpr qreal initial_scene_y = -2400.0;
constexpr qreal initial_scene_width = 6400.0;
constexpr qreal initial_scene_height = 4800.0;
constexpr qreal edge_trigger_margin = 120.0;
constexpr qreal scene_growth_step = 1600.0;

}

QRectF initial_board_scene_rect() {
    return {initial_scene_x, initial_scene_y, initial_scene_width, initial_scene_height};
}

std::optional<QRectF> expand_scene_rect_for_target(
    const QRectF& current_scene_rect,
    const QRectF& target_rect,
    BoardCanvasGrowthLocks& locks) {
    QRectF expanded = current_scene_rect;
    const QRectF target = target_rect.normalized();
    bool changed = false;

    const bool near_right = target.right() >= expanded.right() - edge_trigger_margin;
    const bool near_left = target.left() <= expanded.left() + edge_trigger_margin;
    const bool near_bottom = target.bottom() >= expanded.bottom() - edge_trigger_margin;
    const bool near_top = target.top() <= expanded.top() + edge_trigger_margin;

    if (!near_right) {
        locks.right = false;
    }
    if (!near_left) {
        locks.left = false;
    }
    if (!near_bottom) {
        locks.bottom = false;
    }
    if (!near_top) {
        locks.top = false;
    }

    if (near_right && !locks.right) {
        const qreal overshoot = std::max(0.0, target.right() - expanded.right());
        expanded.setRight(expanded.right() + scene_growth_step + overshoot);
        changed = true;
        locks.right = true;
    }

    if (near_left && !locks.left) {
        const qreal overshoot = std::max(0.0, expanded.left() - target.left());
        expanded.setLeft(expanded.left() - scene_growth_step - overshoot);
        changed = true;
        locks.left = true;
    }

    if (near_bottom && !locks.bottom) {
        const qreal overshoot = std::max(0.0, target.bottom() - expanded.bottom());
        expanded.setBottom(expanded.bottom() + scene_growth_step + overshoot);
        changed = true;
        locks.bottom = true;
    }

    if (near_top && !locks.top) {
        const qreal overshoot = std::max(0.0, expanded.top() - target.top());
        expanded.setTop(expanded.top() - scene_growth_step - overshoot);
        changed = true;
        locks.top = true;
    }

    if (!changed) {
        return std::nullopt;
    }
    return expanded;
}

std::optional<QRectF> expand_scene_rect_to_fit_content(
    const QRectF& current_scene_rect,
    const QRectF& content_bounds) {
    QRectF expanded = current_scene_rect;
    bool changed = false;
    if (content_bounds.left() < expanded.left()) {
        expanded.setLeft(content_bounds.left() - edge_trigger_margin);
        changed = true;
    }
    if (content_bounds.right() > expanded.right()) {
        expanded.setRight(content_bounds.right() + edge_trigger_margin);
        changed = true;
    }
    if (content_bounds.top() < expanded.top()) {
        expanded.setTop(content_bounds.top() - edge_trigger_margin);
        changed = true;
    }
    if (content_bounds.bottom() > expanded.bottom()) {
        expanded.setBottom(content_bounds.bottom() + edge_trigger_margin);
        changed = true;
    }
    if (!changed) {
        return std::nullopt;
    }
    return expanded;
}

}  // namespace online_board::client_qt
