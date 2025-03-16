#include "SessionManager.h"
#include <algorithm>
#include "Logger.h"

SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

void SessionManager::add_session(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mtx_);
    sessions_.push_back(session);
}

void SessionManager::remove_session(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mtx_);
    sessions_.erase(std::remove(sessions_.begin(), sessions_.end(), session), sessions_.end());
}

void SessionManager::broadcast(const std::string &message, std::shared_ptr<Session> exclude) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& session : sessions_) {
        if (session != exclude) {
            session->deliver(message);
        }
    }
}

void SessionManager::close_all_sessions() {
    std::lock_guard<std::mutex> lock(mtx_);
    Logger::instance().log(LogLevel::INFO, "Closing all sessions for server shutdown...");
    for (auto &sess : sessions_) {
        sess->deliver("Server is shutting down now. You will be disconnected.");
        sess->force_disconnect();
    }
    sessions_.clear();
} 