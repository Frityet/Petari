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
#include "Game/compat/TplTexture.hpp"
#include "RendererService.hpp"

class SimpleLayout {
public:
    SimpleLayout(const char* pName, const char* pLayoutName, u32 animLayerNum, int drawType);
    virtual ~SimpleLayout();

    void initWithoutIter();
    void initEffectKeeper(int effectNum, const char* pEffectName, const void* pSystem);
    void appear();
    void kill();
    void update();

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

    // SMGPC debug
    [[nodiscard]] std::size_t debugAnimLayerCount() const;
    [[nodiscard]] std::string_view debugAnimName(u32 animLayer) const;
    [[nodiscard]] f32 debugAnimEndFrame(u32 animLayer) const;
    [[nodiscard]] f32 debugAnimRate(u32 animLayer) const;
    [[nodiscard]] bool debugAnimLooping(u32 animLayer) const;
    [[nodiscard]] bool debugAnimStopped(u32 animLayer) const;

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
        float alpha = 255.0F;
        bool visible = true;
    };

    [[nodiscard]] AnimationState& animation(u32 animLayer);
    [[nodiscard]] const AnimationState& animation(u32 animLayer) const;
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
    std::optional< std::filesystem::path > mArchivePath;
    std::array< AnimationState, 4 > mAnimations = {};
    bool mRenderDataLoaded = false;
    smgpc::game::BrlytLayout mBrlytLayout = {};
    std::vector< RenderTexture > mRenderTextures = {};
    std::vector< RenderFont > mRenderFonts = {};
    std::vector< RenderTextTexture > mRenderTextTextures = {};
    std::unordered_map< std::string, smgpc::game::BrlanAnimation > mRenderAnimations = {};
    std::unordered_map< std::string, smgpc::game::BrlanPaneFrame > mCommittedPaneFrames = {};
};

class SimpleEffectLayout : public SimpleLayout {
public:
    SimpleEffectLayout(const char* pName, const char* pLayoutName, u32 animLayerNum, int drawType);
};
