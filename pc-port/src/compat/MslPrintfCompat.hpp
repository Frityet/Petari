#pragma once
#include <cstdarg>
#include <cstddef>

// Only bounded/unbounded string-output formatting is redirected for original
// Game callers. Host numeric conversions and host FILE/stdout logging remain
// native; MSL's narrow/wide string rules are provided by this boundary.
extern "C" int smgpc_msl_snprintf(char *, std::size_t, const char *, ...);
extern "C" int smgpc_msl_vsnprintf(char *, std::size_t, const char *, va_list);
extern "C" int smgpc_msl_sprintf(char *, const char *, ...);
extern "C" int smgpc_msl_vsprintf(char *, const char *, va_list);
