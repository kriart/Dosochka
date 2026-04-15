#include "server_connection.hpp"

#include <QJsonDocument>

#include "online_board/protocol/message_types.hpp"

namespace online_board::client_qt {

ServerConnection::ServerConnection(QObject* parent)
    : QObject(parent) {
    QObject::connect(&socket_, &QTcpSocket::readyRead, this, [this]() { handle_ready_read(); });
    QObject::connect(&socket_, &QTcpSocket::connected, this, [this]() {
        if (on_connected) {
            on_connected();
        }
    });
    QObject::connect(&socket_, &QTcpSocket::disconnected, this, [this]() {
        if (on_disconnected) {
            on_disconnected();
        }
    });
    QObject::connect(&socket_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (on_error) {
            on_error(socket_.errorString());
        }
    });
}

void ServerConnection::connect_to_server(const QString& host, quint16 port) {
    if (socket_.state() == QAbstractSocket::ConnectedState) {
        return;
    }
    socket_.connectToHost(host, port);
}

void ServerConnection::disconnect_from_server() {
    buffer_.clear();
    if (socket_.state() == QAbstractSocket::UnconnectedState) {
        return;
    }
    socket_.disconnectFromHost();
    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.waitForDisconnected(1000);
    }
}

bool ServerConnection::is_connected() const {
    return socket_.state() == QAbstractSocket::ConnectedState;
}

void ServerConnection::register_user(const QString& login, const QString& display_name, const QString& password) {
    send_message(QString::fromUtf8(online_board::protocol::message_type::register_request.data()), {{"login", login}, {"display_name", display_name}, {"password", password}});
}

void ServerConnection::login(const QString& login, const QString& password) {
    send_message(QString::fromUtf8(online_board::protocol::message_type::login_request.data()), {{"login", login}, {"password", password}});
}

void ServerConnection::enter_guest(const QString& nickname) {
    send_message(QString::fromUtf8(online_board::protocol::message_type::guest_enter_request.data()), {{"nickname", nickname}});
}

void ServerConnection::restore_session(const QString& token) {
    send_message(QString::fromUtf8(online_board::protocol::message_type::session_restore_request.data()), {{"token", token}});
}

void ServerConnection::create_board(const QString& title, const QString& access_mode, const QString& guest_policy, const QString& password) {
    QJsonObject payload{{"title", title}, {"access_mode", access_mode}, {"guest_policy", guest_policy}};
    if (!password.isEmpty()) {
        payload.insert("password", password);
    }
    send_message(QString::fromUtf8(online_board::protocol::message_type::create_board_request.data()), payload);
}

void ServerConnection::list_user_boards() {
    send_message(QString::fromUtf8(online_board::protocol::message_type::list_user_boards_request.data()), {});
}

void ServerConnection::delete_board(const QString& board_id) {
    send_message(
        QString::fromUtf8(online_board::protocol::message_type::delete_board_request.data()),
        {{"board_id", board_id}});
}

void ServerConnection::join_board(const QString& board_id, const QString& password) {
    QJsonObject payload{{"board_id", board_id}};
    if (!password.isEmpty()) {
        payload.insert("password", password);
    }
    send_message(QString::fromUtf8(online_board::protocol::message_type::join_board_request.data()), payload);
}

void ServerConnection::leave_board() {
    send_message(QString::fromUtf8(online_board::protocol::message_type::leave_board_request.data()), {});
}

void ServerConnection::send_operation(const QString& board_id, quint64 base_revision, const QJsonObject& operation) {
    send_message(
        QString::fromUtf8(online_board::protocol::message_type::operation_request.data()),
        {{"board_id", board_id}, {"base_revision", static_cast<qint64>(base_revision)}, {"operation", operation}});
}

void ServerConnection::send_message(const QString& type, const QJsonObject& payload) {
    if (!is_connected()) {
        if (on_error) {
            on_error(QStringLiteral("Not connected to server"));
        }
        return;
    }

    const QJsonDocument document(QJsonObject{{"type", type}, {"payload", payload}});
    auto bytes = document.toJson(QJsonDocument::Compact);
    bytes.push_back('\n');
    socket_.write(bytes);
}

void ServerConnection::handle_ready_read() {
    buffer_.append(socket_.readAll());

    while (true) {
        const auto newline_index = buffer_.indexOf('\n');
        if (newline_index < 0) {
            return;
        }

        QByteArray line = buffer_.left(newline_index);
        buffer_.remove(0, newline_index + 1);
        if (!line.isEmpty() && line.endsWith('\r')) {
            line.chop(1);
        }

        QJsonParseError parse_error;
        const auto document = QJsonDocument::fromJson(line, &parse_error);
        if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
            if (on_error) {
                on_error(QStringLiteral("Failed to parse server message: %1").arg(parse_error.errorString()));
            }
            continue;
        }

        const auto object = document.object();
        const auto type = object.value("type").toString();
        const auto payload = object.value("payload").toObject();
        if (on_message) {
            on_message(type, payload);
        }
    }
}

}  // namespace online_board::client_qt
