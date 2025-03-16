#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

LogLevel Logger::currentLevel = LogLevel::INFO;
std::mutex Logger::logMutex;
std::ofstream Logger::logFile("server.log");

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : current_level_(LogLevel::DEBUG) {
    log_file_.open("server.log", std::ios::app);
}

void Logger::setLogLevel(const std::string& level) {
    std::string upperLevel = level;
    std::transform(upperLevel.begin(), upperLevel.end(), upperLevel.begin(), ::toupper);
    
    if (upperLevel == "DEBUG") setLogLevel(LogLevel::DEBUG);
    else if (upperLevel == "INFO") setLogLevel(LogLevel::INFO);
    else if (upperLevel == "WARN") setLogLevel(LogLevel::WARN);
    else if (upperLevel == "ERROR") setLogLevel(LogLevel::ERROR);
    else setLogLevel(LogLevel::INFO); // Default to INFO if unknown
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
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
    if (level >= currentLevel) {
        std::lock_guard<std::mutex> lock(logMutex);
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::string timestamp = std::to_string(std::mktime(&tm));
        std::string levelStr = levelToString(level);
        
        std::string logMessage = "[" + timestamp + "] [" + levelStr + "] " + message + "\n";
        std::cout << logMessage;
        logFile << logMessage;
        logFile.flush();
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    log(message, level);
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
} 