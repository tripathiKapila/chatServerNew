#include "CommandRouter.h"
#include "Session.h"
#include "AuthManager.h"
#include "UserManager.h"
#include "SessionManager.h"
#include "Database.h"
#include "ChatHistoryCache.h"
#include "Logger.h"
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <atomic>

extern std::atomic<int> g_messageCount;
extern boost::asio::io_context* g_io_context_ptr;

CommandRouter::CommandRouter() {}

void CommandRouter::set_session(std::shared_ptr<Session> session) {
    session_ = session;
    register_commands();
}

void CommandRouter::handle_command(const std::string &cmd_line) {
    // Example: /cmd args...
    std::istringstream iss(cmd_line);
    std::string command;
    iss >> command; 
    // remove leading slash if present
    if (!command.empty() && command[0] == '/') {
        command = command.substr(1);
    }

    std::string args;
    std::getline(iss, args);
    boost::algorithm::trim(args);

    auto it = commands_.find(command);
    if (it != commands_.end()) {
        it->second(args);
    } else {
        if (session_) {
            session_->deliver("Unknown command: /" + command);
        }
    }
}

void CommandRouter::handle_broadcast(const std::string &message) {
    if (!session_) return;
    std::string broadcast_msg = session_->get_username() + ": " + message;

    // Log to DB (batch aggregator) and store in memory cache
    Database::getInstance().log_message(session_->get_username(), message);
    ChatHistoryCache::getInstance().add_message(broadcast_msg);

    // Broadcast to others
    SessionManager::getInstance().broadcast(broadcast_msg, session_);
}

void CommandRouter::register_commands() {
    commands_["login"] = [this](const std::string &args){ cmd_login(args); };
    commands_["logout"] = [this](const std::string &args){ cmd_logout(args); };
    commands_["msg"] = [this](const std::string &args){ cmd_msg(args); };
    commands_["history"] = [this](const std::string &args){ cmd_history(args); };
    commands_["status"] = [this](const std::string &args){ cmd_status(args); };
    commands_["undo"] = [this](const std::string &args){ cmd_undo(args); };
    commands_["shutdown"] = [this](const std::string &args){ cmd_shutdown(args); };
    commands_["list"] = [this](const std::string &args){ cmd_list(args); };
    commands_["search"] = [this](const std::string &args){ cmd_search(args); };
    commands_["offline"] = [this](const std::string &args){ cmd_offline(args); };
}

// ---------------------- Command Handlers ----------------------

void CommandRouter::cmd_login(const std::string &args) {
    if (!session_) return;
    if (session_->is_authenticated()) {
        session_->deliver("You are already logged in.");
        return;
    }

    std::istringstream iss(args);
    std::string uname, pwd;
    iss >> uname >> pwd;
    if (uname.empty() || pwd.empty()) {
        session_->deliver("Usage: /login <username> <password>");
        return;
    }

    // Attempt authentication
    if (AuthManager::getInstance().authenticate(uname, pwd)) {
        session_->set_authenticated(true);
        session_->set_username(uname);
        UserManager::getInstance().add_user(uname, session_);
        SessionManager::getInstance().add_session(session_);

        // Deliver offline messages if any
        auto offline_msgs = Database::getInstance().retrieve_offline_messages(uname);
        if (!offline_msgs.empty()) {
            session_->deliver("You have offline messages:");
            for (auto &msg : offline_msgs) {
                session_->deliver(msg);
            }
            // Clear them from DB
            Database::getInstance().clear_offline_messages(uname);
        }

        session_->deliver("Login successful. Welcome, " + uname + "!");
        Logger::log("User logged in: " + uname, LogLevel::INFO);
    } else {
        session_->deliver("Authentication failed. Use /login <username> <password>");
    }
}

void CommandRouter::cmd_logout(const std::string &/*args*/) {
    if (!session_) return;
    if (!session_->is_authenticated()) {
        session_->deliver("You are not logged in.");
        return;
    }
    std::string uname = session_->get_username();
    session_->set_authenticated(false);
    session_->set_username("");
    UserManager::getInstance().remove_user(uname);
    SessionManager::getInstance().remove_session(session_);
    session_->deliver("You have been logged out.");
    Logger::log("User logged out: " + uname, LogLevel::INFO);
}

