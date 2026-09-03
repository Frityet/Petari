#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Animation/XanimeCore.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/DisplayListMaker.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"

namespace {
    f32 sAnimRateScale = 1.0f;
}

namespace MR {

    void calcAnimDirect(LiveActor* pActor) {
        bool isNoCalcAnim = pActor->mFlag.mIsNoCalcAnim;

        pActor->mFlag.mIsNoCalcAnim = false;
        pActor->calcAnim();

        if (isNoCalcAnim) {
            pActor->mFlag.mIsNoCalcAnim = true;
        }
    }

    void setBaseTRMtx(LiveActor* pActor, MtxPtr pMtx) {
        PSMTXCopy(pMtx, (MtxPtr)&getJ3DModel(pActor)->mBaseTransformMtx);
    }

    void setBaseTRMtx(LiveActor* pActor, const TPos3f& rMtx) {
        PSMTXCopy((MtxPtr)rMtx.mMtx, (MtxPtr)&getJ3DModel(pActor)->mBaseTransformMtx);
    }

    void setBaseTRMtx(LiveActor* pActor, const TQuat4f& rQuat) {
        TPos3f mtx;
        f32 two = 2.0f;
        f32 y = rQuat.y;
        f32 x = rQuat.x;
        f32 z = rQuat.z;
        f32 w = rQuat.w;

        f32 ty = two * y;
        f32 tx = two * x;
        f32 tz = two * z;
        f32 tw = two * w;
        f32 one = 1.0f;
        f32 yy = ty * y;
        f32 xx = tx * x;
        f32 xy = tx * y;
        f32 zz = tz * z;
        f32 wz = tw * z;
        f32 m00 = (one - yy) - zz;
        f32 m11 = (one - xx) - zz;
        f32 m01 = xy - wz;
        f32 m10 = xy + wz;
        f32 m22 = (one - xx) - yy;

        f32 xz = tx * z;
        f32 wy = tw * y;
        f32 yz = ty * z;
        f32 wx = tw * x;

        f32 m02 = xz + wy;
        f32 m20 = xz - wy;
        f32 m12 = yz - wx;
        f32 m21 = yz + wx;

        mtx.mMtx[0][0] = m00;
        mtx.mMtx[0][1] = m01;
        mtx.mMtx[1][1] = m11;
        mtx.mMtx[1][0] = m10;
        mtx.mMtx[2][2] = m22;
        mtx.mMtx[0][2] = m02;
        mtx.mMtx[1][2] = m12;
        mtx.mMtx[2][0] = m20;
        mtx.mMtx[2][1] = m21;

        mtx.mMtx[0][3] = pActor->mPosition.x;
        mtx.mMtx[1][3] = pActor->mPosition.y;
        mtx.mMtx[2][3] = pActor->mPosition.z;

        PSMTXCopy((MtxPtr)mtx.mMtx, (MtxPtr)&getJ3DModel(pActor)->mBaseTransformMtx);
    }

    void setBaseScale(LiveActor* pActor, const TVec3f& rScale) {
        J3DModel* pModel = getJ3DModel(pActor);
        pModel->mBaseScale.x = rScale.x;
        pModel->mBaseScale.y = rScale.y;
        pModel->mBaseScale.z = rScale.z;
    }

    ResourceHolder* getResourceHolder(const LiveActor* pActor) {
        if (pActor->mModelManager != nullptr) {
            return pActor->mModelManager->getResourceHolder();
        }

        return nullptr;
    }

    ResourceHolder* getModelResourceHolder(const LiveActor* pActor) {
        if (pActor->mModelManager != nullptr) {
            return pActor->mModelManager->getModelResourceHolder();
        }

        return nullptr;
    }

    ResTIMG* getTexFromModel(const char* pName, const LiveActor* pActor) {
        s32 texIndex = getJ3DModelData(pActor)->mMaterialTable.getTextureName()->getIndex(pName);
        return getJ3DModelData(pActor)->mMaterialTable.getTexture()->getResTIMG(texIndex);
    }

