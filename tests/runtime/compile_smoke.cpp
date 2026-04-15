#include <chrono>

#include "online_board/application/services/auth_service.hpp"
#include "online_board/application/services/board_access_service.hpp"
#include "online_board/application/services/board_service.hpp"
#include "online_board/application/services/board_state_service.hpp"
#include "online_board/application/services/operation_service.hpp"
#include "online_board/application/services/presence_service.hpp"
#include "online_board/persistence/in_memory_repositories.hpp"
#include "online_board/runtime/board_registry.hpp"

namespace {

struct SmokeClock final : online_board::common::IClock {
    [[nodiscard]] online_board::common::Timestamp now() const override {
        return std::chrono::system_clock::now();
    }
};

struct SmokeHasher final : online_board::application::IPasswordHasher {
    [[nodiscard]] std::string hash(const std::string& value) const override {
        return value;
    }

    [[nodiscard]] bool verify(const std::string& value, const std::string& hash_value) const override {
        return value == hash_value;
    }
};

struct SmokeTokenGenerator final : online_board::application::ITokenGenerator {
    [[nodiscard]] std::string next_token() override {
        return "token";
    }
};

struct SmokeIdGenerator final : online_board::application::IIdGenerator {
    [[nodiscard]] std::string next_id() const override {
        return "id";
    }
};

}  // namespace

int main() {
    using namespace online_board;

    persistence::InMemoryUserRepository user_repository;
    persistence::InMemorySessionRepository session_repository;
    persistence::InMemoryBoardRepository board_repository;
    persistence::InMemoryBoardMemberRepository member_repository;
    persistence::InMemoryBoardObjectRepository object_repository;
    persistence::InMemoryBoardOperationRepository operation_repository;
    persistence::InMemoryBoardRuntimePersistence runtime_persistence(
        board_repository,
        object_repository,
        operation_repository);

    SmokeClock clock;
    SmokeHasher hasher;
    SmokeTokenGenerator token_generator;
    SmokeIdGenerator id_generator;

    application::BoardAccessService access_service;
    application::BoardStateService state_service;
    application::PresenceService presence_service;
    application::OperationService operation_service(access_service, state_service, clock);
    application::AuthService auth_service(
        user_repository,
        session_repository,
        hasher,
        token_generator,
        id_generator,
        clock);
    application::BoardService board_service(
        board_repository,
        member_repository,
        object_repository,
        operation_repository,
        access_service,
        id_generator,
        hasher,
        clock);
    runtime::BoardRegistry registry;

    auto runtime = registry.get_or_create(
        domain::Board{.id = common::BoardId{"board"}, .owner_user_id = common::UserId{"owner"}, .title = "t"},
        domain::BoardSnapshot{.board_id = common::BoardId{"board"}, .revision = 0},
        operation_service,
        presence_service,
        clock,
        runtime_persistence);

    (void)auth_service;
    (void)board_service;
    (void)presence_service;
    (void)registry;
    (void)operation_service;
    (void)runtime;
    return 0;
}
