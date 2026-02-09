#include "core/Logger.hpp"

#include <chrono>
#include <cstdio>
#include <mutex>

namespace pcport {
namespace {

std::mutex gLogMutex;
LogLevel gLogLevel = LogLevel::Info;

const char* ToString(LogLevel level) {
    switch (level) {
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Debug:
        return "DEBUG";
    }
    return "UNKNOWN";
}

const char* ToString(LogCategory category) {
    switch (category) {
    case LogCategory::App:
        return "app";
    case LogCategory::Assets:
        return "assets";
    case LogCategory::Layout:
        return "layout";
    case LogCategory::Menu:
        return "menu";
    case LogCategory::Stub:
        return "stub";
    case LogCategory::Test:
        return "test";
    }
    return "unknown";
}

}  // namespace

void SetLogLevel(LogLevel level) {
    gLogLevel = level;
}

LogLevel GetLogLevel() {
    return gLogLevel;
}

void Log(LogLevel level, LogCategory category, std::string_view message) {
    if (static_cast<int>(level) > static_cast<int>(gLogLevel)) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto epoch = now.time_since_epoch();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    std::lock_guard<std::mutex> lock(gLogMutex);
    std::fprintf(stderr, "[%lld][%s][%s] %.*s\n", static_cast<long long>(ms), ToString(level), ToString(category),
                 static_cast<int>(message.size()), message.data());
}

}  // namespace pcport
