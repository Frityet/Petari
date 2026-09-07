#pragma once

#include <revolution/gx/GXStruct.h>

namespace nw4r {
    namespace ut {
        struct Color : public GXColor {
        public:
            static const int ALPHA_MAX = 255;

            static const u32 WHITE = 0xFFFFFFFF;

            Color() { *this = 0xFFFFFFFF; }

            Color(u32 color) { *this = color; }

            Color(const GXColor& color) { *this = color; }

            Color& operator=(u32 color) {
                r = static_cast<u8>(color >> 24);
                g = static_cast<u8>(color >> 16);
                b = static_cast<u8>(color >> 8);
                a = static_cast<u8>(color);
                return *this;
            }

            Color& operator=(const GXColor& color) { r = color.r; g = color.g; b = color.b; a = color.a; return *this; }

            ~Color() {}

            operator u32() const { return (u32(r) << 24) | (u32(g) << 16) | (u32(b) << 8) | a; }

        };
    };  // namespace ut
};  // namespace nw4r
