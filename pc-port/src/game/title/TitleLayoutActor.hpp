#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "layout/Brfnt.hpp"
#include "TitleAssets.hpp"
#include "layout/LayoutDrawList.hpp"

namespace smgpc::game::title {

class TitleLayoutActor {
public:
    explicit TitleLayoutActor(const TitleLayoutResource *resource);

    void appear();
    void kill();
    [[nodiscard]] bool isDead() const;

    void startAnim(const char *pAnimName, std::uint32_t layer);
    [[nodiscard]] bool isAnimStopped(std::uint32_t layer) const;
    void setAnimFrameAndStop(float frame, std::uint32_t layer);

    void update(float deltaFrames);

    void emitEffect(const char *);
    void deleteEffectAll();

    void appendDrawCommands(
        render::layout::LayoutDrawList *pDrawList,
        const std::unordered_map<std::string, assets::layout::BrfntFont> &rFontsByName) const;

private:
    struct RuntimePaneState {
        bool visible {true};
        std::uint8_t alpha {255U};
        std::array<std::uint8_t, 16> vertexColor {};

        float tx {};
        float ty {};
        float tz {};
        float sx {1.0F};
        float sy {1.0F};
        float width {};
        float height {};

        float texOffsetU {};
        float texOffsetV {};
        float texScaleU {1.0F};
        float texScaleV {1.0F};
    };

    struct AnimationSlot {
        const assets::layout::BrlanAnimation *animation {};
        float frame {};
        bool paused {};
    };

    struct TransformContext {
        float originX {};
        float originY {};
        float scaleX {1.0F};
        float scaleY {1.0F};
        float alpha {1.0F};
        bool visible {true};
    };

    [[nodiscard]] static std::string normalizeName(std::string name);
    [[nodiscard]] static float anchorOffsetX(std::uint8_t basePosition, float width);
    [[nodiscard]] static float anchorOffsetY(std::uint8_t basePosition, float height);
    [[nodiscard]] static std::uint8_t clampU8(float value);

    [[nodiscard]] const assets::layout::PaneDefinition *paneDefinition(std::size_t index) const;
    [[nodiscard]] const RuntimePaneState *paneState(std::size_t index) const;

    void resetPose();
    void rebuildPose();
    void applyAnimation(const assets::layout::BrlanAnimation &animation, float frame);

    void applyPicture(
        std::size_t paneIndex,
        const TransformContext &parent,
        render::layout::LayoutDrawList *pDrawList) const;

    void applyText(
        std::size_t paneIndex,
        const TransformContext &parent,
        render::layout::LayoutDrawList *pDrawList,
        const std::unordered_map<std::string, assets::layout::BrfntFont> &rFontsByName) const;

    void appendPaneRecursive(
        std::size_t paneIndex,
        const TransformContext &parent,
        render::layout::LayoutDrawList *pDrawList,
        const std::unordered_map<std::string, assets::layout::BrfntFont> &rFontsByName) const;

    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolveTextureByLayoutIndex(std::int32_t textureIndex) const;
    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolveTextureForMaterial(std::int32_t materialIndex) const;
    [[nodiscard]] static bool hasVariableAlpha(const assets::layout::tpl::DecodedImage &image);
    void rebuildPicLogoGalaxyComposite(const assets::layout::tpl::DecodedImage &baseTexture, const assets::layout::tpl::DecodedImage &maskTexture) const;

    const TitleLayoutResource *mResource {};

    bool mIsAlive {};
    std::array<AnimationSlot, 2> mAnimationLayers {};

    std::vector<RuntimePaneState> mBaseStates {};
    std::vector<RuntimePaneState> mCurrentStates {};
    std::unordered_map<std::string, std::size_t> mPaneIndexByName {};

    mutable assets::layout::tpl::DecodedImage mPicLogoGalaxyComposite {};
    mutable const assets::layout::tpl::DecodedImage *mPicLogoGalaxyBaseTexture {};
    mutable const assets::layout::tpl::DecodedImage *mPicLogoGalaxyMaskTexture {};
};

}  // namespace smgpc::game::title
