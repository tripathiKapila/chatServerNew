#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

struct UserRecord {
    std::string salt;      // Random salt
    std::string passHash;  // Hashed password with salt
};

struct DBMessage {
    std::string username;
    std::string message;
    DBMessage(const std::string &u, const std::string &m) : username(u), message(m) {}
};

class Database {
public:
    static Database& instance();
    void log_message(const std::string &username, const std::string &message);
    std::string get_chat_history();
    void store_offline_message(const std::string &to_user, const std::string &message);
    std::vector<std::string> retrieve_offline_messages(const std::string &username);
    void clear_offline_messages(const std::string &username);
    void stop_aggregator();
    bool verifyUser(const std::string& username, const std::string& password);

private:
    Database();
    ~Database();
    bool exec_sql(const std::string &sql);
    void db_aggregator_main();
    static std::string generateSalt(size_t length = 16);
    static std::string hashPassword(const std::string& password, const std::string& salt);
    void loadUsers();

    sqlite3 *db_;
    std::queue<DBMessage> message_queue_;
    std::mutex queue_mtx_;
    std::condition_variable queue_cv_;
    std::thread aggregator_thread_;
    bool running_;
    std::unordered_map<std::string, UserRecord> users;
};

#endif  // DATABASE_H
