#include "AuthManager.h"
#include "Logger.h"
#include "Config.h"
#include <functional> // for std::hash
#include <sstream>

AuthManager& AuthManager::instance() {
    static AuthManager instance;
    return instance;
}

AuthManager::AuthManager() {
    // Load admin user/pass from config
    // If not found, defaults to "admin":"admin123"
    auto admin_u = Config::instance().get("admin_user", "admin");
    auto admin_p = Config::instance().get("admin_pass", "admin123");

    std::hash<std::string> hasher;
    admin_user_ = admin_u;
    admin_hash_ = hasher(admin_p);

    // Insert admin credentials into map
    credentials_[admin_user_] = admin_hash_;

    Logger::instance().log(LogLevel::INFO,
        "AuthManager: admin user loaded: " + admin_user_);
}

bool AuthManager::authenticate(const std::string &username, const std::string &password) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::hash<std::string> hasher;
    auto pass_hash = hasher(password);

    auto it = credentials_.find(username);
    if (it != credentials_.end()) {
        return (it->second == pass_hash);
    }
    return false;
}

bool AuthManager::register_user(const std::string &username, const std::string &password) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (credentials_.find(username) != credentials_.end()) {
        // user already exists
        return false;
    }
    std::hash<std::string> hasher;
    credentials_[username] = hasher(password);
    return true;
}

bool AuthManager::is_admin(const std::string &username) {
    return (username == admin_user_);
}

void AuthManager::push_status(const std::string &username, const std::string &status) {
    std::lock_guard<std::mutex> lock(mtx_);
    status_history_[username].push(status);
}

std::string AuthManager::pop_status(const std::string &username) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto &stack = status_history_[username];
    if (stack.empty()) {
        return "";
    }
    std::string old_status = stack.top();
    stack.pop();
    return old_status;
} 