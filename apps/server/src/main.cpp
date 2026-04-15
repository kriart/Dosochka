#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>

#include "json_tcp_server.hpp"

int main(int argc, char* argv[]) {
    try {
        const auto port = argc > 1 ? static_cast<unsigned short>(std::stoi(argv[1])) : 4000;
        const char* database_url = std::getenv("DOSOCHKA_DATABASE_URL");
        online_board::server::ServerPersistenceConfig persistence_config;
        if (database_url && *database_url) {
            persistence_config.backend = online_board::server::PersistenceBackend::postgres;
            persistence_config.postgres_connection_string = database_url;
        }

        boost::asio::io_context io_context;
        online_board::server::JsonTcpServer server(
            io_context,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port),
            std::move(persistence_config));

        const auto worker_count = std::max(2u, std::thread::hardware_concurrency());
        std::vector<std::thread> workers;
        workers.reserve(worker_count - 1);
        for (unsigned int index = 1; index < worker_count; ++index) {
            workers.emplace_back([&io_context]() { io_context.run(); });
        }

        std::cout << "Dosochka server listening on port " << port;
        if (database_url && *database_url) {
            std::cout << " using PostgreSQL";
        } else {
            std::cout << " using in-memory storage";
        }
        std::cout << std::endl;
        io_context.run();

        for (auto& worker : workers) {
            worker.join();
        }
    } catch (const std::exception& exception) {
        std::cerr << "Server error: " << exception.what() << std::endl;
        return 1;
    }

    return 0;
}
