#include "client_session.hpp"

#include <utility>

namespace online_board::server {

ClientSession::ClientSession(tcp::socket socket, ServerState& state)
    : socket_(std::move(socket)),
      strand_(socket_.get_executor()),
      state_(state) {
}

void ClientSession::start() {
    do_read();
}

void ClientSession::send_json(std::string json_message) {
    boost::asio::post(
        strand_,
        [self = shared_from_this(), json_message = std::move(json_message)]() mutable {
            const auto needs_write = self->outgoing_.empty();
            json_message.push_back('\n');
            self->outgoing_.push_back(std::move(json_message));
            if (needs_write) {
                self->do_write();
            }
        });
}

}  // namespace online_board::server
