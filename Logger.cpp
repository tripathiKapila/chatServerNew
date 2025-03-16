#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

LogLevel Logger::currentLevel = LogLevel::INFO;
std::mutex Logger::logMutex;

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : current_level_(LogLevel::DEBUG) {
    log_file_.open("server.log", std::ios::app);
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::rotate_if_needed() {
    if (log_file_.tellp() >= max_size_) {
        log_file_.close();
        std::string new_name = "server_" + std::to_string(std::time(nullptr)) + ".log";
        fs::rename("server.log", new_name);
        log_file_.open("server.log", std::ios::trunc);
    }
}

void Logger::log(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (level < currentLevel) return;

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &tt);
#else
    localtime_r(&tt, &localTime);
#endif
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &localTime);

    // Convert LogLevel to string
    std::string levelStr;
    switch (level) {
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO:  levelStr = "INFO";  break;
        case LogLevel::WARN:  levelStr = "WARN";  break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
    }

    std::cerr << "[" << timeBuf << "] "
              << "[" << levelStr << "] "
              << message << std::endl;
} 