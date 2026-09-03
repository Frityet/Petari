#include "Game/Player/MarioModule.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"

void MarioModule::changeAnimationNonStop(const char* pAnim) {
    if (!mActor->_B90) {
        if (pAnim) {
            changeAnimation(pAnim, (const char*)nullptr);
        }

        XanimeFrameCtrl* pFrameCtrl = mActor->mMarioAnim->mXanimePlayer->_20;
        if (pFrameCtrl->getAttribute() == 0) {
            pFrameCtrl->setAttribute(1);
        }
    }
}

void MarioModule::changeAnimationWithAttr(const char* pAnim, u32 attribute) {
    if (!mActor->_B90) {
        if (pAnim) {
            changeAnimation(pAnim, (const char*)nullptr);
        }

        mActor->mMarioAnim->mXanimePlayer->_20->setAttribute(attribute);
    }
}
