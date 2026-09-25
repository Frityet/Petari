#include "compat/Cp932Literal.hpp"
#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/Screen/LayoutCoreUtil.hpp"
#include "Game/Screen/StarPointerController.hpp"
#include "Game/Screen/StarPointerGuidance.hpp"
#include "Game/Screen/StarPointerLayout.hpp"
#include "Game/Screen/StarPointerBlur.hpp"
#include "Game/Screen/StarPointerCommandStream.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/System/DrawSyncManager.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include <JSystem/JGeometry/TMatrix.hpp>
#include <JSystem/JMath/JMath.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <aurora/guest_thread.hpp>
#include <revolution/gx/GXGet.h>

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

StarPointerDirector::~StarPointerDirector() {
    const aurora::os::GuestThreadExecutionScope execution;
    DrawSyncManager::quiesceNativeCallbacks();
    if (auto* manager = DrawSyncManager::sInstance) {
        for (auto& range : manager->mTokenRanges) {
            if (range.mCallback == mPeekZ) {
                range = {};
            }
        }
    }

    if (auto* guidance = mGuidance) {
        delete guidance->mSpineFrame1P;
        delete guidance->mSpineGuidance;
        delete guidance->mSpineFrame2P;
        delete guidance;
        mGuidance = nullptr;
    }
    if (auto* layouts = mStarPointerLayouts) {
        for (s32 port = 0; port < 2; ++port) {
            auto& layout = layouts[port];
            delete layout.mNumber;
            delete layout.mCommandStream;
            if (auto* blur = layout.mBlur) {
                delete blur->mTexture;
                delete[] blur->mBlurPoints;
                delete[] blur->mBlurThicks;
                delete[] blur->mBlurTexCoords;
                delete blur;
            }
        }
        // Each original layout is an array element, not a separately owned
        // NameObj allocation. Its real destructor releases the native layout.
        delete[] layouts;
        mStarPointerLayouts = nullptr;
    }
    delete[] mControllers;
    delete mTransHolder;
    if (auto* peek = mPeekZ) {
        delete[] peek->mInfos;
        delete peek;
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
    // Preserve the original null-array check without native null-pointer arithmetic.
    return mStarPointerLayouts ? &mStarPointerLayouts[channel] : nullptr;
}

void StarPointerDirector::createLayout() {
    mStarPointerLayouts = new StarPointerLayout[StarPointerFunction::getNumStarPointer()];

    for (u32 channel = 0; channel < StarPointerFunction::getNumStarPointer(); channel++) {
        mStarPointerLayouts[channel].initWithPort(channel);
        mStarPointerLayouts[channel].mDirector = this;
    }

    mGuidance = new StarPointerGuidance(CP932("スターポインタガイダンス"));
    mGuidance->initWithoutIter();
}

namespace {
    static f32 mtx_identity[3][4] = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};
};  // namespace

StarPointerTransformHolder::StarPointerTransformHolder() : mViewMtx(mtx_identity) {
    // Retail copies 16 floats from its 12-float identity symbol, reading into
    // the adjacent vtable. Copy the defined rows on native hosts; the real
    // camera supplies all 16 projection elements before depth submission.
    for (s32 row = 0; row < 3; ++row) {
        for (s32 column = 0; column < 4; ++column) {
            mProjMtx.mMtx[row][column] = mtx_identity[row][column];
        }
    }
}

void StarPointerTransformHolder::movement() {
    f32 fovyRad = PI_180 * getFovy();
    mFocalLength = ((MR::getScreenHeight() * 0.5f) / MR::tan(fovyRad * 0.5f));
}

StarPointerPeekZ::StarPointerPeekZ() {
    mInfos = new DpdInfo*[2];
    mToken = DrawSyncManager::sInstance->setCallback(2, 1, this);
}

void StarPointerPeekZ::setDrawSyncToken() {
    GXGetProjectionv(mProjectionParameters);
    GXGetViewportv(mViewportParameters);
    DrawSyncManager::sInstance->pushBreakPoint();
    GXSetDrawSync(mToken);
}

void StarPointerPeekZ::drawSyncCallback(u16 token) {
    for (s32 channel = 0; channel < StarPointerFunction::getNumStarPointer(); channel++) {
        if (MR::isInRange(mInfos[channel]->mPos.x, 0.0f, MR::getScreenWidth() - 1) &&
            MR::isInRange(mInfos[channel]->mPos.y, 0.0f, MR::getScreenHeight() - 1)) {
            TVec2f pos;
            MR::convertScreenPosToFrameBufferPos(&pos, mInfos[channel]->mPos);
            GXPeekZ(pos.x, pos.y, &mInfos[channel]->mZDepth);
            mInfos[channel]->mDrawReady = true;
        }
    }
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

    bool forceInsideScreenEdge(TVec2f* pPos) {
        bool forced = false;

        f32 margin = 0.0f;
        f32 width = MR::getScreenWidth() - margin;
        f32 height = MR::getScreenHeight() - margin;

        if (pPos->x < margin) {
            pPos->x = margin;
            forced = true;
        } else if (width < pPos->x) {
            pPos->x = width;
            forced = true;
        }

        if (pPos->y < margin) {
            pPos->y = margin;
            forced = true;
        } else if (height < pPos->y) {
            pPos->y = height;
            forced = true;
        }

        return forced;
    }

    StarPointerDirector* getStarPointerDirector() {
        return SingletonHolder< GameSystem >::get()->mObjHolder->mStarPointerDirector;
    }

    s32 getNumStarPointer() {
        return 2;
    }

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
};  // namespace StarPointerFunction
