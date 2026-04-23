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

void ClientSession::stop() {
    boost::asio::post(
        strand_,
        [self = shared_from_this()]() {
            self->cleanup();
        });
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

void ClientSession::on_board_deleted(const common::BoardId& board_id) {
    boost::asio::post(
        strand_,
        [self = shared_from_this(), board_id]() {
            self->handle_board_deleted(board_id);
        });
}

}  // namespace online_board::server
