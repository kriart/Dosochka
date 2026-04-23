#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <gtest/gtest.h>

#include "json_tcp_server.hpp"
#include "online_board/protocol/message_types.hpp"

namespace {

namespace asio = boost::asio;
namespace json = boost::json;
using tcp = asio::ip::tcp;

struct ReceivedMessage {
    std::string type;
    json::object payload;
};

class TcpJsonClient {
public:
    explicit TcpJsonClient(unsigned short port)
        : socket_(io_context_) {
        socket_.connect(tcp::endpoint(asio::ip::address_v4::loopback(), port));
    }

    ~TcpJsonClient() {
        boost::system::error_code ignored;
        socket_.close(ignored);
    }

    void send(std::string_view type, json::object payload) {
        auto bytes = json::serialize(json::object{
            {"type", std::string(type)},
            {"payload", std::move(payload)},
        });
        bytes.push_back('\n');
        asio::write(socket_, asio::buffer(bytes));
    }

    void send_raw(std::string bytes) {
        if (bytes.empty() || bytes.back() != '\n') {
            bytes.push_back('\n');
        }
        asio::write(socket_, asio::buffer(bytes));
    }

    ReceivedMessage receive() {
        asio::read_until(socket_, buffer_, '\n');

        std::istream input(&buffer_);
        std::string line;
        std::getline(input, line);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const auto parsed = json::parse(line).as_object();
        return ReceivedMessage{
            .type = std::string(parsed.at("type").as_string().c_str()),
            .payload = parsed.at("payload").as_object(),
        };
    }

    ReceivedMessage receive_until(std::string_view expected_type) {
        while (true) {
            auto message = receive();
            if (message.type == expected_type) {
                return message;
            }
        }
    }

    std::optional<ReceivedMessage> try_receive_for(std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            boost::system::error_code error;
            if (buffer_.size() > 0 || socket_.available(error) > 0) {
                return receive();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return std::nullopt;
    }

    std::vector<ReceivedMessage> drain_for(std::chrono::milliseconds timeout) {
        std::vector<ReceivedMessage> messages;
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
            if (remaining <= std::chrono::milliseconds::zero()) {
                break;
            }
            auto message = try_receive_for((std::min)(remaining, std::chrono::milliseconds(25)));
            if (message.has_value()) {
                messages.push_back(std::move(*message));
            }
        }
        return messages;
    }

    bool wait_for_disconnect(std::chrono::milliseconds timeout) {
        socket_.non_blocking(true);
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        char buffer[1];
        while (std::chrono::steady_clock::now() < deadline) {
            boost::system::error_code error;
            const auto bytes_read = socket_.read_some(asio::buffer(buffer), error);
            if (error == asio::error::eof || error == asio::error::connection_reset
                || error == asio::error::operation_aborted) {
                return true;
            }
            if (!error && bytes_read > 0) {
                continue;
            }
            if (error == asio::error::would_block || error == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            if (error) {
                return false;
            }
        }
        return false;
    }

private:
    asio::io_context io_context_;
    tcp::socket socket_;
    asio::streambuf buffer_;
};

class ServerIntegrationTests : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<online_board::server::JsonTcpServer>(
            io_context_,
            tcp::endpoint(tcp::v4(), 0),
            online_board::server::ServerPersistenceConfig{});
        port_ = server_->port();
        worker_ = std::thread([this]() { io_context_.run(); });
    }

    void TearDown() override {
        server_->stop();
        io_context_.stop();
        if (worker_.joinable()) {
            worker_.join();
        }
        server_.reset();
    }

    [[nodiscard]] unsigned short port() const {
        return port_;
    }

    void stop_server() {
        server_->stop();
    }

private:
    asio::io_context io_context_;
    std::unique_ptr<online_board::server::JsonTcpServer> server_;
    unsigned short port_ {0};
    std::thread worker_;
};

