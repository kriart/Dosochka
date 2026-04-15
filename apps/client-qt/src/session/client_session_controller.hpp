#pragma once

#include <functional>
#include <vector>

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include "board_session_state.hpp"
#include "server_connection.hpp"

namespace online_board::client_qt {

class ClientSessionController final {
public:
    ClientSessionController();

    struct UserBoardEntry {
        QString board_id;
        QString title;
        QString role;
        QString access_mode;
        bool can_delete {false};
    };

    enum class IdentityMode {
        none,
        registered_user,
        guest,
    };

    void connect_to_server(const QString& host, quint16 port);
    void register_user(const QString& login, const QString& display_name, const QString& password);
    void login(const QString& login, const QString& password);
    void enter_guest(const QString& nickname);
    void create_board(const QString& title, const QString& access_mode, const QString& guest_policy, const QString& password);
    void list_user_boards();
    void delete_board(const QString& board_id);
    void join_board(const QString& board_id, const QString& password);
    void leave_board();
    void logout();
    void enqueue_operation(QJsonObject operation);

    [[nodiscard]] bool is_connected() const;
    [[nodiscard]] bool is_authenticated() const;
    [[nodiscard]] bool can_list_user_boards() const;
    [[nodiscard]] bool has_board() const;
    [[nodiscard]] QString logout_action_text() const;
    [[nodiscard]] QString next_object_id();
    [[nodiscard]] qint64 next_z_index() const;

    std::function<void(const QString& text, const QString& state)> on_status_changed;
    std::function<void(const QString& text)> on_account_changed;
    std::function<void()> on_show_auth_page;
    std::function<void()> on_show_cabinet_page;
    std::function<void()> on_show_board_page;
    std::function<void()> on_session_state_changed;
    std::function<void(const QString& board_id)> on_join_board_id_suggested;
    std::function<void(const QString& title, const QString& subtitle, const QString& board_id)> on_board_context_changed;
    std::function<void(const domain::BoardSnapshot& snapshot, domain::BoardRole role)> on_board_opened;
    std::function<void(const domain::BoardSnapshot& snapshot)> on_board_snapshot_changed;
    std::function<void(const std::vector<UserBoardEntry>& boards)> on_user_boards_changed;
    std::function<void(const QStringList& participants)> on_participants_changed;
    std::function<void()> on_board_cleared;
    std::function<void(const QString& title, const QString& message)> on_warning_requested;

private:
    void bind_connection_callbacks();
    void emit_status(const QString& text, const QString& state) const;
    void emit_account_changed(const QString& text) const;
    void emit_session_state_changed() const;
    void show_auth_page() const;
    void show_cabinet_page() const;
    void show_board_page() const;
    void handle_message(const QString& type, const QJsonObject& payload);
    void handle_error_message(const QJsonObject& payload);
    void handle_auth_success(const QJsonObject& payload);
    void handle_guest_success(const QJsonObject& payload);
    void handle_create_board_success(const QJsonObject& payload);
    void handle_list_user_boards_success(const QJsonObject& payload);
    void handle_delete_board_success(const QJsonObject& payload);
    void handle_join_board_success(const QJsonObject& payload);
    void handle_board_deleted(const QJsonObject& payload);
    void handle_presence_update(const QJsonObject& payload);
    void handle_operation_applied_message(const QJsonObject& payload);
    void flush_operation_queue();
    void clear_board_session();
    void emit_user_boards_changed(const std::vector<UserBoardEntry>& boards) const;
    void emit_participants_changed() const;
    static QStringList participant_labels(const domain::BoardSnapshot& snapshot);
    static QString role_text(domain::BoardRole role);

    ServerConnection connection_;
    application::BoardStateService board_state_service_;
    BoardSessionState session_state_;
    IdentityMode identity_mode_ {IdentityMode::none};
    bool logout_requested_ {false};
};

}  // namespace online_board::client_qt
