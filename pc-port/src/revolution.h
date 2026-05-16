#pragma once

#include "revolution/types.h"

#ifndef NO_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define NO_INLINE __attribute__((noinline))
#else
#define NO_INLINE
#endif
#endif

constexpr s32 WPAD_CHAN0 = 0;

[[nodiscard]] OSTime OSGetTime();
[[nodiscard]] s64 OSTicksToSeconds(OSTime ticks);

