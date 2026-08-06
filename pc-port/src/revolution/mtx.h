#pragma once

// Preserve the Revolution SDK include path while using Aurora's compatible
// Dolphin matrix and vector implementation.
#include <dolphin/mtx.h>

// RVL SDK matrix aliases used by original JSystem headers but absent from the
// narrower Aurora matrix surface.
typedef f32 Mtx23[2][3];
typedef f32 (*Mtx23P)[3];
typedef f32 Mtx33[3][3];
typedef f32 (*Mtx3P)[3];