TEST_F(ServerIntegrationTests, RegisterAndDuplicateRegisterAreHandledOverTcp) {
    TcpJsonClient first(port());
    first.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "alice"},
            {"display_name", "Alice"},
            {"password", "secret"},
        });

    const auto register_response = first.receive_until(online_board::protocol::message_type::register_response);
    EXPECT_EQ(register_response.payload.at("display_name").as_string(), "Alice");
    EXPECT_FALSE(register_response.payload.at("token").as_string().empty());

    TcpJsonClient second(port());
    second.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "alice"},
            {"display_name", "Alice Again"},
            {"password", "secret-2"},
        });

    const auto error_response = second.receive_until(online_board::protocol::message_type::error);
    EXPECT_EQ(error_response.payload.at("code").as_string(), "already_exists");
}

TEST_F(ServerIntegrationTests, CreateBoardAndListBoardsWorkOverTcp) {
    TcpJsonClient client(port());
    client.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "owner"},
            {"display_name", "Owner"},
            {"password", "secret"},
        });
    static_cast<void>(client.receive_until(online_board::protocol::message_type::register_response));

    client.send(
        online_board::protocol::message_type::create_board_request,
        {
            {"title", "Sprint review"},
            {"access_mode", "private_board"},
            {"guest_policy", "guest_disabled"},
        });

    const auto create_response = client.receive_until(online_board::protocol::message_type::create_board_response);
    const auto& board = create_response.payload.at("board").as_object();
    EXPECT_EQ(board.at("title").as_string(), "Sprint review");

    client.send(online_board::protocol::message_type::list_user_boards_request, {});
    const auto list_response = client.receive_until(online_board::protocol::message_type::list_user_boards_response);
    const auto& boards = list_response.payload.at("boards").as_array();
    ASSERT_EQ(boards.size(), 1);
    EXPECT_EQ(boards[0].as_object().at("board").as_object().at("title").as_string(), "Sprint review");
    EXPECT_EQ(boards[0].as_object().at("role").as_string(), "owner");
}

TEST_F(ServerIntegrationTests, SessionRestoreReturnsRegisteredUserFromToken) {
    TcpJsonClient register_client(port());
    register_client.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "restorable"},
            {"display_name", "Restorable User"},
            {"password", "secret"},
        });

    const auto register_response = register_client.receive_until(online_board::protocol::message_type::register_response);
    const auto token = std::string(register_response.payload.at("token").as_string().c_str());

    TcpJsonClient restore_client(port());
    restore_client.send(
        online_board::protocol::message_type::session_restore_request,
        {
            {"token", token},
        });

    const auto restore_response = restore_client.receive_until(online_board::protocol::message_type::session_restore_response);
    EXPECT_EQ(restore_response.payload.at("display_name").as_string(), "Restorable User");
    EXPECT_FALSE(restore_response.payload.at("user_id").as_string().empty());
}

TEST_F(ServerIntegrationTests, UnknownMessageTypeReturnsError) {
    TcpJsonClient client(port());
    client.send("totally_unknown_message", {});

    const auto error_response = client.receive_until(online_board::protocol::message_type::error);
    EXPECT_EQ(error_response.payload.at("code").as_string(), "invalid_argument");
}

TEST_F(ServerIntegrationTests, UnauthenticatedBoardActionsAreRejected) {
    TcpJsonClient client(port());

    client.send(
        online_board::protocol::message_type::create_board_request,
        {
            {"title", "Should fail"},
            {"access_mode", "private_board"},
            {"guest_policy", "guest_disabled"},
        });
    const auto create_error = client.receive_until(online_board::protocol::message_type::error);
    EXPECT_EQ(create_error.payload.at("code").as_string(), "access_denied");

    client.send(online_board::protocol::message_type::list_user_boards_request, {});
    const auto list_error = client.receive_until(online_board::protocol::message_type::error);
    EXPECT_EQ(list_error.payload.at("code").as_string(), "access_denied");
}

