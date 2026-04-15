#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include <boost/json.hpp>

#include "online_board/application/dto.hpp"
#include "online_board/common/result.hpp"

namespace online_board::server::json_helpers {

namespace json = boost::json;
using namespace std::chrono;

struct IncomingMessage {
    std::string type;
    json::object payload;
};

std::int64_t to_unix_millis(common::Timestamp timestamp);
const json::object& expect_object(const json::value& value, std::string_view name);
const json::array& expect_array(const json::value& value, std::string_view name);
const json::value& expect_field(const json::object& object, std::string_view key);
std::string expect_string(const json::object& object, std::string_view key);
std::optional<std::string> optional_string(const json::object& object, std::string_view key);
bool expect_bool(const json::object& object, std::string_view key);
double expect_double(const json::object& object, std::string_view key);
std::int64_t expect_int64(const json::object& object, std::string_view key);
std::uint64_t expect_uint64(const json::object& object, std::string_view key);

template <typename Id>
inline Id parse_id(const json::object& object, std::string_view key) {
    return Id{expect_string(object, key)};
}

std::string to_string(common::ErrorCode code);
domain::AccessMode access_mode_from_string(const std::string& value);
domain::GuestPolicy guest_policy_from_string(const std::string& value);
std::string to_string(domain::BoardRole role);
domain::ShapeType shape_type_from_string(const std::string& value);
std::string to_string(domain::ShapeType shape_type);
json::object to_json(const domain::Point& point);
json::object to_json(const domain::Size& size);
json::object to_json(const domain::Rect& rect);
json::object to_json(const domain::Principal& principal);

}  // namespace online_board::server::json_helpers
