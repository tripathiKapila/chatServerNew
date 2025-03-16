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

int main(int argc, char* argv[]) {
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        boost::asio::io_context io_context;
        g_io_context_ptr = &io_context;

        // Load configuration
        Config config("server.config");
        int port = std::stoi(config.getValue("port", "12345"));
        int maxConnections = std::stoi(config.getValue("max_connections", "100"));
        std::string logLevel = config.getValue("log_level", "INFO");

        // Set up logging
        if (logLevel == "DEBUG") Logger::setLogLevel(LogLevel::DEBUG);
        else if (logLevel == "WARN") Logger::setLogLevel(LogLevel::WARN);
        else if (logLevel == "ERROR") Logger::setLogLevel(LogLevel::ERROR);
        else Logger::setLogLevel(LogLevel::INFO);

        Logger::log("Starting chat server...", LogLevel::INFO);
        Logger::log("Configuration loaded:", LogLevel::DEBUG);
        Logger::log("Port: " + std::to_string(port), LogLevel::DEBUG);
        Logger::log("Max connections: " + std::to_string(maxConnections), LogLevel::DEBUG);
        Logger::log("Log level: " + logLevel, LogLevel::DEBUG);

        // Initialize Database (also starts the DB aggregator thread)
        Database::instance(); 

        // Start performance monitoring
        performance_monitor(io_context);

        // Create and start server
        Server server(io_context, static_cast<short>(port), maxConnections);
        server.start();

        // Main loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Clean shutdown
        Logger::log("Shutting down server...", LogLevel::INFO);
        server.stop();

        // Ensure DB aggregator thread stops gracefully
        Database::instance().stop_aggregator();

    } catch (const std::exception& e) {
        Logger::log("Fatal error: " + std::string(e.what()), LogLevel::ERROR);
        return 1;
    }

    return 0;
} 