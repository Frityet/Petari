#pragma once

#include <array>
#include <memory>

#include <revolution/types.h>

class JUTTexture;

namespace smgpc::runtime {
    // Owns the native screen-alpha provider's actual textures for one runtime.
    // The original Game ScreenAlphaCapture class is a separate, inactive owner.
    class ScreenAlphaCaptureService final {
    public:
        ScreenAlphaCaptureService();
        ~ScreenAlphaCaptureService();
        ScreenAlphaCaptureService(const ScreenAlphaCaptureService&) = delete;
        ScreenAlphaCaptureService& operator=(const ScreenAlphaCaptureService&) = delete;
        static ScreenAlphaCaptureService& active();
        void ensure_texture(s32 index, f32 scale);
        JUTTexture* texture(s32 index) const;
    private:
        std::array<std::unique_ptr<JUTTexture>, 4> _textures;
    };
}
