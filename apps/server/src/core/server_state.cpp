#include "server_state.hpp"

#include <algorithm>
#include <stdexcept>

#include "client_session.hpp"

namespace online_board::server {

ServerState::ServerState(ServerPersistenceConfig config)
#ifdef ONLINE_BOARD_HAS_POSTGRES
    : postgres_connection_provider_(
          config.backend == PersistenceBackend::postgres
              ? std::make_shared<persistence::PostgresConnectionProvider>(config.postgres_connection_string)
              : nullptr),
      in_memory_storage_(
          config.backend == PersistenceBackend::in_memory
              ? std::make_shared<persistence::InMemoryStorage>()
              : nullptr),
      user_repository_(
          config.backend == PersistenceBackend::postgres
              ? std::unique_ptr<application::IUserRepository>(
                    std::make_unique<persistence::PostgresUserRepository>(postgres_connection_provider_))
              : std::unique_ptr<application::IUserRepository>(
                    std::make_unique<persistence::InMemoryUserRepository>(in_memory_storage_))),
      session_repository_(
          config.backend == PersistenceBackend::postgres
              ? std::unique_ptr<application::ISessionRepository>(
                    std::make_unique<persistence::PostgresSessionRepository>(postgres_connection_provider_))
              : std::unique_ptr<application::ISessionRepository>(
                    std::make_unique<persistence::InMemorySessionRepository>(in_memory_storage_))),
      board_repository_(
          config.backend == PersistenceBackend::postgres
              ? std::unique_ptr<application::IBoardRepository>(
                    std::make_unique<persistence::PostgresBoardRepository>(postgres_connection_provider_))
              : std::unique_ptr<application::IBoardRepository>(
                    std::make_unique<persistence::InMemoryBoardRepository>(in_memory_storage_))),
      board_member_repository_(
          config.backend == PersistenceBackend::postgres
              ? std::unique_ptr<application::IBoardMemberRepository>(
                    std::make_unique<persistence::PostgresBoardMemberRepository>(postgres_connection_provider_))
              : std::unique_ptr<application::IBoardMemberRepository>(
                    std::make_unique<persistence::InMemoryBoardMemberRepository>(in_memory_storage_))),
      board_object_repository_(
          config.backend == PersistenceBackend::postgres
              ? std::unique_ptr<application::IBoardObjectRepository>(
                    std::make_unique<persistence::PostgresBoardObjectRepository>(postgres_connection_provider_))
              : std::unique_ptr<application::IBoardObjectRepository>(
                    std::make_unique<persistence::InMemoryBoardObjectRepository>(in_memory_storage_))),
      board_operation_repository_(
          config.backend == PersistenceBackend::postgres
              ? std::unique_ptr<application::IBoardOperationRepository>(
                    std::make_unique<persistence::PostgresBoardOperationRepository>(postgres_connection_provider_))
              : std::unique_ptr<application::IBoardOperationRepository>(
                    std::make_unique<persistence::InMemoryBoardOperationRepository>(in_memory_storage_))),
      board_lifecycle_persistence_(
          config.backend == PersistenceBackend::postgres
              ? std::unique_ptr<application::IBoardLifecyclePersistence>(
                    std::make_unique<persistence::PostgresBoardLifecyclePersistence>(postgres_connection_provider_))
              : std::unique_ptr<application::IBoardLifecyclePersistence>(
                    std::make_unique<persistence::InMemoryBoardLifecyclePersistence>(in_memory_storage_))),
      board_runtime_persistence_(
          config.backend == PersistenceBackend::postgres
              ? std::unique_ptr<application::IBoardRuntimePersistence>(
                    std::make_unique<persistence::PostgresBoardRuntimePersistence>(postgres_connection_provider_))
              : std::unique_ptr<application::IBoardRuntimePersistence>(
                    std::make_unique<persistence::InMemoryBoardRuntimePersistence>(in_memory_storage_))),
#else
    : in_memory_storage_(std::make_shared<persistence::InMemoryStorage>()),
      user_repository_(std::make_unique<persistence::InMemoryUserRepository>(in_memory_storage_)),
      session_repository_(std::make_unique<persistence::InMemorySessionRepository>(in_memory_storage_)),
      board_repository_(std::make_unique<persistence::InMemoryBoardRepository>(in_memory_storage_)),
      board_member_repository_(std::make_unique<persistence::InMemoryBoardMemberRepository>(in_memory_storage_)),
      board_object_repository_(std::make_unique<persistence::InMemoryBoardObjectRepository>(in_memory_storage_)),
      board_operation_repository_(std::make_unique<persistence::InMemoryBoardOperationRepository>(in_memory_storage_)),
      board_lifecycle_persistence_(std::make_unique<persistence::InMemoryBoardLifecyclePersistence>(in_memory_storage_)),
      board_runtime_persistence_(std::make_unique<persistence::InMemoryBoardRuntimePersistence>(in_memory_storage_)),
#endif
      auth_service_(*user_repository_, *session_repository_, password_hasher_, token_generator_, id_generator_, clock_),
      operation_service_(board_access_service_, board_state_service_, clock_),
      board_service_(
          *board_repository_,
          *board_member_repository_,
          *board_object_repository_,
          *board_operation_repository_,
          *board_lifecycle_persistence_,
          board_access_service_,
          id_generator_,
          password_hasher_,
          clock_) {
#ifndef ONLINE_BOARD_HAS_POSTGRES
    if (config.backend == PersistenceBackend::postgres) {
        throw std::runtime_error("Server was built without PostgreSQL support");
    }
#endif
}

void ServerState::subscribe(const common::BoardId& board_id, const std::shared_ptr<ClientSession>& session) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_[board_id.value].push_back(session);
}

