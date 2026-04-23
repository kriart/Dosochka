#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>

#include <boost/asio.hpp>

#include "json_helpers.hpp"
#include "server_state.hpp"

namespace online_board::server {

class ClientSession final : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::ip::tcp::socket socket, ServerState& state);

    void start();
    void stop();
    void send_json(std::string json_message);
    void on_board_deleted(const common::BoardId& board_id);

private:
    using tcp = boost::asio::ip::tcp;

    void do_read();
    void do_write();
    void handle_message(const std::string& line);

    void handle_register(const boost::json::object& payload);
    void handle_login(const boost::json::object& payload);
    void handle_guest_enter(const boost::json::object& payload);
    void handle_session_restore(const boost::json::object& payload);
    void handle_create_board(const boost::json::object& payload);
    void handle_list_user_boards();
    void handle_delete_board(const boost::json::object& payload);
    void handle_join_board(const boost::json::object& payload);
    void handle_leave_board();
    void handle_operation(const boost::json::object& payload);
    void handle_ping();

    [[nodiscard]] std::string resolved_principal_name() const;
    [[nodiscard]] bool ensure_authenticated();
    void set_registered_principal(const domain::User& user);
    void set_guest_principal(const application::GuestEnterResponse& response);
    void send_error(const common::Error& error);
    void leave_current_board();
    void handle_board_deleted(const common::BoardId& board_id);
    void cleanup();

    tcp::socket socket_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    boost::asio::streambuf buffer_;
    std::deque<std::string> outgoing_;
    ServerState& state_;
    std::optional<domain::Principal> principal_;
    std::string principal_display_name_;
    std::optional<common::BoardId> joined_board_id_;
    std::optional<domain::BoardRole> joined_role_;
    std::shared_ptr<runtime::BoardRuntimeManager> runtime_;
    bool stopped_ {false};
};

}  // namespace online_board::server
