#pragma once

#include <mutex>
#include <memory>
#include <vector>

#include <boost/asio.hpp>

#include "server_state.hpp"

namespace online_board::server {

class ClientSession;

class JsonTcpServer {
public:
    JsonTcpServer(
        boost::asio::io_context& io_context,
        const boost::asio::ip::tcp::endpoint& endpoint,
        ServerPersistenceConfig persistence_config = {});

    [[nodiscard]] unsigned short port() const;
    void stop();

private:
    void do_accept();
    void track_session(const std::shared_ptr<ClientSession>& session);

    boost::asio::ip::tcp::acceptor acceptor_;
    ServerState state_;
    std::mutex sessions_mutex_;
    std::vector<std::weak_ptr<ClientSession>> sessions_;
};

}  // namespace online_board::server
