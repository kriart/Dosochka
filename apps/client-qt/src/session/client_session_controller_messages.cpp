#include "client_session_controller.hpp"

#include <QJsonArray>
#include <QTimer>

#include "client_json.hpp"
#include "online_board/protocol/message_types.hpp"

namespace online_board::client_qt {
namespace {

using client_json::applied_operation_from_json;
using client_json::board_from_json;
using client_json::board_role_from_string;
using client_json::participants_from_json;
using client_json::snapshot_from_json;

}  // namespace

void ClientSessionController::handle_message(const QString& type, const QJsonObject& payload) {
    if (type == QString::fromUtf8(online_board::protocol::message_type::error.data())) {
        handle_error_message(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::register_response.data()) ||
        type == QString::fromUtf8(online_board::protocol::message_type::login_response.data()) ||
        type == QString::fromUtf8(online_board::protocol::message_type::session_restore_response.data())) {
        handle_auth_success(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::guest_enter_response.data())) {
        handle_guest_success(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::create_board_response.data())) {
        handle_create_board_success(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::list_user_boards_response.data())) {
        handle_list_user_boards_success(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::delete_board_response.data())) {
        handle_delete_board_success(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::join_board_response.data())) {
        handle_join_board_success(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::leave_board_response.data())) {
        emit_status(QStringLiteral("Returned to cabinet"), QStringLiteral("success"));
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::board_deleted.data())) {
        handle_board_deleted(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::presence_update.data())) {
        handle_presence_update(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::operation_applied.data())) {
        handle_operation_applied_message(payload);
        return;
    }

    if (type == QString::fromUtf8(online_board::protocol::message_type::pong.data())) {
        emit_status(QStringLiteral("Server responded"), QStringLiteral("success"));
    }
}

void ClientSessionController::handle_error_message(const QJsonObject& payload) {
    const auto code = payload.value("code").toString();
    const auto message = payload.value("message").toString();
    emit_status(message, QStringLiteral("error"));

    if (code == QStringLiteral("conflict")) {
        session_state_.handle_operation_conflict();
        QTimer::singleShot(0, [this]() { flush_operation_queue(); });
        return;
    }

    session_state_.discard_front_operation_group();
    QTimer::singleShot(0, [this]() { flush_operation_queue(); });
    if (on_warning_requested) {
        on_warning_requested(QStringLiteral("Server error"), message);
    }
}

void ClientSessionController::handle_auth_success(const QJsonObject& payload) {
    identity_mode_ = IdentityMode::registered_user;
    emit_account_changed(payload.value("display_name").toString());
    emit_status(QStringLiteral("Authenticated"), QStringLiteral("success"));
    show_cabinet_page();
    list_user_boards();
    emit_session_state_changed();
}

void ClientSessionController::handle_guest_success(const QJsonObject& payload) {
    identity_mode_ = IdentityMode::guest;
    emit_account_changed(QStringLiteral("Guest: %1").arg(payload.value("nickname").toString()));
    emit_status(QStringLiteral("Guest session active"), QStringLiteral("success"));
    emit_user_boards_changed({});
    show_cabinet_page();
    emit_session_state_changed();
}

void ClientSessionController::handle_create_board_success(const QJsonObject& payload) {
    const auto board = board_from_json(payload.value("board").toObject());
    list_user_boards();
    if (on_join_board_id_suggested) {
        on_join_board_id_suggested(QString::fromStdString(board.id.value));
    }
    emit_status(QStringLiteral("Board created"), QStringLiteral("success"));
}

void ClientSessionController::handle_list_user_boards_success(const QJsonObject& payload) {
    std::vector<UserBoardEntry> boards;
    for (const auto& board_value : payload.value("boards").toArray()) {
        const auto board_entry = board_value.toObject();
        const auto board = board_from_json(board_entry.value("board").toObject());
        const auto role = board_role_from_string(board_entry.value("role").toString());
        boards.push_back(UserBoardEntry{
            .board_id = QString::fromStdString(board.id.value),
            .title = QString::fromStdString(board.title),
            .role = role_text(role),
            .access_mode = board.access_mode == domain::AccessMode::password_protected
                ? QStringLiteral("password")
                : QStringLiteral("private"),
            .can_delete = role == domain::BoardRole::owner,
        });
    }
    emit_user_boards_changed(boards);
}

void ClientSessionController::handle_delete_board_success(const QJsonObject& payload) {
    const auto deleted_board_id = payload.value("board_id").toString();
    if (session_state_.has_board() && deleted_board_id == QString::fromStdString(session_state_.board_id().value)) {
        clear_board_session();
        show_cabinet_page();
    }
    list_user_boards();
    emit_status(QStringLiteral("Board deleted"), QStringLiteral("success"));
}

void ClientSessionController::handle_join_board_success(const QJsonObject& payload) {
    const auto board = board_from_json(payload.value("board").toObject());
    const auto role = board_role_from_string(payload.value("role").toString());
    session_state_.attach(board.id, role, snapshot_from_json(payload.value("snapshot").toObject()));

    if (on_board_opened) {
        on_board_opened(session_state_.snapshot(), role);
    }
    emit_participants_changed();
    if (on_board_context_changed) {
        on_board_context_changed(
            QString::fromStdString(board.title),
            QStringLiteral("Role: %1").arg(role_text(role)),
            QString::fromStdString(board.id.value));
    }
    emit_status(QStringLiteral("Board ready"), QStringLiteral("success"));
    show_board_page();
    emit_session_state_changed();
}

void ClientSessionController::handle_board_deleted(const QJsonObject& payload) {
    const auto deleted_board_id = payload.value("board_id").toString();
    if (!session_state_.has_board() || deleted_board_id != QString::fromStdString(session_state_.board_id().value)) {
        list_user_boards();
        return;
    }

    clear_board_session();
    show_cabinet_page();
    list_user_boards();
    emit_status(QStringLiteral("Board was deleted"), QStringLiteral("neutral"));
    emit_session_state_changed();
    if (on_warning_requested) {
        on_warning_requested(
            QStringLiteral("Board deleted"),
            QStringLiteral("The current board was deleted and is no longer available."));
    }
}

void ClientSessionController::handle_presence_update(const QJsonObject& payload) {
    session_state_.set_active_participants(participants_from_json(payload.value("active_participants").toArray()));
    emit_participants_changed();
}

void ClientSessionController::handle_operation_applied_message(const QJsonObject& payload) {
    const auto applied_operation = applied_operation_from_json(payload.value("operation").toObject());
    if (!session_state_.has_board() || applied_operation.board_id != session_state_.board_id()) {
        return;
    }

    auto result = session_state_.apply_remote_operation(applied_operation, board_state_service_);
    if (!common::is_ok(result)) {
        emit_status(QString::fromStdString(common::error(result).message), QStringLiteral("error"));
        return;
    }

    if (on_board_snapshot_changed) {
        on_board_snapshot_changed(session_state_.snapshot());
    }
    emit_participants_changed();
    emit_status(QStringLiteral("Synced"), QStringLiteral("success"));
    flush_operation_queue();
}

}  // namespace online_board::client_qt
