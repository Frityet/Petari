#pragma once

#include "Game/Scene/SceneObjHolder.hpp"
#include <dolphin/gx.h>
#include <array>

namespace smgpc::test {
// Match SceneFunction::initForLiveActor ordering. Capture the original white
// GX commands so a data-only fixture needs no graphics device submission.
inline NameObj* create_area_container(SceneObjHolder& holder) {
    if (!holder.isExist(SceneObj_LightDirector)) {
        alignas(32) std::array<u8, 4096> commands{};
        GXBeginDisplayList(commands.data(), commands.size());
        try {
            holder.create(SceneObj_LightDirector);
        } catch (...) {
            GXEndDisplayList();
            throw;
        }
        GXEndDisplayList();
    }
    return holder.create(SceneObj_AreaObjContainer);
}
}
