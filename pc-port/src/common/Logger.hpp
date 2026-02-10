#include <fmt/format.h>

namespace smgpc::logging
{
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    enum class Category {
        APP,
        RENDERER,
        GAME
    };
}

namespace fmt
{
    template<>
    struct fmt::formatter<smgpc::logging::Level> {
        constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

        template <typename FormatContext>
        auto format(smgpc::logging::Level p, FormatContext &ctx) const {
            switch (p) {
            case smgpc::logging::Level::DEBUG:  return fmt::format_to(ctx.out(), "DEBUG");
            case smgpc::logging::Level::INFO:   return fmt::format_to(ctx.out(), "INFO");
            case smgpc::logging::Level::WARNING:return fmt::format_to(ctx.out(), "WARNING");
            case smgpc::logging::Level::ERROR:  return fmt::format_to(ctx.out(), "ERROR");
            case smgpc::logging::Level::FATAL:  return fmt::format_to(ctx.out(), "FATAL");
            }
        }
    };

    template<>
    struct fmt::formatter<smgpc::logging::Category> {
        constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

        template <typename FormatContext>
        auto format(smgpc::logging::Category p, FormatContext &ctx) const {
            switch (p) {
            case smgpc::logging::Category::APP:     return fmt::format_to(ctx.out(), "APP");
            case smgpc::logging::Category::RENDERER:return fmt::format_to(ctx.out(), "RENDERER");
            case smgpc::logging::Category::GAME:    return fmt::format_to(ctx.out(), "GAME");
            }
        }
    };
}

namespace smgpc::logging
{

    constexpr inline void vlog(std::FILE *to, std::string_view file, int line, Level lvl, Category cat, std::string_view format, fmt::format_args &&args)
    {
        fmt::println(to, "[{}:{}][{}][{}] {}", file, line, cat, lvl, fmt::vformat(format, args));
    }

    constexpr inline void log(std::FILE *to, std::string_view file, int line, Level lvl, Category cat, std::string_view format, auto ...args)
    {
        return vlog(to, file, line, lvl, cat, format, fmt::make_format_args(args...));
    }

#   define debug(...) log(stdout, __FILE__, __LINE__, smgpc::logging::Level::DEBUG, __VA_ARGS__)
#   define info(...) log(stdout, __FILE__, __LINE__, smgpc::logging::Level::INFO, __VA_ARGS__)
#   define warning(...) log(stderr, __FILE__, __LINE__, smgpc::logging::Level::WARNING, __VA_ARGS__)
#   define error(...) log(stderr, __FILE__, __LINE__, smgpc::logging::Level::ERROR, __VA_ARGS__)
#   define fatal(...) log(stderr, __FILE__, __LINE__, smgpc::logging::Level::FATAL, __VA_ARGS__)
// #   define log(...) log(stdout, __FILE__, __LINE__, __VA_ARGS__)

}
