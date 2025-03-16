#pragma once

#include <string>
#include <vector>
#include <deque>
#include <mutex>

class ChatHistoryCache {
public:
    static ChatHistoryCache& getInstance() {
        static ChatHistoryCache instance;
        return instance;
    }

    void add_message(const std::string& message);
    std::vector<std::string> get_recent_messages(size_t count = 10);
    std::vector<std::string> search(const std::string& keyword);

private:
    ChatHistoryCache() = default;
    ~ChatHistoryCache() = default;

    ChatHistoryCache(const ChatHistoryCache&) = delete;
    ChatHistoryCache& operator=(const ChatHistoryCache&) = delete;

    std::deque<std::string> messages;
    std::mutex cacheMutex;
    static const size_t MAX_CACHE_SIZE = 1000;
}; 