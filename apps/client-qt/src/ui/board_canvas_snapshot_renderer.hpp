#pragma once

#include <QGraphicsScene>
#include <QRectF>

#include "online_board/domain/board/board_snapshot.hpp"

namespace online_board::client_qt {

QRectF render_board_snapshot(
    QGraphicsScene& scene,
    const domain::BoardSnapshot& snapshot,
    const QRectF& initial_bounds);

}  // namespace online_board::client_qt
