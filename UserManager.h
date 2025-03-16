#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>
#include "Session.h"

class UserManager {
public:
    static UserManager& instance();
    void add_user(const std::string &username, std::shared_ptr<Session> session);
    void remove_user(const std::string &username);
    std::shared_ptr<Session> get_user(const std::string &username);
    void update_status(const std::string &username, const std::string &status);
    std::vector<std::string> get_all_users();

private:
    UserManager() = default;
    std::unordered_map<std::string, std::shared_ptr<Session>> users_;
    std::unordered_map<std::string, std::string> user_status_;
    std::mutex mtx_;
};

#endif // USERMANAGER_H 