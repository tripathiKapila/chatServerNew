#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

// Undefine Windows' ERROR macro if defined.
#ifdef ERROR
#undef ERROR
#endif

enum class LogLevel { DEBUG, INFO, ERROR };

class Logger {
public:
    static Logger& instance();
    void log(LogLevel level, const std::string &message);
    void set_log_level(LogLevel level);

private:
    Logger();
    void rotate_if_needed();

    std::ofstream log_file_;
    std::mutex mtx_;
    LogLevel current_level_;
    const std::streamoff max_size_ = 1024 * 1024; // 1 MB rotation threshold
};

#endif // LOGGER_H
