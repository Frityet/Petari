#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/LiveActor/ActorAnimKeeper.hpp"
#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/System/ResourceHolder.hpp"

namespace {
    void changeBckForEffectKeeper(const LiveActor* pActor) NO_INLINE {
        EffectKeeper* keeper = pActor->mEffectKeeper;
        if (keeper != nullptr) {
            keeper->changeBck();
        }
    }
}

namespace MR {

    void startAction(const LiveActor* pActor, const char* pName) {
        if (pActor->mAnimKeeper != nullptr && pActor->mAnimKeeper->start(pName)) {
            return;
        }

        tryStartAllAnim(pActor, pName);
    }

    bool isActionEnd(const LiveActor* pActor) {
        return pActor->mModelManager->isBckStopped();
    }

    bool isActionStart(const LiveActor* pActor, const char* pName) {
        if (pActor->mAnimKeeper != nullptr) {
            return pActor->mAnimKeeper->isPlaying(pName);
        }

        return isBckPlaying(pActor->mModelManager->mXanimePlayer, pName);
    }

    bool tryStartAction(const LiveActor* pActor, const char* pName) {
        if (!isActionStart(pActor, pName)) {
            startAction(pActor, pName);
            return true;
        }

        return false;
    }

    void startAllAnim(const LiveActor* pActor, const char* pName) {
        tryStartAllAnim(pActor, pName);
    }

    bool tryStartAllAnim(const LiveActor* pActor, const char* pName) {
        bool started = false;
        if (startBckIfExist(pActor, pName)) {
            started = true;
        }

        if (startBtkIfExist(pActor, pName)) {
            started = true;
        }

        if (startBrkIfExist(pActor, pName)) {
            started = true;
        }

        if (startBpkIfExist(pActor, pName)) {
            started = true;
        }

        if (startBtpIfExist(pActor, pName)) {
            started = true;
        }

        if (startBvaIfExist(pActor, pName)) {
            started = true;
        }

        return started;
    }

    void startBck(const LiveActor* pActor, const char* pBckName, const char* pBrkName) {
        pActor->mModelManager->startBck(pBckName, pBrkName);
        ::changeBckForEffectKeeper(pActor);
        startBas(pActor, pBckName, false, 0.0f, 0.0f);
    }

    void startBckWithInterpole(const LiveActor* pActor, const char* pBckName, s32 interpole) {
        pActor->mModelManager->startBckWithInterpole(pBckName, interpole);
        startBas(pActor, pBckName, false, 0.0f, 0.0f);
        ::changeBckForEffectKeeper(pActor);
    }

    void startBckNoInterpole(const LiveActor* pActor, const char* pBckName) {
        pActor->mModelManager->startBckWithInterpole(pBckName, 0);
        startBas(pActor, pBckName, false, 0.0f, 0.0f);
        ::changeBckForEffectKeeper(pActor);
    }

    void startBckAtFirstStep(const LiveActor* pActor, const char* pBckName) {
        if (!isStep(pActor, 0)) {
            return;
        }

        pActor->mModelManager->startBck(pBckName, nullptr);
        ::changeBckForEffectKeeper(pActor);
        startBas(pActor, pBckName, false, 0.0f, 0.0f);
    }

    bool tryStartBck(const LiveActor* pActor, const char* pBckName, const char* pBrkName) {
        if (!isBckPlaying(pActor->mModelManager->mXanimePlayer, pBckName)) {
            pActor->mModelManager->startBck(pBckName, pBrkName);
            ::changeBckForEffectKeeper(pActor);
            startBas(pActor, pBckName, false, 0.0f, 0.0f);
            return true;
        }

        return false;
    }

    bool tryStartBckAndBtp(const LiveActor* pActor, const char* pBckName, const char* pBtpName) {
        if (tryStartBck(pActor, pBckName, pBtpName)) {
            if (isExistBtp(pActor, pBckName)) {
                pActor->mModelManager->startBtp(pBckName);
            }

            return true;
        }

        return false;
    }

