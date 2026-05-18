#pragma once

#include <revolution.h>

namespace MR {
    void hidePlayer();
    void setPlayerBaseMtx(MtxPtr matrix);
    MtxPtr getPlayerBaseMtx();
    void startBckPlayer(const char* pName, const char* pFileName);
    f32 getBckFrameMaxPlayer(const char* pName);
    bool isBckStoppedPlayer();
    void initPlayerAfterOpeningDemo();
}  // namespace MR
