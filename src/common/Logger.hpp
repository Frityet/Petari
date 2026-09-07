#pragma once

#include <cstdio>
#include <memory>
#include <source_location>
#include <string_view>
#include <utility>
#include <source_location>

#include <fmt/format.h>

namespace smgpc::logging {

enum class Level {
    DEBUG, INFO, WARNING, ERROR, FATAL
};

enum class Category {
    APP, RENDERER
};

struct Message {
    std::string_view format;
    std::source_location location;

    constexpr inline Message(std::string_view message, std::source_location source = std::source_location::current()):
        format(message),
        location(source) {}
};

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void write(std::FILE *to, std::source_location location, Level level, Category category, std::string_view message) = 0;

    template <typename... Args>
    inline void log(std::FILE *to, Level level, Category category, Message message, Args &&...args)
    {
        write(to, message.location, level, category, fmt::format(fmt::runtime(message.format), std::forward<Args>(args)...));
    }

    template <typename... Args>
    inline void debug(Category category, Message message, Args &&...args)
    {
        log(stdout, Level::DEBUG, category, message, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void info(Category category, Message message, Args &&...args)
    {
        log(stdout, Level::INFO, category, message, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void warning(Category category, Message message, Args &&...args)
    {
        log(stderr, Level::WARNING, category, message, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void error(Category category, Message message, Args &&...args)
    {
        log(stderr, Level::ERROR, category, message, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void fatal(Category category, Message message, Args &&...args)
    {
        log(stderr, Level::FATAL, category, message, std::forward<Args>(args)...);
    }
};

class ConsoleLogger final : public ILogger {
public:
    void write(std::FILE *to, std::source_location location, Level level, Category category, std::string_view message) override;
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
        }

        return fmt::format_to(ctx.out(), "UNKNOWN");
    }
};

}  // namespace fmt
