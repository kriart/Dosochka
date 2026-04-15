#pragma once

#include <functional>

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QTcpSocket>

namespace online_board::client_qt {

class ServerConnection final : public QObject {
public:
    explicit ServerConnection(QObject* parent = nullptr);

    void connect_to_server(const QString& host, quint16 port);
    void disconnect_from_server();
    bool is_connected() const;

    void register_user(const QString& login, const QString& display_name, const QString& password);
    void login(const QString& login, const QString& password);
    void enter_guest(const QString& nickname);
    void restore_session(const QString& token);
    void create_board(const QString& title, const QString& access_mode, const QString& guest_policy, const QString& password = {});
    void list_user_boards();
    void delete_board(const QString& board_id);
    void join_board(const QString& board_id, const QString& password = {});
    void leave_board();
    void send_operation(const QString& board_id, quint64 base_revision, const QJsonObject& operation);

    std::function<void(const QString& type, const QJsonObject& payload)> on_message;
    std::function<void(const QString& text)> on_error;
    std::function<void()> on_connected;
    std::function<void()> on_disconnected;

private:
    void send_message(const QString& type, const QJsonObject& payload);
    void handle_ready_read();

    QTcpSocket socket_;
    QByteArray buffer_;
};

}  // namespace online_board::client_qt
