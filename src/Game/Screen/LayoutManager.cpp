#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutCoreUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <nw4r/lyt/textBox.h>

void LayoutManager::initTextBoxRecursive(nw4r::lyt::Pane* pPane, nw4r::lyt::Pane* pUserDataPane, const char* pLayoutName, u32 allocSize) {
    nw4r::lyt::TextBox* textBox = nw4r::ut::DynamicCast<nw4r::lyt::TextBox*>(pPane);
    char userData[9];
    LayoutCoreUtil::getPaneUserData(pPane, userData);
    if (!MR::isEqualString("", userData)) {
        pUserDataPane = pPane;
    }

    if (textBox != nullptr) {
        if (pUserDataPane != nullptr) {
            LayoutCoreUtil::getPaneUserData(pUserDataPane, userData);
            char messageId[256];
            MR::getLayoutMessageID(messageId, pLayoutName, userData);
            LayoutCoreUtil::initTextBoxPane(textBox, messageId, 256);
        } else {
            LayoutCoreUtil::initTextBoxPane(textBox, nullptr, allocSize);
        }
    }

    nw4r::lyt::PaneList& children = pPane->mChildList;
    for (nw4r::lyt::PaneList::Iterator it = children.GetBeginIter(); it != children.GetEndIter(); ++it) {
        initTextBoxRecursive(&*it, pUserDataPane, pLayoutName, allocSize);
    }
}
