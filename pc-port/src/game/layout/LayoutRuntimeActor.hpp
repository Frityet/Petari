#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "compat/LayoutTextureCompat.hpp"
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
    void setAnimFrame(float frame, std::uint32_t layer);
    void setAnimRate(float rate, std::uint32_t layer);
    [[nodiscard]] float getAnimFrame(std::uint32_t layer) const;
    [[nodiscard]] float getAnimRate(std::uint32_t layer) const;
    [[nodiscard]] float getAnimFrameMax(std::uint32_t layer) const;
    [[nodiscard]] float getAnimFrameMax(const char *pAnimName) const;

    void update(float deltaFrames);

    void emitEffect(const char *);
    void deleteEffectAll();

    void setRootTranslation(float x, float y);
    void setPaneVisible(const char *pPaneName, bool visible);
    void setPaneVisibleRecursive(const char *pPaneName, bool visible);
    void setTextBoxTextRecursive(const char *pPaneName, std::u16string text);
    void clearTextBoxTextRecursive(const char *pPaneName);
    void setTextBoxVerticalPositionRecursive(const char *pPaneName, std::uint8_t position);
    [[nodiscard]] bool getPaneTrans(const char *pPaneName, float *pX, float *pY) const;
    [[nodiscard]] bool getPaneBounds(const char *pPaneName, float *pX0, float *pY0, float *pX1, float *pY1) const;
    [[nodiscard]] bool hasPane(const char *pPaneName) const;
    void startPaneAnim(const char *pPaneName, const char *pAnimName, std::uint32_t slotIndex);
    [[nodiscard]] bool isPaneAnimStopped(const char *pPaneName, std::uint32_t slotIndex) const;
    void setPaneAnimFrame(const char *pPaneName, float frame, std::uint32_t slotIndex);
    [[nodiscard]] float getPaneAnimFrame(const char *pPaneName, std::uint32_t slotIndex) const;
    void setPaneAnimRate(const char *pPaneName, float rate, std::uint32_t slotIndex);

    void appendDrawCommands(render::layout::LayoutDrawList *pDrawList) const;

    [[nodiscard]] const LayoutArchiveData *resource() const;

    void setPaneFollowPosition(const char *pPaneName, const float *pX, const float *pY);
    void replacePaneTexture(const char *pPaneName, const nw4r::lyt::TexMap *pTexMap, std::uint8_t slot);
    [[nodiscard]] nw4r::lyt::TexMap *getPaneTexture(const char *pPaneName, std::uint8_t slot) const;

