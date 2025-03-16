#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <memory>
#include <vector>
#include <mutex>
#include "Session.h"

class SessionManager {
public:
    static SessionManager& instance();
    void add_session(std::shared_ptr<Session> session);
    void remove_session(std::shared_ptr<Session> session);
    void broadcast(const std::string &message, std::shared_ptr<Session> exclude = nullptr);

    // For graceful shutdown
    void close_all_sessions();

private:
    SessionManager() = default;
    std::vector<std::shared_ptr<Session>> sessions_;
    std::mutex mtx_;
};

#endif // SESSIONMANAGER_H 