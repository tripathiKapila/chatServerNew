#include "ChatHistoryCache.h"
#include <algorithm>

ChatHistoryCache& ChatHistoryCache::instance() {
    static ChatHistoryCache instance;
    return instance;
}

ChatHistoryCache::ChatHistoryCache() : max_messages_(50) {}

void ChatHistoryCache::add_message(const std::string &message) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (messages_.size() >= max_messages_) {
        messages_.erase(messages_.begin());
    }
    messages_.push_back(message);
}

std::vector<std::string> ChatHistoryCache::get_recent_messages() {
    std::lock_guard<std::mutex> lock(mtx_);
    return messages_;
}

std::vector<std::string> ChatHistoryCache::search(const std::string &keyword) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<std::string> results;
    for (auto &msg : messages_) {
        if (msg.find(keyword) != std::string::npos) {
            results.push_back(msg);
        }
    }
    return results;
} 