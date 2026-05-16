#pragma once

#include <cstdio>
#include <memory>
#include <string_view>
#include <utility>

#include <fmt/format.h>

namespace smgpc::logging {

enum class Level {
    DEBUG, INFO, WARNING, ERROR, FATAL
};

enum class Category {
    APP, RENDERER, GAME, ASSET
};

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void write(std::FILE *to, std::string_view file, int line, Level level, Category category, std::string_view message) = 0;

    template <typename... Args>
    void log(std::FILE *to, std::string_view file, int line, Level level, Category category, std::string_view format, Args &&...args) {
        write(to, file, line, level, category, fmt::format(fmt::runtime(format), std::forward<Args>(args)...));
    }

    template <typename... Args>
    void debug(std::string_view file, int line, Category category, std::string_view format, Args &&...args) {
        log(stdout, file, line, Level::DEBUG, category, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::string_view file, int line, Category category, std::string_view format, Args &&...args) {
        log(stdout, file, line, Level::INFO, category, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warning(std::string_view file, int line, Category category, std::string_view format, Args &&...args) {
        log(stderr, file, line, Level::WARNING, category, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::string_view file, int line, Category category, std::string_view format, Args &&...args) {
        log(stderr, file, line, Level::ERROR, category, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal(std::string_view file, int line, Category category, std::string_view format, Args &&...args) {
        log(stderr, file, line, Level::FATAL, category, format, std::forward<Args>(args)...);
    }
};

class ConsoleLogger final : public ILogger {
public:
    void write(std::FILE *to, std::string_view file, int line, Level level, Category category, std::string_view message) override;
};

[[nodiscard]] std::unique_ptr<ILogger> create_default_logger();

}  // namespace smgpc::logging

namespace fmt {

template <>
struct formatter<smgpc::logging::Level> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(smgpc::logging::Level value, FormatContext &ctx) const {
        switch (value) {
        case smgpc::logging::Level::DEBUG:
            return fmt::format_to(ctx.out(), "DEBUG");
        case smgpc::logging::Level::INFO:
            return fmt::format_to(ctx.out(), "INFO");
        case smgpc::logging::Level::WARNING:
            return fmt::format_to(ctx.out(), "WARNING");
        case smgpc::logging::Level::ERROR:
            return fmt::format_to(ctx.out(), "ERROR");
        case smgpc::logging::Level::FATAL:
            return fmt::format_to(ctx.out(), "FATAL");
        }

        return fmt::format_to(ctx.out(), "UNKNOWN");
    }
};

template <>
struct formatter<smgpc::logging::Category> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(smgpc::logging::Category value, FormatContext &ctx) const {
        switch (value) {
        case smgpc::logging::Category::APP:
            return fmt::format_to(ctx.out(), "APP");
        case smgpc::logging::Category::RENDERER:
            return fmt::format_to(ctx.out(), "RENDERER");
        case smgpc::logging::Category::GAME:
            return fmt::format_to(ctx.out(), "GAME");
        case smgpc::logging::Category::ASSET:
            return fmt::format_to(ctx.out(), "ASSET");
        }

        return fmt::format_to(ctx.out(), "UNKNOWN");
    }
};

}  // namespace fmt
