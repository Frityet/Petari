#include "compat/MetrowerksPrintf.hpp"

extern "C" int test_original_printf_alias(char *output, std::size_t size) {
    const char *absent = nullptr;
    return std::snprintf(output, size, "%s/%s/%03d", absent, "directory", 7);
}
extern "C" int test_original_sprintf_alias(char *output) {
    return ::sprintf(output, "%s:%s", static_cast<const char *>(nullptr), "tail");
}

extern "C" int test_original_vsnprintf_alias(char *output, std::size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int result = std::vsnprintf(output, size, format, args);
    va_end(args);
    return result;
}
extern "C" int test_original_vsprintf_alias(char *output, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int result = ::vsprintf(output, format, args);
    va_end(args);
    return result;
}
