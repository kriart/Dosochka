#include "client_session.hpp"

#include <istream>
#include <string>

namespace online_board::server {

void ClientSession::do_read() {
    boost::asio::async_read_until(
        socket_,
        buffer_,
        '\n',
        boost::asio::bind_executor(
            strand_,
            [self = shared_from_this()](const boost::system::error_code& error, std::size_t) {
                if (error) {
                    self->cleanup();
                    return;
                }

                std::istream input(&self->buffer_);
                std::string line;
                std::getline(input, line);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                self->handle_message(line);
                self->do_read();
            }));
}

void ClientSession::do_write() {
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(outgoing_.front()),
        boost::asio::bind_executor(
            strand_,
            [self = shared_from_this()](const boost::system::error_code& error, std::size_t) {
                if (error) {
                    self->cleanup();
                    return;
                }
                self->outgoing_.pop_front();
                if (!self->outgoing_.empty()) {
                    self->do_write();
                }
            }));
}

void ClientSession::cleanup() {
    if (stopped_) {
        return;
    }
    stopped_ = true;
    leave_current_board();
    boost::system::error_code ignored;
    socket_.cancel(ignored);
    socket_.close(ignored);
}

}  // namespace online_board::server
