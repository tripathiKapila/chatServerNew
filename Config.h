#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>
#include <mutex>

class Config {
public:
    static Config& instance();
    bool load(const std::string& filename);
    std::string get(const std::string &key, const std::string &default_value = "");
    int get_int(const std::string &key, int default_value);
    void reload(const std::string& filename);

private:
    Config();
    std::unordered_map<std::string, std::string> config_map_;
    std::mutex mtx_;
};

#endif // CONFIG_H 