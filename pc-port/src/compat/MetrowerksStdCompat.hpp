#pragma once

#include <functional>

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
