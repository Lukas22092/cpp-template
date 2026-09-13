/**
 * @file Logger.cpp
 * @brief Implementation of the Logger and RAII helper classes.
 */

#include "Logger.hpp"

/** @brief Constructs the logger and starts its background worker thread. */
Logger::Logger()
    : thread_([this]() { log(); })
{
}

/** @brief Background worker loop that drains the queue and writes each message to "logs.log". */
void Logger::log()
{
    std::ofstream log_file("logs.log", std::ios::app);

    while (true)
    {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this] { return !queue_.empty() || stop_; });

        if (stop_ && queue_.empty()) {
            break;
        }

        while (!queue_.empty())
        {
            std::string msg = std::move(queue_.front());
            queue_.pop();
            
            lk.unlock(); 
            if (log_file.is_open()) {
                log_file << msg << '\n';
            }
            lk.lock();
        }
        if (log_file.is_open()) 
                log_file.flush(); 
                
    }
}

/** @brief Adds a message to the logging queue and wakes the worker thread. */
void Logger::push(std::string&& msg) 
{
    {
        std::lock_guard<std::mutex> lk(m_);
        queue_.push(std::move(msg));
    }
    cv_.notify_one();
}

/** @brief Signals the worker thread to stop, wakes it, and joins it before destruction completes. */
Logger::~Logger()
{
    stop_ = true;
    cv_.notify_all();
    if(thread_.joinable()) thread_.join();
}

/** @brief Access the internal stream buffer. */
std::stringstream& LogStream::getStream()
{
    return stream_;
}

/** @brief Captures the severity, thread id, and originating function for this log entry. */
LogStream::LogStream(LogLevel log_level, std::thread::id thread_id, const char* function_name)
    : 
    log_level_(log_level),
    function_name_(function_name),
    thread_id_(thread_id)
{
auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto local_time = *std::localtime(&time_t_now);

    std::string full_func(function_name);
    std::string clean_func = function_name;

    size_t paren = full_func.find('(');
    if (paren != std::string::npos) {
        clean_func = full_func.substr(0, paren);
    }

    size_t space = clean_func.rfind(' ');
    if (space != std::string::npos) {
        clean_func = clean_func.substr(space + 1);
    }

    size_t lambda_idx = clean_func.find("::(anonymous class)");
    if (lambda_idx != std::string::npos) {
        clean_func = clean_func.substr(0, lambda_idx) + " [Lambda]";
    }
    size_t colon_pos = clean_func.find("::");
        while (colon_pos != std::string::npos) {
            clean_func.replace(colon_pos, 2, ".");
            colon_pos = clean_func.find("::", colon_pos + 1);
        }

    char time_buffer[9]; // "HH:MM:SS\0" is 9 bytes
    std::strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &local_time);

    stream_ << "[" << EnumToString(log_level) << "] " 
            << thread_id << " " 
            << time_buffer << " " 

            << clean_func << " | ";
}
bool Logger::isLevelEnabled(LogLevel level) const {
            std::lock_guard<std::mutex> lk(m_);
            return logged_levels_.empty() || logged_levels_.contains(level);
        }

/** @brief Forwards the accumulated stream text to Logger::push(). */
LogStream::~LogStream()
{
    if (Logger::getInstance().is_logging_enabled_ && Logger::getInstance().isLevelEnabled(log_level_)) 
    {
        Logger::getInstance().push(stream_.str());
    }
}

/** @brief Records the current time so the destructor can compute how long the traced function ran. */
LogEntryExit::LogEntryExit(const char* function_name, bool should_log)
    : function_name_(function_name), should_log_(should_log)
{
    if (should_log_) {
        t0 = std::chrono::high_resolution_clock::now();
        
        LogStream(LogLevel::Trace, std::this_thread::get_id(), function_name_) << "ENTRY";
    }
}

/** @brief Computes the elapsed time since construction and emits a Trace-level "EXIT - <seconds>s" log entry. */
LogEntryExit::~LogEntryExit()
{
    if (should_log_) {
        const auto t1 = std::chrono::high_resolution_clock::now();
        std::string s = std::to_string(std::chrono::duration<double>(t1 - t0).count()) + "s";
        LogStream(LogLevel::Trace, std::this_thread::get_id(), function_name_) << "EXIT - " << s;
    }
}