    void setAllAnimFrame(const LiveActor* pActor, const char* pName, f32 frame) {
        if (isExistBck(pActor, pName)) {
            getBckCtrl(pActor)->setFrame(frame);
        }

        if (isExistBtk(pActor, pName)) {
            getBtkCtrl(pActor)->setFrame(frame);
        }

        if (isExistBpk(pActor, pName)) {
            getBpkCtrl(pActor)->setFrame(frame);
        }

        if (isExistBtp(pActor, pName)) {
            getBtpCtrl(pActor)->setFrame(frame);
        }

        if (isExistBrk(pActor, pName)) {
            getBrkCtrl(pActor)->setFrame(frame);
        }

        if (isExistBva(pActor, pName)) {
            getBvaCtrl(pActor)->setFrame(frame);
        }
    }

    void setAllAnimFrameAndStop(const LiveActor* pActor, const char* pName, f32 frame) {
        if (isExistBck(pActor, pName)) {
            setBckFrameAndStop(pActor, frame);
        }

        if (isExistBtk(pActor, pName)) {
            setBtkFrameAndStop(pActor, frame);
        }

        if (isExistBpk(pActor, pName)) {
            setBpkFrameAndStop(pActor, frame);
        }

        if (isExistBtp(pActor, pName)) {
            setBtpFrameAndStop(pActor, frame);
        }

        if (isExistBrk(pActor, pName)) {
            setBrkFrameAndStop(pActor, frame);
        }

        if (isExistBva(pActor, pName)) {
            setBvaFrameAndStop(pActor, frame);
        }
    }

    void setAllAnimFrameAtEnd(const LiveActor* pActor, const char* pName) {
        if (isExistBck(pActor, pName)) {
            setBckFrame(pActor, getBckCtrl(pActor)->getEnd());
        }

        if (isExistBtk(pActor, pName)) {
            setBtkFrame(pActor, getBtkCtrl(pActor)->getEnd());
        }

        if (isExistBpk(pActor, pName)) {
            setBpkFrame(pActor, getBpkCtrl(pActor)->getEnd());
        }

        if (isExistBtp(pActor, pName)) {
            setBtpFrame(pActor, getBtpCtrl(pActor)->getEnd());
        }

        if (isExistBrk(pActor, pName)) {
            setBrkFrame(pActor, getBrkCtrl(pActor)->getEnd());
        }

        if (isExistBva(pActor, pName)) {
            setBrkFrame(pActor, getBvaCtrl(pActor)->getEnd());
        }
    }

    bool isAnyAnimStopped(const LiveActor* pActor, const char* pName) {
        if (isExistBck(pActor, pName) && isBckStopped(pActor)) {
            return true;
        }

        if (isExistBtk(pActor, pName) && isBtkStopped(pActor)) {
            return true;
        }

        if (isExistBpk(pActor, pName) && isBpkStopped(pActor)) {
            return true;
        }

        if (isExistBtp(pActor, pName) && isBtpStopped(pActor)) {
            return true;
        }

        if (isExistBrk(pActor, pName) && isBrkStopped(pActor)) {
            return true;
        }

        if (isExistBva(pActor, pName) && isBvaStopped(pActor)) {
            return true;
        }

        return false;
    }

    bool isAnyAnimOneTimeAndStopped(const LiveActor* pActor, const char* pName) {
        if (isExistBck(pActor, pName) && isBckStopped(pActor)) {
            return true;
        }

        if (isExistBtk(pActor, pName) && isBtkStopped(pActor)) {
            return true;
        }

        if (isExistBpk(pActor, pName) && isBpkStopped(pActor)) {
            return true;
        }

        if (isExistBtp(pActor, pName) && isBtpStopped(pActor)) {
            return true;
        }

        if (isExistBrk(pActor, pName) && isBrkStopped(pActor)) {
            return true;
        }

        if (isExistBva(pActor, pName) && isBvaStopped(pActor)) {
            return true;
        }

        return false;
    }

    bool startBckIfExist(const LiveActor* pActor, const char* pBckName) {
        if (getResourceHolder(pActor)->mMotionResTable->isExistRes(pBckName)) {
            pActor->mModelManager->startBck(pBckName, nullptr);
            ::changeBckForEffectKeeper(pActor);
            startBas(pActor, pBckName, false, 0.0f, 0.0f);
            return true;
        }

        return false;
    }

}  // namespace MR
