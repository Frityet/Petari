#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutCoreUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include <nw4r/lyt/pane.h>

namespace MR {
void createAndAddGroupCtrl(LayoutActor* pActor, const char* pGroupName, u32 animLayerNum) {
    pActor->getLayoutManager()->createAndAddGroupCtrl(pGroupName, animLayerNum);
}
nw4r::lyt::Pane* getRootPane(const LayoutActor* pActor) {
    return pActor->getLayoutManager()->getPane(nullptr);
}
void copyPaneRotate(TVec3f* pRotate, const LayoutActor* pActor, const char* pPaneName) {
    nw4r::lyt::Pane* pPane = pActor->getLayoutManager()->getPane(pPaneName);
    pRotate->set<f32>(pPane->mRotate.x, pPane->mRotate.y, pPane->mRotate.z);
}
void convertPaneLocalPosToScreenPos(TVec2f* pScreenPos, const nw4r::lyt::Pane* pPane, const TVec2f& rLocalPos) {
    TVec3f pos(rLocalPos);
    TVec3f layoutPos;
    PSMTXMultVec(pPane->mGlbMtx.m, pos, layoutPos);
    convertLayoutPosToScreenPos(pScreenPos, layoutPos);
}
} // namespace MR
