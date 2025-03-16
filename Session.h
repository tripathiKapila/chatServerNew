#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <stack>

#include "CommandRouter.h"

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket);
    void start();

    // Send a message to this session
    void deliver(const std::string &msg);

    // Authentication state
    bool is_authenticated() const;
    void set_authenticated(bool auth);

    // Username management
    void set_username(const std::string &name);
    std::string get_username() const;

    // Utility for referencing command router
    CommandRouter& get_command_router() { return command_router_; }

    // For graceful shutdown from within commands
    void force_disconnect();

private:
    void do_read();
    void process_message(const std::string &msg);
    void start_idle_timer();
    void reset_idle_timer();

    boost::asio::ip::tcp::socket socket_;
    boost::asio::steady_timer idle_timer_;

    // Buffer for incoming data
    static const std::size_t max_length_ = 2048;
    char data_[max_length_];

    bool authenticated_;
    std::string username_;
    int idle_timeout_seconds_;

    // The command router (each session has one to handle commands)
    CommandRouter command_router_;
}; 