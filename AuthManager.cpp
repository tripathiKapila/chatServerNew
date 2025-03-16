#include "AuthManager.h"
#include "Logger.h"
#include "Config.h"
#include <functional> // for std::hash
#include <sstream>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <iomanip>

AuthManager::AuthManager() {
    // Load admin credentials from config
    adminUser_ = Config::getInstance().getValue("admin_user", "admin");
    std::string adminPass = Config::getInstance().getValue("admin_pass", "admin123");
    
    // Hash admin password
    std::string hashedPass = hashPassword(adminPass);
    
    // Store admin credentials
    credentials_[adminUser_] = hashedPass;
    userRoles_[adminUser_] = UserRole::ADMIN;
    
    Logger::log("AuthManager initialized with admin user: " + adminUser_, LogLevel::INFO);
}

bool AuthManager::authenticate(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string hashedPass = hashPassword(password);
    auto it = credentials_.find(username);
    if (it != credentials_.end()) {
        return (it->second == hashedPass);
    }
    return false;
}

bool AuthManager::registerUser(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (credentials_.find(username) != credentials_.end()) {
        return false; // User already exists
    }
    credentials_[username] = hashPassword(password);
    userRoles_[username] = UserRole::USER;
    return true;
}

bool AuthManager::isAdmin(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = userRoles_.find(username);
    return (it != userRoles_.end() && it->second == UserRole::ADMIN);
}

void AuthManager::setUserRole(const std::string& username, UserRole role) {
    std::lock_guard<std::mutex> lock(mtx_);
    userRoles_[username] = role;
}

UserRole AuthManager::getUserRole(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = userRoles_.find(username);
    return (it != userRoles_.end() ? it->second : UserRole::USER);
}

void AuthManager::pushStatus(const std::string& username, const std::string& status) {
    std::lock_guard<std::mutex> lock(mtx_);
    statusHistory_[username].push(status);
}

std::string AuthManager::popStatus(const std::string& username) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto& stack = statusHistory_[username];
    if (stack.empty()) {
        return "";
    }
    std::string oldStatus = stack.top();
    stack.pop();
    return oldStatus;
}

std::string AuthManager::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
} 