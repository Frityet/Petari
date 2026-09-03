#pragma once

// Original Player/JSystem code names the Revolution SDK path; Aurora exposes
// the same display-list writer through Dolphin's host header.
#include <dolphin/gd/GDBase.h>

static inline void GDTexCoord2f32(f32 x, f32 y) {
    GDWrite_f32(x);
    GDWrite_f32(y);
}