    ResTIMG* getTexFromArc(const char* pName, const LiveActor* pActor) {
        return (ResTIMG*)getResourceHolder(pActor)->mFileInfoTable->getRes(pName);
    }

    const char* getModelResName(const LiveActor* pActor) {
        return getModelResourceHolder(pActor)->mModelResTable->getResName(static_cast< u32 >(0));
    }

    bool isExistAnim(const LiveActor* pActor, const char* pName) {
        if (isExistBck(pActor, pName)) {
            return true;
        }

        if (isExistBtk(pActor, pName)) {
            return true;
        }

        if (isExistBrk(pActor, pName)) {
            return true;
        }

        if (isExistBtp(pActor, pName)) {
            return true;
        }

        if (isExistBpk(pActor, pName)) {
            return true;
        }

        return isExistBva(pActor, pName);
    }

    bool isExistBck(const LiveActor* pActor, const char* pName) {
        if (pName == nullptr) {
            return getResourceHolder(pActor)->mMotionResTable->mCount != 0;
        }

        return getResourceHolder(pActor)->mMotionResTable->isExistRes(pName);
    }

    bool isExistBtk(const LiveActor* pActor, const char* pName) {
        if (pName == nullptr) {
            return getResourceHolder(pActor)->mBtkResTable->mCount != 0;
        }

        return getResourceHolder(pActor)->mBtkResTable->isExistRes(pName);
    }

    bool isExistBrk(const LiveActor* pActor, const char* pName) {
        if (pName == nullptr) {
            return getResourceHolder(pActor)->mBrkResTable->mCount != 0;
        }

        return getResourceHolder(pActor)->mBrkResTable->isExistRes(pName);
    }

    bool isExistBtp(const LiveActor* pActor, const char* pName) {
        if (pName == nullptr) {
            return getResourceHolder(pActor)->mBtpResTable->mCount != 0;
        }

        return getResourceHolder(pActor)->mBtpResTable->isExistRes(pName);
    }

    bool isExistBpk(const LiveActor* pActor, const char* pName) {
        if (pName == nullptr) {
            return getResourceHolder(pActor)->mBpkResTable->mCount != 0;
        }

        return getResourceHolder(pActor)->mBpkResTable->isExistRes(pName);
    }

    bool isExistBva(const LiveActor* pActor, const char* pName) {
        if (pName == nullptr) {
            return getResourceHolder(pActor)->mBvaResTable->mCount != 0;
        }

        return getResourceHolder(pActor)->mBvaResTable->isExistRes(pName);
    }

    bool isExistTexture(const LiveActor* pActor, const char* pName) {
        return getJ3DModelData(pActor)->mMaterialTable.getTextureName()->getIndex(pName) != -1;
    }

    void newDifferedDLBuffer(LiveActor* pActor) {
        pActor->mModelManager->newDifferedDLBuffer();
    }

    void initDLMakerFog(LiveActor* pActor, bool enable) {
        pActor->mModelManager->mDisplayListMaker->addFogCtrl(enable);
    }

    bool isBckStopped(const LiveActor* pActor) {
        return pActor->mModelManager->isBckStopped();
    }

    bool isBtkStopped(const LiveActor* pActor) {
        return pActor->mModelManager->isBtkStopped();
    }

    bool isBrkStopped(const LiveActor* pActor) {
        return pActor->mModelManager->isBrkStopped();
    }

    bool isBtpStopped(const LiveActor* pActor) {
        return pActor->mModelManager->isBtpStopped();
    }

    bool isBpkStopped(const LiveActor* pActor) {
        return pActor->mModelManager->isBpkStopped();
    }

    bool isBvaStopped(const LiveActor* pActor) {
        return pActor->mModelManager->isBvaStopped();
    }

    bool isBckOneTimeAndStopped(const LiveActor* pActor) {
        return pActor->mModelManager->isBckStopped();
    }

    bool isBrkOneTimeAndStopped(const LiveActor* pActor) {
        return pActor->mModelManager->isBrkStopped();
    }

    bool isBckLooped(const LiveActor* pActor) {
        return (getBckCtrl(pActor)->mState & 0x2) != 0;
    }

