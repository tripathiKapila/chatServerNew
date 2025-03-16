#include "Logger.h"
#include <iostream>
#include <ctime>
#include <chrono>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : current_level_(LogLevel::DEBUG) {
    log_file_.open("server.log", std::ios::app);
}

void Logger::set_log_level(LogLevel level) {
    current_level_ = level;
}

void Logger::rotate_if_needed() {
    if (log_file_.tellp() >= max_size_) {
        log_file_.close();
        std::string new_name = "server_" + std::to_string(std::time(nullptr)) + ".log";
        fs::rename("server.log", new_name);
        log_file_.open("server.log", std::ios::trunc);
    }
}

void Logger::log(LogLevel level, const std::string &message) {
    if (static_cast<int>(level) < static_cast<int>(current_level_)) return;
    std::lock_guard<std::mutex> lock(mtx_);

    rotate_if_needed();

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::ctime(&now_time);
    std::string time_str = ss.str();
    if (!time_str.empty() && time_str.back() == '\n') {
        time_str.pop_back();
    }

    std::string level_str;
    switch(level) {
        case LogLevel::DEBUG: level_str = "DEBUG"; break;
        case LogLevel::INFO: level_str = "INFO"; break;
        case LogLevel::ERROR: level_str = "ERROR"; break;
    }

    std::string log_entry = "[" + time_str + "][" + level_str + "] " + message;
    std::cout << log_entry << std::endl;
    log_file_ << log_entry << std::endl;
    log_file_.flush();
} 