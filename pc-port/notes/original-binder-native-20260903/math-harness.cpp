#include "Game/Util/MathUtil.hpp"
f32 PSVECKillElement(const Vec*, const Vec*, const Vec*);
extern "C" float native_kill(const Vec* from, const Vec* direction, Vec* result) {
    return PSVECKillElement(from, direction, result);
}