    bool checkPassBckFrame(const LiveActor* pActor, f32 f) {
        return getBckCtrl(pActor)->checkPass(f) == 1;
    }

    void setBckFrameAndStop(const LiveActor* pActor, f32 frame) {
        getBckCtrl(pActor)->setFrame(frame);
        getBckCtrl(pActor)->setRate(0.0f);
    }

    void setBtkFrameAndStop(const LiveActor* pActor, f32 frame) {
        getBtkCtrl(pActor)->setFrame(frame);
        getBtkCtrl(pActor)->setRate(0.0f);
    }

    void setBrkFrameAndStop(const LiveActor* pActor, f32 frame) {
        getBrkCtrl(pActor)->setFrame(frame);
        getBrkCtrl(pActor)->setRate(0.0f);
    }

    void setBtpFrameAndStop(const LiveActor* pActor, f32 frame) {
        getBtpCtrl(pActor)->setFrame(frame);
        getBtpCtrl(pActor)->setRate(0.0f);
    }

    void setBpkFrameAndStop(const LiveActor* pActor, f32 frame) {
        getBpkCtrl(pActor)->setFrame(frame);
        getBpkCtrl(pActor)->setRate(0.0f);
    }

    void setBvaFrameAndStop(const LiveActor* pActor, f32 frame) {
        getBvaCtrl(pActor)->setFrame(frame);
        getBvaCtrl(pActor)->setRate(0.0f);
    }

    void setBrkFrameEndAndStop(const LiveActor* pActor) {
        getBrkCtrl(pActor)->setFrame(getBrkCtrl(pActor)->getEnd());
        getBrkCtrl(pActor)->setRate(0.0f);
    }

    void startBtkAndSetFrameAndStop(const LiveActor* pActor, const char* pBtkName, f32 frame) {
        startBtk(pActor, pBtkName);
        setBtkFrameAndStop(pActor, frame);
    }

    void startBrkAndSetFrameAndStop(const LiveActor* pActor, const char* pBrkName, f32 frame) {
        startBrk(pActor, pBrkName);
        setBrkFrameAndStop(pActor, frame);
    }

    void startBtpAndSetFrameAndStop(const LiveActor* pActor, const char* pBtpName, f32 frame) {
        startBtp(pActor, pBtpName);
        setBtpFrameAndStop(pActor, frame);
    }

    void startBtk(const LiveActor* pActor, const char* pBtkName) {
        pActor->mModelManager->startBtk(pBtkName);
    }

    void startBrk(const LiveActor* pActor, const char* pBrkName) {
        pActor->mModelManager->startBrk(pBrkName);
    }

    void startBtp(const LiveActor* pActor, const char* pBtpName) {
        pActor->mModelManager->startBtp(pBtpName);
    }

    void startBpk(const LiveActor* pActor, const char* pBpkName) {
        pActor->mModelManager->startBpk(pBpkName);
    }

    void startBva(const LiveActor* pActor, const char* pBvaName) {
        pActor->mModelManager->startBva(pBvaName);
    }

    bool startBtkIfExist(const LiveActor* pActor, const char* pBtkName) {
        if (getResourceHolder(pActor)->mBtkResTable->isExistRes(pBtkName)) {
            pActor->mModelManager->startBtk(pBtkName);
            return true;
        }

        return false;
    }

    bool startBrkIfExist(const LiveActor* pActor, const char* pBrkName) {
        if (getResourceHolder(pActor)->mBrkResTable->isExistRes(pBrkName)) {
            pActor->mModelManager->startBrk(pBrkName);
            return true;
        }

        return false;
    }

    bool startBtpIfExist(const LiveActor* pActor, const char* pBtpName) {
        if (getResourceHolder(pActor)->mBtpResTable->isExistRes(pBtpName)) {
            pActor->mModelManager->startBtp(pBtpName);
            return true;
        }

        return false;
    }

    bool startBpkIfExist(const LiveActor* pActor, const char* pBpkName) {
        if (getResourceHolder(pActor)->mBpkResTable->isExistRes(pBpkName)) {
            pActor->mModelManager->startBpk(pBpkName);
            return true;
        }

        return false;
    }

