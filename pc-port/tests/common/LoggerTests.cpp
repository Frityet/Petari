#include "common/Logger.hpp"
#include "tests/TestHarness.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct LogEntry {
    std::FILE *to {};
    std::string file {};
    int line {};
    smgpc::logging::Level level {};
    smgpc::logging::Category category {};
    std::string message {};
};

class RecordingLogger final : public smgpc::logging::ILogger {
public:
    void write(std::FILE *to, std::string_view file, int line, smgpc::logging::Level level, smgpc::logging::Category category, std::string_view message) override {
        entries.push_back(LogEntry {
            .to = to, .file = std::string(file), .line = line, .level = level, .category = category, .message = std::string(message)
        });
    }

    std::vector<LogEntry> entries {};
};

[[nodiscard]] std::string read_all_from_file(std::FILE *file) {
    std::fflush(file);
    std::rewind(file);

    std::string output {};
    char buffer[512];
    while (std::fgets(buffer, static_cast<int>(sizeof(buffer)), file) != nullptr) {
        output += buffer;
    }

    return output;
}

}  // namespace

$test("ConsoleLogger::write emits expected log format") {
    auto *tmp = std::tmpfile();
    $pc_port_require(tmp != nullptr);

    smgpc::logging::ConsoleLogger logger {};
    logger.write(tmp, "foo/bar.cpp", 17, smgpc::logging::Level::INFO, smgpc::logging::Category::APP, "hello world");

    const auto output = read_all_from_file(tmp);
    std::fclose(tmp);

    $pc_port_require(output.find("[foo/bar.cpp:17][APP - INFO] hello world") != std::string::npos);
}

$test("ILogger helper methods route to expected levels and streams") {
    RecordingLogger logger {};

    logger.debug("debug.cpp", 3, smgpc::logging::Category::RENDERER, "debug {}", 1);
    logger.info("info.cpp", 5, smgpc::logging::Category::APP, "info {}", "x");
    logger.warning("warn.cpp", 7, smgpc::logging::Category::GAME, "warn {}", 2);
    logger.error("error.cpp", 9, smgpc::logging::Category::APP, "error {}", 3);
    logger.fatal("fatal.cpp", 11, smgpc::logging::Category::RENDERER, "fatal {}", 4);

    $pc_port_require_eq(logger.entries.size(), static_cast<std::size_t>(5));

    $pc_port_require(logger.entries[0].to == stdout);
    $pc_port_require_eq(logger.entries[0].file, std::string("debug.cpp"));
    $pc_port_require_eq(logger.entries[0].line, 3);
    $pc_port_require(logger.entries[0].level == smgpc::logging::Level::DEBUG);
    $pc_port_require_eq(logger.entries[0].message, std::string("debug 1"));

    $pc_port_require(logger.entries[1].to == stdout);
    $pc_port_require(logger.entries[1].level == smgpc::logging::Level::INFO);
    $pc_port_require(logger.entries[1].category == smgpc::logging::Category::APP);

    $pc_port_require(logger.entries[2].to == stderr);
    $pc_port_require(logger.entries[2].level == smgpc::logging::Level::WARNING);
    $pc_port_require_eq(logger.entries[2].message, std::string("warn 2"));

    $pc_port_require(logger.entries[3].to == stderr);
    $pc_port_require(logger.entries[3].level == smgpc::logging::Level::ERROR);
    $pc_port_require_eq(logger.entries[3].message, std::string("error 3"));

    $pc_port_require(logger.entries[4].to == stderr);
    $pc_port_require(logger.entries[4].level == smgpc::logging::Level::FATAL);
    $pc_port_require_eq(logger.entries[4].message, std::string("fatal 4"));
}

$test("logging::create_default_logger returns ConsoleLogger") {
    auto logger = smgpc::logging::create_default_logger();

    $pc_port_require(logger != nullptr);
    $pc_port_require(dynamic_cast<smgpc::logging::ConsoleLogger *>(logger.get()) != nullptr);
}

$test("Logging formatters render readable enum names") {
    const auto level_text = fmt::format("{}", smgpc::logging::Level::WARNING);
    const auto category_text = fmt::format("{}", smgpc::logging::Category::GAME);

    $pc_port_require_eq(level_text, std::string("WARNING"));
    $pc_port_require_eq(category_text, std::string("GAME"));
}
