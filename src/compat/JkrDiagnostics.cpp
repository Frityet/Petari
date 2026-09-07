#include "compat/JkrDiagnostics.hpp"

#include <dolphin/os.h>

#include <cstdarg>
#include <cstdio>

namespace smgpc::compat {
    void jkr_warning(const char* message) {
        OSReport("%s", message);
    }

    void jkr_warning_f(const char* format, ...) {
        va_list args;
        va_start(args, format);
        OSVReport(format, args);
        va_end(args);
    }

    void jkr_report(const char* message) {
        OSReport("%s", message);
    }

    void jkr_report_f(const char* format, ...) {
        va_list args;
        va_start(args, format);
        OSVReport(format, args);
        va_end(args);
    }

    [[noreturn]] void jkr_panic(const char* file, int line, const char* format, ...) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        std::vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        OSPanic(file, line, "%s", buffer);
    }
}
