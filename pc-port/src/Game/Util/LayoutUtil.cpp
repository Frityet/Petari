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
#include "layout/LayoutHost.hpp"
#include "layout/LayoutRuntime.hpp"
#include "layout/LytTexMap.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TplTexture.hpp"
#include "runtime/RuntimeContext.hpp"

namespace {
    [[nodiscard]] std::u16string utf16_from_wide(const wchar_t* pText) {
        if (pText == nullptr) {
            throw std::invalid_argument("Layout text conversion requires real source text");
        }

        auto text = std::u16string{};
        while (*pText != L'\0') {
            const auto code = static_cast< char32_t >(*pText++);
            text.push_back(static_cast< char16_t >(std::min< char32_t >(code, 0xffffU)));
        }

        return text;
    }

    [[nodiscard]] std::u16string runtime_message(const char* pMessageId) {
        if (pMessageId == nullptr || *pMessageId == '\0') {
            throw std::invalid_argument("Layout message lookup requires a real message tag");
        }
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            throw std::logic_error("Layout message lookup requires the active message archive");
        }
        const auto* message = runtime->messages().message_utf16(pMessageId);
        if (message == nullptr) {
            throw std::runtime_error("Layout message does not exist: " + std::string(pMessageId));
        }
        return *message;
    }

    [[nodiscard]] std::u16string runtime_raw_message(const char* pMessageId) {
        if (pMessageId == nullptr || *pMessageId == '\0') {
            throw std::invalid_argument("Raw layout message lookup requires a real message tag");
        }
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            throw std::logic_error("Raw layout message lookup requires the active message archive");
        }
        const auto* message = runtime->messages().message_raw_utf16(pMessageId);
        if (message == nullptr) {
            throw std::runtime_error("Raw layout message does not exist: " + std::string(pMessageId));
        }
        return *message;
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

    [[nodiscard]] LayoutManager& require_layout_manager(LayoutActor* layout, std::string_view operation) {
        if (layout == nullptr || layout->getLayoutManager() == nullptr) {
            throw std::logic_error(std::string(operation) + " requires an initialized layout manager");
        }
        return *layout->getLayoutManager();
    }

    [[nodiscard]] const LayoutManager& require_layout_manager(const LayoutActor* layout, std::string_view operation) {
        if (layout == nullptr || layout->getLayoutManager() == nullptr) {
            throw std::logic_error(std::string(operation) + " requires an initialized layout manager");
        }
        return *layout->getLayoutManager();
    }

    [[nodiscard]] LayoutPaneCtrl& require_pane_ctrl(LayoutActor* layout, const char* paneName, std::string_view operation) {
        auto& manager = require_layout_manager(layout, operation);
        auto* pane = manager.getPaneCtrl(paneName);
        if (pane == nullptr) {
            throw std::runtime_error(std::string(operation) + " requires a real pane control");
        }
        return *pane;
    }
}  // namespace

namespace MR {

    bool isDead(const SimpleLayout* pLayout) {
        return smgpc::layout::is_layout_actor_dead(pLayout);
    }

    bool isDead(const LayoutActor* pLayout) {
        return smgpc::layout::is_layout_actor_dead(pLayout);
    }

    void startAnim(SimpleLayout* pLayout, const char* pAnimName, u32 animLayer) {
        smgpc::layout::start_layout_anim(pLayout, pAnimName, animLayer);
    }

    void startAnim(LayoutActor* pLayout, const char* pAnimName, u32 animLayer) {
        smgpc::layout::start_layout_anim(pLayout, pAnimName, animLayer);
    }

    bool isAnimStopped(SimpleLayout* pLayout, u32 animLayer) {
        return smgpc::layout::is_layout_anim_stopped(pLayout, animLayer);
    }

    bool isAnimStopped(LayoutActor* pLayout, u32 animLayer) {
        return smgpc::layout::is_layout_anim_stopped(pLayout, animLayer);
    }

    void setAnimFrameAndStop(SimpleLayout* pLayout, f32 frame, u32 animLayer) {
        smgpc::layout::set_layout_anim_frame_and_stop(pLayout, frame, animLayer);
    }

    void setAnimFrameAndStop(LayoutActor* pLayout, f32 frame, u32 animLayer) {
        smgpc::layout::set_layout_anim_frame_and_stop(pLayout, frame, animLayer);
    }

    void setAnimFrame(SimpleLayout* pLayout, f32 frame, u32 animLayer) {
        smgpc::layout::set_layout_anim_frame(pLayout, frame, animLayer);
    }

    void setAnimFrame(LayoutActor* pLayout, f32 frame, u32 animLayer) {
        smgpc::layout::set_layout_anim_frame(pLayout, frame, animLayer);
    }

