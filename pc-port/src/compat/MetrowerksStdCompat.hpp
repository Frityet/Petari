#pragma once

#include <revolution/types.h>

#include <cfloat>
#include <cmath>
#include <functional>

inline constexpr f32 FLOAT_MAX = FLT_MAX;

#ifndef NO_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define NO_INLINE __attribute__((noinline))
#else
#define NO_INLINE
#endif
#endif

#if !defined(__MWERKS__)
inline f32 __fabs(f32 value) {
    return std::fabs(value);
}
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (static_cast<s32>(sizeof(array) / sizeof((array)[0])))
#endif

// The original Metrowerks Standard Library exposes std::mem_func, while
// modern host standard libraries expose the equivalent std::mem_fn.
#if !defined(__MWERKS__)
namespace std {
    template <typename MemberPointer>
    constexpr auto mem_func(MemberPointer member) {
        return std::mem_fn(member);
    }
}  // namespace std
#endif