    bool startBvaIfExist(const LiveActor* pActor, const char* pBvaName) {
        if (getResourceHolder(pActor)->mBvaResTable->isExistRes(pBvaName)) {
            pActor->mModelManager->startBva(pBvaName);
            return true;
        }

        return false;
    }

    bool isBtkPlaying(const LiveActor* pActor, const char* pBtkName) {
        return pActor->mModelManager->isBtkPlaying(pBtkName);
    }

    bool isBrkPlaying(const LiveActor* pActor, const char* pBrkName) {
        return pActor->mModelManager->isBrkPlaying(pBrkName);
    }

    bool isBtpPlaying(const LiveActor* pActor, const char* pBtpName) {
        return pActor->mModelManager->isBtpPlaying(pBtpName);
    }

    bool isBpkPlaying(const LiveActor* pActor, const char* pBpkName) {
        return pActor->mModelManager->isBpkPlaying(pBpkName);
    }

    bool isBvaPlaying(const LiveActor* pActor, const char* pBvaName) {
        return pActor->mModelManager->isBvaPlaying(pBvaName);
    }

    bool isBckExist(const LiveActor* pActor, const char* pBckName) {
        ResTable* pBckResTable = getResourceHolder(pActor)->mMotionResTable;

        if (pBckResTable->mCount != 0) {
            if (pBckResTable->isExistRes(pBckName)) {
                return true;
            }
        }

        return false;
    }

    bool isBtkExist(const LiveActor* pActor, const char* pBtkName) {
        ResTable* pBtkResTable = getResourceHolder(pActor)->mBtkResTable;

        if (pBtkResTable->mCount != 0) {
            if (pBtkResTable->isExistRes(pBtkName)) {
                return true;
            }
        }

        return false;
    }

    bool isBrkExist(const LiveActor* pActor, const char* pBrkName) {
        ResTable* pBrkResTable = getResourceHolder(pActor)->mBrkResTable;

        if (pBrkResTable->mCount != 0) {
            if (pBrkResTable->isExistRes(pBrkName)) {
                return true;
            }
        }

        return false;
    }

    bool isBpkExist(const LiveActor* pActor, const char* pBpkName) {
        ResTable* pBpkResTable = getResourceHolder(pActor)->mBpkResTable;

        if (pBpkResTable->mCount != 0) {
            if (pBpkResTable->isExistRes(pBpkName)) {
                return true;
            }
        }

        return false;
    }

    bool isBtpExist(const LiveActor* pActor, const char* pBtpName) {
        ResTable* pBtpResTable = getResourceHolder(pActor)->mBtpResTable;

        if (pBtpResTable->mCount != 0) {
            if (pBtpResTable->isExistRes(pBtpName)) {
                return true;
            }
        }

        return false;
    }

    bool isBvaExist(const LiveActor* pActor, const char* pBvaName) {
        ResTable* pBvaResTable = getResourceHolder(pActor)->mBvaResTable;

        if (pBvaResTable->mCount != 0) {
            if (pBvaResTable->isExistRes(pBvaName)) {
                return true;
            }
        }

        return false;
    }

    void stopBck(const LiveActor* pActor) {
        getBckCtrl(pActor)->setRate(0.0f);
    }

    void stopBtk(const LiveActor* pActor) {
        pActor->mModelManager->stopBtk();
    }

    void stopBrk(const LiveActor* pActor) {
        pActor->mModelManager->stopBrk();
    }

    void stopBtp(const LiveActor* pActor) {
        pActor->mModelManager->stopBtp();
    }

    void stopBva(const LiveActor* pActor) {
        pActor->mModelManager->stopBva();
    }

    void setBckRate(const LiveActor* pActor, f32 rate) {
        getBckCtrl(pActor)->mRate = rate * ::sAnimRateScale;
    }

    void setBtkRate(const LiveActor* pActor, f32 rate) {
        getBtkCtrl(pActor)->mRate = rate * ::sAnimRateScale;
    }

