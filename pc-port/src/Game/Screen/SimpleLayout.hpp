#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <revolution.h>

#include "Game/compat/BrfntFont.hpp"
#include "Game/compat/BrlanAnimation.hpp"
#include "Game/compat/BrlytLayout.hpp"
#include "Game/compat/BmgMessageArchive.hpp"
#include "Game/compat/TplTexture.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "RendererService.hpp"

namespace nw4r::lyt {
    class TexMap;
}

class SimpleLayout {
public:
    struct PaneBounds {
        f32 left = 0.0F;
        f32 top = 0.0F;
        f32 right = 0.0F;
        f32 bottom = 0.0F;
    };

    SimpleLayout(const char* pName, const char* pLayoutName, u32 animLayerNum, int drawType);
    virtual ~SimpleLayout();

    void initWithoutIter();
    void initEffectKeeper(int effectNum, const char* pEffectName, const void* pSystem);
    void appear();
    void kill();
    void update();
    void setTrans(f32 x, f32 y);
    void setScale(f32 x, f32 y);

    [[nodiscard]] bool isDead() const;
    [[nodiscard]] const std::string& getName() const;
    [[nodiscard]] const std::string& getLayoutName() const;
    [[nodiscard]] const std::optional< std::filesystem::path >& getArchivePath() const;

    void draw(smgpc::render::IRendererEngine& renderer);

    void startAnim(const char* pAnimName, u32 animLayer);
    void setAnimFrameAndStop(f32 frame, u32 animLayer);
    void setAnimFrame(f32 frame, u32 animLayer);
    void setAnimRate(f32 rate, u32 animLayer);
    [[nodiscard]] f32 getAnimFrame(u32 animLayer) const;
    [[nodiscard]] bool isAnimStopped(u32 animLayer);
    void setTextBoxNumberRecursive(const char* pPaneName, s32 number);
    void setTextBoxStringRecursive(const char* pPaneName, std::u16string_view text);
    void replacePaneTexture(std::string_view paneName, const nw4r::lyt::TexMap& texMap, u8 texMapIndex);
    void setPaneAlpha(std::string_view paneName, f32 alpha);
    void setPaneVisible(std::string_view paneName, bool visible);
    void setTextBoxHorizontalPosition(std::string_view paneName, u8 position);
    void setTextBoxVerticalPosition(std::string_view paneName, u8 position);
    [[nodiscard]] bool isPaneVisible(std::string_view paneName) const;
    [[nodiscard]] bool hasPane(std::string_view paneName) const;
    [[nodiscard]] std::optional< PaneBounds > paneBounds(std::string_view paneName) const;
    [[nodiscard]] std::optional< TVec2f > paneScale(std::string_view paneName) const;
    [[nodiscard]] bool copyPaneMatrix(std::string_view paneName, Mtx matrix) const;
    [[nodiscard]] bool isPointingPane(std::string_view paneName, f32 screenX, f32 screenY) const;
    void startPaneAnim(std::string_view paneName, const char* pAnimName, u32 animLayer);
    void stopPaneAnim(std::string_view paneName, u32 animLayer);
    void setPaneAnimFrame(std::string_view paneName, f32 frame, u32 animLayer);
    void setPaneAnimRate(std::string_view paneName, f32 rate, u32 animLayer);
    [[nodiscard]] f32 getPaneAnimFrame(std::string_view paneName, u32 animLayer) const;
    [[nodiscard]] bool isPaneAnimStopped(std::string_view paneName, u32 animLayer) const;
    [[nodiscard]] f32 getAnimFrameMax(u32 animLayer) const;
    [[nodiscard]] f32 getAnimRate(u32 animLayer) const;
    [[nodiscard]] f32 getAnimDuration(const char* pAnimName) const;
    [[nodiscard]] bool isAnimLooping(const char* pAnimName) const;
    [[nodiscard]] bool isAnimLooping(u32 animLayer) const;

#ifndef NDEBUG
    // SMGPC debug
    [[nodiscard]] f32 debugPaneAnimEndFrame(std::string_view paneName, u32 animLayer) const;
    [[nodiscard]] std::size_t debugAnimLayerCount() const;
    [[nodiscard]] std::string_view debugAnimName(u32 animLayer) const;
    [[nodiscard]] f32 debugAnimDuration(const char* pAnimName) const;
    [[nodiscard]] bool debugAnimLooping(const char* pAnimName) const;
    [[nodiscard]] f32 debugAnimEndFrame(u32 animLayer) const;
    [[nodiscard]] f32 debugAnimRate(u32 animLayer) const;
    [[nodiscard]] bool debugAnimLooping(u32 animLayer) const;
    [[nodiscard]] bool debugAnimStopped(u32 animLayer) const;
    [[nodiscard]] std::size_t debugPaneCount() const;
    [[nodiscard]] std::size_t debugPictureCount() const;
    [[nodiscard]] std::size_t debugTextBoxCount() const;
    [[nodiscard]] std::size_t debugMaterialCount() const;
    [[nodiscard]] std::size_t debugTextureCount() const;
    [[nodiscard]] std::size_t debugFontCount() const;
    [[nodiscard]] std::size_t debugCommittedPaneFrameCount() const;

    struct DebugPaneContentState {
        std::string kind;
        std::string name;
        s32 material_index = -1;
        std::string material_name;
        std::string texture_name;
        std::string font_name;
        bool visible = true;
    };

    struct DebugPaneState {
        std::size_t index = 0U;
        std::string name;
        s32 parent_index = -1;
        bool base_visible = true;
        bool effective_visible = true;
        f32 translate_x = 0.0F;
        f32 translate_y = 0.0F;
        f32 scale_x = 1.0F;
        f32 scale_y = 1.0F;
        f32 alpha = 255.0F;
        f32 width = 0.0F;
        f32 height = 0.0F;
        std::vector< DebugPaneContentState > contents;
    };

