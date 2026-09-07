#pragma once

// This must precede every stdio declaration: Darwin's system declarations
// already carry asm labels, and the first declaration determines that label.
#include <cstdarg>
#include <cstddef>

#if defined(__clang__) || defined(__GNUC__)
#define SMGPC_MSL_STRINGIFY_INNER(value) #value
#define SMGPC_MSL_STRINGIFY(value) SMGPC_MSL_STRINGIFY_INNER(value)
#define SMGPC_MSL_LABEL(name) SMGPC_MSL_STRINGIFY(__USER_LABEL_PREFIX__) #name
#if defined(__linux__)
#define SMGPC_MSL_NOTHROW noexcept
#else
#define SMGPC_MSL_NOTHROW
#endif
extern "C" int sprintf(char *, const char *, ...) SMGPC_MSL_NOTHROW __asm__(SMGPC_MSL_LABEL(smgpc_msl_sprintf));
extern "C" int snprintf(char *, std::size_t, const char *, ...) SMGPC_MSL_NOTHROW __asm__(SMGPC_MSL_LABEL(smgpc_msl_snprintf));
extern "C" int vsprintf(char *, const char *, va_list)
SMGPC_MSL_NOTHROW __asm__(SMGPC_MSL_LABEL(smgpc_msl_vsprintf));
extern "C" int vsnprintf(char *, std::size_t, const char *, va_list)
SMGPC_MSL_NOTHROW __asm__(SMGPC_MSL_LABEL(smgpc_msl_vsnprintf));
#undef SMGPC_MSL_NOTHROW
#undef SMGPC_MSL_LABEL
#undef SMGPC_MSL_STRINGIFY
#undef SMGPC_MSL_STRINGIFY_INNER
#else
#error The original MSL formatted-string boundary requires compiler symbol-label support.
#endif

#include <cstdio>