void ServerState::unsubscribe(const common::BoardId& board_id, ClientSession* session) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = subscriptions_.find(board_id.value);
    if (it == subscriptions_.end()) {
        return;
    }

    auto& sessions = it->second;
    sessions.erase(
        std::remove_if(
            sessions.begin(),
            sessions.end(),
            [session](const std::weak_ptr<ClientSession>& weak_session) {
                const auto shared = weak_session.lock();
                return !shared || shared.get() == session;
            }),
        sessions.end());
}

void ServerState::broadcast(const common::BoardId& board_id, const std::string& json_message) {
    std::vector<std::shared_ptr<ClientSession>> recipients;
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        auto it = subscriptions_.find(board_id.value);
        if (it == subscriptions_.end()) {
            return;
        }

        auto& sessions = it->second;
        sessions.erase(
            std::remove_if(
                sessions.begin(),
                sessions.end(),
                [&](const std::weak_ptr<ClientSession>& weak_session) {
                    const auto shared = weak_session.lock();
                    if (!shared) {
                        return true;
                    }
                    recipients.push_back(shared);
                    return false;
                }),
            sessions.end());
    }

    for (const auto& recipient : recipients) {
        recipient->send_json(json_message);
    }
}

void ServerState::broadcast_presence(const common::BoardId& board_id, const std::vector<domain::BoardParticipant>& participants) {
    broadcast(board_id, json_helpers::serialize_presence_update(participants));
}

void ServerState::close_board(const common::BoardId& board_id, ClientSession* initiator) {
    board_registry_.remove(board_id);
    board_access_coordinator_.forget(board_id);

    std::vector<std::shared_ptr<ClientSession>> subscribers;
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        auto it = subscriptions_.find(board_id.value);
        if (it != subscriptions_.end()) {
            for (const auto& weak_session : it->second) {
                if (const auto session = weak_session.lock()) {
                    subscribers.push_back(session);
                }
            }
            subscriptions_.erase(it);
        }
    }

    for (const auto& session : subscribers) {
        session->on_board_deleted(board_id);
        if (session.get() != initiator) {
            session->send_json(json_helpers::serialize_board_deleted(board_id));
        }
    }
}

application::IUserRepository& ServerState::user_repository() noexcept {
    return *user_repository_;
}

application::ISessionRepository& ServerState::session_repository() noexcept {
    return *session_repository_;
}

application::IBoardRepository& ServerState::board_repository() noexcept {
    return *board_repository_;
}

application::IBoardMemberRepository& ServerState::board_member_repository() noexcept {
    return *board_member_repository_;
}

application::IBoardObjectRepository& ServerState::board_object_repository() noexcept {
    return *board_object_repository_;
}

application::IBoardOperationRepository& ServerState::board_operation_repository() noexcept {
    return *board_operation_repository_;
}

application::IBoardLifecyclePersistence& ServerState::board_lifecycle_persistence() noexcept {
    return *board_lifecycle_persistence_;
}

application::IBoardRuntimePersistence& ServerState::board_runtime_persistence() noexcept {
    return *board_runtime_persistence_;
}

common::SystemClock& ServerState::clock() noexcept {
    return clock_;
}

application::AuthService& ServerState::auth_service() noexcept {
    return auth_service_;
}

application::BoardService& ServerState::board_service() noexcept {
    return board_service_;
}

application::OperationService& ServerState::operation_service() noexcept {
    return operation_service_;
}

application::PresenceService& ServerState::presence_service() noexcept {
    return presence_service_;
}

runtime::BoardRegistry& ServerState::board_registry() noexcept {
    return board_registry_;
}

BoardAccessCoordinator& ServerState::board_access_coordinator() noexcept {
    return board_access_coordinator_;
}

}  // namespace online_board::server
