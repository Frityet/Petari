#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/Animation/LayoutAnmPlayer.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutCoreUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include <nw4r/lyt/pane.h>

LayoutPaneCtrl::LayoutPaneCtrl(LayoutManager* pHost, const char* pPaneName, u32 animLayerNum)
    : mHost(pHost), mPane(nullptr), mPaneIndex(-1), mAnmPlayerArray(animLayerNum), mFollowType(0), mFollowPos(nullptr) {
    mPane = mHost->getPane(pPaneName);

    for (u32 i = 0; i < mAnmPlayerArray.size(); i++) {
        mAnmPlayerArray[i] = new LayoutAnmPlayer(pHost);
    }
}

void LayoutPaneCtrl::movement() {
    for (u32 i = 0; i < mAnmPlayerArray.size(); i++) {
        mAnmPlayerArray[i]->movement();
    }
}

void LayoutPaneCtrl::calcAnim() {
    for (u32 i = 0; i < mAnmPlayerArray.size(); i++) {
        mAnmPlayerArray[i]->reflectFrame();
    }
}

void LayoutPaneCtrl::start(const char* pAnimName, u32 layer) {
    LayoutAnmPlayer* pAnmPlayer = mAnmPlayerArray[layer];

    if (pAnmPlayer->mAnimTransform != nullptr) {
        if (mHost->_61) {
            mPane->UnbindAnimation(pAnmPlayer->mAnimTransform, true);
        } else {
            mHost->unbindPaneCtrlAnim(this, pAnmPlayer->mAnimTransform);
        }
    }

    pAnmPlayer->start(pAnimName);

    if (mHost->_61) {
        mPane->UnbindAnimation(pAnmPlayer->mAnimTransform, true);
        mPane->BindAnimation(pAnmPlayer->mAnimTransform, true);
    } else {
        mHost->bindPaneCtrlAnim(this, pAnmPlayer->mAnimTransform);
    }
}

void LayoutPaneCtrl::stop(u32 layer) {
    mAnmPlayerArray[layer]->stop();
}

bool LayoutPaneCtrl::isAnimStopped(u32 layer) const {
    return mAnmPlayerArray[layer]->isStop();
}

void LayoutPaneCtrl::reflectFollowPos() {
    if (mFollowPos == nullptr) {
        return;
    }

    nw4r::math::MTX34 matrix;
    PSMTXCopy(mPane->mGlbMtx, matrix);
    switch (mFollowType) {
    case 0: {
        TVec2f position;
        MR::convertScreenPosToLayoutPos(&position, *mFollowPos);
        matrix._03 = position.x;
        matrix._13 = position.y;
        break;
    }
    case 1: {
        TVec2f origin;
        MR::convertLayoutPosToScreenPos(&origin, TVec2f(0.0f, 0.0f));
        TVec2f offset(origin + *mFollowPos);
        MR::convertScreenPosToLayoutPos(&offset, offset);
        matrix._03 += offset.x;
        matrix._13 += offset.y;
        break;
    }
    case 2: {
        nw4r::math::MTX34 local = mPane->mMtx;
        nw4r::math::MTX34 inverse;
        PSMTXInverse(local, inverse);
        PSMTXConcat(matrix, inverse, matrix);
        nw4r::math::MTX34 replacement;
        PSMTXCopy(local, replacement);
        replacement._03 = mFollowPos->x;
        replacement._13 = mFollowPos->y;
        PSMTXConcat(matrix, replacement, matrix);
        break;
    }
    case 3: {
        nw4r::math::VEC3 offset;
        offset.x = mFollowPos->x;
        offset.y = mFollowPos->y;
        offset.z = 0.0f;
        nw4r::math::VEC3TransformNormal(&offset, &mPane->mMtx, &offset);
        matrix._03 += offset.x;
        matrix._13 += offset.y;
        break;
    }
    }
    mPane->mGlbMtx = matrix;
    recalcChildGlobalMtx(mPane);
}

J3DFrameCtrl* LayoutPaneCtrl::getFrameCtrl(u32 layer) const {
    return &mAnmPlayerArray[layer]->mFrameCtrl;
}

void LayoutPaneCtrl::recalcChildGlobalMtx(nw4r::lyt::Pane* pPane) {
    for (nw4r::lyt::PaneList::Iterator it = pPane->mChildList.GetBeginIter(); it != pPane->mChildList.GetEndIter(); ++it) {
        nw4r::math::MTX34 matrix;
        PSMTXConcat(it->mpParent->mGlbMtx, it->mMtx, matrix);
        it->mGlbMtx = matrix;
        recalcChildGlobalMtx(&*it);
    }
}
