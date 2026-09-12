#include "Game/NPC/TalkTextFormer.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/MessageUtil.hpp"

#include "Game/Screen/CustomTagProcessor.hpp"
#include "Game/System/Language.hpp"
#include <nw4r/ut/RuntimeTypeInfo.h>

namespace {
    nw4r::lyt::TextBox* getTextBoxPane(LayoutActor* actor, const char* paneName) NO_INLINE;

    nw4r::lyt::TextBox* getTextBoxPane(LayoutActor* actor, const char* paneName) {
        return nw4r::ut::DynamicCast< nw4r::lyt::TextBox* >(MR::getPane(actor, paneName));
    }
};  // namespace

namespace MR {
    void initTagProcessorRecursive(LayoutActor* actor, const char* paneName, s32 mode) {
        nw4r::lyt::TextBox* textBox = getTextBoxPane(actor, paneName);
        if (textBox != nullptr) {
            CustomTagProcessor* processor = static_cast< CustomTagProcessor* >(textBox->mpTagProcessor);
            f32 languageRate = 1.25f;
            if (getLanguage() == 0x10) {
                languageRate = 1.0f;
            } else if (getLanguage() == 0x37) {
                languageRate = 0.75f;
            }

            switch (mode) {
            case 2:
                processor->initAlpha(0.0f, 0.0f, 0, 0);
                break;
            case 0:
                processor->initAlpha(0.8f * languageRate, 0.9f, 0, 0);
                break;
            case 1:
                processor->initAlpha(0.35f * languageRate, 0.9f, 0, 0);
                break;
            }
        }

        nw4r::lyt::Pane* pane = getPane(actor, paneName);
        for (nw4r::lyt::PaneList::Iterator iter = pane->mChildList.GetBeginIter(); iter != pane->mChildList.GetEndIter(); ++iter) {
            initTagProcessorRecursive(actor, (*iter).mName, mode);
        }
    }

    void nextStepTagProcessorRecursive(LayoutActor* actor, const char* paneName) {
        nw4r::lyt::TextBox* textBox = getTextBoxPane(actor, paneName);
        if (textBox != nullptr) {
            CustomTagProcessor* processor = static_cast< CustomTagProcessor* >(textBox->mpTagProcessor);
            processor->_32 = false;
            processor->_31 = false;
            processor->mAlphaCtrl.update();
        }

        nw4r::lyt::Pane* pane = getPane(actor, paneName);
        for (nw4r::lyt::PaneList::Iterator iter = pane->mChildList.GetBeginIter(); iter != pane->mChildList.GetEndIter(); ++iter) {
            nextStepTagProcessorRecursive(actor, (*iter).mName);
        }
    }

    bool isEndStepTagProcessorRecursive(const LayoutActor* actor, const char* paneName, bool isEnd) {
        nw4r::lyt::TextBox* textBox = nw4r::ut::DynamicCast< nw4r::lyt::TextBox* >(getPane(actor, paneName));
        if (textBox != nullptr) {
            CustomTagProcessor* processor = static_cast< CustomTagProcessor* >(textBox->mpTagProcessor);
            isEnd &= processor->mAlphaCtrl.isEnd();
        }

        nw4r::lyt::Pane* pane = getPane(actor, paneName);
        for (nw4r::lyt::PaneList::Iterator iter = pane->mChildList.GetBeginIter(); iter != pane->mChildList.GetEndIter(); ++iter) {
            isEnd &= isEndStepTagProcessorRecursive(actor, (*iter).mName, true);
        }
        return isEnd;
    }
};  // namespace MR

TalkTextFormer::TalkTextFormer(LayoutActor* actor, const char* paneName) : mHostActor(actor), mMsg(nullptr), _8(0), mPaneName(paneName) {
}

bool TalkTextFormer::nextPage() {
    const wchar_t* message = MR::getNextMessagePage(mMsg);
    if (message != nullptr) {
        mMsg = message;
        formMessage(message, _8);
        return true;
    }

    return false;
}

bool TalkTextFormer::hasNextPage() const {
    if (mMsg != nullptr) {
        return MR::getNextMessagePage(mMsg) != nullptr;
    }

    return false;
}

void TalkTextFormer::updateTalking() {
    MR::nextStepTagProcessorRecursive(mHostActor, mPaneName);
}

bool TalkTextFormer::isTextAppearedAll() const {
    if (_8 == 2) {
        return true;
    }

    return MR::isEndStepTagProcessorRecursive(mHostActor, mPaneName, true);
}

void TalkTextFormer::setArg(const CustomTagArg& tag, s32 arg2) {
    if (tag.mArgType == CustomTagArg::Type_Int) {
        MR::setTextBoxArgNumberRecursive(mHostActor, mPaneName, tag.mIntArg, arg2);
    } else if (tag.mArgType == CustomTagArg::Type_Char) {
        MR::setTextBoxArgStringRecursive(mHostActor, mPaneName, tag.mCharArg, arg2);
    }

    MR::initTagProcessorRecursive(mHostActor, mPaneName, _8);
}

void TalkTextFormer::formMessage(const wchar_t* message, s32 arg2) {
    mMsg = message;
    _8 = arg2;

    MR::setTextBoxMessageRecursive(mHostActor, mPaneName, mMsg);
    MR::initTagProcessorRecursive(mHostActor, mPaneName, _8);
}
