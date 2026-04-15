#pragma once

#include <map>
#include <string>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"

namespace online_board::persistence {

class InMemoryUserRepository final : public application::IUserRepository {
public:
    std::optional<domain::User> find_by_id(const common::UserId& user_id) const override {
        auto it = users_by_id_.find(user_id);
        if (it == users_by_id_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<domain::User> find_by_login(const std::string& login) const override {
        auto it = user_id_by_login_.find(login);
        if (it == user_id_by_login_.end()) {
            return std::nullopt;
        }
        return find_by_id(it->second);
    }

    void save(domain::User user) override {
        user_id_by_login_[user.login] = user.id;
        users_by_id_[user.id] = std::move(user);
    }

private:
    std::map<common::UserId, domain::User> users_by_id_;
    std::map<std::string, common::UserId> user_id_by_login_;
};

}  // namespace online_board::persistence
