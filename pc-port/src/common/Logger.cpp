#include "Logger.hpp"

#include <memory>

namespace smgpc::logging {

void ConsoleLogger::write(std::FILE *to, std::source_location location, Level level, Category category, std::string_view message) {
    fmt::println(to, "[{}:{}][{} - {}] {}", location.file_name(), location.line(), category, level, message);
}

std::unique_ptr<ILogger> create_default_logger() {
    return std::make_unique<ConsoleLogger>();
}

}  // namespace smgpc::logging
