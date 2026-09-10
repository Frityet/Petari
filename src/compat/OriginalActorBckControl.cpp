#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Animation/BckCtrl.hpp"
#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelManager.hpp"

namespace MR {
    void reflectBckCtrlData(LiveActor* pActor, const BckCtrlData& rBck) {
        BckCtrlFunction::reflectBckCtrlData(rBck, pActor->mModelManager->mXanimePlayer);

        AudAnmSoundObject* pSoundObj = pActor->mSoundObject;
        if (pSoundObj != nullptr && pSoundObj->hasAnim()) {
            if (rBck.mRepeatFrame < 0) {
                pSoundObj->setLoopFrame((f32)rBck.mStartFrame, (f32)rBck.mEndFrame);
            } else {
                pSoundObj->setLoopFrame((f32)rBck.mRepeatFrame, (f32)rBck.mEndFrame);
            }

            pSoundObj->setStartPos((f32)rBck.mStartFrame);
        }
    }

}
