#include "server_state.hpp"

#include <algorithm>
#include <stdexcept>

#include "client_session.hpp"

namespace online_board::server {

namespace {

#ifdef ONLINE_BOARD_HAS_POSTGRES
template <typename Interface, typename InMemoryRepository, typename PostgresRepository>
std::unique_ptr<Interface> make_repository(
    const ServerPersistenceConfig& config,
    const persistence::SharedPostgresConnectionProvider& postgres_connection_provider) {
    if (config.backend == PersistenceBackend::postgres) {
        return std::make_unique<PostgresRepository>(postgres_connection_provider);
    }
    return std::make_unique<InMemoryRepository>();
}

std::unique_ptr<application::IBoardRuntimePersistence> make_runtime_persistence(
    const ServerPersistenceConfig& config,
    const persistence::SharedPostgresConnectionProvider& postgres_connection_provider,
    application::IBoardRepository& board_repository,
    application::IBoardObjectRepository& object_repository,
    application::IBoardOperationRepository& operation_repository) {
    if (config.backend == PersistenceBackend::postgres) {
        return std::make_unique<persistence::PostgresBoardRuntimePersistence>(postgres_connection_provider);
    }
    return std::make_unique<persistence::InMemoryBoardRuntimePersistence>(
        board_repository,
        object_repository,
        operation_repository);
}
#else
template <typename Interface, typename InMemoryRepository>
std::unique_ptr<Interface> make_repository(const ServerPersistenceConfig& config) {
    if (config.backend == PersistenceBackend::postgres) {
        throw std::runtime_error("Server was built without PostgreSQL support");
    }
    return std::make_unique<InMemoryRepository>();
}

std::unique_ptr<application::IBoardRuntimePersistence> make_runtime_persistence(
    const ServerPersistenceConfig& config,
    application::IBoardRepository& board_repository,
    application::IBoardObjectRepository& object_repository,
    application::IBoardOperationRepository& operation_repository) {
    if (config.backend == PersistenceBackend::postgres) {
        throw std::runtime_error("Server was built without PostgreSQL support");
    }
    return std::make_unique<persistence::InMemoryBoardRuntimePersistence>(
        board_repository,
        object_repository,
        operation_repository);
}
#endif

}  // namespace

ServerState::ServerState(ServerPersistenceConfig config)
#ifdef ONLINE_BOARD_HAS_POSTGRES
    : postgres_connection_provider_(
          config.backend == PersistenceBackend::postgres
              ? std::make_shared<persistence::PostgresConnectionProvider>(config.postgres_connection_string)
              : nullptr),
      user_repository_(make_repository<
                       application::IUserRepository,
                       persistence::InMemoryUserRepository,
                       persistence::PostgresUserRepository>(config, postgres_connection_provider_)),
      session_repository_(make_repository<
                          application::ISessionRepository,
                          persistence::InMemorySessionRepository,
                          persistence::PostgresSessionRepository>(config, postgres_connection_provider_)),
      board_repository_(make_repository<
                        application::IBoardRepository,
                        persistence::InMemoryBoardRepository,
                        persistence::PostgresBoardRepository>(config, postgres_connection_provider_)),
      board_member_repository_(make_repository<
                               application::IBoardMemberRepository,
                               persistence::InMemoryBoardMemberRepository,
                               persistence::PostgresBoardMemberRepository>(config, postgres_connection_provider_)),
      board_object_repository_(make_repository<
                               application::IBoardObjectRepository,
                               persistence::InMemoryBoardObjectRepository,
                               persistence::PostgresBoardObjectRepository>(config, postgres_connection_provider_)),
      board_operation_repository_(make_repository<
                                  application::IBoardOperationRepository,
                                  persistence::InMemoryBoardOperationRepository,
                                  persistence::PostgresBoardOperationRepository>(config, postgres_connection_provider_)),
      board_runtime_persistence_(make_runtime_persistence(
          config,
          postgres_connection_provider_,
          *board_repository_,
          *board_object_repository_,
          *board_operation_repository_)),
#else
    : user_repository_(make_repository<application::IUserRepository, persistence::InMemoryUserRepository>(config)),
      session_repository_(make_repository<application::ISessionRepository, persistence::InMemorySessionRepository>(config)),
      board_repository_(make_repository<application::IBoardRepository, persistence::InMemoryBoardRepository>(config)),
      board_member_repository_(
          make_repository<application::IBoardMemberRepository, persistence::InMemoryBoardMemberRepository>(config)),
      board_object_repository_(
          make_repository<application::IBoardObjectRepository, persistence::InMemoryBoardObjectRepository>(config)),
      board_operation_repository_(make_repository<
                                  application::IBoardOperationRepository,
                                  persistence::InMemoryBoardOperationRepository>(config)),
      board_runtime_persistence_(make_runtime_persistence(
          config,
          *board_repository_,
          *board_object_repository_,
          *board_operation_repository_)),
#endif
      auth_service_(*user_repository_, *session_repository_, password_hasher_, token_generator_, id_generator_, clock_),
      operation_service_(board_access_service_, board_state_service_, clock_),
      board_service_(
          *board_repository_,
          *board_member_repository_,
          *board_object_repository_,
          *board_operation_repository_,
          board_access_service_,
          id_generator_,
          password_hasher_,
          clock_) {
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

}  // namespace online_board::server
