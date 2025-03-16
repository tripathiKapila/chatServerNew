#include "Server.h"
#include "Logger.h"
#include <iostream>

Server::Server(boost::asio::io_context& io_context, short port, int maxConnections)
    : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      io_context_(io_context),
      maxConnections_(maxConnections),
      running_(false),
      threadPool_(maxConnections) {
    Logger::log("Server constructed (port=" + std::to_string(port) +
                ", maxConnections=" + std::to_string(maxConnections) + ")", LogLevel::DEBUG);
}

Server::~Server() {
    stop();
}

void Server::start() {
    Logger::log("Starting server...", LogLevel::INFO);
    running_ = true;

    // Start accepting connections
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                Logger::log("New connection accepted", LogLevel::INFO);
                handleClient(std::move(socket));
            }
            if (running_) {
                acceptLoop();
            }
        });
}

void Server::stop() {
    if (!running_) return;
    running_ = false;
    
    boost::system::error_code ec;
    acceptor_.close(ec);
    
    Logger::log("Server stopped.", LogLevel::INFO);
}

void Server::acceptLoop() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                Logger::log("New connection accepted", LogLevel::INFO);
                handleClient(std::move(socket));
            }
            if (running_) {
                acceptLoop();
            }
        });
}

void Server::handleClient(boost::asio::ip::tcp::socket socket) {
    Logger::log("Handling new client connection", LogLevel::INFO);
    
    // Create a new session and start it
    auto session = std::make_shared<Session>(std::move(socket));
    session->start();
} 