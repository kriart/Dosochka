#include "client_session_controller.hpp"

#include <utility>

namespace online_board::client_qt {

ClientSessionController::ClientSessionController() {
    bind_connection_callbacks();
}

void ClientSessionController::connect_to_server(const QString& host, quint16 port) {
    connection_.connect_to_server(host, port);
    emit_session_state_changed();
}

void ClientSessionController::register_user(const QString& login, const QString& display_name, const QString& password) {
    connection_.register_user(login, display_name, password);
}

void ClientSessionController::login(const QString& login, const QString& password) {
    connection_.login(login, password);
}

void ClientSessionController::enter_guest(const QString& nickname) {
    connection_.enter_guest(nickname);
}

void ClientSessionController::create_board(
    const QString& title,
    const QString& access_mode,
    const QString& guest_policy,
    const QString& password) {
    connection_.create_board(title, access_mode, guest_policy, password);
}

void ClientSessionController::list_user_boards() {
    if (identity_mode_ != IdentityMode::registered_user || !connection_.is_connected()) {
        emit_user_boards_changed({});
        return;
    }
    connection_.list_user_boards();
}

void ClientSessionController::delete_board(const QString& board_id) {
    if (identity_mode_ != IdentityMode::registered_user || board_id.isEmpty()) {
        return;
    }
    connection_.delete_board(board_id);
}

void ClientSessionController::join_board(const QString& board_id, const QString& password) {
    connection_.join_board(board_id, password);
}

void ClientSessionController::leave_board() {
    if (connection_.is_connected()) {
        connection_.leave_board();
    }
    clear_board_session();
    show_cabinet_page();
    list_user_boards();
    emit_status(QStringLiteral("Returned to cabinet"), QStringLiteral("success"));
    emit_session_state_changed();
}

void ClientSessionController::logout() {
    logout_requested_ = true;
    if (session_state_.has_board()) {
        connection_.leave_board();
        clear_board_session();
    }

    if (connection_.is_connected()) {
        connection_.disconnect_from_server();
        return;
    }

    identity_mode_ = IdentityMode::none;
    emit_user_boards_changed({});
    emit_account_changed(QStringLiteral("Anonymous"));
    show_auth_page();
    emit_status(QStringLiteral("Logged out"), QStringLiteral("neutral"));
    logout_requested_ = false;
    emit_session_state_changed();
}

void ClientSessionController::enqueue_operation(QJsonObject operation) {
    session_state_.enqueue_operation(std::move(operation));
    flush_operation_queue();
}

bool ClientSessionController::is_connected() const {
    return connection_.is_connected();
}

bool ClientSessionController::is_authenticated() const {
    return connection_.is_connected() && identity_mode_ != IdentityMode::none;
}

bool ClientSessionController::can_list_user_boards() const {
    return identity_mode_ == IdentityMode::registered_user;
}

bool ClientSessionController::has_board() const {
    return session_state_.has_board();
}

QString ClientSessionController::logout_action_text() const {
    return identity_mode_ == IdentityMode::registered_user
        ? QStringLiteral("Log out")
        : QStringLiteral("End guest session");
}

QString ClientSessionController::next_object_id() {
    return session_state_.next_object_id();
}

qint64 ClientSessionController::next_z_index() const {
    return session_state_.next_z_index();
}

void ClientSessionController::flush_operation_queue() {
    if (!session_state_.has_operation_ready()) {
        return;
    }

    session_state_.mark_operation_dispatched();
    connection_.send_operation(
        QString::fromStdString(session_state_.board_id().value),
        session_state_.revision(),
        session_state_.front_operation());
}

void ClientSessionController::clear_board_session() {
    session_state_.clear();
    if (on_board_cleared) {
        on_board_cleared();
    }
    if (on_participants_changed) {
        on_participants_changed({});
    }
}

void ClientSessionController::emit_user_boards_changed(const std::vector<UserBoardEntry>& boards) const {
    if (on_user_boards_changed) {
        on_user_boards_changed(boards);
    }
}

void ClientSessionController::emit_status(const QString& text, const QString& state) const {
    if (on_status_changed) {
        on_status_changed(text, state);
    }
}

void ClientSessionController::emit_account_changed(const QString& text) const {
    if (on_account_changed) {
        on_account_changed(text);
    }
}

void ClientSessionController::emit_session_state_changed() const {
    if (on_session_state_changed) {
        on_session_state_changed();
    }
}

void ClientSessionController::show_auth_page() const {
    if (on_show_auth_page) {
        on_show_auth_page();
    }
}

void ClientSessionController::show_cabinet_page() const {
    if (on_show_cabinet_page) {
        on_show_cabinet_page();
    }
}

void ClientSessionController::show_board_page() const {
    if (on_show_board_page) {
        on_show_board_page();
    }
}

void ClientSessionController::emit_participants_changed() const {
    if (on_participants_changed) {
        on_participants_changed(participant_labels(session_state_.snapshot()));
    }
}

QStringList ClientSessionController::participant_labels(const domain::BoardSnapshot& snapshot) {
    QStringList names;
    for (const auto& participant : snapshot.active_participants) {
        const auto name = participant.nickname.empty()
            ? QString::fromStdString(domain::display_name_for(participant.principal))
            : QString::fromStdString(participant.nickname);
        names.push_back(QStringLiteral("%1 (%2)").arg(name, role_text(participant.role)));
    }
    return names;
}

QString ClientSessionController::role_text(domain::BoardRole role) {
    switch (role) {
        case domain::BoardRole::owner:
            return QStringLiteral("owner");
        case domain::BoardRole::editor:
            return QStringLiteral("editor");
        case domain::BoardRole::viewer:
            return QStringLiteral("viewer");
    }
    return QStringLiteral("viewer");
}

}  // namespace online_board::client_qt
