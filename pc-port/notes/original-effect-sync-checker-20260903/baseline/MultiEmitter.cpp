#include "Game/Effect/MultiEmitter.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/MultiEmitterCallBack.hpp"
#include "Game/Effect/MultiEmitterParticleCallBack.hpp"
#include "Game/Effect/SingleEmitter.hpp"
#include "Game/Effect/SyncBckEffectInfo.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <algorithm>
#include <cstring>

MultiEmitter::MultiEmitter(const char* pName, const TVec3f* pScale, const TVec3f* pRotation, const TVec3f* pTranslation, const TVec3f& rVec)
    : mEmitters(), mCallBack(), mParticleCallBack(), _24(), _28(), _2C(), _30(), mHash(), mFlags() {
    mCallBack = new MultiEmitterCallBack(this, pScale, pRotation, pTranslation, rVec);
    mParticleCallBack = new MultiEmitterParticleCallBack();

    allocateEmitter(pName);
}

MultiEmitter::MultiEmitter(const char* pName, MtxPtr pMtx, const TVec3f& rVec)
    : mEmitters(), mCallBack(), mParticleCallBack(), _24(), _28(), _2C(), _30(), mHash(), mFlags() {
    mCallBack = new MultiEmitterCallBack(this, pMtx, rVec);
    mParticleCallBack = new MultiEmitterParticleCallBack();

    allocateEmitter(pName);
}

MultiEmitter::MultiEmitter(const char* pName, MtxPtr pMtx, const TVec3f* pTranslation, const TVec3f& rVec)
    : mEmitters(), mCallBack(), mParticleCallBack(), _24(), _28(), _2C(), _30(), mHash(), mFlags() {
    mCallBack = new MultiEmitterCallBack(this, pMtx, pTranslation, rVec);
    mParticleCallBack = new MultiEmitterParticleCallBack();

    allocateEmitter(pName);
}

MultiEmitter::MultiEmitter(const char* pName) : mEmitters(), mCallBack(), mParticleCallBack(), _24(), _28(), _2C(), _30(), mHash(), mFlags() {
    mCallBack = new MultiEmitterCallBack(this, TVec3f(0.0f, 0.0f, 0.0f));
    mParticleCallBack = new MultiEmitterParticleCallBack();

    allocateEmitter(pName);
}

void MultiEmitter::createEmitter() {
    create(MR::getEffectSystem());
}

void MultiEmitter::createEmitterWithCallBack(MultiEmitterCallBackBase* pCallBackBase) {
    for (SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        MR::getEffectSystem()->createSingleEmitter(pEmitter, pCallBackBase, nullptr);
    }
}

void MultiEmitter::deleteEmitter() {
    std::for_each_array(mEmitters.begin(), mEmitters.end(), std::mem_func(&SingleEmitter::deleteEmitter));
    std::for_each(mChildren.begin(), mChildren.end(), std::mem_func(&MultiEmitter::deleteEmitter));
}

void MultiEmitter::forceDeleteEmitter() {
    for (SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        MR::getEffectSystem()->forceDeleteSingleEmitter(pEmitter);
    }

    std::for_each(mChildren.begin(), mChildren.end(), std::mem_func(&MultiEmitter::forceDeleteEmitter));
}

void MultiEmitter::forceDelete(EffectSystem* pSystem) {
    for (SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        pSystem->forceDeleteSingleEmitter(pEmitter);
    }
}

void MultiEmitter::deleteForeverEmitter() {
    for (SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (pEmitter->isValid() && !pEmitter->isOneTime()) {
            pEmitter->deleteEmitter();
        }
    }

    std::for_each(mChildren.begin(), mChildren.end(), std::mem_func(&MultiEmitter::deleteForeverEmitter));
}

void MultiEmitter::playCalcAndDeleteForeverEmitter() {
    playCalcEmitter(-1);
    deleteForeverEmitter();
}

bool MultiEmitter::isValid() const {
    for (const SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (pEmitter->isValid()) {
            return true;
        }
    }

    return false;
}

bool MultiEmitter::isExistOneTimeEmitter() const {
    for (const SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (pEmitter->isOneTime()) {
            return true;
        }
    }

    return false;
}

void MultiEmitter::setHostSRT(const TVec3f* pScale, const TVec3f* pRotation, const TVec3f* pTranslation) {
    mCallBack->setHostSRT(pScale, pRotation, pTranslation);
}

void MultiEmitter::setHostMtx(MtxPtr pMtx) {
    mCallBack->setHostMtx(pMtx);
}

void MultiEmitter::setOffset(const TVec3f& rOffset) {
    mCallBack->_18.set(rOffset);
}

void MultiEmitter::setBaseScale(f32 scale) {
    mCallBack->setBaseScale(scale);
}

void MultiEmitter::setName(const char* pName) {
    mHash = MR::getHashCode(pName);
}

bool MultiEmitter::isEqualName(u16 hash) const {
    return mHash == hash;
}

ParticleEmitter* MultiEmitter::getParticleEmitter(int idx) const {
    const SingleEmitter* pEmitter = &mEmitters[idx];
    if (pEmitter->isValid()) {
        return pEmitter->mEmitter;
    }

    return nullptr;
}

void MultiEmitter::create(EffectSystem* pSystem) {
    for (SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        pSystem->createSingleEmitter(pEmitter, mCallBack, mParticleCallBack);
    }

    mCallBack->resetFollowCurrent();

    for (MultiEmitter** pEmitter = mChildren.begin(); pEmitter != mChildren.end(); pEmitter++) {
        (*pEmitter)->create(pSystem);
    }
}

