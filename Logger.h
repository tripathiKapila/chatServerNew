#pragma once

#include <string>
#include <fstream>
#include <mutex>

// Undefine Windows' ERROR macro if defined.
#ifdef ERROR
#undef ERROR
#endif

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static void setLogLevel(const std::string& level);
    static void setLogLevel(LogLevel level);
    static void log(const std::string& message, LogLevel level = LogLevel::INFO);
    static void log(LogLevel level, const std::string& message);

private:
    static LogLevel currentLevel;
    static std::mutex logMutex;
    static std::ofstream logFile;
    static std::string levelToString(LogLevel level);
};
