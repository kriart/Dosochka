#pragma once

#include <optional>
#include <string>

#include "online_board/domain/auth/user.hpp"

namespace online_board::application {

struct IUserRepository {
    virtual ~IUserRepository() = default;
    virtual std::optional<domain::User> find_by_id(const common::UserId& user_id) const = 0;
    virtual std::optional<domain::User> find_by_login(const std::string& login) const = 0;
    virtual void save(domain::User user) = 0;
};

}  // namespace online_board::application