void MultiEmitter::scanParticleEmitter(EffectSystem* pSystem) {
    std::for_each_array(mEmitters.begin(), mEmitters.end(), std::bind2nd(std::mem_func(&SingleEmitter::scanParticleEmitter), pSystem));
}

void MultiEmitter::forceFollowOn() {
    mCallBack->forceFollowOn();
}

void MultiEmitter::forceFollowOff() {
    mCallBack->forceFollowOff();
}

void MultiEmitter::forceScaleOn() {
    mCallBack->forceScaleOn();
}

void MultiEmitter::initSyncBck(XanimePlayer* pPlayer, const char* pName, s32 count, f32 startFrame) {
    _24 = new SyncBckEffectInfo(pPlayer, pName, count, 0.0f, -1.0f, false);
    _24->mStartFrame = startFrame;
}

void MultiEmitter::onDeleteSyncBck(bool, f32 endFrame) {
    _24->mEndFrame = endFrame;
}

void MultiEmitter::addSyncBck(const XanimePlayer* pPlayer, const char* pName) {
    _24->addBck(pPlayer, pName);
}

void MultiEmitter::setContinueBckEnd(bool continueBckEnd) {
    _24->mContinueBckEnd = continueBckEnd;
}

void MultiEmitter::onCreateSyncClipping() {
    turnFlagOn(JPAEmtrStts_FirstEmit);
}

void MultiEmitter::onForceDeleteSyncClipping() {
    turnFlagOn(JPAEmtrStts_Immortal);
}

void MultiEmitter::stopEmitterOnClipped() {
    if (isFlagOn(JPAEmtrStts_Immortal)) {
        forceDeleteEmitter();
    } else if (isFlagOn(JPAEmtrStts_RateStepEmit)) {
        deleteEmitter();
    } else if (_24 != nullptr) {
        stopCalcEmitter(-1);
        stopDrawParticle(-1);
    }

    for (SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (pEmitter->isValid() && !pEmitter->isOneTime()) {
            pEmitter->mEmitter->mEmitter->setStatus(JPAEmtrStts_StopCalc);
            pEmitter->mEmitter->mEmitter->setStatus(JPAEmtrStts_StopDraw);
        }
    }
}

void MultiEmitter::playEmitterOffClipped() {
    if (isFlagOn(JPAEmtrStts_FirstEmit)) {
        create(MR::getEffectSystem());
    } else if (_24 != nullptr) {
        playCalcEmitter(-1);
        playDrawParticle(-1);
    }

    for (SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (pEmitter->isValid() && !pEmitter->isOneTime()) {
            pEmitter->mEmitter->mEmitter->clearStatus(JPAEmtrStts_StopCalc);
            pEmitter->mEmitter->mEmitter->clearStatus(JPAEmtrStts_StopDraw);
        }
    }
}
/*
void MultiEmitter::setDrawOrder(s32 idx) {
}
 */
void MultiEmitter::addChildEmitter(MultiEmitter* pChild) {
    mChildren.push_back(pChild);
}

void MultiEmitter::setGlobalRotationDegree(const TVec3f& rRotation, s32 idx) {
    setGlobalRotation(TVec3s(rRotation.x * DEGREE_TO_S16, rRotation.y * DEGREE_TO_S16, rRotation.z * DEGREE_TO_S16), idx);
}

void MultiEmitter::allocateEmitter(const char* pName) {
    MR::Vector< MR::FixedArray< u16, 32 > > indices;
    bool isNumbered = !MR::hasStringSpace(pName) && !MR::isDigitStringTail(pName, 2);

    if (isNumbered) {
        u16 index = 0;
        while (MR::Effect::isExistInResource(&index, pName, indices.size())) {
            indices.push_back(index);
        }
    } else {
        u32 length = strlen(pName);
        s32 nameLength = 0;
        char name[40];
        MR::zeroMemory(name, sizeof(name));

        for (u32 i = 0; i <= length; i++) {
            char c = pName[i];
            if (c == ' ' || c == '\0') {
                name[nameLength] = '\0';
                u16 index;
                MR::Effect::isExistInResource(&index, name);
                indices.push_back(index);
                nameLength = 0;
                MR::zeroMemory(name, sizeof(name));
            } else {
                name[nameLength++] = c;
            }
        }
    }

    mEmitters.init(indices.size());
    for (s32 i = 0; i < indices.size(); i++) {
        mEmitters[i].init(indices[i]);
    }

    if (isNumbered) {
        mHash = MR::getHashCode(pName);
    }
}

SingleEmitter* MultiEmitter::getValidEmitter(s32 idx, bool) {
    SingleEmitter* pEmitter = &mEmitters[idx];
    if (pEmitter->isValid()) {
        return pEmitter;
    }

    return nullptr;
}

void MultiEmitter::createOneTimeEmitter() {
    for (SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (pEmitter->isOneTime()) {
            MR::getEffectSystem()->createSingleEmitter(pEmitter, mCallBack, nullptr);
        }
    }
}

void MultiEmitter::createForeverEmitter() {
    for (SingleEmitter* pEmitter = mEmitters.begin(); pEmitter != mEmitters.end(); pEmitter++) {
        if (!pEmitter->isOneTime()) {
            MR::getEffectSystem()->createSingleEmitter(pEmitter, mCallBack, nullptr);
        }
    }
}

void SingleEmitter::setGroupID(u8 groupId) {
    mGroupId = groupId;
}
