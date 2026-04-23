#pragma once

#include <string>
#include <utility>

#include "online_board/application/repository_interfaces.hpp"
#include "online_board/persistence/in_memory/in_memory_storage.hpp"

namespace online_board::persistence {

class InMemoryUserRepository final : public application::IUserRepository {
public:
    InMemoryUserRepository()
        : storage_(std::make_shared<InMemoryStorage>()) {
    }

    explicit InMemoryUserRepository(SharedInMemoryStorage storage)
        : storage_(std::move(storage)) {
    }

    std::optional<domain::User> find_by_id(const common::UserId& user_id) const override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        auto it = storage_->users_by_id.find(user_id);
        if (it == storage_->users_by_id.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<domain::User> find_by_login(const std::string& login) const override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        auto it = storage_->user_id_by_login.find(login);
        if (it == storage_->user_id_by_login.end()) {
            return std::nullopt;
        }
        auto user_it = storage_->users_by_id.find(it->second);
        if (user_it == storage_->users_by_id.end()) {
            return std::nullopt;
        }
        return user_it->second;
    }

    common::Result<std::monostate> create(domain::User user) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        if (storage_->user_id_by_login.contains(user.login)) {
            return common::fail<std::monostate>(
                common::ErrorCode::already_exists,
                "Login is already taken");
        }
        storage_->user_id_by_login[user.login] = user.id;
        storage_->users_by_id[user.id] = std::move(user);
        return std::monostate{};
    }

    void save(domain::User user) override {
        std::lock_guard<std::mutex> lock(storage_->mutex);
        storage_->user_id_by_login[user.login] = user.id;
        storage_->users_by_id[user.id] = std::move(user);
    }

private:
    SharedInMemoryStorage storage_;
};

}  // namespace online_board::persistence
