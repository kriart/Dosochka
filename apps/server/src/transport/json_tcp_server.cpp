#include "json_tcp_server.hpp"

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

void JsonTcpServer::do_accept() {
    acceptor_.async_accept([this](const boost::system::error_code& error, boost::asio::ip::tcp::socket socket) {
        if (!error) {
            std::make_shared<ClientSession>(std::move(socket), state_)->start();
        }
        do_accept();
    });
}

}  // namespace online_board::server
