#pragma once

#include <vector>

#include "json_common.hpp"

namespace online_board::server::json_helpers {

domain::Point point_from_json(const json::value& value);
domain::Size size_from_json(const json::value& value);
domain::Rect rect_from_json(const json::value& value);
domain::OperationPayload operation_payload_from_json(const json::value& value);
common::Result<IncomingMessage> parse_message(std::string_view text);

}  // namespace online_board::server::json_helpers
