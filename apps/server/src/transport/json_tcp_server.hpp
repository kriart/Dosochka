#pragma once

#include <boost/asio.hpp>

#include "server_state.hpp"

namespace online_board::server {

class JsonTcpServer {
public:
    JsonTcpServer(
        boost::asio::io_context& io_context,
        const boost::asio::ip::tcp::endpoint& endpoint,
        ServerPersistenceConfig persistence_config = {});

private:
    void do_accept();

    boost::asio::ip::tcp::acceptor acceptor_;
    ServerState state_;
};

}  // namespace online_board::server
