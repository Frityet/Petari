#pragma once

#include <JSystem/JGeometry/TVec.hpp>
#include <revolution.h>

namespace MR {
    void showPlayer();
    void hidePlayer();
    TVec3f* getPlayerPos();
    TVec3f* getPlayerCenterPos();
    void setPlayerPos(const TVec3f& position);
    TVec3f* getPlayerRotate();
    TVec3f* getPlayerVelocity();
    TVec3f* getPlayerGravity();
    f32 calcDistanceToPlayer(const TVec3f& position);
    void getPlayerUpVec(TVec3f* pOut);
    void getPlayerFrontVec(TVec3f* pOut);
    void getPlayerSideVec(TVec3f* pOut);
    void setPlayerBaseMtx(MtxPtr matrix);
    MtxPtr getPlayerBaseMtx();
    void startBckPlayer(const char* pName, const char* pFileName);
    f32 getBckFrameMaxPlayer(const char* pName);
    bool isBckStoppedPlayer();
    void initPlayerAfterOpeningDemo();
    void incPlayerOxygen(u32 amount);
}  // namespace MR
