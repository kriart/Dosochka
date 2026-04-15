#pragma once

#include <chrono>

namespace online_board::common {

using Timestamp = std::chrono::system_clock::time_point;

struct IClock {
    virtual ~IClock() = default;
    virtual Timestamp now() const = 0;
};

struct SystemClock final : IClock {
    [[nodiscard]] Timestamp now() const override {
        return std::chrono::system_clock::now();
    }
};

}  // namespace online_board::common
