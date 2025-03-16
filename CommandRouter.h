#ifndef COMMANDROUTER_H
#define COMMANDROUTER_H

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>

class Session;

class CommandRouter {
public:
    CommandRouter();
    void set_session(std::shared_ptr<Session> session);
    void handle_command(const std::string &cmd_line);
    void handle_broadcast(const std::string &message);

private:
    void register_commands();

    // Command handlers
    void cmd_login(const std::string &args);
    void cmd_logout(const std::string &args);
    void cmd_msg(const std::string &args);
    void cmd_history(const std::string &args);
    void cmd_status(const std::string &args);
    void cmd_undo(const std::string &args);
    void cmd_shutdown(const std::string &args);
    void cmd_list(const std::string &args);
    void cmd_search(const std::string &args);
    void cmd_offline(const std::string &args); // demonstration command to show offline messaging

    std::shared_ptr<Session> session_;
    std::unordered_map<std::string, std::function<void(const std::string&)>> commands_;
};

#endif // COMMANDROUTER_H 