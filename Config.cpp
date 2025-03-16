#include "Config.h"
#include <fstream>
#include <sstream>

Config& Config::instance() {
    static Config instance;
    return instance;
}

Config::Config() {}

bool Config::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    config_map_.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            config_map_[key] = value;
        }
    }
    return true;
}

std::string Config::get(const std::string &key, const std::string &default_value) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = config_map_.find(key);
    return (it != config_map_.end()) ? it->second : default_value;
}

int Config::get_int(const std::string &key, int default_value) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = config_map_.find(key);
    if (it != config_map_.end()) {
        try { return std::stoi(it->second); }
        catch (...) { return default_value; }
    }
    return default_value;
}

void Config::reload(const std::string& filename) {
    load(filename);
} 