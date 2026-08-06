#pragma once

#include <revolution/types.h>

#include <functional>

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
