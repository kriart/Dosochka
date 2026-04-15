#pragma once

#include <cstdint>
#include <variant>

#include "online_board/common/clock.hpp"
#include "online_board/common/ids.hpp"
#include "online_board/domain/core/enums.hpp"
#include "online_board/domain/auth/principal.hpp"
#include "online_board/domain/core/value_types.hpp"

namespace online_board::domain {

using BoardObjectPayload = std::variant<StrokePayload, ShapePayload, TextPayload>;

struct BoardObject {
    common::ObjectId object_id;
    common::BoardId board_id;
    ObjectType type {ObjectType::shape};
    Principal created_by;
    common::Timestamp created_at;
    common::Timestamp updated_at;
    bool is_deleted {false};
    std::int64_t z_index {0};
    BoardObjectPayload payload;
};

}  // namespace online_board::domain
