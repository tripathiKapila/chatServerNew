#include "Config.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

bool Config::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mtx_);
    configFileName = filename;
    loadConfig();
    return true;
}

void Config::loadConfig() {
    std::ifstream file(configFileName);
    if (!file.is_open()) {
        Logger::log("Failed to open config file: " + configFileName, LogLevel::ERROR);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        // Parse key=value pairs
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            configData[key] = value;
        }
    }

    Logger::log("Loaded configuration from " + configFileName, LogLevel::INFO);
}

std::string Config::getValue(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = configData.find(key);
    if (it != configData.end()) {
        return it->second;
    }
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = configData.find(key);
    if (it != configData.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

void Config::reload(const std::string& filename) {
    load(filename);
} 