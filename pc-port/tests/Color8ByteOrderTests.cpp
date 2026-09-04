#include "Game/Util/Color.hpp"
#include <cassert>
#include <iostream>

static_assert(sizeof(Color8) == sizeof(GXColor));

int main() {
    unsigned cases = 0;
    for (u32 channel = 0; channel < 4; ++channel) {
        for (u32 value = 0; value < 256; ++value) {
            const auto shift = 8 * channel;
            const u32 packed = (0x12345678U & ~(0xffU << shift)) | (value << shift);
            const Color8 color(packed);
            const auto gpu = static_cast<GXColor>(color);
            assert(gpu.r == ((packed >> 24) & 0xff) && gpu.g == ((packed >> 16) & 0xff));
            assert(gpu.b == ((packed >> 8) & 0xff) && gpu.a == (packed & 0xff));
            assert(static_cast<u32>(color) == packed);
            const Color8 channels(gpu.r, gpu.g, gpu.b, gpu.a);
            assert(static_cast<u32>(channels) == packed);
            const Color8 from_gpu(gpu);
            assert(static_cast<u32>(from_gpu) == packed);
            Color8 copy;
            copy = color;
            assert(static_cast<u32>(copy) == packed);
            copy.set(gpu);
            assert(static_cast<u32>(copy) == packed);
            ++cases;
        }
    }
    assert(static_cast<u32>(Color8()) == 0xffffffffU);
    std::cout << "color8_numeric_rgba_cases=" << cases << " gx_component_order=pass copy_and_set=pass default_white=pass\n";
}
