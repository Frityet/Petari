#pragma once

#include <revolution/gx.h>

namespace JUtility {
    struct TColor : public GXColor {
    public:
        TColor(u8 r, u8 g, u8 b, u8 a) {
            set(r, g, b, a);
        }
        TColor() {
            set(0xffffffff);
        }
        TColor(u32 u32Color) {
            set(u32Color);
        }
        TColor(GXColor color) {
            set(color);
        }

        TColor& operator=(const TColor& color);

        void set(u8 cR, u8 cG, u8 cB, u8 cA) {
            r = cR;
            g = cG;
            b = cB;
            a = cA;
        }

        void set(u32 u32Color) {
            set(static_cast<u8>(u32Color >> 24), static_cast<u8>(u32Color >> 16),
                static_cast<u8>(u32Color >> 8), static_cast<u8>(u32Color));
        }

        operator u32() const {
            return toUInt32();
        }
        u32 toUInt32() const {
            return (static_cast<u32>(r) << 24) | (static_cast<u32>(g) << 16) | (static_cast<u32>(b) << 8) | a;
        }

        void set(GXColor gxColor) {
            GXColor* temp = this;
            *temp = gxColor;
        }
    };
};  // namespace JUtility