TEST_F(ServerIntegrationTests, AppliedOperationIsBroadcastToBoardParticipants) {
    TcpJsonClient owner(port());
    owner.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "owner"},
            {"display_name", "Owner"},
            {"password", "secret"},
        });
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::register_response));

    owner.send(
        online_board::protocol::message_type::create_board_request,
        {
            {"title", "Shared board"},
            {"access_mode", "password_protected"},
            {"guest_policy", "guest_edit_allowed"},
            {"password", "pw"},
        });
    const auto create_response = owner.receive_until(online_board::protocol::message_type::create_board_response);
    const auto board_id = std::string(
        create_response.payload.at("board").as_object().at("id").as_string().c_str());

    owner.send(
        online_board::protocol::message_type::join_board_request,
        {
            {"board_id", board_id},
            {"password", "pw"},
        });
    const auto owner_join = owner.receive_until(online_board::protocol::message_type::join_board_response);
    EXPECT_EQ(owner_join.payload.at("role").as_string(), "owner");
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::presence_update));

    TcpJsonClient guest(port());
    guest.send(
        online_board::protocol::message_type::guest_enter_request,
        {
            {"nickname", "Guest One"},
        });
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::guest_enter_response));

    guest.send(
        online_board::protocol::message_type::join_board_request,
        {
            {"board_id", board_id},
            {"password", "pw"},
        });
    const auto guest_join = guest.receive_until(online_board::protocol::message_type::join_board_response);
    EXPECT_EQ(guest_join.payload.at("role").as_string(), "editor");
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::presence_update));
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::presence_update));

    owner.send(
        online_board::protocol::message_type::operation_request,
        {
            {"board_id", board_id},
            {"base_revision", 0},
            {"operation", json::object{
                {"type", "create_text"},
                {"object_id", "text-1"},
                {"position", json::object{{"x", 10.0}, {"y", 20.0}}},
                {"size", json::object{{"width", 120.0}, {"height", 40.0}}},
                {"text", "hello"},
                {"font_family", "Cascadia Code"},
                {"font_size", 16.0},
                {"color", "#112233"},
                {"z_index", 1},
            }},
        });

    const auto owner_applied = owner.receive_until(online_board::protocol::message_type::operation_applied);
    const auto guest_applied = guest.receive_until(online_board::protocol::message_type::operation_applied);

    EXPECT_EQ(owner_applied.payload.at("operation").as_object().at("revision").as_int64(), 1);
    EXPECT_EQ(guest_applied.payload.at("operation").as_object().at("revision").as_int64(), 1);
    EXPECT_EQ(
        guest_applied.payload.at("operation").as_object().at("payload").as_object().at("type").as_string(),
        "create_text");
}

