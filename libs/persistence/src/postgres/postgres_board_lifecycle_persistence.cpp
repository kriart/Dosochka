#include "online_board/persistence/postgres/postgres_board_lifecycle_persistence.hpp"

#include <exception>
#include <utility>

#include "online_board/persistence/postgres/postgres_json.hpp"

namespace online_board::persistence {

namespace {

void rollback_noexcept(PGconn* connection) {
    if (connection == nullptr) {
        return;
    }
    auto rollback_result = make_result(PQexec(connection, "ROLLBACK"));
    (void)rollback_result;
}

}  // namespace

PostgresBoardLifecyclePersistence::PostgresBoardLifecyclePersistence(
    SharedPostgresConnectionProvider connection_provider)
    : connection_provider_(std::move(connection_provider)) {
}

common::Result<std::monostate> PostgresBoardLifecyclePersistence::create_board_with_owner(
    const domain::Board& board,
    const domain::BoardMember& owner_member) {
    try {
        auto connection = connection_provider_->open();
        exec_params(connection->get(), "BEGIN", {});

        try {
            exec_params(
                connection->get(),
                "INSERT INTO boards (id, owner_user_id, title, access_mode, guest_policy, "
                "password_hash, last_revision, created_at_ms, updated_at_ms) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)",
                {
                    board.id.value,
                    board.owner_user_id.value,
                    board.title,
                    postgres_json::to_string(board.access_mode),
                    postgres_json::to_string(board.guest_policy),
                    board.password_hash,
                    std::to_string(board.last_revision),
                    std::to_string(postgres_json::to_unix_millis(board.created_at)),
                    std::to_string(postgres_json::to_unix_millis(board.updated_at)),
                });

            exec_params(
                connection->get(),
                "INSERT INTO board_members (board_id, user_id, role, created_at_ms) "
                "VALUES ($1, $2, $3, $4)",
                {
                    owner_member.board_id.value,
                    owner_member.user_id.value,
                    postgres_json::to_string(owner_member.role),
                    std::to_string(postgres_json::to_unix_millis(owner_member.created_at)),
                });

            exec_params(connection->get(), "COMMIT", {});
        } catch (...) {
            rollback_noexcept(connection->get());
            throw;
        }

        return std::monostate{};
    } catch (const std::exception& exception) {
        return common::fail<std::monostate>(
            common::ErrorCode::internal_error,
            exception.what());
    }
}

common::Result<bool> PostgresBoardLifecyclePersistence::upsert_member_and_touch_board(
    const domain::Board& board,
    const domain::BoardMember& member) {
    try {
        auto connection = connection_provider_->open();
        exec_params(connection->get(), "BEGIN", {});

        try {
            auto board_update = exec_params(
                connection->get(),
                "UPDATE boards SET updated_at_ms = $2 WHERE id = $1",
                {
                    board.id.value,
                    std::to_string(postgres_json::to_unix_millis(board.updated_at)),
                });
            if (affected_rows(board_update.get()) == 0) {
                rollback_noexcept(connection->get());
                return false;
            }

            exec_params(
                connection->get(),
                "INSERT INTO board_members (board_id, user_id, role, created_at_ms) "
                "VALUES ($1, $2, $3, $4) "
                "ON CONFLICT (board_id, user_id) DO UPDATE SET "
                "role = EXCLUDED.role, "
                "created_at_ms = EXCLUDED.created_at_ms",
                {
                    member.board_id.value,
                    member.user_id.value,
                    postgres_json::to_string(member.role),
                    std::to_string(postgres_json::to_unix_millis(member.created_at)),
                });

            exec_params(connection->get(), "COMMIT", {});
            return true;
        } catch (...) {
            rollback_noexcept(connection->get());
            throw;
        }
    } catch (const std::exception& exception) {
        return common::fail<bool>(
            common::ErrorCode::internal_error,
            exception.what());
    }
}

common::Result<bool> PostgresBoardLifecyclePersistence::remove_member_and_touch_board(
    const domain::Board& board,
    const common::UserId& user_id) {
    try {
        auto connection = connection_provider_->open();
        exec_params(connection->get(), "BEGIN", {});

        try {
            auto member_delete = exec_params(
                connection->get(),
                "DELETE FROM board_members WHERE board_id = $1 AND user_id = $2",
                {board.id.value, user_id.value});
            if (affected_rows(member_delete.get()) == 0) {
                rollback_noexcept(connection->get());
                return false;
            }

            auto board_update = exec_params(
                connection->get(),
                "UPDATE boards SET updated_at_ms = $2 WHERE id = $1",
                {
                    board.id.value,
                    std::to_string(postgres_json::to_unix_millis(board.updated_at)),
                });
            if (affected_rows(board_update.get()) == 0) {
                rollback_noexcept(connection->get());
                return false;
            }

            exec_params(connection->get(), "COMMIT", {});
            return true;
        } catch (...) {
            rollback_noexcept(connection->get());
            throw;
        }
    } catch (const std::exception& exception) {
        return common::fail<bool>(
            common::ErrorCode::internal_error,
            exception.what());
    }
}

common::Result<bool> PostgresBoardLifecyclePersistence::delete_board_cascade(const common::BoardId& board_id) {
    try {
        auto connection = connection_provider_->open();
        exec_params(connection->get(), "BEGIN", {});

        try {
            exec_params(
                connection->get(),
                "DELETE FROM board_members WHERE board_id = $1",
                {board_id.value});
            exec_params(
                connection->get(),
                "DELETE FROM board_objects WHERE board_id = $1",
                {board_id.value});
            exec_params(
                connection->get(),
                "DELETE FROM board_operations WHERE board_id = $1",
                {board_id.value});

            auto board_delete = exec_params(
                connection->get(),
                "DELETE FROM boards WHERE id = $1",
                {board_id.value});
            if (affected_rows(board_delete.get()) == 0) {
                rollback_noexcept(connection->get());
                return false;
            }

            exec_params(connection->get(), "COMMIT", {});
            return true;
        } catch (...) {
            rollback_noexcept(connection->get());
            throw;
        }
    } catch (const std::exception& exception) {
        return common::fail<bool>(
            common::ErrorCode::internal_error,
            exception.what());
    }
}

}  // namespace online_board::persistence
