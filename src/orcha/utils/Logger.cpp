#include "Logger.hpp"


Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {
    worker_ = std::thread(&Logger::logging_thread, this);
}

Logger::~Logger() {
    shutdown(); // Ensure logs are flushed on static destruction
}

void Logger::set_log_file(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!filename.empty()) {
        std::filesystem::path p(filename);
        auto dir = p.parent_path();
        if (!dir.empty()) {
            std::filesystem::create_directories(dir);
        }
    }
    log_filename_ = filename;
    file_.open(filename, std::ios::app);
    use_file_ = file_.is_open();
}

void Logger::log(LogLevel level, const std::string& msg) {
    std::ostringstream oss;
    oss << "[" << timestamp() << "][" << level_to_string(level) << "] " << msg << '\n';
    enqueue(oss.str());
}

void Logger::enqueue(const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        log_queue_.push(message);
    }
    cv_.notify_one();
}

void Logger::logging_thread() {
    while (running_ || !log_queue_.empty()) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&] { return !log_queue_.empty() || !running_; });

        while (!log_queue_.empty()) {
            const std::string message = log_queue_.front();
            log_queue_.pop();
            lock.unlock();

            // Output to appropriate streams
            if (message.find("[ERROR]") != std::string::npos || message.find("[WARN]") != std::string::npos)
                std::cerr << message;
            else
                std::cout << message;

            if (use_file_ && file_.is_open()) {
                file_ << message;
                file_.flush(); // Optional, or batch flush for better performance
            }
            lock.lock();
        }
    }
}

void Logger::shutdown() {
    if (running_.exchange(false)) {
        cv_.notify_one();
        if (worker_.joinable())
            worker_.join();
        if (file_.is_open())
            file_.close();
    }
}

std::string Logger::timestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    auto t_c = system_clock::to_time_t(now);
    std::tm tm{};
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
