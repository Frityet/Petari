#include "Logger.hpp"

#include <memory>

namespace smgpc::logging {

void ConsoleLogger::write(std::FILE *to, std::string_view file, int line, Level level, Category category, std::string_view message) {
    fmt::println(to, "[{}:{}][{} - {}] {}", file, line, category, level, message);
}

std::shared_ptr<ILogger> create_default_logger() {
    return std::make_shared<ConsoleLogger>();
}

}  // namespace smgpc::logging
