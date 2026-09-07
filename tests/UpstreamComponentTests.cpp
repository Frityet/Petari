#include "Game/AudioLib/AudBgmVolumeController.hpp"
#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/Map/CollisionCode.hpp"
#include "Game/System/WPadRumbleData.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/JGeometry/TBox.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    bool near(float actual, float expected) {
        return std::abs(actual - expected) < 0.0001F;
    }
}

int main() {
    AudBgmVolumeController volume;
    require(near(volume.getVolume(), 1.0F), "BGM starts at full volume");
    volume.moveAuxVolume(0.4F, 4);
    volume.update();
    require(near(volume.getVolume(), 0.85F), "BGM fade advances one frame at a time");
    for (int i = 0; i < 3; ++i) volume.update();
    require(near(volume.getVolume(), 0.4F), "BGM fade reaches its target");
    volume.moveNoteFairyVolume(0.5F, 1);
    volume.update();
    require(near(volume.getVolume(), 0.2F), "Independent BGM faders multiply");
    volume.mIsMuted = true;
    require(near(volume.getVolume(), 0.0F), "Mute overrides active faders");
    volume.mIsMuted = false;
    require(near(volume.getVolume(), 0.2F), "Unmuting preserves fader state");

    AudBgmVolumeController interruption;
    for (int i = 0; i < 8; ++i) {
        interruption.interruptedByOther();
        interruption.update();
    }
    require(near(interruption.getVolume(), 0.0F), "Repeated interruption holds BGM silent");
    for (int i = 0; i < 61; ++i) interruption.update();
    require(near(interruption.getVolume(), 1.0F), "BGM recovers when interruption ends");

    AnimScaleController scale(nullptr);
    scale.startAndAddScaleVelocityY(0.15F);
    scale.update();
    require(scale._C.y > 1.0F && scale._C.x < 1.0F, "Scale impulse stretches Y and contracts X");
    require(near(scale._C.x * scale._C.y * scale._C.z, 1.0F), "Scale animation preserves volume");
    for (int i = 0; i < 300; ++i) scale.update();
    require(near(scale._C.y, 1.0F), "Scale impulse settles back to rest");
    scale.startDpdHitVibration();
    for (int i = 0; i < 20; ++i) scale.update();
    require(std::isfinite(scale._C.y) && scale._C.y > 0.0F, "DPD vibration has a valid scale");
    scale.startCrush();
    for (int i = 0; i < 25; ++i) scale.update();
    require(std::isfinite(scale._C.x) && scale._C.y > 0.0F, "Crush animation has a valid scale");
    scale.stopAndReset();
    require(near(scale._C.x, 1.0F) && near(scale._C.y, 1.0F) && near(scale._C.z, 1.0F), "Reset restores unit scale");

    TBox3f box;
    box.zero();
    box.extend(TVec3f(-2.0F, -4.0F, -6.0F), TVec3f(4.0F, 6.0F, 8.0F));
    box.pad(1.0F);
    TVec3f center;
    box.getCenter(&center);
    require(near(center.x, 1.0F) && near(center.y, 1.0F) && near(center.z, 1.0F), "Padding preserves box center");
    require(box.intersectsPoint(TVec3f(-2.5F, 0.0F, 0.0F)), "Extended box includes padding");
    const auto orthogonal = TVec3f(0.0F, 1.0F, 0.0F).getOrthogonal(TVec3f(2.0F, 3.0F, 4.0F));
    require(near(orthogonal.x, 2.0F) && near(orthogonal.y, 0.0F) && near(orthogonal.z, 4.0F), "Orthogonal projection removes the normal component");
    require(near(TVec2f(1.0F, 2.0F).distance(TVec2f(4.0F, 6.0F)), 5.0F), "Vector distance uses both axes");
    J3DFrameCtrl frame;
    frame.mState = 0b1010;
    require(frame.andState(0b1100) == 0b1000, "Frame state preserves the mask value");

    CollisionCode collision;
    require(std::string_view(collision.mFloorTable->getString(CollisionFloorCode_NoSlip)) == "NoSlip",
            "Collision tables retain named floor attributes");
    require(std::string_view(collision.mSoundTable->getString(CollisionSoundCode_Lawn)) == "Lawn",
            "Collision tables retain named sound attributes");
    const JMapInfoIter absent;
    require(collision.getFloorCode(absent) == CollisionFloorCode_Normal &&
                collision.getCameraID(absent) == static_cast<u32>(-1),
            "Missing collision attributes use retail defaults");
    for (auto* table : {collision.mFloorTable, collision.mWallTable, collision.mSoundTable, collision.mCameraTable}) {
        delete[] table->mHashTable;
        delete[] table->mCodeTable;
        delete[] table->mNameTable;
        delete table;
    }

    RumbleData::initHashValue();
    require(RumbleData::getPattern("missing rumble pattern") == nullptr, "Unknown rumble names remain absent");
    const auto* rumble = RumbleData::getPattern("最強");
    require(rumble != nullptr && rumble->mFrame == 30, "Rumble names resolve to upstream frame patterns");
    std::cout << "Upstream component tests passed\n";
}
