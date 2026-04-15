#pragma once

#include <string>

namespace online_board::application {

struct IPasswordHasher {
    virtual ~IPasswordHasher() = default;
    virtual std::string hash(const std::string& value) const = 0;
    virtual bool verify(const std::string& value, const std::string& hash_value) const = 0;
};

struct ITokenGenerator {
    virtual ~ITokenGenerator() = default;
    virtual std::string next_token() = 0;
};

struct IIdGenerator {
    virtual ~IIdGenerator() = default;
    virtual std::string next_id() const = 0;
};

}  // namespace online_board::application
