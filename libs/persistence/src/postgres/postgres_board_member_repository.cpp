#include "online_board/persistence/postgres/postgres_board_member_repository.hpp"

#include <utility>

#include "postgres_repository_utils.hpp"

namespace online_board::persistence {

PostgresBoardMemberRepository::PostgresBoardMemberRepository(
    SharedPostgresConnectionProvider connection_provider)
    : connection_provider_(std::move(connection_provider)) {
}

std::optional<domain::BoardMember> PostgresBoardMemberRepository::find_member(
    const common::BoardId& board_id,
    const common::UserId& user_id) const {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "SELECT board_id, user_id, role, created_at_ms "
        "FROM board_members WHERE board_id = $1 AND user_id = $2",
        {board_id.value, user_id.value});
    return map_optional_single_row<domain::BoardMember>(result.get(), [&](int row) {
        return member_from_result(result.get(), row);
    });
}

std::vector<domain::BoardMember> PostgresBoardMemberRepository::list_members(
    const common::BoardId& board_id) const {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "SELECT board_id, user_id, role, created_at_ms "
        "FROM board_members WHERE board_id = $1 ORDER BY created_at_ms, user_id",
        {board_id.value});
    return map_rows_to_vector<domain::BoardMember>(result.get(), [&](int row) {
        return member_from_result(result.get(), row);
    });
}

std::vector<domain::BoardMember> PostgresBoardMemberRepository::list_memberships_for_user(
    const common::UserId& user_id) const {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "SELECT board_id, user_id, role, created_at_ms "
        "FROM board_members WHERE user_id = $1 ORDER BY created_at_ms, board_id",
        {user_id.value});
    return map_rows_to_vector<domain::BoardMember>(result.get(), [&](int row) {
        return member_from_result(result.get(), row);
    });
}

void PostgresBoardMemberRepository::save(domain::BoardMember member) {
    auto connection = connection_provider_->open();
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
}

bool PostgresBoardMemberRepository::remove_member(
    const common::BoardId& board_id,
    const common::UserId& user_id) {
    auto connection = connection_provider_->open();
    auto result = exec_params(
        connection->get(),
        "DELETE FROM board_members WHERE board_id = $1 AND user_id = $2",
        {board_id.value, user_id.value});
    return affected_rows(result.get()) > 0;
}

void PostgresBoardMemberRepository::remove_board(const common::BoardId& board_id) {
    auto connection = connection_provider_->open();
    exec_params(
        connection->get(),
        "DELETE FROM board_members WHERE board_id = $1",
        {board_id.value});
}

domain::BoardMember PostgresBoardMemberRepository::member_from_result(const PGresult* result, int row) {
    return {
        common::BoardId {field(result, row, "board_id")},
        common::UserId {field(result, row, "user_id")},
        postgres_json::board_role_from_string(field(result, row, "role")),
        postgres_json::from_unix_millis(int64_field(result, row, "created_at_ms")),
    };
}

}  // namespace online_board::persistence
