#include "Game/Util/LayoutUtil.hpp"

#include <algorithm>
#include <optional>
#include <string>

#include "Game/Screen/IconAButton.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutActorFlag.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"
#include "Game/Util/NerveUtil.hpp"

namespace {
    [[nodiscard]] std::u16string utf16_from_wide(const wchar_t* pText) {
        auto text = std::u16string{};
        if (pText == nullptr) {
            return text;
        }

        while (*pText != L'\0') {
            const auto code = static_cast< char32_t >(*pText++);
            text.push_back(static_cast< char16_t >(std::min< char32_t >(code, 0xffffU)));
        }

        return text;
    }

    [[nodiscard]] std::u16string utf16_from_utf8_lossy(std::string_view text) {
        auto out = std::u16string{};
        out.reserve(text.size());
        for (const auto ch : text) {
            out.push_back(static_cast< char16_t >(static_cast< unsigned char >(ch)));
        }
        return out;
    }

    [[nodiscard]] std::string runtime_message_or_tag(const char* pMessageId) {
        const auto tag = pMessageId != nullptr ? std::string_view(pMessageId) : std::string_view{};
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            return runtime->messages().message_or(tag, tag);
        }
        return std::string(tag);
    }
}  // namespace

namespace MR {

    bool isDead(const SimpleLayout* pLayout) {
        return pLayout == nullptr || pLayout->isDead();
    }

    bool isDead(const LayoutActor* pLayout) {
        return pLayout == nullptr || pLayout->isDead();
    }

    void startAnim(SimpleLayout* pLayout, const char* pAnimName, u32 animLayer) {
        pLayout->startAnim(pAnimName, animLayer);
    }

    void startAnim(LayoutActor* pLayout, const char* pAnimName, u32 animLayer) {
        pLayout->startAnim(pAnimName, animLayer);
    }

    bool isAnimStopped(SimpleLayout* pLayout, u32 animLayer) {
        return pLayout->isAnimStopped(animLayer);
    }

    bool isAnimStopped(LayoutActor* pLayout, u32 animLayer) {
        return pLayout->isAnimStopped(animLayer);
    }

    void setAnimFrameAndStop(SimpleLayout* pLayout, f32 frame, u32 animLayer) {
        pLayout->setAnimFrameAndStop(frame, animLayer);
    }

    void setAnimFrameAndStop(LayoutActor* pLayout, f32 frame, u32 animLayer) {
        pLayout->setAnimFrameAndStop(frame, animLayer);
    }

    void setAnimFrame(SimpleLayout* pLayout, f32 frame, u32 animLayer) {
        pLayout->setAnimFrame(frame, animLayer);
    }

    void setAnimFrame(LayoutActor* pLayout, f32 frame, u32 animLayer) {
        pLayout->setAnimFrame(frame, animLayer);
    }

    f32 getAnimFrame(SimpleLayout* pLayout, u32 animLayer) {
        return pLayout->getAnimFrame(animLayer);
    }

    f32 getAnimFrame(LayoutActor* pLayout, u32 animLayer) {
        return pLayout->getAnimFrame(animLayer);
    }

    J3DFrameCtrl* getAnimCtrl(LayoutActor* pLayout, u32 animLayer) {
        return pLayout->getAnimCtrl(animLayer);
    }

    void setAnimRate(SimpleLayout* pLayout, f32 rate, u32 animLayer) {
        pLayout->setAnimRate(rate, animLayer);
    }

    void setAnimRate(LayoutActor* pLayout, f32 rate, u32 animLayer) {
        pLayout->setAnimRate(rate, animLayer);
    }

    void stopAnim(LayoutActor* pLayout, u32 animLayer) {
        if (pLayout != nullptr) {
            pLayout->setAnimRate(0.0F, animLayer);
        }
    }

    void replacePaneTexture(LayoutActor*, const char*, const nw4r::lyt::TexMap*, u8) {
    }

    void startAnimAtFirstStep(LayoutActor* pLayout, const char* pAnimName, u32 animLayer) {
        if (MR::isFirstStep(pLayout)) {
            MR::startAnim(pLayout, pAnimName, animLayer);
        }
    }

