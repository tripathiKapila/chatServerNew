#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <stack>

class AuthManager {
public:
    static AuthManager& instance();
    bool authenticate(const std::string &username, const std::string &password);
    bool register_user(const std::string &username, const std::string &password);
    bool is_admin(const std::string &username);

    // For status undo
    void push_status(const std::string &username, const std::string &status);
    std::string pop_status(const std::string &username);

private:
    AuthManager();
    std::unordered_map<std::string, std::size_t> credentials_; // hashed passwords
    std::unordered_map<std::string, std::stack<std::string>> status_history_;
    std::mutex mtx_;

    // We store admin credentials from config
    std::string admin_user_;
    std::size_t admin_hash_;
};

#endif // AUTHMANAGER_H 