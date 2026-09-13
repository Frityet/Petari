#include "Game/Animation/AnmPlayer.hpp"
#include "Game/System/ResourceInfo.hpp"
#include "Game/Util/StringUtil.hpp"
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3DGraphAnimator/J3DModelData.hpp>
#include <JSystem/JGeometry/TQuat.hpp>

namespace JGeometry {
    template <>
    void TQuat4< f32 >::setEuler(f32 rx, f32 ry, f32 rz) {
        f32 cosX = cos(static_cast< f64 >(0.5f * rx));
        f32 cosY = cos(static_cast< f64 >(0.5f * ry));
        f32 cosZ = cos(static_cast< f64 >(0.5f * rz));
        f32 sinX = sin(static_cast< f64 >(0.5f * rx));
        f32 sinY = sin(static_cast< f64 >(0.5f * ry));
        f32 sinZ = sin(static_cast< f64 >(0.5f * rz));

        x = (cosY * cosZ) * sinX - (sinY * sinZ) * cosX;
        y = (sinY * cosZ) * cosX + (cosY * sinZ) * sinX;
        z = (cosY * sinZ) * cosX - (sinY * cosZ) * sinX;
        w = (cosY * cosZ) * cosX + (sinY * sinZ) * sinX;
    }
}  // namespace JGeometry

AnmPlayerBase::AnmPlayerBase(const ResTable* pResTable) : mResTable(pResTable), mAnmRes(nullptr), mFrameCtrl(0) {
}

void AnmPlayerBase::update() {
    if (mAnmRes != nullptr) {
        mFrameCtrl.update();
    }
}

void AnmPlayerBase::reflectFrame() {
    if (mAnmRes != nullptr) {
        mAnmRes->mFrame = mFrameCtrl.mFrame;
    }
}

void AnmPlayerBase::start(const char* pResName) {
    J3DAnmBase* pAnmRes = reinterpret_cast< J3DAnmBase* >(mResTable->getRes(pResName));

    if (pAnmRes != mAnmRes) {
        changeAnimation(pAnmRes);
        mAnmRes = pAnmRes;
    }

    mFrameCtrl.init(mAnmRes->mFrameMax);
    mFrameCtrl.setAttribute(mAnmRes->mAttribute);
    mFrameCtrl.setFrame(0.0f);
    mFrameCtrl.setRate(1.0f);
}

void AnmPlayerBase::stop() {
    stopAnimation();
    mFrameCtrl.setRate(0.0f);
}

bool AnmPlayerBase::isPlaying(const char* pAnimName) const {
    if (mAnmRes != nullptr) {
        if (MR::isEqualStringCase(pAnimName, mResTable->getResName(mAnmRes))) {
            return true;
        }
    }

    return false;
}

MaterialAnmPlayerBase::MaterialAnmPlayerBase(const ResTable* pResTable, J3DModelData* pModelData) : AnmPlayerBase(pResTable), mModelData(pModelData) {
}

void MaterialAnmPlayerBase::beginDiff() {
    if (mAnmRes != nullptr) {
        reflectFrame();
        attach(mAnmRes, mModelData);
    }
}

void MaterialAnmPlayerBase::endDiff() {
    if (mAnmRes != nullptr) {
        detach(mAnmRes, mModelData);
    }
}

void AnmPlayerBase::changeAnimation(J3DAnmBase*) {
}

void AnmPlayerBase::stopAnimation() {
}
