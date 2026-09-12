#include "nw4r/db/assert.h"
#include "compat/JkrAllocationDomain.hpp"
#include <aurora/exception.hpp>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace nw4r::db {
    void Panic(const char* file, int line, const char* format, ...) {
        smgpc::compat::JkrHostAllocationScope host;
        char message[1024]{};
        va_list args;
        va_start(args, format);
        std::vsnprintf(message, sizeof(message), format, args);
        va_end(args);
        aurora::throw_host_exception<std::runtime_error>(std::string(file) + ":" + std::to_string(line) + ": " + message);
    }
}
