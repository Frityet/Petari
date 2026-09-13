#include "Game/Util/LayoutUtil.hpp"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cwchar>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Game/Effect/MultiEmitter.hpp"
#include "Game/Screen/IconAButton.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/PaneEffectKeeper.hpp"
#include "Game/Screen/LayoutActorFlag.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "core/RenderTypes.hpp"
#include "layout/LayoutHost.hpp"
#include "layout/LayoutRuntime.hpp"
#include "layout/LytTexMap.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TplTexture.hpp"

namespace {
    [[nodiscard]] LayoutManager& require_layout_manager(LayoutActor* layout, std::string_view operation) {
        if (layout == nullptr || layout->getLayoutManager() == nullptr) {
            throw std::logic_error(std::string(operation) + " requires an initialized layout manager");
        }
        return *layout->getLayoutManager();
    }

}  // namespace

namespace MR {

    void startAnim(LayoutActor* pLayout, const char* pAnimName, u32 animLayer) {
        smgpc::layout::start_layout_anim(pLayout, pAnimName, animLayer);
    }

    nw4r::lyt::TexMap* createLytTexMap(const char* pArchiveName, const char* pTextureName) {
        if (!pArchiveName || !pTextureName)
            throw std::invalid_argument("MR::createLytTexMap requires archive and texture names");
        auto* service = smgpc::compat::ResourceHolderService::active();
        if (!service)
            throw std::logic_error("MR::createLytTexMap requires the actual resource holder owner");
        return service->create_layout_texture(pArchiveName, pTextureName);
    }


    void showPane(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_pane_visible(&require_layout_manager(pLayout, "Showing a pane"), pPaneName, true, false);
    }

    void hidePane(LayoutActor* pLayout, const char* pPaneName) {
        smgpc::layout::set_pane_visible(&require_layout_manager(pLayout, "Hiding a pane"), pPaneName, false, false);
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

    void setLayoutScalePosAtPaneScaleTrans(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        setLayoutPosAtPaneTrans(pDst, pSrc, pPaneName);
        setLayoutScaleAtPaneScale(pDst, pSrc, pPaneName);
    }

    void setLayoutScalePosAtPaneScaleTransIfExecCalcAnim(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName) {
        setLayoutPosAtPaneTrans(pDst, pSrc, pPaneName);
        setLayoutScaleAtPaneScale(pDst, pSrc, pPaneName);
    }

}  // namespace MR