TEST_F(ServerIntegrationTests, ConcurrentOperationsOnSameBoardProduceOneConflict) {
    TcpJsonClient owner(port());
    owner.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "owner"},
            {"display_name", "Owner"},
            {"password", "secret"},
        });
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::register_response));

    owner.send(
        online_board::protocol::message_type::create_board_request,
        {
            {"title", "Concurrent board"},
            {"access_mode", "password_protected"},
            {"guest_policy", "guest_edit_allowed"},
            {"password", "pw"},
        });
    const auto create_response = owner.receive_until(online_board::protocol::message_type::create_board_response);
    const auto board_id = std::string(
        create_response.payload.at("board").as_object().at("id").as_string().c_str());

    owner.send(
        online_board::protocol::message_type::join_board_request,
        {
            {"board_id", board_id},
            {"password", "pw"},
        });
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::join_board_response));
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::presence_update));

    TcpJsonClient guest(port());
    guest.send(
        online_board::protocol::message_type::guest_enter_request,
        {
            {"nickname", "Guest One"},
        });
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::guest_enter_response));

    guest.send(
        online_board::protocol::message_type::join_board_request,
        {
            {"board_id", board_id},
            {"password", "pw"},
        });
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::join_board_response));
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::presence_update));
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::presence_update));

    std::atomic<bool> go {false};
    std::thread owner_sender([&]() {
        while (!go.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        owner.send(
            online_board::protocol::message_type::operation_request,
            {
                {"board_id", board_id},
                {"base_revision", 0},
                {"operation", json::object{
                    {"type", "create_text"},
                    {"object_id", "owner-text"},
                    {"position", json::object{{"x", 10.0}, {"y", 20.0}}},
                    {"size", json::object{{"width", 100.0}, {"height", 30.0}}},
                    {"text", "owner"},
                    {"font_family", "Cascadia Code"},
                    {"font_size", 14.0},
                    {"color", "#111111"},
                    {"z_index", 1},
                }},
            });
    });
    std::thread guest_sender([&]() {
        while (!go.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        guest.send(
            online_board::protocol::message_type::operation_request,
            {
                {"board_id", board_id},
                {"base_revision", 0},
                {"operation", json::object{
                    {"type", "create_text"},
                    {"object_id", "guest-text"},
                    {"position", json::object{{"x", 30.0}, {"y", 40.0}}},
                    {"size", json::object{{"width", 100.0}, {"height", 30.0}}},
                    {"text", "guest"},
                    {"font_family", "Cascadia Code"},
                    {"font_size", 14.0},
                    {"color", "#222222"},
                    {"z_index", 2},
                }},
            });
    });

    go.store(true, std::memory_order_release);
    owner_sender.join();
    guest_sender.join();

    auto owner_messages = owner.drain_for(std::chrono::milliseconds(400));
    auto guest_messages = guest.drain_for(std::chrono::milliseconds(400));
    owner_messages.insert(owner_messages.end(), guest_messages.begin(), guest_messages.end());

    std::size_t applied_count = 0;
    std::size_t conflict_count = 0;
    for (const auto& message : owner_messages) {
        if (message.type == online_board::protocol::message_type::operation_applied) {
            ++applied_count;
            EXPECT_EQ(message.payload.at("operation").as_object().at("revision").as_int64(), 1);
            continue;
        }
        if (message.type == online_board::protocol::message_type::error) {
            if (message.payload.at("code").as_string() == "conflict") {
                ++conflict_count;
            }
        }
    }

    EXPECT_EQ(applied_count, 2U);
    EXPECT_EQ(conflict_count, 1U);
}

TEST_F(ServerIntegrationTests, DeleteBoardNotifiesParticipantsAndRevokesJoinedState) {
    TcpJsonClient owner(port());
    owner.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "owner"},
            {"display_name", "Owner"},
            {"password", "secret"},
        });
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::register_response));

    owner.send(
        online_board::protocol::message_type::create_board_request,
        {
            {"title", "Disposable board"},
            {"access_mode", "password_protected"},
            {"guest_policy", "guest_edit_allowed"},
            {"password", "pw"},
        });
    const auto create_response = owner.receive_until(online_board::protocol::message_type::create_board_response);
    const auto board_id = std::string(
        create_response.payload.at("board").as_object().at("id").as_string().c_str());

    owner.send(
        online_board::protocol::message_type::join_board_request,
        {
            {"board_id", board_id},
            {"password", "pw"},
        });
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::join_board_response));
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::presence_update));

    TcpJsonClient guest(port());
    guest.send(
        online_board::protocol::message_type::guest_enter_request,
        {
            {"nickname", "Guest One"},
        });
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::guest_enter_response));

    guest.send(
        online_board::protocol::message_type::join_board_request,
        {
            {"board_id", board_id},
            {"password", "pw"},
        });
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::join_board_response));
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::presence_update));
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::presence_update));

    owner.send(
        online_board::protocol::message_type::delete_board_request,
        {
            {"board_id", board_id},
        });

    const auto owner_delete = owner.receive_until(online_board::protocol::message_type::delete_board_response);
    EXPECT_EQ(owner_delete.payload.at("board_id").as_string(), board_id);

    const auto guest_deleted = guest.receive_until(online_board::protocol::message_type::board_deleted);
    EXPECT_EQ(guest_deleted.payload.at("board_id").as_string(), board_id);

    guest.send(
        online_board::protocol::message_type::operation_request,
        {
            {"board_id", board_id},
            {"base_revision", 0},
            {"operation", json::object{
                {"type", "delete_object"},
                {"object_id", "any-object"},
            }},
        });

    const auto guest_error = guest.receive_until(online_board::protocol::message_type::error);
    EXPECT_EQ(guest_error.payload.at("code").as_string(), "access_denied");
}

