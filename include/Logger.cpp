#include "Logger.hpp"
#include "enum_tools.hpp"
#include <fstream>
#include <iostream>
#include <thread>


Logger::Logger()
    : thread_([this]() { log(); })
{
}

void Logger::log()
{
    std::ofstream log_file("Logs/logs.log", std::ios::app);

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
                log_file << msg << "\n";
            }
            lk.lock();
        }
    }
}

void Logger::push(std::string msg) 
{
    {
        std::lock_guard<std::mutex> lk(m_);
        queue_.push(std::move(msg));
    }
    cv_.notify_one();
}

Logger::~Logger()
{
    stop_ = true;
    cv_.notify_all();
    if(thread_.joinable()) thread_.join();
}


// --- LogStream Implementations ---

std::stringstream& LogStream::getStream()
{
    return stream_;
}

LogStream::LogStream(LogLevel log_level, std::__thread_id thread_id, const char* function_name)
    : 
    log_level(log_level),
    function_name_(function_name),
    thread_id(thread_id)
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

    stream_  << "[" << EnumToString(log_level) << "] " << thread_id << " " <<  std::put_time(&local_time, "%H:%M:%S") << " " << clean_func << " | "; 
}

LogStream::~LogStream()
{
    Logger::getInstance().push(stream_.str());
}


// --- LogEntryExit Implementations ---

LogEntryExit::LogEntryExit(const char* function_name)
: function_name_(function_name)
{
    t0 = std::chrono::high_resolution_clock::now();
}

LogEntryExit::~LogEntryExit()
{
    const auto t1 = std::chrono::high_resolution_clock::now();
    std::string s = std::to_string(std::chrono::duration<double>(t1 - t0).count()) + "s";
    LogStream(LogLevel::Trace, std::this_thread::get_id(), function_name_) << "EXIT - " << s;
}