#include "Database.h"
#include "Logger.h"
#include <sstream>
#include <chrono>
#include <fstream>
#include <random>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <iomanip>

static int callback(void *unused, int count, char **data, char **columns) {
    // we won't use this, just a placeholder
    return 0;
}

Database& Database::instance() {
    static Database instance;
    return instance;
}

Database::Database() : db_(nullptr), running_(false) {
    int rc = sqlite3_open("chat.db", &db_);
    if (rc) {
        Logger::log("Can't open database: " + std::string(sqlite3_errmsg(db_)), LogLevel::ERROR);
        return;
    }

    // Create tables if they don't exist
    exec_sql("CREATE TABLE IF NOT EXISTS messages ("
             "id INTEGER PRIMARY KEY AUTOINCREMENT,"
             "username TEXT NOT NULL,"
             "message TEXT NOT NULL,"
             "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);");

    exec_sql("CREATE TABLE IF NOT EXISTS offline_messages ("
             "id INTEGER PRIMARY KEY AUTOINCREMENT,"
             "to_user TEXT NOT NULL,"
             "message TEXT NOT NULL,"
             "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);");

    exec_sql("CREATE TABLE IF NOT EXISTS users ("
             "username TEXT PRIMARY KEY,"
             "salt TEXT NOT NULL,"
             "password_hash TEXT NOT NULL);");

    loadUsers();
    running_ = true;
    aggregator_thread_ = std::thread(&Database::db_aggregator_main, this);
}

Database::~Database() {
    stop_aggregator();
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::exec_sql(const std::string &sql) {
    char *errmsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), callback, 0, &errmsg) != SQLITE_OK) {
        Logger::instance().log(LogLevel::ERROR, "SQL error: " + std::string(errmsg));
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

void Database::log_message(const std::string &username, const std::string &message) {
    // Instead of writing directly, we push to a queue
    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        message_queue_.push({username, message});
    }
    queue_cv_.notify_one();
}

void Database::db_aggregator_main() {
    // Batches messages every 2 seconds or so
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mtx_);
        if (message_queue_.empty()) {
            // Wait until there's a message or we time out
            queue_cv_.wait_for(lock, std::chrono::seconds(2));
        }

        if (!running_) break;

        // Gather all messages currently in the queue
        std::vector<DBMessage> batch;
        while (!message_queue_.empty()) {
            batch.push_back(message_queue_.front());
            message_queue_.pop();
        }
        lock.unlock();

        if (!batch.empty() && db_) {
            // Perform a transaction to insert them
            const char *begin_tx = "BEGIN TRANSACTION;";
            const char *commit_tx = "COMMIT;";
            sqlite3_exec(db_, begin_tx, 0, 0, nullptr);

            std::string sql = "INSERT INTO messages (username, message) VALUES (?, ?);";
            for (auto &msg : batch) {
                sqlite3_stmt *stmt = nullptr;
                if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, msg.username.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_text(stmt, 2, msg.message.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(stmt) != SQLITE_DONE) {
                        Logger::instance().log(LogLevel::ERROR, "Failed to execute batch insert.");
                    }
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_exec(db_, commit_tx, 0, 0, nullptr);
        }
    }
}

std::string Database::get_chat_history() {
    std::lock_guard<std::mutex> lock(queue_mtx_);
    if (!db_) {
        return "Database not available.\n";
    }
    std::string history;
    const char *sql = "SELECT timestamp, username, message FROM messages ORDER BY id ASC;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "Failed to retrieve history.\n";
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *timestamp = sqlite3_column_text(stmt, 0);
        const unsigned char *username = sqlite3_column_text(stmt, 1);
        const unsigned char *message = sqlite3_column_text(stmt, 2);
        history += (timestamp ? (const char*)timestamp : "") ;
        history += " ";
        history += (username ? (const char*)username : "");
        history += ": ";
        history += (message ? (const char*)message : "");
        history += "\n";
    }
    sqlite3_finalize(stmt);
    return history;
}

void Database::store_offline_message(const std::string &to_user, const std::string &message) {
    // Immediate insert for offline messages
    if (!db_) return;
    std::lock_guard<std::mutex> lock(queue_mtx_);
    std::string sql = "INSERT INTO offline_msgs (to_user, message) VALUES (?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, to_user.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, message.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            Logger::instance().log(LogLevel::ERROR, "Failed to store offline message.");
        }
    }
    sqlite3_finalize(stmt);
}

std::vector<std::string> Database::retrieve_offline_messages(const std::string &username) {
    std::vector<std::string> results;
    if (!db_) return results;

    std::lock_guard<std::mutex> lock(queue_mtx_);
    std::string sql = "SELECT id, message FROM offline_msgs WHERE to_user = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *msg_text = sqlite3_column_text(stmt, 1);
            if (msg_text) {
                results.push_back((const char*)msg_text);
            }
        }
    }
    sqlite3_finalize(stmt);
    return results;
}

void Database::clear_offline_messages(const std::string &username) {
    if (!db_) return;
    std::lock_guard<std::mutex> lock(queue_mtx_);
    std::string sql = "DELETE FROM offline_msgs WHERE to_user = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void Database::stop_aggregator() {
    running_ = false;
    queue_cv_.notify_all();
    if (aggregator_thread_.joinable()) {
        aggregator_thread_.join();
    }
}

bool Database::verifyUser(const std::string& username, const std::string& password) {
    Logger::log("Verifying user: " + username, LogLevel::DEBUG);
    auto it = users.find(username);
    if (it == users.end()) {
        return false;
    }
    // Hash the incoming password with the stored salt
    std::string hashedIncoming = hashPassword(password, it->second.salt);
    return (hashedIncoming == it->second.passHash);
}

void Database::loadUsers() {
    const char* sql = "SELECT username, salt, password_hash FROM users;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        Logger::log("Failed to load users: " + std::string(sqlite3_errmsg(db_)), LogLevel::ERROR);
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string passHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        users[username] = UserRecord{salt, passHash};
    }
    sqlite3_finalize(stmt);
    Logger::log("Users loaded from database", LogLevel::INFO);
}

std::string Database::generateSalt(size_t length) {
    static const char chars[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    thread_local static std::random_device rd;
    thread_local static std::mt19937 gen(rd());
    thread_local static std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);

    std::string salt;
    salt.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        salt.push_back(chars[dist(gen)]);
    }
    return salt;
}

std::string Database::hashPassword(const std::string& password, const std::string& salt) {
    // For demonstration, just do SHA256( salt + password )
    std::string combined = salt + password;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.size(), hash);

    // Convert to hex string
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
} 