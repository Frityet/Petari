#pragma once

#include <revolution.h>

class JUTTexture;

namespace MR {
    void createScreenAlphaSceneObj(s32 index, f32 scale);
    void captureScreenAlpha(s32 index);
    void loadScreenAlphaTexture(s32 index, GXTexMapID tex_map_id);
    JUTTexture* getScreenAlphaTexture(s32 index);
}  // namespace MR
