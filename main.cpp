#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <signal.h>

#include "Server.h"
#include "Logger.h"
#include "Config.h"
#include "Database.h"

// We'll store a pointer to the io_context globally
// so we can stop it gracefully on shutdown.
boost::asio::io_context* g_io_context_ptr = nullptr;

// Counter for performance monitoring
std::atomic<int> g_messageCount(0);

std::atomic<bool> running(true);

void signalHandler(int signum) {
    running = false;
    if (g_io_context_ptr) {
        g_io_context_ptr->stop();
    }
}

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
        // Set up signal handling
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // Load configuration
        Config::getInstance().load("server.config");
        int port = Config::getInstance().getInt("port", 12345);
        int maxConnections = Config::getInstance().getInt("max_connections", 100);
        std::string logLevel = Config::getInstance().getValue("log_level", "INFO");

        // Set up logging
        Logger::setLogLevel(logLevel);

        // Create and start server
        boost::asio::io_context io_context;
        g_io_context_ptr = &io_context;
        
        Server server(io_context, port, maxConnections);
        server.start();

        // Run the io_context
        while (running) {
            io_context.run_one();
        }

        // Clean shutdown
        server.stop();
        Logger::log("Server shutdown complete", LogLevel::INFO);

    } catch (std::exception& e) {
        Logger::log("Exception: " + std::string(e.what()), LogLevel::ERROR);
        return 1;
    }

    return 0;
} 