#include <aurora/allocation.hpp>
#include "resource/TextEncoding.hpp"
#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/Screen/LayoutCoreUtil.hpp"
#include "Game/Screen/StarPointerController.hpp"
#include "Game/Screen/StarPointerLayout.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "compat/StarPointerDepthOwnership.hpp"

namespace {
    // These stable Game names outlive every owner that borrows their bytes.
    std::string encode_owner_name(std::string_view name) {
        aurora::allocation::HostAllocationScope host;
        return smgpc::resource::encode_cp932(name);
    }

    const std::string cStarPointerGuidanceName = encode_owner_name("スターポインタガイダンス");
}  // namespace

StarPointerDirector::StarPointerDirector()
    : mIsUpdateTransHolder(false), mIsAllowP1StarPieceShot(false), mIsAllowP2StarPieceShot(false), mControllers(nullptr),
      mStarPointerLayouts(nullptr), mTransHolder(nullptr), mPeekZ(nullptr), mGuidance(nullptr), mNozzleAimPos(0.0f, 0.0f, 0.0f) {
    mPeekZ = new StarPointerPeekZ();
    mTransHolder = new StarPointerTransformHolder();
    mControllers = new StarPointerController[StarPointerFunction::getNumStarPointer()];
    for (s32 channel = 0; channel < StarPointerFunction::getNumStarPointer(); channel++) {
        mControllers[channel].initAndSetPort(channel);
        mPeekZ->mInfos[channel] = &mControllers[channel].mInfo;
    }
}

void StarPointerDirector::update() {
    if (MR::isStarPointerModeHomeButton() || MR::isStarPointerModeErrorWindow()) {
        mIsOSPointerMode = true;
        return;
    }

    if (mIsOSPointerMode && mStarPointerLayouts != nullptr) {
        for (s32 channel = 0; channel < StarPointerFunction::getNumStarPointer(); channel++) {
            getStarPointerLayout(channel)->hideBlur();
        }
    }

    mIsOSPointerMode = false;

    if (_18 < 0) {
        _18++;
    }

    mTransHolder->movement();

    for (s32 channel = 0; channel < StarPointerFunction::getNumStarPointer(); channel++) {
        if (MR::isConnectedWPad(channel)) {
            getStarPointerController(channel)->movement(mPeekZ->mProjectionParameters, mPeekZ->mViewportParameters);
        }

        if (mStarPointerLayouts != nullptr) {
            getStarPointerLayout(channel)->movement();
            getStarPointerLayout(channel)->calcAnim();
        }
    }

    if (mGuidance != nullptr) {
        mGuidance->movement();
        mGuidance->calcAnim();
    }
}

void StarPointerDirector::draw() {
    if (mGuidance != nullptr) {
        mGuidance->draw();
    }

    if (mStarPointerLayouts != nullptr) {
        for (s32 channel = 0; channel < StarPointerFunction::getNumStarPointer(); channel++) {
            getStarPointerLayout(channel)->draw();
        }
    }
}

void StarPointerDirector::startHandPointer() {
    if (mStarPointerLayouts != nullptr) {
        mStarPointerLayouts[0].changeLayout(StarPointerKind_HandPointer);
    }
}

void StarPointerDirector::startHandPointerReactionWithCrossCursor() {
    if (mStarPointerLayouts != nullptr) {
        mStarPointerLayouts[0].changeLayout(StarPointerKind_HandPointerReactionWithCrossCursor);
    }
}

void StarPointerDirector::startFingerPointer() {
    if (mStarPointerLayouts != nullptr) {
        mStarPointerLayouts[0].changeLayout(StarPointerKind_FingerPointer);
    }
}

void StarPointerDirector::startStarPointer() {
    if (mStarPointerLayouts != nullptr) {
        mStarPointerLayouts[0].changeLayout(StarPointerKind_StarPointer);
    }
}

void StarPointerDirector::startStarPointerNozzle() {
    if (mStarPointerLayouts != nullptr) {
        mStarPointerLayouts[0].changeLayout(StarPointerKind_StarPointerNozzle);
    }
}

void StarPointerDirector::setGameSceneCameraMtx() {
    if (mIsUpdateTransHolder) {
        mTransHolder->mViewMtx.setInline(MR::getCameraViewMtx());
        mTransHolder->mProjMtx.setInline(MR::getCameraProjectionMtx());
        mTransHolder->mFovy = MR::getFovy();
    }
}

StarPointerController* StarPointerDirector::getStarPointerController(s32 channel) const {
    return &mControllers[channel];
}

StarPointerLayout* StarPointerDirector::getStarPointerLayout(s32 channel) const {
    // Preserve the retail null-array test without native null-pointer arithmetic.
    return mStarPointerLayouts ? &mStarPointerLayouts[channel] : nullptr;
}

void StarPointerDirector::createLayout() {
    mStarPointerLayouts = new StarPointerLayout[StarPointerFunction::getNumStarPointer()];

    for (u32 channel = 0; channel < StarPointerFunction::getNumStarPointer(); channel++) {
        mStarPointerLayouts[channel].initWithPort(channel);
        mStarPointerLayouts[channel].mDirector = this;
    }

    mGuidance = new StarPointerGuidance(cStarPointerGuidanceName.c_str());
    mGuidance->initWithoutIter();
}

namespace StarPointerFunction {
    bool isOnScreenEdge(const TVec2f& rPos, f32 marginX, f32 marginY) {
        if (rPos.x <= marginX) {
            return true;
        }

        if (rPos.y <= marginY) {
            return true;
        }

        if (MR::getScreenWidth() - marginX <= rPos.x) {
            return true;
        }

        if (MR::getScreenHeight() - marginY <= rPos.y) {
            return true;
        }

        return false;
    }

    bool isOnScreenEdge(s32 channel) {
        TVec2f pos;
        MR::convertPaneLocalPosToScreenPos(&pos, MR::getRootPane(getStarPointerDirector()->getStarPointerLayout(channel)), TVec2f(0.0f, 0.0f));
        return isOnScreenEdge(pos, 0.0f, 0.0f);
    }

    StarPointerDirector* getStarPointerDirector() { return &smgpc::compat::require_star_pointer_depth().director(); }

    s32 getPastPointNum(s32 channel) {
        return getStarPointerDirector()->getStarPointerController(channel)->mPastPointNum;
    }

    const TVec2f& getPastPosition(s32 channel, s32 index) {
        return getStarPointerDirector()->getStarPointerController(channel)->mPastPosition[index];
    }

    s32 getNextPastPointNum(s32 channel) {
        return getStarPointerDirector()->getStarPointerController(channel)->mNextPastPointNum;
    }

    bool canShoot(s32 channel) {
        return getStarPointerDirector()->getStarPointerLayout(channel)->mShootDisabled == false;
    }

} // namespace StarPointerFunction
