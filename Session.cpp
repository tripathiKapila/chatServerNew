#include "Session.h"
#include "UserManager.h"
#include "SessionManager.h"
#include "Logger.h"
#include "Config.h"
#include <boost/algorithm/string.hpp>
#include <string>
#include <iostream>
#include <atomic>

// global for performance counting
extern std::atomic<int> g_messageCount;

// forward-declared in main.cpp
extern boost::asio::io_context* g_io_context_ptr;

Session::Session(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket)),
      idle_timer_(socket_.get_executor()),
      authenticated_(false),
      idle_timeout_seconds_(Config::instance().get_int("idle_timeout", 300))
{
    // Attach "this" session pointer to the command router
    command_router_.set_session(shared_from_this());
}

void Session::start() {
    deliver("Welcome! Please login with: /login <username> <password>");
    start_idle_timer();
    do_read();
}

void Session::deliver(const std::string &msg) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(msg + "\n"),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (ec) {
                Logger::instance().log(LogLevel::ERROR,
                    "Error delivering message to " + username_ + ": " + ec.message());
            }
        });
}

bool Session::is_authenticated() const {
    return authenticated_;
}

void Session::set_authenticated(bool auth) {
    authenticated_ = auth;
}

void Session::set_username(const std::string &name) {
    username_ = name;
}

std::string Session::get_username() const {
    return username_;
}

void Session::force_disconnect() {
    boost::system::error_code ignored;
    socket_.close(ignored);
}

void Session::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length_),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                reset_idle_timer();
                // If length is abnormally large, we can handle that
                if (length > max_length_) {
                    deliver("Message too long!");
                    return;
                }

                std::string msg(data_, length);
                boost::algorithm::trim(msg);
                process_message(msg);
                do_read();
            } else {
                Logger::instance().log(LogLevel::INFO, "Session ended for user: " + username_);
                UserManager::instance().remove_user(username_);
                SessionManager::instance().remove_session(self);
            }
        });
}

void Session::process_message(const std::string &msg) {
    // Increase performance counter
    g_messageCount++;

    // If the user isn't authenticated, only allow /login
    if (!is_authenticated()) {
        if (msg.size() >= 6 && msg.substr(0, 6) == "/login") {
            // Pass entire command to router
            command_router_.handle_command(msg);
        } else {
            deliver("You must /login <username> <password> before issuing other commands.");
        }
        return;
    }

    // If message is a slash command, route it
    if (!msg.empty() && msg[0] == '/') {
        command_router_.handle_command(msg);
    } else {
        // Otherwise, treat as broadcast
        command_router_.handle_broadcast(msg);
    }
}

void Session::start_idle_timer() {
    auto self(shared_from_this());
    idle_timer_.expires_after(std::chrono::seconds(idle_timeout_seconds_));
    idle_timer_.async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec) {
            deliver("Idle timeout. Disconnecting...");
            Logger::instance().log(LogLevel::INFO, "Session timed out for user: " + username_);
            force_disconnect();
        }
    });
}

void Session::reset_idle_timer() {
    boost::system::error_code ec;
    idle_timer_.cancel(ec);
    start_idle_timer();
} 