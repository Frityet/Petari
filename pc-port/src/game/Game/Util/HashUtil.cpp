#include "Game/Util/HashUtil.hpp"

#include <cctype>

namespace MR {

u32 getHashCode(const char *pStr) {
    u32 hash = 0;
    if (pStr == nullptr) {
        return hash;
    }

    for (; *pStr != '\0'; ++pStr) {
        hash = static_cast<u8>(*pStr) + hash * 31U;
    }
    return hash;
}

u32 getHashCodeLower(const char *pStr) {
    u32 hash = 0;
    if (pStr == nullptr) {
        return hash;
    }

    for (; *pStr != '\0'; ++pStr) {
        hash = static_cast<u32>(std::tolower(static_cast<unsigned char>(*pStr))) + hash * 31U;
    }
    return hash;
}

}  // namespace MR
