#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

class Config {
public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    std::string getValue(const std::string& key, const std::string& defaultValue = "") const;
    bool load(const std::string& filename);
    int getInt(const std::string& key, int defaultValue = 0) const;
    void reload(const std::string& filename);

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    std::string configFileName;
    std::unordered_map<std::string, std::string> configData;
    void loadConfig();
    mutable std::mutex mtx_;
}; 