    void setAnimFrameAndStopAdjustTextHeight(LayoutActor* pLayout, const char*, u32 animLayer) {
        if (pLayout != nullptr) {
            pLayout->setAnimFrameAndStop(pLayout->getAnimFrame(animLayer), animLayer);
        }
    }

    void setTextBoxNumberRecursive(LayoutActor* pLayout, const char* pPaneName, s32 number) {
        pLayout->setTextBoxNumberRecursive(pPaneName, number);
    }

    void setTextBoxGameMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const char* pMessageId) {
        if (pLayout != nullptr) {
            pLayout->setTextBoxStringRecursive(pPaneName, utf16_from_utf8_lossy(runtime_message_or_tag(pMessageId)));
        }
    }

    void setTextBoxLayoutMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const char* pMessageId) {
        setTextBoxGameMessageRecursive(pLayout, pPaneName, pMessageId);
    }

    void setTextBoxSystemMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const char* pMessageId) {
        setTextBoxGameMessageRecursive(pLayout, pPaneName, pMessageId);
    }

    void setTextBoxMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const wchar_t* pMessage) {
        if (pLayout != nullptr) {
            pLayout->setTextBoxStringRecursive(pPaneName, utf16_from_wide(pMessage));
        }
    }

    void clearTextBoxMessageRecursive(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr) {
            pLayout->setTextBoxStringRecursive(pPaneName, std::u16string_view{});
        }
    }

    void setTextBoxArgNumberRecursive(LayoutActor* pLayout, const char* pPaneName, s32 number, s32) {
        setTextBoxNumberRecursive(pLayout, pPaneName, number);
    }

    void setTextBoxArgStringRecursive(LayoutActor* pLayout, const char* pPaneName, const wchar_t* pMessage, s32) {
        setTextBoxMessageRecursive(pLayout, pPaneName, pMessage);
    }

    void setTextBoxVerticalPositionCenterRecursive(LayoutActor*, const char*) {
    }

    void setTextBoxVerticalPositionBottomRecursive(LayoutActor*, const char*) {
    }

    void createAndAddPaneCtrl(LayoutActor* pLayout, const char* pPaneName, u32 animLayerNum) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->createAndAddPaneCtrl(pPaneName, animLayerNum);
        }
    }

    bool isExistPaneCtrl(LayoutActor* pLayout, const char* pPaneName) {
        return pLayout != nullptr && pLayout->getLayoutManager() != nullptr && pLayout->getLayoutManager()->isExistPaneCtrl(pPaneName);
    }

    void showPane(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->showPane(pPaneName);
        }
    }

    void showPaneRecursive(LayoutActor* pLayout, const char* pPaneName) {
        showPane(pLayout, pPaneName);
    }

    void hidePane(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->hidePane(pPaneName);
        }
    }

    void hidePaneRecursive(LayoutActor* pLayout, const char* pPaneName) {
        hidePane(pLayout, pPaneName);
    }

    void showLayout(LayoutActor* pLayout) {
        if (pLayout != nullptr) {
            pLayout->mFlag.mIsHidden = false;
        }
    }

    void hideLayout(LayoutActor* pLayout) {
        if (pLayout != nullptr) {
            pLayout->mFlag.mIsHidden = true;
        }
    }

    void setFollowPos(const TVec2f* pPos, LayoutActor* pLayout, const char*) {
        if (pLayout != nullptr && pPos != nullptr) {
            pLayout->setTrans(*pPos);
        }
    }

    void copyPaneTrans(TVec2f* pPos, const LayoutActor* pLayout, const char* pPaneName) {
        if (pPos != nullptr) {
            *pPos = {};
            const auto* layout = pLayout != nullptr ? pLayout->getSimpleLayout() : nullptr;
            const auto bounds = layout != nullptr ? layout->paneBounds(pPaneName != nullptr ? pPaneName : "") : std::optional< SimpleLayout::PaneBounds >{};
            if (bounds.has_value()) {
                pPos->x = (bounds->left + bounds->right) * 0.5F;
                pPos->y = (bounds->top + bounds->bottom) * 0.5F;
            }
        }
    }

    void setLayoutPosAtPaneTrans(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        auto pos = TVec2f{};
        copyPaneTrans(&pos, pSrc, pPaneName);
        setFollowPos(&pos, pDst, nullptr);
    }

    void setLayoutScalePosAtPaneScaleTransIfExecCalcAnim(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        setLayoutPosAtPaneTrans(pDst, pSrc, pPaneName);
    }

    void startPaneAnim(LayoutActor* pLayout, const char* pPaneName, const char* pAnimName, u32 animLayer) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->getPaneCtrl(pPaneName)->start(pAnimName, animLayer);
        }
    }

    void stopPaneAnim(LayoutActor* pLayout, const char* pPaneName, u32 animLayer) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->getPaneCtrl(pPaneName)->stop(animLayer);
        }
    }

    void setPaneAnimFrame(LayoutActor* pLayout, const char* pPaneName, f32 frame, u32 animLayer) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->getPaneCtrl(pPaneName)->setFrame(frame, animLayer);
        }
    }

    void setPaneAnimFrameAndStop(LayoutActor* pLayout, const char* pPaneName, f32 frame, u32 animLayer) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            auto* pane_ctrl = pLayout->getLayoutManager()->getPaneCtrl(pPaneName);
            pane_ctrl->setFrame(frame, animLayer);
            pane_ctrl->stop(animLayer);
        }
    }

    void setPaneAnimRate(LayoutActor* pLayout, const char* pPaneName, f32 rate, u32 animLayer) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->getPaneCtrl(pPaneName)->setRate(rate, animLayer);
        }
    }

    f32 getPaneAnimFrame(LayoutActor* pLayout, const char* pPaneName, u32 animLayer) {
        if (pLayout == nullptr || pLayout->getLayoutManager() == nullptr) {
            return 0.0F;
        }

        return pLayout->getLayoutManager()->getPaneAnimFrame(pPaneName, animLayer);
    }

    bool isPaneAnimStopped(LayoutActor* pLayout, const char* pPaneName, u32 animLayer) {
        return pLayout == nullptr || pLayout->getLayoutManager() == nullptr || pLayout->getLayoutManager()->isPaneAnimStopped(pPaneName, animLayer);
    }

    void setNerveAtPaneAnimStopped(LayoutActor* pLayout, const char* pPaneName, const Nerve* pNerve, u32 animLayer) {
        if (pLayout != nullptr && isPaneAnimStopped(pLayout, pPaneName, animLayer)) {
            pLayout->setNerve(pNerve);
        }
    }

    void setNerveAtAnimStopped(LayoutActor* pLayout, const Nerve* pNerve, u32 animLayer) {
        if (pLayout != nullptr && MR::isAnimStopped(pLayout, animLayer)) {
            pLayout->setNerve(pNerve);
        }
    }

    void killAtAnimStopped(LayoutActor* pLayout, u32 animLayer) {
        if (pLayout != nullptr && MR::isAnimStopped(pLayout, animLayer)) {
            pLayout->kill();
        }
    }

    s16 getAnimFrameMax(LayoutActor* pLayout, const char* pAnimName) {
        if (pLayout == nullptr || pLayout->getLayoutManager() == nullptr) {
            return 0;
        }

        return static_cast< s16 >(pLayout->getLayoutManager()->getAnimFrameMax(pAnimName));
    }

    void invalidateParentAnim(LayoutActor*) {
    }

    IconAButton* createAndSetupIconAButton(LayoutActor* pActor, bool connectToScene, bool connectToPause) {
        auto* icon = new IconAButton(connectToScene, connectToPause);
        icon->initWithoutIter();
        icon->setFollowActorPane(pActor, "AButtonPosition");
        return icon;
    }

    void emitEffect(SimpleLayout* pLayout, const char* pEffectName) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->emit_effect(pLayout->getName(), pEffectName);
        }
    }

    void emitEffect(LayoutActor* pLayout, const char* pEffectName) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->emit_effect(pLayout->getName(), pEffectName);
        }
    }

    void deleteEffectAll(SimpleLayout* pLayout) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->delete_effect_all(pLayout->getName());
        }
    }

    void deleteEffectAll(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->delete_effect_all(pLayout->getName());
        }
    }

}  // namespace MR
