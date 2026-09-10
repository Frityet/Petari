#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
using u32 = uint32_t;
namespace MR {
    void extractString(char* pDst, const char* pSrc, u32 num, u32) {
        strncpy(pDst, pSrc, num);

        pDst[num] = '\0';
    }
}
namespace {
const char* cAttributeEffectTag = "Attr";
    bool makeAttibuteEffectBaseName(char* pDst, u32 size, const char* pSrc) {
        const char* tag = strstr(pSrc, cAttributeEffectTag);

        if (tag == nullptr) {
            return false;
        }

        u32 tagLength = strlen(tag);
        MR::extractString(pDst, pSrc, strlen(pSrc) - tagLength, size);

        return true;
    }
}
int main() {
    char output[256];
    std::memset(output, '#', sizeof(output));
    assert(makeAttibuteEffectBaseName(output, sizeof(output), "RunSmokeAttrWater"));
    assert(std::strcmp(output, "RunSmoke") == 0 && output[9] == '#');
    std::strcpy(output, "untouched");
    assert(!makeAttibuteEffectBaseName(output, sizeof(output), "RunSmoke"));
    assert(std::strcmp(output, "untouched") == 0);
    assert(makeAttibuteEffectBaseName(output, sizeof(output), "AttrWater"));
    assert(output[0] == '\0');
    assert(makeAttibuteEffectBaseName(output, sizeof(output), "JumpAttr"));
    assert(std::strcmp(output, "Jump") == 0);
    assert(makeAttibuteEffectBaseName(output, sizeof(output), "AAttrBAttrC"));
    assert(std::strcmp(output, "A") == 0);
    // The actual destination is ample; retail intentionally ignores this argument.
    assert(makeAttibuteEffectBaseName(output, 0, "RunSmokeAttrWater"));
    assert(std::strcmp(output, "RunSmoke") == 0);
    std::cout << "Exact extracted effect helper: 6 cases passed\n";
}