    f32 getAnimFrame(SimpleLayout* pLayout, u32 animLayer) {
        return smgpc::layout::layout_anim_frame(pLayout, animLayer);
    }

    f32 getAnimFrame(LayoutActor* pLayout, u32 animLayer) {
        return smgpc::layout::layout_anim_frame(pLayout, animLayer);
    }

    J3DFrameCtrl* getAnimCtrl(LayoutActor* pLayout, u32 animLayer) {
        return smgpc::layout::layout_anim_ctrl(pLayout, animLayer);
    }

    void setAnimRate(SimpleLayout* pLayout, f32 rate, u32 animLayer) {
        smgpc::layout::set_layout_anim_rate(pLayout, rate, animLayer);
    }

    void setAnimRate(LayoutActor* pLayout, f32 rate, u32 animLayer) {
        smgpc::layout::set_layout_anim_rate(pLayout, rate, animLayer);
    }

    void stopAnim(LayoutActor* pLayout, u32 animLayer) {
        smgpc::layout::set_layout_anim_rate(pLayout, 0.0F, animLayer);
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
        const auto* entry = archive.find_by_basename(pTextureName);
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
        if (pTexMap == nullptr) {
            throw std::invalid_argument("Replacing a pane texture requires a real texture");
        }
        smgpc::layout::replace_pane_texture(&require_layout_manager(pLayout, "Replacing a pane texture"), pPaneName,
                                            *pTexMap, texMapIndex);
    }

    void startAnimAtFirstStep(LayoutActor* pLayout, const char* pAnimName, u32 animLayer) {
        if (MR::isFirstStep(pLayout)) {
            MR::startAnim(pLayout, pAnimName, animLayer);
        }
    }

    void setAnimFrameAndStopAdjustTextHeight(LayoutActor* pLayout, const char*, u32 animLayer) {
        smgpc::layout::set_layout_anim_frame_and_stop(pLayout, smgpc::layout::layout_anim_frame(pLayout, animLayer), animLayer);
    }

    void setTextBoxNumberRecursive(LayoutActor* pLayout, const char* pPaneName, s32 number) {
        smgpc::layout::set_text_box_number(pLayout, pPaneName, number);
    }

