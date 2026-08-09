#pragma once

class SaveDataHandleSequence;
class JKRHeap;

namespace smgpc::game {
    [[noreturn]] SaveDataHandleSequence &save_data_handle_sequence();
}

#include <math_types.hpp>
#include <revolution/types.h>

#include <cmath>
#include <cstddef>
#include <functional.hpp>

// Metrowerks exposes integer-alignment placement new for JKR allocations.
// The native provider keeps the same source contract while using host-owned
// aligned storage; ordinary delete/delete[] remain valid for the result.
void* operator new(std::size_t size, int alignment);
void* operator new[](std::size_t size, int alignment);
void* operator new(std::size_t size, JKRHeap* heap, int alignment);
void* operator new[](std::size_t size, JKRHeap* heap, int alignment);
void operator delete(void* memory, int alignment) noexcept;
void operator delete[](void* memory, int alignment) noexcept;
void operator delete(void* memory, JKRHeap* heap, int alignment) noexcept;
void operator delete[](void* memory, JKRHeap* heap, int alignment) noexcept;

#include "compat/GameGravityCompat.hpp"
#include "compat/CollisionPartsCompat.hpp"

// Metrowerks targets ILP32, so recovered `long` integer literals bind to the
// retail s32 overload. Keep that call shape unambiguous on LP64 hosts without
// changing the recovered Game translation units.
namespace MR {
    s32 getRandom(long min, long max);
}

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
