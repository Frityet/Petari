#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "LayoutArchiveLoader.hpp"
#include "layout/LayoutDrawList.hpp"

namespace smgpc::game::layout {

class LayoutRuntimeActor {
public:
    explicit LayoutRuntimeActor(std::shared_ptr<const LayoutArchiveData> resource);

    void appear();
    void kill();
    [[nodiscard]] bool isDead() const;

    void startAnim(const char *pAnimName, std::uint32_t layer);
    [[nodiscard]] bool isAnimStopped(std::uint32_t layer) const;
    void setAnimFrameAndStop(float frame, std::uint32_t layer);

    void update(float deltaFrames);

    void emitEffect(const char *);
    void deleteEffectAll();

    void appendDrawCommands(render::layout::LayoutDrawList *pDrawList) const;

    [[nodiscard]] const LayoutArchiveData *resource() const;

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

        std::array<float, 4> texOffsetU {};
        std::array<float, 4> texOffsetV {};
        std::array<float, 4> texScaleU {1.0F, 1.0F, 1.0F, 1.0F};
        std::array<float, 4> texScaleV {1.0F, 1.0F, 1.0F, 1.0F};
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
        render::layout::LayoutDrawList *pDrawList) const;

    void appendPaneRecursive(
        std::size_t paneIndex,
        const TransformContext &parent,
        render::layout::LayoutDrawList *pDrawList) const;

    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolveTextureByLayoutIndex(std::int32_t textureIndex) const;
    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolveTextureForMaterial(std::int32_t materialIndex) const;
    [[nodiscard]] bool chooseColorAndMaskTextures(
        std::int32_t materialIndex,
        const assets::layout::tpl::DecodedImage **colorTexture,
        std::size_t *colorStage,
        const assets::layout::tpl::DecodedImage **maskTexture,
        std::size_t *maskStage,
        bool *invertMask,
        bool *maskUsesAlpha) const;

    std::shared_ptr<const LayoutArchiveData> mResource {};

    bool mIsAlive {};
    std::array<AnimationSlot, 2> mAnimationLayers {};

    std::vector<RuntimePaneState> mBaseStates {};
    std::vector<RuntimePaneState> mCurrentStates {};
    std::unordered_map<std::string, std::size_t> mPaneIndexByName {};
};

}  // namespace smgpc::game::layout
