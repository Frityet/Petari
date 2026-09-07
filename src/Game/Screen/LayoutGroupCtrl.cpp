#include "Game/Screen/LayoutGroupCtrl.hpp"
#include "Game/Animation/LayoutAnmPlayer.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include <nw4r/lyt/group.h>

LayoutGroupCtrl::LayoutGroupCtrl(LayoutManager* pHost, const char* pGroupName, u32 animLayerNum)
    : mHost(pHost), mGroup(nullptr), mAnmPlayerArray(animLayerNum), _10(true) {
    mGroup = pHost->getGroup(pGroupName);

    for (u32 i = 0; i < mAnmPlayerArray.size(); i++) {
        mAnmPlayerArray[i] = new LayoutAnmPlayer(pHost);
    }
}

void LayoutGroupCtrl::movement() {
    for (u32 i = 0; i < mAnmPlayerArray.size(); i++) {
        mAnmPlayerArray[i]->movement();
    }
}

void LayoutGroupCtrl::calcAnim() {
    for (u32 i = 0; i < mAnmPlayerArray.size(); i++) {
        mAnmPlayerArray[i]->reflectFrame();
    }
}

u32 LayoutGroupCtrl::getPaneNum() const {
    return mGroup->GetPaneList().GetSize();
}

nw4r::lyt::Pane* LayoutGroupCtrl::getPane(u32 index) const {
    for (nw4r::lyt::PaneLinkList::Iterator iter = mGroup->GetPaneList().GetBeginIter(); iter != mGroup->GetPaneList().GetEndIter(); iter++) {
        if (index == 0) {
            return iter->mTarget;
        }
        index--;
    }
    return nullptr;
}
