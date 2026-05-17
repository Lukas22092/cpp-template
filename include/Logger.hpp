#pragma once
#include "enum_tools.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
class Logger
{
  private:
    std::queue<std::string> queue_{};
    std::mutex m_{};
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    std::thread thread_;

    Logger();
    void log();

  public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    inline static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }

    void push(std::string msg);
    ~Logger();
};

struct LogStream
{
  private:
    LogLevel log_level;
    const char* function_name_;
    std::__thread_id thread_id;
    std::stringstream stream_;

  public:
    std::stringstream& getStream();
    LogStream(LogLevel log_level, std::__thread_id thread_id, const char* function_name);
    ~LogStream();

    // MUST stay in the header file because it's a template
    template <typename T> LogStream& operator<<(const T& value)
    {
        stream_ << value;
        return *this;
    }
};

struct LogEntryExit
{
  private:
    const char* function_name_;
    std::chrono::time_point<std::chrono::high_resolution_clock> t0;

  public:
    LogEntryExit(const char* function_name);
    ~LogEntryExit();
};

#define LOG(LogLevel) LogStream(LogLevel, std::this_thread::get_id(), __PRETTY_FUNCTION__)

#define LOG_ENTRY_EXIT                                                                             \
    do                                                                                             \
    {                                                                                              \
        LogStream(LogLevel::Trace, std::this_thread::get_id(), __PRETTY_FUNCTION__) << "ENTRY";    \
    } while (0);                                                                                   \
    LogEntryExit instance_##__LINE__(__PRETTY_FUNCTION__);