    void setTextBoxGameMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const char* pMessageId) {
        smgpc::layout::set_text_box_tagged_string(&require_layout_manager(pLayout, "Setting a game message"), pPaneName,
                                                  runtime_raw_message(pMessageId), runtime_message(pMessageId));
    }

    void setTextBoxLayoutMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const char* pMessageId) {
        setTextBoxGameMessageRecursive(pLayout, pPaneName, pMessageId);
    }

    void setTextBoxSystemMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const char* pMessageId) {
        setTextBoxGameMessageRecursive(pLayout, pPaneName, pMessageId);
    }

    void setTextBoxMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const wchar_t* pMessage) {
        smgpc::layout::set_text_box_string(pLayout, pPaneName, utf16_from_wide(pMessage));
    }

    void clearTextBoxMessageRecursive(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_text_box_string(pLayout, pPaneName, std::u16string_view{});
    }

    void setTextBoxArgNumberRecursive(LayoutActor* pLayout, const char* pPaneName, s32 number, s32 argIndex) {
        smgpc::layout::set_text_box_arg_number(&require_layout_manager(pLayout, "Setting a text-box argument"),
                                               pPaneName, number, argIndex);
    }

    void setTextBoxArgStringRecursive(LayoutActor* pLayout, const char* pPaneName, const wchar_t* pMessage, s32 argIndex) {
        smgpc::layout::set_text_box_arg_string(&require_layout_manager(pLayout, "Setting a text-box argument"),
                                               pPaneName, utf16_from_wide(pMessage), argIndex);
    }

    void setTextBoxHorizontalPositionCenterRecursive(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_text_box_horizontal_position(
            &require_layout_manager(pLayout, "Setting text-box horizontal position"), pPaneName, 1U);
    }

    void setTextBoxHorizontalPositionLeftRecursive(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_text_box_horizontal_position(
            &require_layout_manager(pLayout, "Setting text-box horizontal position"), pPaneName, 0U);
    }

    void setTextBoxVerticalPositionTopRecursive(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_text_box_vertical_position(
            &require_layout_manager(pLayout, "Setting text-box vertical position"), pPaneName, 0U);
    }

    void setTextBoxVerticalPositionCenterRecursive(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_text_box_vertical_position(
            &require_layout_manager(pLayout, "Setting text-box vertical position"), pPaneName, 1U);
    }

    void setTextBoxVerticalPositionBottomRecursive(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_text_box_vertical_position(
            &require_layout_manager(pLayout, "Setting text-box vertical position"), pPaneName, 2U);
    }

    void createAndAddPaneCtrl(LayoutActor* pLayout, const char* pPaneName, u32 animLayerNum) {
        require_layout_manager(pLayout, "Creating a pane control").createAndAddPaneCtrl(pPaneName, animLayerNum);
    }

    bool isExistPaneCtrl(LayoutActor* pLayout, const char* pPaneName) {
        return pLayout != nullptr && pLayout->getLayoutManager() != nullptr && pLayout->getLayoutManager()->isExistPaneCtrl(pPaneName);
    }

    void showPane(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_pane_visible(&require_layout_manager(pLayout, "Showing a pane"), pPaneName, true, false);
    }

    void showPaneRecursive(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_pane_visible(&require_layout_manager(pLayout, "Showing a pane tree"), pPaneName, true, true);
    }

    void hidePane(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_pane_visible(&require_layout_manager(pLayout, "Hiding a pane"), pPaneName, false, false);
    }

    void hidePaneRecursive(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_pane_visible(&require_layout_manager(pLayout, "Hiding a pane tree"), pPaneName, false, true);
    }

    void setPaneAlphaFloat(LayoutActor* pLayout, const char* pPaneName, f32 alpha) {
        smgpc::layout::set_pane_alpha(&require_layout_manager(pLayout, "Setting pane alpha"), pPaneName, alpha);
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
            throw std::invalid_argument("Screen-to-layout conversion requires output storage");
        }

        const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
        pLayoutPos->x = rScreenPos.x * static_cast< f32 >(smgpc::render::core::kWiiLayoutWidth) /
                            static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) -
                        static_cast< f32 >(smgpc::render::core::kWiiLayoutWidth) * 0.5F;
        pLayoutPos->y = -(rScreenPos.y - half_height);
    }

    void convertLayoutPosToScreenPos(TVec2f* pScreenPos, const TVec2f& rLayoutPos) {
        if (pScreenPos == nullptr) {
            throw std::invalid_argument("Layout-to-screen conversion requires output storage");
        }

        const auto half_width = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F;
        const auto half_height = static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
        pScreenPos->x = rLayoutPos.x * static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) /
                            static_cast< f32 >(smgpc::render::core::kWiiLayoutWidth) +
                        half_width;
        pScreenPos->y = half_height - rLayoutPos.y;
    }

    void setFollowPos(const TVec2f* pPos, LayoutActor* pLayout, const char*) {
        if (pLayout == nullptr || pPos == nullptr) {
            throw std::invalid_argument("Setting a layout follow position requires a layout and position");
        }
        auto screen_pos = TVec2f{};
        convertLayoutPosToScreenPos(&screen_pos, *pPos);
        pLayout->setTrans(screen_pos);
    }

    void copyPaneTrans(TVec2f* pPos, const LayoutActor* pLayout, const char* pPaneName) {
        if (pPos == nullptr) {
            throw std::invalid_argument("Copying a pane translation requires output storage");
        }
        const auto& layout = smgpc::layout::require_layout_runtime(pLayout, "Copying a pane translation");
        const auto pane_name = pPaneName != nullptr ? std::string_view(pPaneName) : std::string_view{};
        Mtx pane_matrix{};
        if (!layout.copyPaneMatrix(pane_name, pane_matrix)) {
            throw std::runtime_error("Layout " + layout.getLayoutName() + " has no pane " + std::string(pane_name));
        }
        pPos->x = pane_matrix[0][3];
        pPos->y = pane_matrix[1][3];
    }

    void copyPaneScale(TVec2f* pScale, const LayoutActor* pLayout, const char* pPaneName) {
        if (pScale == nullptr) {
            throw std::invalid_argument("Copying a pane scale requires output storage");
        }

        const auto& layout = smgpc::layout::require_layout_runtime(pLayout, "Copying a pane scale");
        const auto scale = layout.paneScale(pPaneName != nullptr ? pPaneName : "");
        if (!scale.has_value()) {
            throw std::runtime_error("Cannot copy scale from an absent layout pane");
        }
        *pScale = *scale;
    }

    void setLayoutScaleAtPaneScale(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        auto scale = TVec2f{};
        copyPaneScale(&scale, pSrc, pPaneName);
        smgpc::layout::set_layout_scale(pDst, scale.x, scale.y);
    }

    void setLayoutPosAtPaneTrans(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        auto pos = TVec2f{};
        copyPaneTrans(&pos, pSrc, pPaneName);
        setFollowPos(&pos, pDst, nullptr);
    }

    void setLayoutScalePosAtPaneScaleTrans(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        setLayoutPosAtPaneTrans(pDst, pSrc, pPaneName);
        setLayoutScaleAtPaneScale(pDst, pSrc, pPaneName);
    }

    void setLayoutScalePosAtPaneScaleTransIfExecCalcAnim(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        setLayoutPosAtPaneTrans(pDst, pSrc, pPaneName);
        setLayoutScaleAtPaneScale(pDst, pSrc, pPaneName);
    }

    void startPaneAnim(LayoutActor* pLayout, const char* pPaneName, const char* pAnimName, u32 animLayer) {
        require_pane_ctrl(pLayout, pPaneName, "Starting a pane animation").start(pAnimName, animLayer);
    }

    void stopPaneAnim(LayoutActor* pLayout, const char* pPaneName, u32 animLayer) {
        require_pane_ctrl(pLayout, pPaneName, "Stopping a pane animation").stop(animLayer);
    }

    void setPaneAnimFrame(LayoutActor* pLayout, const char* pPaneName, f32 frame, u32 animLayer) {
        smgpc::layout::set_pane_anim_frame(&require_pane_ctrl(pLayout, pPaneName, "Setting a pane animation frame"),
                                           frame, animLayer);
    }

    void setPaneAnimFrameAndStop(LayoutActor* pLayout, const char* pPaneName, f32 frame, u32 animLayer) {
        auto& pane_ctrl = require_pane_ctrl(pLayout, pPaneName, "Stopping at a pane animation frame");
        smgpc::layout::set_pane_anim_frame(&pane_ctrl, frame, animLayer);
        pane_ctrl.stop(animLayer);
    }

    void setPaneAnimRate(LayoutActor* pLayout, const char* pPaneName, f32 rate, u32 animLayer) {
        smgpc::layout::set_pane_anim_rate(&require_pane_ctrl(pLayout, pPaneName, "Setting a pane animation rate"),
                                          rate, animLayer);
    }

    f32 getPaneAnimFrame(LayoutActor* pLayout, const char* pPaneName, u32 animLayer) {
        return smgpc::layout::pane_animation_frame(&require_layout_manager(pLayout, "Reading a pane animation frame"),
                                                   pPaneName, animLayer);
    }

    s16 getPaneAnimFrameMax(const LayoutActor* pLayout, const char* pPaneName, u32 animLayer) {
        return static_cast< s16 >(smgpc::layout::pane_animation_frame_max(
            &require_layout_manager(pLayout, "Reading a pane animation duration"), pPaneName, animLayer));
    }

    bool isPaneAnimStopped(LayoutActor* pLayout, const char* pPaneName, u32 animLayer) {
        return smgpc::layout::is_pane_animation_stopped(
            &require_layout_manager(pLayout, "Reading a pane animation state"), pPaneName, animLayer);
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
        return static_cast< s16 >(smgpc::layout::animation_duration(
            &require_layout_manager(pLayout, "Reading layout animation metadata"), pAnimName));
    }

    s16 getAnimFrameMax(LayoutActor* pLayout, u32 animLayer) {
        return static_cast< s16 >(smgpc::layout::layout_anim_frame_max(pLayout, animLayer));
    }

    void startAnimReverseOneTime(LayoutActor* pLayout, const char* pAnimName, u32 animLayer) {
        MR::startAnim(pLayout, pAnimName, animLayer);
        MR::setAnimFrame(pLayout, static_cast< f32 >(MR::getAnimFrameMax(pLayout, animLayer)), animLayer);
        MR::setAnimRate(pLayout, -1.0F, animLayer);
    }

    void invalidateParentAnim(LayoutActor*) {
        throw std::logic_error("Invalidating a parent NW4R layout animation is unavailable");
    }

    IconAButton* createAndSetupIconAButton(LayoutActor* pActor, bool connectToScene, bool connectToPause) {
        auto* icon = new IconAButton(connectToScene, connectToPause);
        icon->initWithoutIter();
        icon->setFollowActorPane(pActor, "AButtonPosition");
        return icon;
    }

    void emitEffect(SimpleLayout* pLayout, const char* pEffectName) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_effect(pLayout->getName(), pEffectName, pLayout);
        }
    }

    void emitEffect(LayoutActor* pLayout, const char* pEffectName) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_effect(pLayout->getName(), pEffectName, pLayout);
        }
    }

    void deleteEffectAll(SimpleLayout* pLayout) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->delete_effect_all(pLayout->getName(), pLayout);
        }
    }

    void deleteEffectAll(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->delete_effect_all(pLayout->getName(), pLayout);
        }
    }

}  // namespace MR
