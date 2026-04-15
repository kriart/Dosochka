#pragma once

#include <compare>
#include <string>

namespace online_board::common {

template <typename Tag>
struct StrongId {
    std::string value;

    auto operator<=>(const StrongId&) const = default;

    [[nodiscard]] bool empty() const noexcept {
        return value.empty();
    }
};

using UserId = StrongId<struct UserIdTag>;
using SessionId = StrongId<struct SessionIdTag>;
using GuestSessionId = StrongId<struct GuestSessionIdTag>;
using BoardId = StrongId<struct BoardIdTag>;
using ObjectId = StrongId<struct ObjectIdTag>;
using OperationId = StrongId<struct OperationIdTag>;

}  // namespace online_board::common
