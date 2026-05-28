#include "Game/Util/LayoutUtil.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Game/Screen/IconAButton.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutActorFlag.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "core/RenderTypes.hpp"
#include "layout/LytTexMap.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TextEncoding.hpp"
#include "resource/TplTexture.hpp"
#include "runtime/RuntimeContext.hpp"

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

    [[nodiscard]] std::u16string runtime_message_or_tag(const char* pMessageId) {
        const auto tag = pMessageId != nullptr ? std::string_view(pMessageId) : std::string_view{};
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            return runtime->messages().message_utf16_or(tag, smgpc::resource::utf16_from_utf8_lossy(tag));
        }
        return smgpc::resource::utf16_from_utf8_lossy(tag);
    }

    [[nodiscard]] std::u16string runtime_raw_message_or_tag(const char* pMessageId) {
        const auto tag = pMessageId != nullptr ? std::string_view(pMessageId) : std::string_view{};
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            return runtime->messages().message_raw_utf16_or(tag, smgpc::resource::utf16_from_utf8_lossy(tag));
        }
        return smgpc::resource::utf16_from_utf8_lossy(tag);
    }

    [[nodiscard]] bool ends_with(std::string_view text, std::string_view suffix) {
        return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
    }

    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast< char >(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] std::string base_name(std::string_view path) {
        const auto slash = path.find_last_of('/');
        if (slash == std::string_view::npos) {
            return std::string(path);
        }
        return std::string(path.substr(slash + 1U));
    }

    [[nodiscard]] std::string archive_file_name(std::string_view archiveName) {
        auto name = base_name(archiveName);
        if (!ends_with(lower_copy(name), ".arc")) {
            name.append(".arc");
        }
        return name;
    }

    [[nodiscard]] std::optional< std::filesystem::path > find_layout_texture_archive(smgpc::runtime::RuntimeContext& runtime,
                                                                                     std::string_view archiveName) {
        const auto archive = archive_file_name(archiveName);
        return runtime.dvd().find_first({
            std::filesystem::path(archiveName),
            std::filesystem::path("KrKorean") / "LayoutData" / archive,
            std::filesystem::path("LayoutData") / archive,
            std::filesystem::path("ObjectData") / archive,
        });
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

    nw4r::lyt::TexMap* createLytTexMap(const char* pArchiveName, const char* pTextureName) {
        if (pArchiveName == nullptr || pTextureName == nullptr) {
            throw std::runtime_error("MR::createLytTexMap requires archive and texture names");
        }

        auto& runtime = smgpc::runtime::RuntimeContext::instance();
        const auto archive_path = find_layout_texture_archive(runtime, pArchiveName);
        if (!archive_path.has_value()) {
            throw std::runtime_error("Layout texture archive does not exist: " + std::string(pArchiveName));
        }

        const auto& archive = runtime.dvd().archive_for_path(*archive_path);
        const auto* entry = find_entry_by_basename(archive, pTextureName);
        if (entry == nullptr) {
            throw std::runtime_error("Layout texture does not exist: " + std::string(pTextureName));
        }

        const auto entry_name = lower_copy(base_name(entry->path));
        if (ends_with(entry_name, ".tpl")) {
            return new nw4r::lyt::TexMap(entry_name, smgpc::resource::decode_tpl_texture(archive.file_data(*entry)), 0U, 0U, 0U, 0U);
        }

        const auto bti = smgpc::resource::decode_bti_texture(archive.file_data(*entry));
        return new nw4r::lyt::TexMap(entry_name, bti.image, bti.wrap_s, bti.wrap_t, bti.min_filter, bti.mag_filter);
    }

    void replacePaneTexture(LayoutActor* pLayout, const char* pPaneName, const nw4r::lyt::TexMap* pTexMap, u8 texMapIndex) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr && pTexMap != nullptr) {
            pLayout->getSimpleLayout()->replacePaneTexture(pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{}, *pTexMap,
                                                           texMapIndex);
        }
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
            pLayout->setTextBoxStringRecursive(pPaneName, runtime_message_or_tag(pMessageId));
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

    void setTextBoxArgNumberRecursive(LayoutActor* pLayout, const char* pPaneName, s32 number, s32 argIndex) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr) {
            pLayout->getSimpleLayout()->setTextBoxArgNumberRecursive(pPaneName, number, argIndex);
        }
    }

    void setTextBoxArgStringRecursive(LayoutActor* pLayout, const char* pPaneName, const wchar_t* pMessage, s32 argIndex) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr) {
            pLayout->getSimpleLayout()->setTextBoxArgStringRecursive(pPaneName, utf16_from_wide(pMessage), argIndex);
        }
    }

    void setTextBoxHorizontalPositionCenterRecursive(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr) {
            pLayout->getSimpleLayout()->setTextBoxHorizontalPosition(pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{}, 1U);
        }
    }

    void setTextBoxHorizontalPositionLeftRecursive(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr) {
            pLayout->getSimpleLayout()->setTextBoxHorizontalPosition(pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{}, 0U);
        }
    }

    void setTextBoxVerticalPositionTopRecursive(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr) {
            pLayout->getSimpleLayout()->setTextBoxVerticalPosition(pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{}, 0U);
        }
    }

    void setTextBoxVerticalPositionCenterRecursive(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr) {
            pLayout->getSimpleLayout()->setTextBoxVerticalPosition(pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{}, 1U);
        }
    }

    void setTextBoxVerticalPositionBottomRecursive(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr) {
            pLayout->getSimpleLayout()->setTextBoxVerticalPosition(pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{}, 2U);
        }
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
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->showPaneRecursive(pPaneName);
        }
    }

    void hidePane(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->hidePane(pPaneName);
        }
    }

    void hidePaneRecursive(LayoutActor* pLayout, const char* pPaneName) {
        if (pLayout != nullptr && pLayout->getLayoutManager() != nullptr) {
            pLayout->getLayoutManager()->hidePaneRecursive(pPaneName);
        }
    }

    void setPaneAlphaFloat(LayoutActor* pLayout, const char* pPaneName, f32 alpha) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr) {
            pLayout->getSimpleLayout()->setPaneAlpha(pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{}, alpha);
        }
    }

    void setPaneAlphaFloat(LayoutActor* pLayout, const char* pPaneName, f32 alpha) {
        if (pLayout != nullptr && pLayout->getSimpleLayout() != nullptr) {
            pLayout->getSimpleLayout()->setPaneAlpha(pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{}, alpha);
        }
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

    void convertScreenPosToLayoutPos(TVec2f* pLayoutPos, const TVec2f& rScreenPos) {
        if (pLayoutPos == nullptr) {
            return;
        }

        const auto half_width = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F;
        const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
        pLayoutPos->x = rScreenPos.x - half_width;
        pLayoutPos->y = rScreenPos.y - half_height;
    }

    void convertLayoutPosToScreenPos(TVec2f* pScreenPos, const TVec2f& rLayoutPos) {
        if (pScreenPos == nullptr) {
            return;
        }

        const auto half_width = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F;
        const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
        pScreenPos->x = rLayoutPos.x + half_width;
        pScreenPos->y = rLayoutPos.y + half_height;
    }

    void setFollowPos(const TVec2f* pPos, LayoutActor* pLayout, const char*) {
        if (pLayout != nullptr && pPos != nullptr) {
            auto screen_pos = TVec2f{};
            convertLayoutPosToScreenPos(&screen_pos, *pPos);
            pLayout->setTrans(screen_pos);
        }
    }

    void copyPaneTrans(TVec2f* pPos, const LayoutActor* pLayout, const char* pPaneName) {
        if (pPos != nullptr) {
            *pPos = {};
            const auto* layout = pLayout != nullptr ? pLayout->getSimpleLayout() : nullptr;
            const auto bounds =
                layout != nullptr ? layout->paneBounds(pPaneName != nullptr ? pPaneName : "") : std::optional< SimpleLayout::PaneBounds >{};
            if (bounds.has_value()) {
                pPos->x = (bounds->left + bounds->right) * 0.5F;
                pPos->y = (bounds->top + bounds->bottom) * 0.5F;
            }
        }
    }

    void copyPaneScale(TVec2f* pScale, const LayoutActor* pLayout, const char* pPaneName) {
        if (pScale == nullptr) {
            return;
        }

        *pScale = TVec2f{1.0F, 1.0F};
        const auto* layout = pLayout != nullptr ? pLayout->getSimpleLayout() : nullptr;
        const auto scale = layout != nullptr ? layout->paneScale(pPaneName != nullptr ? pPaneName : "") : std::optional< TVec2f >{};
        if (scale.has_value()) {
            *pScale = *scale;
        }
    }

    void setLayoutScaleAtPaneScale(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        if (pDst == nullptr || pDst->getSimpleLayout() == nullptr) {
            return;
        }

        auto scale = TVec2f{};
        copyPaneScale(&scale, pSrc, pPaneName);
        pDst->getSimpleLayout()->setScale(scale.x, scale.y);
    }

    void setLayoutPosAtPaneTrans(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        auto pos = TVec2f{};
        copyPaneTrans(&pos, pSrc, pPaneName);
        setFollowPos(&pos, pDst, nullptr);
    }

    void setLayoutScalePosAtPaneScaleTrans(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        setLayoutPosAtPaneTrans(pDst, pSrc, pPaneName);
    }

    void setLayoutScalePosAtPaneScaleTransIfExecCalcAnim(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        setLayoutPosAtPaneTrans(pDst, pSrc, pPaneName);
        setLayoutScaleAtPaneScale(pDst, pSrc, pPaneName);
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

    s16 getPaneAnimFrameMax(const LayoutActor* pLayout, const char* pPaneName, u32 animLayer) {
        if (pLayout == nullptr || pLayout->getLayoutManager() == nullptr) {
            return 0;
        }

        return static_cast< s16 >(pLayout->getLayoutManager()->getPaneAnimFrameMax(pPaneName, animLayer));
    }

    bool isPaneAnimStopped(LayoutActor* pLayout, const char* pPaneName, u32 animLayer) {
        return pLayout == nullptr || pLayout->getLayoutManager() == nullptr || pLayout->getLayoutManager()->isPaneAnimStopped(pPaneName, animLayer);
    }

    bool isLessStep(const LayoutActor* pActor, s32 step) {
        return pActor != nullptr && pActor->getNerveStep() >= 0 && pActor->getNerveStep() < step;
    }

    bool isGreaterEqualStep(const LayoutActor* pActor, s32 step) {
        return pActor != nullptr && pActor->getNerveStep() >= step;
    }

    f32 calcNerveRate(const LayoutActor* pActor, s32 stepMax) {
        if (pActor == nullptr || stepMax <= 0) {
            return 1.0F;
        }

        return std::clamp(static_cast< f32 >(pActor->getNerveStep()) / static_cast< f32 >(stepMax), 0.0F, 1.0F);
    }

    f32 calcNerveRate(const LayoutActor* pActor, s32 stepMin, s32 stepMax) {
        if (pActor == nullptr || stepMax <= stepMin) {
            return 1.0F;
        }

        return std::clamp(static_cast< f32 >(pActor->getNerveStep() - stepMin) / static_cast< f32 >(stepMax - stepMin), 0.0F, 1.0F);
    }

    void setNerveAtStep(LayoutActor* pLayout, const Nerve* pNerve, s32 step) {
        if (pLayout != nullptr && pLayout->getNerveStep() == step) {
            pLayout->setNerve(pNerve);
        }
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

    s16 getAnimFrameMax(LayoutActor* pLayout, u32 animLayer) {
        if (pLayout == nullptr || pLayout->getSimpleLayout() == nullptr) {
            return 0;
        }

        return static_cast< s16 >(pLayout->getSimpleLayout()->getAnimFrameMax(animLayer));
    }

    void startAnimReverseOneTime(LayoutActor* pLayout, const char* pAnimName, u32 animLayer) {
        if (pLayout == nullptr) {
            return;
        }

        MR::startAnim(pLayout, pAnimName, animLayer);
        MR::setAnimFrame(pLayout, static_cast< f32 >(MR::getAnimFrameMax(pLayout, animLayer)), animLayer);
        MR::setAnimRate(pLayout, -1.0F, animLayer);
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
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_effect(pLayout->getName(), pEffectName);
        }
    }

    void emitEffect(LayoutActor* pLayout, const char* pEffectName) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_effect(pLayout->getName(), pEffectName);
        }
    }

    void deleteEffectAll(SimpleLayout* pLayout) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->delete_effect_all(pLayout->getName());
        }
    }

    void deleteEffectAll(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->delete_effect_all(pLayout->getName());
        }
    }

}  // namespace MR
