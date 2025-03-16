#include "UserManager.h"
#include "Logger.h"

UserManager& UserManager::instance() {
    static UserManager instance;
    return instance;
}

void UserManager::add_user(const std::string &username, std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mtx_);
    users_[username] = session;
    user_status_[username] = "online";
    Logger::instance().log(LogLevel::INFO, "User added: " + username);
}

void UserManager::remove_user(const std::string &username) {
    std::lock_guard<std::mutex> lock(mtx_);
    users_.erase(username);
    user_status_.erase(username);
    Logger::instance().log(LogLevel::INFO, "User removed: " + username);
}

std::shared_ptr<Session> UserManager::get_user(const std::string &username) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = users_.find(username);
    if (it != users_.end()) {
        return it->second;
    }
    return nullptr;
}

void UserManager::update_status(const std::string &username, const std::string &status) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = user_status_.find(username);
    if (it != user_status_.end()) {
        it->second = status;
        Logger::instance().log(LogLevel::INFO, "User " + username + " status updated to " + status);
    }
}

std::vector<std::string> UserManager::get_all_users() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<std::string> result;
    for (auto &pair : users_) {
        result.push_back(pair.first);
    }
    return result;
} 