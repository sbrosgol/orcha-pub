#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <fstream>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger {
public:
    static Logger& instance();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void info(const std::string& msg)   { instance().log(LogLevel::INFO, msg); }
    static void warn(const std::string& msg)   { instance().log(LogLevel::WARNING, msg); }
    static void error(const std::string& msg)  { instance().log(LogLevel::ERROR, msg); }
    static void debug(const std::string& msg)  { instance().log(LogLevel::DEBUG, msg); }

    void set_log_file(const std::string& filename);
    void log(LogLevel level, const std::string& msg);
    void shutdown(); // flush + exit

private:
    Logger();
    ~Logger();

    void logging_thread(); // Background loop
    void enqueue(const std::string& message);

    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::string> log_queue_;
    std::thread worker_;
    std::atomic<bool> running_{true};

    std::ofstream file_;
    std::string log_filename_;
    bool use_file_ = false;

    static std::string timestamp();
    static std::string level_to_string(LogLevel level);
};
