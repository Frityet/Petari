#include "Game/AudioLib/AudBgm.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "Game/Util/SoundUtil.hpp"
#include <JSystem/JAudio2/JAISound.hpp>

namespace MR {
    void moveVolumeSubBGM(f32 volume, u32 frames) {
        AudBgm* bgm = AudWrap::getSubBgm();
        if (bgm != nullptr) {
            bgm->moveVolume(volume, frames);
        }
    }

    bool isPlayingSubBgmID(u32 id) {
        AudBgm* bgm = AudWrap::getSubBgm();
        if (bgm != nullptr && !bgm->isStopping() && bgm->getSoundID() == id) {
            return true;
        }
        return false;
    }
}
