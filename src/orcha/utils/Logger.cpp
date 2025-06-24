#include "Logger.hpp"

#include <iostream>
#include <sstream>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() = default;

void Logger::set_log_file(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mtx_);
    file_.open(filename, std::ios::app);
    use_file_ = file_.is_open();
}

std::string Logger::timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t_c = system_clock::to_time_t(now);
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &t_c);
#else
    localtime_r(&t_c, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%F %T");
    return oss.str();
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::DEBUG: return "DEBUG";
        default: return "UNK";
    }
}

void Logger::log(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ostringstream oss;
    oss << "[" << timestamp() << "][" << level_to_string(level) << "] " << msg << std::endl;
    std::string line = oss.str();

    // Write to appropriate stream
    if (level == LogLevel::INFO || level == LogLevel::DEBUG)
        std::cout << line;
    else
        std::cerr << line;

    if (use_file_) file_ << line;
}
