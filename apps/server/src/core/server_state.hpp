#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "board_access_coordinator.hpp"
#include "online_board/application/services/auth_service.hpp"
#include "online_board/application/services/board_access_service.hpp"
#include "online_board/application/services/board_service.hpp"
#include "online_board/application/services/board_state_service.hpp"
#include "online_board/application/services/operation_service.hpp"
#include "online_board/application/services/presence_service.hpp"
#include "online_board/common/clock.hpp"
#include "online_board/persistence/in_memory_repositories.hpp"
#ifdef ONLINE_BOARD_HAS_POSTGRES
#include "online_board/persistence/postgres_repositories.hpp"
#endif
#include "online_board/runtime/board_registry.hpp"
#include "security_primitives.hpp"

namespace online_board::server {

class ClientSession;

enum class PersistenceBackend {
    in_memory,
    postgres
};

struct ServerPersistenceConfig {
    PersistenceBackend backend {PersistenceBackend::in_memory};
    std::string postgres_connection_string;
};

class ServerState {
public:
    explicit ServerState(ServerPersistenceConfig config = {});

    void subscribe(const common::BoardId& board_id, const std::shared_ptr<ClientSession>& session);
    void unsubscribe(const common::BoardId& board_id, ClientSession* session);
    void broadcast(const common::BoardId& board_id, const std::string& json_message);
    void broadcast_presence(const common::BoardId& board_id, const std::vector<domain::BoardParticipant>& participants);
    void close_board(const common::BoardId& board_id, ClientSession* initiator = nullptr);

    [[nodiscard]] application::IUserRepository& user_repository() noexcept;
    [[nodiscard]] application::ISessionRepository& session_repository() noexcept;
    [[nodiscard]] application::IBoardRepository& board_repository() noexcept;
    [[nodiscard]] application::IBoardMemberRepository& board_member_repository() noexcept;
    [[nodiscard]] application::IBoardObjectRepository& board_object_repository() noexcept;
    [[nodiscard]] application::IBoardOperationRepository& board_operation_repository() noexcept;
    [[nodiscard]] application::IBoardLifecyclePersistence& board_lifecycle_persistence() noexcept;
    [[nodiscard]] application::IBoardRuntimePersistence& board_runtime_persistence() noexcept;
    [[nodiscard]] common::SystemClock& clock() noexcept;
    [[nodiscard]] application::AuthService& auth_service() noexcept;
    [[nodiscard]] application::BoardService& board_service() noexcept;
    [[nodiscard]] application::OperationService& operation_service() noexcept;
    [[nodiscard]] application::PresenceService& presence_service() noexcept;
    [[nodiscard]] runtime::BoardRegistry& board_registry() noexcept;
    [[nodiscard]] BoardAccessCoordinator& board_access_coordinator() noexcept;

private:
#ifdef ONLINE_BOARD_HAS_POSTGRES
    persistence::SharedPostgresConnectionProvider postgres_connection_provider_;
#endif
    persistence::SharedInMemoryStorage in_memory_storage_;
    std::unique_ptr<application::IUserRepository> user_repository_;
    std::unique_ptr<application::ISessionRepository> session_repository_;
    std::unique_ptr<application::IBoardRepository> board_repository_;
    std::unique_ptr<application::IBoardMemberRepository> board_member_repository_;
    std::unique_ptr<application::IBoardObjectRepository> board_object_repository_;
    std::unique_ptr<application::IBoardOperationRepository> board_operation_repository_;
    std::unique_ptr<application::IBoardLifecyclePersistence> board_lifecycle_persistence_;
    std::unique_ptr<application::IBoardRuntimePersistence> board_runtime_persistence_;
    common::SystemClock clock_;
    SimplePasswordHasher password_hasher_;
    AtomicTokenGenerator token_generator_;
    AtomicIdGenerator id_generator_;
    application::BoardAccessService board_access_service_;
    application::BoardStateService board_state_service_;
    application::PresenceService presence_service_;
    application::AuthService auth_service_;
    application::OperationService operation_service_;
    application::BoardService board_service_;
    runtime::BoardRegistry board_registry_;
    BoardAccessCoordinator board_access_coordinator_;

    std::mutex subscriptions_mutex_;
    std::unordered_map<std::string, std::vector<std::weak_ptr<ClientSession>>> subscriptions_;
};

}  // namespace online_board::server
