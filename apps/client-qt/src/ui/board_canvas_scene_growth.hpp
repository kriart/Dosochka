#pragma once

#include <optional>

#include <QRectF>

namespace online_board::client_qt {

struct BoardCanvasGrowthLocks {
    bool left {false};
    bool right {false};
    bool top {false};
    bool bottom {false};
};

QRectF initial_board_scene_rect();

std::optional<QRectF> expand_scene_rect_for_target(
    const QRectF& current_scene_rect,
    const QRectF& target_rect,
    BoardCanvasGrowthLocks& locks);

std::optional<QRectF> expand_scene_rect_to_fit_content(
    const QRectF& current_scene_rect,
    const QRectF& content_bounds);

}  // namespace online_board::client_qt
