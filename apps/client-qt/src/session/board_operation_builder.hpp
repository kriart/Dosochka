#pragma once

#include <vector>

#include <QColor>
#include <QJsonObject>
#include <QPointF>
#include <QRectF>
#include <QString>

#include "online_board/common/ids.hpp"

namespace online_board::client_qt {

class BoardOperationBuilder final {
public:
    static QJsonObject make_rectangle(
        const QString& object_id,
        qint64 z_index,
        const QRectF& rect,
        const QColor& color,
        int stroke_width);

    static QJsonObject make_text(
        const QString& object_id,
        qint64 z_index,
        const QPointF& point,
        const QString& text,
        const QColor& color);

    static QJsonObject make_delete_object(const common::ObjectId& object_id);

    static std::vector<QJsonObject> make_stroke_sequence(
        const QString& object_id,
        qint64 z_index,
        const std::vector<QPointF>& points,
        const QColor& color,
        int stroke_width);
};

}  // namespace online_board::client_qt
