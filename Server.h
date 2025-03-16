#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include "Session.h"
#include "Network/ThreadPool.h"

class Server {
public:
    Server(boost::asio::io_context& io_context, short port, int maxConnections);
    ~Server();
    void start();
    void stop();

private:
    void acceptLoop();
    void handleClient(boost::asio::ip::tcp::socket socket);

    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    int maxConnections_;
    std::atomic<bool> running_;
    ThreadPool threadPool_;
}; 