void CommandRouter::cmd_msg(const std::string &args) {
    if (!session_) return;
    if (!session_->is_authenticated()) {
        session_->deliver("Please /login first.");
        return;
    }

    std::istringstream iss(args);
    std::string target_user;
    iss >> target_user;
    std::string message;
    std::getline(iss, message);
    boost::algorithm::trim(message);

    if (target_user.empty() || message.empty()) {
        session_->deliver("Usage: /msg <username> <message>");
        return;
    }

    auto target_session = UserManager::getInstance().get_user(target_user);
    if (target_session) {
        // The user is online, deliver immediately
        std::string private_msg = "[Private] " + session_->get_username() + ": " + message;
        target_session->deliver(private_msg);
        session_->deliver("[Private to " + target_user + "] " + message);

        // Log and cache
        Database::getInstance().log_message(session_->get_username(),
                                         "[Private to " + target_user + "] " + message);
        ChatHistoryCache::getInstance().add_message(private_msg);
    } else {
        // The user might be offline, store it as an offline message
        session_->deliver("User " + target_user + " is offline or not found. Storing offline.");
        Database::getInstance().store_offline_message(target_user,
            "[Private] " + session_->get_username() + ": " + message);
    }
}

void CommandRouter::cmd_history(const std::string &/*args*/) {
    if (!session_) return;

    std::string db_history = Database::getInstance().get_chat_history();
    auto recent = ChatHistoryCache::getInstance().get_recent_messages();

    session_->deliver("===== Full Chat History (DB) =====\n" + db_history);
    session_->deliver("===== Recent In-Memory =====");
    for (auto &m : recent) {
        session_->deliver(m);
    }
}

void CommandRouter::cmd_status(const std::string &args) {
    if (!session_) return;
    if (!session_->is_authenticated()) {
        session_->deliver("Please /login first.");
        return;
    }

    std::string status = args;
    boost::algorithm::trim(status);
    if (status != "online" && status != "offline") {
        session_->deliver("Usage: /status <online|offline>");
        return;
    }

    UserManager::getInstance().update_status(session_->get_username(), status);
    session_->deliver("Status updated to " + status);
    Logger::log("User " + session_->get_username() + " updated status to " + status, LogLevel::INFO);

    // We store statuses in AuthManager's user record for undo
    AuthManager::getInstance().pushStatus(session_->get_username(), status);
}

void CommandRouter::cmd_undo(const std::string &/*args*/) {
    if (!session_) return;
    if (!session_->is_authenticated()) {
        session_->deliver("Please /login first.");
        return;
    }

    std::string old_status = AuthManager::getInstance().popStatus(session_->get_username());
    if (old_status.empty()) {
        session_->deliver("No previous status to revert to.");
    } else {
        UserManager::getInstance().update_status(session_->get_username(), old_status);
        session_->deliver("Reverted status to " + old_status);
        Logger::log("User " + session_->get_username() + " reverted status to " + old_status, LogLevel::INFO);
    }
}

void CommandRouter::cmd_shutdown(const std::string &/*args*/) {
    if (!session_) return;
    // Must be admin
    if (!AuthManager::getInstance().isAdmin(session_->get_username())) {
        session_->deliver("You are not authorized to shut down the server.");
        return;
    }

    session_->deliver("Shutting down server...");

    // Gracefully stop all sessions first
    SessionManager::getInstance().close_all_sessions();

    // Now stop io_context
    if (g_io_context_ptr) {
        g_io_context_ptr->stop();
    }
    Logger::log("Server shutdown initiated by admin.", LogLevel::INFO);
}

void CommandRouter::cmd_list(const std::string &/*args*/) {
    if (!session_) return;
    if (!AuthManager::getInstance().isAdmin(session_->get_username())) {
        session_->deliver("You are not authorized to use /list.");
        return;
    }
    auto users = UserManager::getInstance().get_all_users();
    std::string user_list = "Active Users:\n";
    for (const auto &u : users) {
        user_list += "  " + u + "\n";
    }
    session_->deliver(user_list);
}

void CommandRouter::cmd_search(const std::string &args) {
    if (!session_) return;
    // Basic search in ChatHistoryCache
    if (args.empty()) {
        session_->deliver("Usage: /search <keyword>");
        return;
    }
    auto results = ChatHistoryCache::getInstance().search(args);
    if (results.empty()) {
        session_->deliver("No messages found matching '" + args + "'.");
    } else {
        session_->deliver("Search results:");
        for (auto &r : results) {
            session_->deliver(r);
        }
    }
}

void CommandRouter::cmd_offline(const std::string &/*args*/) {
    // Just demonstration: forcibly close the session to test offline messaging
    if (!session_) return;
    session_->deliver("Forcing you offline...");
    session_->force_disconnect();
} 