#include "json_tcp_server.hpp"

#include <algorithm>
#include <memory>
#include <utility>

#include "client_session.hpp"

namespace online_board::server {

JsonTcpServer::JsonTcpServer(
    boost::asio::io_context& io_context,
    const boost::asio::ip::tcp::endpoint& endpoint,
    ServerPersistenceConfig persistence_config)
    : acceptor_(io_context),
      state_(std::move(persistence_config)) {
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
    do_accept();
}

unsigned short JsonTcpServer::port() const {
    return acceptor_.local_endpoint().port();
}

void JsonTcpServer::stop() {
    boost::system::error_code ignored;
    acceptor_.cancel(ignored);
    acceptor_.close(ignored);

    std::vector<std::shared_ptr<ClientSession>> sessions;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.erase(
            std::remove_if(
                sessions_.begin(),
                sessions_.end(),
                [&](const std::weak_ptr<ClientSession>& weak_session) {
                    const auto session = weak_session.lock();
                    if (!session) {
                        return true;
                    }
                    sessions.push_back(session);
                    return false;
                }),
            sessions_.end());
    }

    for (const auto& session : sessions) {
        session->stop();
    }
}

void JsonTcpServer::track_session(const std::shared_ptr<ClientSession>& session) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_.erase(
        std::remove_if(
            sessions_.begin(),
            sessions_.end(),
            [](const std::weak_ptr<ClientSession>& weak_session) {
                return weak_session.expired();
            }),
        sessions_.end());
    sessions_.push_back(session);
}

void JsonTcpServer::do_accept() {
    acceptor_.async_accept([this](const boost::system::error_code& error, boost::asio::ip::tcp::socket socket) {
        if (!error) {
            auto session = std::make_shared<ClientSession>(std::move(socket), state_);
            track_session(session);
            session->start();
        }
        if (acceptor_.is_open()) {
            do_accept();
        }
    });
}

}  // namespace online_board::server