TEST_F(ServerIntegrationTests, LeaveBoardUpdatesPresenceAndClearsSessionBoardState) {
    TcpJsonClient owner(port());
    owner.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "owner"},
            {"display_name", "Owner"},
            {"password", "secret"},
        });
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::register_response));

    owner.send(
        online_board::protocol::message_type::create_board_request,
        {
            {"title", "Presence board"},
            {"access_mode", "password_protected"},
            {"guest_policy", "guest_edit_allowed"},
            {"password", "pw"},
        });
    const auto create_response = owner.receive_until(online_board::protocol::message_type::create_board_response);
    const auto board_id = std::string(
        create_response.payload.at("board").as_object().at("id").as_string().c_str());

    owner.send(
        online_board::protocol::message_type::join_board_request,
        {
            {"board_id", board_id},
            {"password", "pw"},
        });
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::join_board_response));
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::presence_update));

    TcpJsonClient guest(port());
    guest.send(
        online_board::protocol::message_type::guest_enter_request,
        {
            {"nickname", "Guest One"},
        });
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::guest_enter_response));

    guest.send(
        online_board::protocol::message_type::join_board_request,
        {
            {"board_id", board_id},
            {"password", "pw"},
        });
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::join_board_response));
    static_cast<void>(guest.receive_until(online_board::protocol::message_type::presence_update));
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::presence_update));

    guest.send(online_board::protocol::message_type::leave_board_request, {});
    const auto leave_response = guest.receive_until(online_board::protocol::message_type::leave_board_response);
    EXPECT_TRUE(leave_response.payload.empty());

    const auto owner_presence = owner.receive_until(online_board::protocol::message_type::presence_update);
    const auto& participants = owner_presence.payload.at("active_participants").as_array();
    ASSERT_EQ(participants.size(), 1);
    EXPECT_EQ(participants[0].as_object().at("nickname").as_string(), "Owner");

    guest.send(
        online_board::protocol::message_type::operation_request,
        {
            {"board_id", board_id},
            {"base_revision", 0},
            {"operation", json::object{
                {"type", "delete_object"},
                {"object_id", "any-object"},
            }},
        });

    const auto guest_error = guest.receive_until(online_board::protocol::message_type::error);
    EXPECT_EQ(guest_error.payload.at("code").as_string(), "access_denied");
}

TEST_F(ServerIntegrationTests, MalformedJsonReturnsProtocolError) {
    TcpJsonClient client(port());
    client.send_raw("{not valid json");

    const auto error_response = client.receive_until(online_board::protocol::message_type::error);
    EXPECT_EQ(error_response.payload.at("code").as_string(), "invalid_argument");
}

TEST_F(ServerIntegrationTests, InvalidOperationPayloadReturnsError) {
    TcpJsonClient owner(port());
    owner.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "owner"},
            {"display_name", "Owner"},
            {"password", "secret"},
        });
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::register_response));

    owner.send(
        online_board::protocol::message_type::create_board_request,
        {
            {"title", "Ops board"},
            {"access_mode", "private_board"},
            {"guest_policy", "guest_disabled"},
        });
    const auto create_response = owner.receive_until(online_board::protocol::message_type::create_board_response);
    const auto board_id = std::string(
        create_response.payload.at("board").as_object().at("id").as_string().c_str());

    owner.send(
        online_board::protocol::message_type::join_board_request,
        {
            {"board_id", board_id},
        });
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::join_board_response));
    static_cast<void>(owner.receive_until(online_board::protocol::message_type::presence_update));

    owner.send(
        online_board::protocol::message_type::operation_request,
        {
            {"board_id", board_id},
            {"base_revision", 0},
            {"operation", json::object{
                {"type", "unknown_operation"},
            }},
        });

    const auto error_response = owner.receive_until(online_board::protocol::message_type::error);
    EXPECT_EQ(error_response.payload.at("code").as_string(), "invalid_argument");
}

TEST_F(ServerIntegrationTests, GracefulShutdownDisconnectsActiveClientSession) {
    TcpJsonClient client(port());
    client.send(
        online_board::protocol::message_type::register_request,
        {
            {"login", "shutdown-user"},
            {"display_name", "Shutdown User"},
            {"password", "secret"},
        });
    static_cast<void>(client.receive_until(online_board::protocol::message_type::register_response));

    stop_server();

    EXPECT_TRUE(client.wait_for_disconnect(std::chrono::milliseconds(500)));
}

}  // namespace
