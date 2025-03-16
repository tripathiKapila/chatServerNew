#ifndef CHATHISTORYCACHE_H
#define CHATHISTORYCACHE_H

#include <string>
#include <vector>
#include <mutex>

class ChatHistoryCache {
public:
    static ChatHistoryCache& instance();
    void add_message(const std::string &message);
    std::vector<std::string> get_recent_messages();
    
    // Basic search in the recent messages
    std::vector<std::string> search(const std::string &keyword);

private:
    ChatHistoryCache();
    size_t max_messages_;
    std::vector<std::string> messages_;
    std::mutex mtx_;
};

#endif // CHATHISTORYCACHE_H 