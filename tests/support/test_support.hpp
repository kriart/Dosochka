#pragma once

#include <chrono>
#include <string>

#include "online_board/application/security_interfaces.hpp"
#include "online_board/common/clock.hpp"

struct FakeClock final : online_board::common::IClock {
    online_board::common::Timestamp current {
        std::chrono::system_clock::from_time_t(1700000000)
    };

    [[nodiscard]] online_board::common::Timestamp now() const override {
        return current;
    }
};

struct FakePasswordHasher final : online_board::application::IPasswordHasher {
    [[nodiscard]] std::string hash(const std::string& value) const override {
        return "hash:" + value;
    }

    [[nodiscard]] bool verify(const std::string& value, const std::string& hash_value) const override {
        return hash(value) == hash_value;
    }
};

struct SequentialTokenGenerator final : online_board::application::ITokenGenerator {
    int counter {1};

    [[nodiscard]] std::string next_token() override {
        return "token-" + std::to_string(counter++);
    }
};

struct SequentialIdGenerator final : online_board::application::IIdGenerator {
    mutable int counter {1};

    [[nodiscard]] std::string next_id() const override {
        return "id-" + std::to_string(counter++);
    }
};