    struct DebugMaterialTextureState {
        std::size_t slot = 0U;
        std::uint16_t texture_index = 0U;
        std::string texture_name;
        std::uint8_t wrap_s = 0U;
        std::uint8_t wrap_t = 0U;
        std::uint8_t min_filter = 0U;
        std::uint8_t mag_filter = 0U;
    };

    struct DebugMaterialState {
        std::size_t index = 0U;
        std::string name;
        std::size_t texture_count = 0U;
        std::size_t tex_coord_gen_count = 0U;
        std::size_t tev_stage_count = 0U;
        bool alpha_compare_enabled = false;
        bool blend_enabled = false;
        std::vector< DebugMaterialTextureState > textures;
    };

    struct DebugTextureState {
        std::size_t index = 0U;
        std::string name;
        std::uint16_t width = 0U;
        std::uint16_t height = 0U;
        std::uint32_t format_raw = 0U;
        std::string format_name;
        bool uploaded = false;
        std::size_t rgba_byte_count = 0U;
    };

    [[nodiscard]] std::vector< DebugPaneState > debugPanes() const;
    [[nodiscard]] std::vector< DebugMaterialState > debugMaterials() const;
    [[nodiscard]] std::vector< DebugTextureState > debugTextures() const;
#endif

    struct RenderTexture {
        std::string name;
        smgpc::game::DecodedTexture decoded;
        smgpc::render::TextureHandle handle{};
    };

    struct RenderFont {
        std::string name;
        smgpc::game::BrfntFont font;
        std::vector< smgpc::render::TextureHandle > sheet_handles = {};
    };

    struct RenderTextTexture {
        std::size_t text_box_index = 0U;
        std::uint16_t width = 0U;
        std::uint16_t height = 0U;
        std::uint16_t font_width = 1U;
        std::uint16_t font_height = 1U;
        std::vector< std::uint8_t > rgba = {};
        smgpc::render::TextureHandle handle = {};
    };

private:
    struct AnimationState {
        std::string name;
        f32 frame = 0.0f;
        f32 end = 1.0f;
        f32 rate = 1.0f;
        bool stopped = true;
        bool looping = false;
    };

    struct PaneRenderState {
        float translate_x = 0.0F;
        float translate_y = 0.0F;
        float scale_x = 1.0F;
        float scale_y = 1.0F;
        float rotate_z = 0.0F;
        float m00 = 1.0F;
        float m01 = 0.0F;
        float m10 = 0.0F;
        float m11 = 1.0F;
        float alpha = 255.0F;
        bool visible = true;
    };

    struct PaneAnimationState {
        std::string pane_name;
        std::array< AnimationState, 4 > animations = {};
    };

    struct TextBoxTemplateState {
        std::u16string raw_text;
        std::vector< smgpc::game::BmgFormatArg > args;
    };

    [[nodiscard]] AnimationState& animation(u32 animLayer);
    [[nodiscard]] const AnimationState& animation(u32 animLayer) const;
    [[nodiscard]] PaneAnimationState& paneAnimation(std::string_view paneName);
    [[nodiscard]] const PaneAnimationState* findPaneAnimation(std::string_view paneName) const;
    void commitAnimationState(const AnimationState& anim);
    void loadRenderData();
    void ensureTextureUploads(smgpc::render::IRendererEngine& renderer);
    void ensureTextTextureUploads(smgpc::render::IRendererEngine& renderer);
    [[nodiscard]] RenderTextTexture composeTextTexture(std::size_t text_box_index, const RenderFont& font) const;
    void drawTextBoxes(smgpc::render::IRendererEngine& renderer, float alpha);
    [[nodiscard]] PaneRenderState paneRenderState(std::size_t pane_index) const;
    [[nodiscard]] smgpc::game::BrlanPaneFrame animationFrameForPane(std::string_view pane_name) const;
    [[nodiscard]] smgpc::game::BrlanTextureFrame textureFrameForContent(std::string_view content_name) const;
    [[nodiscard]] f32 durationFor(const char* pAnimName) const;
    [[nodiscard]] bool isLoopingAnim(const char* pAnimName) const;
    [[nodiscard]] float visualAlpha() const;
    [[nodiscard]] static f32 duration_for(const char* pAnimName);
    [[nodiscard]] static bool is_looping_anim(const char* pAnimName);

    std::string mName;
    std::string mLayoutName;
    u32 mAnimLayerNum;
    bool mIsDead = true;
    f32 mTransX = 0.0F;
    f32 mTransY = 0.0F;
    f32 mScaleX = 1.0F;
    f32 mScaleY = 1.0F;
    std::optional< std::filesystem::path > mArchivePath;
    std::array< AnimationState, 4 > mAnimations = {};
    bool mRenderDataLoaded = false;
    smgpc::game::BrlytLayout mBrlytLayout = {};
    std::vector< RenderTexture > mRenderTextures = {};
    std::vector< RenderFont > mRenderFonts = {};
    std::vector< RenderTextTexture > mRenderTextTextures = {};
    std::unordered_map< std::string, smgpc::game::BrlanAnimation > mRenderAnimations = {};
    std::unordered_map< std::string, smgpc::game::BrlanPaneFrame > mCommittedPaneFrames = {};
    std::unordered_map< std::string, bool > mPaneVisibilityOverrides = {};
    std::unordered_map< std::string, f32 > mPaneAlphaOverrides = {};
    std::vector< PaneAnimationState > mPaneAnimations = {};
};

class SimpleEffectLayout : public SimpleLayout {
public:
    SimpleEffectLayout(const char* pName, const char* pLayoutName, u32 animLayerNum, int drawType);
};
