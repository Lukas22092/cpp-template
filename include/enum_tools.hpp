#pragma once
#include <string>

template <typename E> inline auto EnumToString(E enum_) noexcept -> const char*
{
    return "UNKNOWN ENUM";
};

enum LogLevel
{
    Trace,
    Debug,
    Error,
    Warn,
    Info
};

template <> inline auto EnumToString(LogLevel enum_) noexcept -> const char*
{
    const char* final_enum = [&]()
    {
        switch (enum_)
        {
        case Trace:
            return "Trace";
        case Debug:
            return "Debug";
        case Error:
            return "Error";
        case Warn:
            return "Warn!";
        case Info:
            return "Info!";
        default:
            return "UNKNOWN ENUM TO STRING CALL";
        }
    }();
    return final_enum;
}