#pragma once

class SaveDataHandleSequence;

namespace smgpc::game {
[[noreturn]] SaveDataHandleSequence& save_data_handle_sequence();
}

#include <revolution/types.h>
#include <math_types.hpp>

#include <cmath>
#include <functional>

#include "compat/GameGravityCompat.hpp"

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

inline f32 __fabsf(f32 value) {
    return std::fabs(value);
}

inline s32 __abs(s32 value) {
    return value < 0 ? -value : value;
}
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (static_cast<s32>(sizeof(array) / sizeof((array)[0])))
#endif

// Retail decompilation translation units can contain unrelated scratch
// emitters with the same placeholder name. Metrowerks linked those units in
// separate REL/DOL contexts; make only placeholder function definitions
// translation-unit local without changing the recovered Game sources.
#define DUMMY(...) static DUMMY(__VA_ARGS__)

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
