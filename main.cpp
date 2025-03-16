#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

#include "Server.h"
#include "Logger.h"
#include "Config.h"
#include "Database.h"

// We'll store a pointer to the io_context globally
// so we can stop it gracefully on shutdown.
boost::asio::io_context* g_io_context_ptr = nullptr;

// Counter for performance monitoring
std::atomic<int> g_messageCount(0);

void performance_monitor(boost::asio::io_context &io_context) {
    using namespace std::chrono_literals;
    boost::asio::steady_timer timer(io_context, std::chrono::seconds(10));
    timer.async_wait([&](const boost::system::error_code& ec){
        if (!ec) {
            Logger::instance().log(LogLevel::INFO,
                "[Performance] Messages processed in last 10 sec: " + std::to_string(g_messageCount.load()));
            g_messageCount.store(0);
            performance_monitor(io_context); // reschedule
        }
    });
}

int main() {
    try {
        boost::asio::io_context io_context;
        g_io_context_ptr = &io_context;

        // Load config file (server.config) if it exists
        if (!Config::instance().load("server.config")) {
            Logger::instance().log(LogLevel::ERROR,
                "Failed to load server.config. Using default settings.");
        } else {
            Logger::instance().log(LogLevel::INFO, "Configuration loaded successfully.");
        }

        // Initialize Database (also starts the DB aggregator thread)
        Database::instance(); 

        // Start performance monitoring
        performance_monitor(io_context);

        int port = Config::instance().get_int("port", 12345);
        Server server(io_context, static_cast<short>(port));

        // Prepare thread pool
        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }

        Logger::instance().log(LogLevel::INFO,
            "Server is running on port " + std::to_string(port) +
            " with " + std::to_string(num_threads) + " threads.");

        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }

        // Ensure DB aggregator thread stops gracefully
        Database::instance().stop_aggregator();

    } catch (std::exception &e) {
        Logger::instance().log(LogLevel::ERROR, std::string("Exception in main: ") + e.what());
    }
    return 0;
} 