/**
 * @file Logger.hpp
 * @brief Thread-safe asynchronous logging system.
 * * Provides a singleton Logger instance, streaming log interfaces,
 * and RAII-based function entry/exit tracking.
 */

#pragma once

#if defined(_MSC_VER) && !defined(__PRETTY_FUNCTION__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#include <atomic>              
#include <chrono>              
#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>              
#include <thread>
#include <ctime>
#include <type_traits>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

enum class LogLevel
{
    Trace,
    Debug,
    Error,
    Warn,
    Info
};

inline auto EnumToString(LogLevel enum_) noexcept -> const char*
{
    const char* final_enum = [&]()
    {
        switch(enum_)
        {
            case LogLevel::Trace : return "Trace";
            case LogLevel::Debug : return "Debug";
            case LogLevel::Error : return "Error";
            case LogLevel::Warn:  return "Warn";
            case LogLevel::Info:  return "Info";
            default:    return "UNKNOWN ENUM TO STRING CALL";
        }
    }();
    return final_enum;
}


/**
 * @brief Singleton that owns a background thread which asynchronously drains
 * queued log messages.
 * @note Every message is appended to "logs.log" (path resolved relative to the
 * process's current working directory at the time the worker thread opens it).
 * ENTRY/EXIT trace markers produced by LOG_ENTRY_EXIT are written to that file
 * but filtered out of stdout to keep the console readable.
 */
class Logger
{
private:
    std::queue<std::string> queue_{}; ///< Queue storing pending log messages.
    mutable std::mutex m_{};                  ///< Mutex for thread-safe access to the queue.
    std::condition_variable cv_;      ///< Signal for the worker thread to process logs.
    std::atomic<bool> stop_{false};   ///< Atomic flag to signal thread shutdown.
    std::thread thread_;              ///< Background worker thread.
    std::unordered_set<LogLevel> logged_levels_{}; ///< sets the currently logged levels.

    /**
     * @brief Constructs the logger and starts its background worker thread.
     * @note Private; obtain the instance via getInstance() instead of constructing directly.
     */
    Logger();

    /**
     * @brief Background worker loop that drains the queue and writes each message to "logs.log".
     * @details Blocks on cv_ until the queue is non-empty or stop_ is set. Every popped
     * message is appended to "logs.log" (opened once, in append mode, for the lifetime
     * of the loop). Messages are also echoed to stdout, except those that are exactly
     * "ENTRY" or start with "EXIT - ", which are suppressed on stdout (they still go
     * to the log file) to keep the console free of per-call trace noise.
     * Runs until stop_ is set and the queue has been fully drained.
     */
    void log();

public:   
    std::atomic<bool> is_logging_enabled_{true}; ///< flag indicating wether the Logger is enabled
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Access the singleton instance.
     * @return Logger& Reference to the global logger.
     */
    inline static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }

    /**
     * @brief Configures which logging severity levels are enabled.
     * @details Clears any previously tracked logging levels and populates the filter 
     * with the provided arguments. If the resulting set is empty, all logging levels 
     * are treated as enabled by default.
     * * @tparam Levels Variadic template parameter pack enforcing that all passed arguments 
     * are exactly of type `LogLevel`.
     * @param to_log A parameter pack of `LogLevel` enums to explicitly enable.
     * * @note Thread-safe: Acquires a lock on the internal mutex `m_` before modifying the 
     * active log level filter set.
     */
    template<typename... Levels>
    requires (std::is_same_v<Levels, LogLevel> && ...)    
    void setLoggingLevel(Levels... to_log)
    {
        std::lock_guard<std::mutex> lk(m_);
        logged_levels_.clear();
        (logged_levels_.insert(to_log), ...);
    }

/**
     * @brief Checks whether a specific log level is currently permitted to log.
     * @param level The `LogLevel` severity to check.
     * @return true If the given level is explicitly enabled, or if no filters have been 
     * set (meaning all levels are enabled).
     * @return false If filters are active and the specified level is not among them.
     * * @note Thread-safe: Acquires a lock on the internal mutex `m_` to safely inspect the 
     * underlying filter set.
     */
    bool isLevelEnabled(LogLevel level) const ;


    /**
     * @brief Adds a message to the logging queue.
     * @param msg The formatted log string to record.
     * @note Thread-safe: locks internally and wakes the worker thread via the condition variable.
     */
    void push(std::string&& msg);
    
    /**
     * @brief Signals the worker thread to stop, wakes it, and joins it before destruction completes.
     */
    ~Logger();
};

/**
 * @brief RAII helper that accumulates a log message via operator<< and forwards
 * the accumulated text to the Logger when it goes out of scope.
 * @note The severity, thread id, and function name passed to the constructor are
 * stored on the instance but are not currently read anywhere afterwards; only the
 * streamed text ends up in the message handed to Logger::push().
 */
struct LogStream
{
private:
    LogLevel log_level_{};            ///< Severity passed to the constructor; not read again afterwards.
    const char* function_name_{};     ///< Function name passed to the constructor; not read again afterwards.
    std::thread::id thread_id_{};     ///< Calling thread id passed to the constructor; not read again afterwards.
    std::stringstream stream_{};      ///< Accumulates the message text appended via operator<<.


public:
    /**
     * @brief Access the internal stream buffer.
     * @return std::stringstream& Reference to the stream.
     */
    std::stringstream& getStream();
    
