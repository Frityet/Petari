#pragma once

// Keep host-only Revolution SDK gaps in the PC compatibility surface rather
// than changing Aurora or imported Game/JSystem sources.
#include_next <revolution.h>

#include <revolution/mtx.h>

#include <cstddef>
#include <cstring>

#if !defined(__MWERKS__)
inline void* __memcpy(void* destination, const void* source, std::size_t size) {
    return std::memcpy(destination, source, size);
}
#endif
