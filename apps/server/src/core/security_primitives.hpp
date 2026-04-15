#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

#include "online_board/application/security_interfaces.hpp"

namespace online_board::server {

class SimplePasswordHasher final : public application::IPasswordHasher {
public:
    [[nodiscard]] std::string hash(const std::string& value) const override {
        const auto hashed = std::hash<std::string>{}(value);
        std::ostringstream stream;
        stream << std::hex << hashed;
        return stream.str();
    }

    [[nodiscard]] bool verify(const std::string& value, const std::string& hash_value) const override {
        return hash(value) == hash_value;
    }
};

class AtomicTokenGenerator final : public application::ITokenGenerator {
public:
    AtomicTokenGenerator()
        : prefix_(make_process_prefix()) {
    }

    [[nodiscard]] std::string next_token() override {
        const auto id = counter_.fetch_add(1, std::memory_order_relaxed);
        return "token-" + prefix_ + "-" + std::to_string(id);
    }

private:
    static std::string make_process_prefix() {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
        const std::uint32_t seed = static_cast<std::uint32_t>(micros) ^ std::random_device{}();
        std::ostringstream stream;
        stream << std::hex << std::nouppercase << std::setw(6) << std::setfill('0') << (seed & 0xFFFFFFu);
        return stream.str();
    }

    std::atomic<std::uint64_t> counter_ {1};
    std::string prefix_;
};

class AtomicIdGenerator final : public application::IIdGenerator {
public:
    AtomicIdGenerator()
        : prefix_(make_process_prefix()) {
    }

    [[nodiscard]] std::string next_id() const override {
        const auto id = counter_.fetch_add(1, std::memory_order_relaxed);
        return prefix_ + "-" + std::to_string(id);
    }

private:
    static std::string make_process_prefix() {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
        const std::uint32_t seed = static_cast<std::uint32_t>(micros) ^ std::random_device{}();
        std::ostringstream stream;
        stream << std::hex << std::nouppercase << std::setw(6) << std::setfill('0') << (seed & 0xFFFFFFu);
        return stream.str();
    }

    mutable std::atomic<std::uint64_t> counter_ {1};
    std::string prefix_;
};

}  // namespace online_board::server
