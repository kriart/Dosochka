#pragma once

#include <QJsonObject>
#include <QRectF>
#include <QString>

#include "online_board/domain/board/board.hpp"
#include "online_board/domain/board/board_participant.hpp"
#include "online_board/domain/board/board_snapshot.hpp"
#include "online_board/domain/operations/applied_operation.hpp"

namespace online_board::client_qt::client_json {

domain::BoardRole board_role_from_string(const QString& value);
domain::Board board_from_json(const QJsonObject& object);
std::vector<domain::BoardParticipant> participants_from_json(const QJsonArray& array);
domain::BoardSnapshot snapshot_from_json(const QJsonObject& object);
domain::AppliedOperation applied_operation_from_json(const QJsonObject& object);

QJsonObject make_point(double x, double y);
QJsonObject make_size(double width, double height);
QJsonObject make_rect(const QRectF& rect);

}  // namespace online_board::client_qt::client_json
