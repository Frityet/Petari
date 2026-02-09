#pragma once

#include "render/TplDecoder.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pcport {

enum class PaneType {
    Pane,
    Picture,
    TextBox,
    Bounding,
};

struct MaterialResource {
    std::string name;
    int firstTextureIndex = -1;
    std::vector<int> textureIndices;
    std::array<std::uint8_t, 4> matColor{255, 255, 255, 255};
    int tevStageCount = 0;
};

struct PaneState {
    bool visible = true;
    std::uint8_t alpha = 255;
    std::array<std::uint8_t, 16> vertexColor{};
    float tx = 0.0F;
    float ty = 0.0F;
    float tz = 0.0F;
    float sx = 1.0F;
    float sy = 1.0F;
    float width = 0.0F;
    float height = 0.0F;
    float texOffsetU = 0.0F;
    float texOffsetV = 0.0F;
    float texScaleU = 1.0F;
    float texScaleV = 1.0F;
};

struct PaneResource {
    std::string name;
    PaneType type = PaneType::Pane;
    int parent = -1;
    std::vector<int> children;
    std::uint8_t basePosition = 4;

    int materialIndex = -1;
    std::array<float, 8> uv{};

    PaneState base;
    PaneState current;
};

class BrlytLayout {
public:
    static BrlytLayout LoadFromDirectory(const std::filesystem::path& layoutDir);

    void ResetAnimationState();

    PaneResource* FindPane(std::string_view name);
    const PaneResource* FindPane(std::string_view name) const;

    const MaterialResource* GetMaterial(int index) const;
    const ImageRGBA* GetTexture(int index) const;

    int GetWidth() const;
    int GetHeight() const;
    int GetRootPaneIndex() const;

    const std::vector<PaneResource>& GetPanes() const;

private:
    int mWidth = 608;
    int mHeight = 456;
    int mRootPaneIndex = -1;

    std::vector<ImageRGBA> mTextures;
    std::vector<MaterialResource> mMaterials;
    std::vector<PaneResource> mPanes;
    std::unordered_map<std::string, int> mPaneByName;
};

}  // namespace pcport