    /**
     * @brief Captures the severity, thread id, and originating function for this log entry.
     * @param log_level Severity tag for this message (stored only; see class @note above).
     * @param thread_id ID of the calling thread.
     * @param function_name Name of the function that issued the log call (typically CURRENT_FUNCTION).
     */
    LogStream(LogLevel log_level, std::thread::id thread_id, const char* function_name);

    /**
     * @brief Forwards the accumulated stream text to Logger::push() so it is queued for the worker thread.
     */
    ~LogStream();

    /**
     * @brief Operator to append data to the log stream.
     * @tparam T Type of the data to append.
     * @param value The value to log.
     * @return LogStream& Reference to this instance for chaining.
     */
    template <typename T>
    LogStream& operator<<(const T& value)
    {
        stream_ << value;
        return *this;
    }
};

/**
 * @brief RAII helper that records a start timestamp on construction and emits a
 * Trace-level "EXIT - <seconds>s" log message (via a temporary LogStream) on destruction.
 * @note Used by LOG_ENTRY_EXIT to measure the wall-clock duration of the enclosing scope.
 */
struct LogEntryExit
{
private:
    const char* function_name_ = nullptr;   ///< Function name reused to tag the exit log message.
    std::chrono::time_point<std::chrono::high_resolution_clock> t0; ///< Timestamp captured at construction, used to compute the elapsed duration in the destructor.
    bool should_log_{true};

public:
    /**
     * @brief Records the current time so the destructor can compute how long the traced function ran.
     * @param function_name Name of the function being traced (typically CURRENT_FUNCTION), reused for the exit log message.
     */
    explicit LogEntryExit(const char* function_name, bool should_log = true);
    /**
     * @brief Computes the elapsed time since construction and emits a Trace-level "EXIT - <seconds>s" log entry.
     */
    ~LogEntryExit();
};

/**
 * @brief Expands to the compiler's function-signature macro: __FUNCSIG__ on
 * MSVC, __PRETTY_FUNCTION__ elsewhere.
 */
#if defined(_WIN32) || defined(_WIN64)
    #define CURRENT_FUNCTION __FUNCSIG__
#else
    #define CURRENT_FUNCTION __PRETTY_FUNCTION__
#endif

/**
 * @brief Macro to initiate a log message, e.g. `LOG(LogLevel::Info) << "message";`.
 * @param LogLevel The severity level of the log.
 * @note The severity value is currently stored on the resulting LogStream but does
 * not appear in the emitted text (see LogStream's class-level note).
 */
#define LOG(LogLevel) LogStream(LogLevel, std::this_thread::get_id(), CURRENT_FUNCTION)

/**
 * @brief Configures the active logging filters by specifying which severity levels are enabled.
 * @details This variadic macro accepts an arbitrary number of `LogLevel` arguments. It clears any
 * previously configured filters and registers the new set globally. Any subsequent log messages 
 * whose level is not included in this set will be silently dropped.
 * * @param ... A comma-separated list of `LogLevel` enums to enable (e.g., `LogLevel::Info, LogLevel::Error`).
 * * @note This provides a clean interface to the underlying variadic template function on the Logger singleton.
 * @see Logger::setLoggingLevel
 */
#define SET_LOG_LEVELS(...) Logger::getInstance().setLoggingLevel(__VA_ARGS__)

/**
 * @brief Macro to trace function entry and duration.
 * @details Immediately logs a Trace-level "ENTRY" message, then declares a local
 * LogEntryExit object whose destructor logs the elapsed time when the enclosing
 * scope ends.
 */
#define LOG_ENTRY_EXIT \
    do { \
        LogStream(LogLevel::Trace, std::this_thread::get_id(), CURRENT_FUNCTION) << "ENTRY"; \
    } while(0); \
    LogEntryExit instance_##__LINE__(CURRENT_FUNCTION);

/**
 * @brief Throttled macro to trace function entry and duration with rate limiting.
 * @details Operates similarly to LOG_ENTRY_EXIT, but uses a static timer to suppress
 * logs if they occur more frequently than the specified interval.
 * @param ms_interval The minimum time (in milliseconds) that must pass between
 * consecutive log entries for this specific function scope.
 */
#define LOG_ENTRY_EXIT_THROTTLED(ms_interval) \
    static std::chrono::steady_clock::time_point s_last_log_time{}; \
    auto _logger_now = std::chrono::steady_clock::now(); \
    bool _logger_should_log = std::chrono::duration_cast<std::chrono::milliseconds>(_logger_now - s_last_log_time).count() >= (ms_interval); \
    if (_logger_should_log) s_last_log_time = _logger_now; \
    LogEntryExit _scoped_log(__func__, _logger_should_log)
/**
 * @brief Asserts that action is not equal to expected; logs an Error-level message
 * if it is (i.e. the assertion is violated).
 * @param action Expression to check (evaluated once).
 * @param expected Value that action is expected to differ from.
 * @param message Text streamed via LOG(LogLevel::Error) when the check fails.
 */
#define EXPECT_NEQ(action, expected, message)                \
    do {                                                     \
        if ((action) == (expected)) {                        \
            LOG(LogLevel::Error) << message;                 \
        }                                                    \
    } while (0)
    
/**
 * @brief Asserts that action is equal to expected; logs an Error-level message
 * if it is not (i.e. the assertion is violated).
 * @param action Expression to check (evaluated once).
 * @param expected Value that action is expected to equal.
 * @param message Text streamed via LOG(LogLevel::Error) when the check fails.
 */
#define EXPECT_EQ(action, expected, message)                 \
    do {                                                     \
        if ((action) != (expected)) {                        \
            LOG(LogLevel::Error) << message;                 \
        }                                                    \
    } while (0)
    