private:
    struct RuntimePaneState {
        bool visible {true};
        std::uint8_t alpha {255U};
        std::array<std::uint8_t, 16> vertexColor {};

        float tx {};
        float ty {};
        float tz {};
        float rz {};
        float sx {1.0F};
        float sy {1.0F};
        float width {};
        float height {};
        std::uint8_t textPosition {};

        std::array<float, 4> texOffsetU {};
        std::array<float, 4> texOffsetV {};
        std::array<float, 4> texRotate {};
        std::array<float, 4> texScaleU {1.0F, 1.0F, 1.0F, 1.0F};
        std::array<float, 4> texScaleV {1.0F, 1.0F, 1.0F, 1.0F};
        std::array<std::optional<std::int32_t>, 4> texturePattern {};
        std::array<std::optional<std::string>, 4> texturePatternName {};
        std::array<const nw4r::lyt::TexMap *, 4> textureOverrides {};
    };

    struct AnimationSlot {
        const assets::layout::BrlanAnimation *animation {};
        float frame {};
        float rate {1.0F};
    };

    struct PaneAnimationSlot {
        std::string pane_name {};
        std::uint32_t slot_index {};
        const assets::layout::BrlanAnimation *animation {};
        float frame {};
        float rate {1.0F};
    };

    struct TransformContext {
        float originX {};
        float originY {};
        float scaleX {1.0F};
        float scaleY {1.0F};
        float rotationZ {};
        float alpha {1.0F};
        bool visible {true};
    };

    struct WindowFrameSize {
        float left {};
        float right {};
        float top {};
        float bottom {};
    };

    struct FollowPosition {
        const float *x {};
        const float *y {};
    };

    [[nodiscard]] static std::string normalizeName(std::string name);
    [[nodiscard]] static float anchorOffsetX(std::uint8_t basePosition, float width);
    [[nodiscard]] static float anchorOffsetY(std::uint8_t basePosition, float height);
    [[nodiscard]] static std::uint8_t clampU8(float value);

    [[nodiscard]] const assets::layout::PaneDefinition *paneDefinition(std::size_t index) const;
    [[nodiscard]] const RuntimePaneState *paneState(std::size_t index) const;
    [[nodiscard]] FollowPosition followPosition(std::size_t paneIndex, const assets::layout::PaneDefinition &pane) const;
    [[nodiscard]] PaneAnimationSlot *findPaneAnimationSlot(const std::string &paneName, std::uint32_t slotIndex);
    [[nodiscard]] const PaneAnimationSlot *findPaneAnimationSlot(const std::string &paneName, std::uint32_t slotIndex) const;
    [[nodiscard]] std::vector<std::size_t> paneIndicesForVisibilityName(const char *pPaneName) const;
    [[nodiscard]] bool hasLocalizedVisibilityVariant(const std::string &paneName) const;
    [[nodiscard]] bool shouldRespectLanguageVisibility(std::size_t paneIndex) const;
    [[nodiscard]] bool paneIsInSubtree(std::size_t paneIndex, std::size_t rootPaneIndex) const;
    void setPaneVisibleAtIndex(std::size_t paneIndex, bool visible, bool respectLanguageVisibility);
    void setPaneVisibleInSubtree(std::size_t paneIndex, bool visible, bool respectLanguageVisibility);
    void setTextBoxTextInSubtree(std::size_t paneIndex, const std::u16string &text, bool *pChanged);
    void setTextBoxVerticalPositionInSubtree(std::size_t paneIndex, std::uint8_t position, bool *pChanged);

    void resetPose();
    void rebuildPose();
    void applyAnimation(const assets::layout::BrlanAnimation &animation, float frame, const std::string *rootPaneName);
    void applyAnimationToStates(
        const assets::layout::BrlanAnimation &animation,
        float frame,
        const std::string *rootPaneName,
        std::vector<RuntimePaneState> *pStates) const;
    void commitAnimationSlot(const AnimationSlot &slot);
    void commitPaneAnimationSlot(const PaneAnimationSlot &slot);

    void applyPicture(
        std::size_t paneIndex,
        const TransformContext &parent,
        render::layout::LayoutDrawList *pDrawList,
        std::int32_t materialIndexOverride = -1) const;

    void applyMaterialQuad(
        std::size_t paneIndex,
        const TransformContext &parent,
        render::layout::LayoutDrawList *pDrawList,
        std::int32_t materialIndex,
        float localX,
        float localY,
        float localWidth,
        float localHeight,
        const std::array<float, 8> &texCoords,
        const std::array<std::uint8_t, 16> &vertexColor,
        bool usePaneTextureTransform) const;

    void applyWindow(
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

    [[nodiscard]] bool getPaneTransRecursive(
        std::size_t paneIndex,
        const TransformContext &parent,
        const std::string &paneName,
        float *pX,
        float *pY) const;

    [[nodiscard]] bool getPaneBoundsRecursive(
        std::size_t paneIndex,
        const TransformContext &parent,
        const std::string &paneName,
        float *pX0,
        float *pY0,
        float *pX1,
        float *pY1) const;

    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolveTextureByLayoutIndex(std::int32_t textureIndex) const;
    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolveTextureByName(std::string_view textureName) const;
    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolveTextureForMaterial(std::int32_t materialIndex) const;
    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolveTexturePatternByLayoutIndex(std::int32_t textureIndex, std::int32_t patternIndex) const;
    [[nodiscard]] const assets::layout::tpl::DecodedImage *resolveTextureForMaterialStage(std::int32_t materialIndex, std::size_t stage, const RuntimePaneState &state) const;
    [[nodiscard]] const std::u16string &textForPane(std::size_t paneIndex) const;
    [[nodiscard]] bool chooseColorAndMaskTextures(
        std::int32_t materialIndex,
        const RuntimePaneState &state,
        const assets::layout::tpl::DecodedImage **colorTexture,
        std::size_t *colorStage,
        const assets::layout::tpl::DecodedImage **maskTexture,
        std::size_t *maskStage,
        bool *invertMask,
        bool *maskUsesAlpha) const;

    std::shared_ptr<const LayoutArchiveData> mResource {};

    bool mIsAlive {};
    float mRootTx {};
    float mRootTy {};
    std::array<AnimationSlot, 2> mAnimationLayers {};
    std::vector<PaneAnimationSlot> mPaneAnimationSlots {};

    std::vector<RuntimePaneState> mBaseStates {};
    std::vector<RuntimePaneState> mCommittedStates {};
    std::vector<RuntimePaneState> mCurrentStates {};
    mutable std::vector<std::array<std::unique_ptr<nw4r::lyt::TexMap>, 4>> mPaneTextureCache {};
    std::vector<bool> mLanguageVisibleStates {};
    std::vector<std::optional<std::u16string>> mTextOverrides {};
    std::unordered_map<std::string, std::size_t> mPaneIndexByName {};
    std::unordered_map<std::string, FollowPosition> mFollowPositions {};
};

}  // namespace smgpc::game::layout
