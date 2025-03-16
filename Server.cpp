#include "Server.h"
#include "Session.h"
#include "Logger.h"

Server::Server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    Logger::instance().log(LogLevel::INFO, "Server initialized on port " + std::to_string(port));
    do_accept();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                Logger::instance().log(LogLevel::INFO, "New connection accepted.");
                auto new_session = std::make_shared<Session>(std::move(socket));
                new_session->start();
            } else {
                Logger::instance().log(LogLevel::ERROR, "Accept error: " + ec.message());
            }
            do_accept(); // keep accepting
        }
    );
} 