    void setBrkRate(const LiveActor* pActor, f32 rate) {
        getBrkCtrl(pActor)->mRate = rate * ::sAnimRateScale;
    }

    void setBvaRate(const LiveActor* pActor, f32 rate) {
        getBvaCtrl(pActor)->mRate = rate * ::sAnimRateScale;
    }

    void setBckFrame(const LiveActor* pActor, f32 frame) {
        getBckCtrl(pActor)->setFrame(frame);
    }

    void setBtkFrame(const LiveActor* pActor, f32 frame) {
        getBtkCtrl(pActor)->setFrame(frame);
    }

    void setBrkFrame(const LiveActor* pActor, f32 frame) {
        getBrkCtrl(pActor)->setFrame(frame);
    }

    void setBtpFrame(const LiveActor* pActor, f32 frame) {
        getBtpCtrl(pActor)->setFrame(frame);
    }

    void setBpkFrame(const LiveActor* pActor, f32 frame) {
        getBpkCtrl(pActor)->setFrame(frame);
    }

    void setBvaFrame(const LiveActor* pActor, f32 frame) {
        getBvaCtrl(pActor)->setFrame(frame);
    }

    bool isBckPlaying(const LiveActor* pActor, const char* pBckName) {
        return MR::isBckPlaying(pActor->mModelManager->mXanimePlayer, pBckName);
    }

    J3DFrameCtrl* getBckCtrl(const LiveActor* pActor) {
        return pActor->mModelManager->getBckCtrl();
    }

    J3DFrameCtrl* getBtkCtrl(const LiveActor* pActor) {
        return pActor->mModelManager->getBtkCtrl();
    }

    J3DFrameCtrl* getBrkCtrl(const LiveActor* pActor) {
        return pActor->mModelManager->getBrkCtrl();
    }

    J3DFrameCtrl* getBtpCtrl(const LiveActor* pActor) {
        return pActor->mModelManager->getBtpCtrl();
    }

    J3DFrameCtrl* getBpkCtrl(const LiveActor* pActor) {
        return pActor->mModelManager->getBpkCtrl();
    }

    J3DFrameCtrl* getBvaCtrl(const LiveActor* pActor) {
        return pActor->mModelManager->getBvaCtrl();
    }

    void updateMaterial(LiveActor* pActor) {
        pActor->mModelManager->updateDL(true);
    }

    void initJointTransform(const LiveActor* pActor) {
        pActor->mModelManager->initJointTransform();
    }

    XjointTransform* getJointTransform(const LiveActor* pActor, const char* pName) {
        return pActor->mModelManager->getJointTransform(pName);
    }

    void setJointTransformLocalMtx(const LiveActor* pActor, const char* pName, MtxPtr pMtx) {
        XjointTransform* pTransform = pActor->mModelManager->getJointTransform(pName);
        pTransform->_68 = pMtx;
    }

    f32 getBckFrame(const LiveActor* pActor) {
        return getBckCtrl(pActor)->getFrame();
    }

    f32 getBrkFrame(const LiveActor* pActor) {
        return getBrkCtrl(pActor)->getFrame();
    }

    f32 getBtpFrame(const LiveActor* pActor) {
        return getBtpCtrl(pActor)->getFrame();
    }

    f32 getBvaFrame(const LiveActor* pActor) {
        return getBvaCtrl(pActor)->getFrame();
    }

    f32 getBckRate(const LiveActor* pActor) {
        return getBckCtrl(pActor)->getRate();
    }

    f32 getBckFrameMax(const LiveActor* pActor) {
        return getBckCtrl(pActor)->getEnd();
    }

    f32 getBtkFrameMax(const LiveActor* pActor) {
        return getBtkCtrl(pActor)->getEnd();
    }

    f32 getBrkFrameMax(const LiveActor* pActor) {
        return getBrkCtrl(pActor)->getEnd();
    }

    void stopAnimFrame(LiveActor* pActor) {
        pActor->mFlag.mIsStoppedAnim = true;
    }

    void releaseAnimFrame(LiveActor* pActor) {
        pActor->mFlag.mIsStoppedAnim = false;
    }

}  // namespace MR
