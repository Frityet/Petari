#pragma once

#include <string_view>

namespace pcport {

enum class LogLevel {
    Error = 0,
    Warn,
    Info,
    Debug,
};

enum class LogCategory {
    App,
    Assets,
    Layout,
    Menu,
    Stub,
    Test,
};

void SetLogLevel(LogLevel level);
LogLevel GetLogLevel();

void Log(LogLevel level, LogCategory category, std::string_view message);

}  // namespace pcport
