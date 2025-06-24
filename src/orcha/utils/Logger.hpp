#pragma once

#include <fstream>
#include <mutex>
#include <string>


enum class LogLevel { INFO, WARNING, ERROR, DEBUG };

class Logger {
public:
    static Logger& instance();

    void log(LogLevel level, const std::string& msg);
    void set_log_file(const std::string& filename);

    static void info(const std::string& msg)   { instance().log(LogLevel::INFO, msg); }
    static void warn(const std::string& msg)   { instance().log(LogLevel::WARNING, msg); }
    static void error(const std::string& msg)  { instance().log(LogLevel::ERROR, msg); }
    static void debug(const std::string& msg)  { instance().log(LogLevel::DEBUG, msg); }
private:
    Logger();
    std::mutex mtx_;
    std::ofstream file_;
    bool use_file_ = false;

    static std::string timestamp();

    static std::string level_to_string(LogLevel level);
};
