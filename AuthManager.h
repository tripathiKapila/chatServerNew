#pragma once
#include <string>
#include <unordered_map>
#include <stack>
#include <mutex>
#include "Logger.h"

enum class UserRole {
    USER,
    ADMIN
};

class AuthManager {
public:
    static AuthManager& getInstance() {
        static AuthManager instance;
        return instance;
    }

    bool authenticate(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password);
    bool isAdmin(const std::string& username) const;
    void setUserRole(const std::string& username, UserRole role);
    UserRole getUserRole(const std::string& username) const;
    void pushStatus(const std::string& username, const std::string& status);
    std::string popStatus(const std::string& username);

private:
    AuthManager();
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    std::string hashPassword(const std::string& password);
    std::unordered_map<std::string, std::string> credentials_;
    std::unordered_map<std::string, UserRole> userRoles_;
    std::unordered_map<std::string, std::stack<std::string>> statusHistory_;
    std::string adminUser_;
    mutable std::mutex mtx_;
}; 