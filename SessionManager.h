#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include "Session.h"

class SessionManager {
public:
    static SessionManager& getInstance() {
        static SessionManager instance;
        return instance;
    }

    void add_session(std::shared_ptr<Session> session);
    void remove_session(std::shared_ptr<Session> session);
    void broadcast(const std::string& message, std::shared_ptr<Session> exclude = nullptr);
    void close_all_sessions();

private:
    SessionManager() = default;
    ~SessionManager() = default;

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    std::vector<std::shared_ptr<Session>> sessions;
    std::mutex sessionsMutex;
}; 