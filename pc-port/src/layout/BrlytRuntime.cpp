#include "layout/BrlytRuntime.hpp"

#include "core/Logger.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace pcport {
namespace {

std::uint16_t ReadU16BE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data.at(offset)) << 8U) | static_cast<std::uint16_t>(data.at(offset + 1U)));
}

std::uint32_t ReadU32BE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return (static_cast<std::uint32_t>(data.at(offset)) << 24U) | (static_cast<std::uint32_t>(data.at(offset + 1U)) << 16U) |
           (static_cast<std::uint32_t>(data.at(offset + 2U)) << 8U) | static_cast<std::uint32_t>(data.at(offset + 3U));
}

float ReadF32BE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    const std::uint32_t raw = ReadU32BE(data, offset);
    return std::bit_cast<float>(raw);
}

std::string ReadCStringBounded(const std::vector<std::uint8_t>& data, std::size_t offset, std::size_t maxLen) {
    std::string out;
    out.reserve(maxLen);
    for (std::size_t i = 0; i < maxLen; ++i) {
        const char c = static_cast<char>(data.at(offset + i));
        if (c == '\0') {
            break;
        }
        out.push_back(c);
    }
    return out;
}

std::string ReadCString(const std::vector<std::uint8_t>& data, std::size_t offset) {
    std::string out;
    for (std::size_t i = offset; i < data.size(); ++i) {
        const char c = static_cast<char>(data[i]);
        if (c == '\0') {
            break;
        }
        out.push_back(c);
    }
    return out;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::vector<std::uint8_t> ReadAll(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open BRLYT: " + path.string());
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

std::array<std::uint8_t, 16> DefaultVertexColor() {
    std::array<std::uint8_t, 16> out{};
    out.fill(255);
    return out;
}

void ParsePaneCommon(const std::vector<std::uint8_t>& data, std::size_t blockOffset, PaneResource& pane) {
    const std::size_t p = blockOffset + 8U;
    const std::uint8_t flag = data.at(p + 0U);
    pane.basePosition = data.at(p + 1U);
    pane.base.alpha = data.at(p + 2U);
    pane.base.visible = (flag & 0x01U) != 0U;
    pane.name = ReadCStringBounded(data, p + 4U, 16U);

    pane.base.tx = ReadF32BE(data, p + 28U);
    pane.base.ty = ReadF32BE(data, p + 32U);
    pane.base.tz = ReadF32BE(data, p + 36U);
    pane.base.sx = ReadF32BE(data, p + 52U);
    pane.base.sy = ReadF32BE(data, p + 56U);
    pane.base.width = ReadF32BE(data, p + 60U);
    pane.base.height = ReadF32BE(data, p + 64U);
    pane.base.vertexColor = DefaultVertexColor();
    pane.current = pane.base;
}

ImageRGBA MakeFallbackTexture() {
    ImageRGBA image;
    image.width = 1;
    image.height = 1;
    image.pixels = {255, 255, 255, 255};
    return image;
}

}  // namespace

BrlytLayout BrlytLayout::LoadFromDirectory(const std::filesystem::path& layoutDir) {
    BrlytLayout layout;

    const std::filesystem::path blytDir = layoutDir / "blyt";
    if (!std::filesystem::exists(blytDir)) {
        throw std::runtime_error("Missing blyt directory: " + blytDir.string());
    }

    std::filesystem::path brlytPath;
    for (const auto& entry : std::filesystem::directory_iterator(blytDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".brlyt") {
            brlytPath = entry.path();
            break;
        }
    }

    if (brlytPath.empty()) {
        throw std::runtime_error("No .brlyt file found in " + blytDir.string());
    }

    Log(LogLevel::Info, LogCategory::Layout, "Loading BRLYT " + brlytPath.string());
    const auto data = ReadAll(brlytPath);
    if (data.size() < 0x10U) {
        throw std::runtime_error("BRLYT too small: " + brlytPath.string());
    }

    if (std::string(reinterpret_cast<const char*>(data.data()), 4) != "RLYT") {
        throw std::runtime_error("Invalid BRLYT signature: " + brlytPath.string());
    }

    const std::uint16_t blockCount = ReadU16BE(data, 0x0EU);

    std::unordered_map<std::string, std::filesystem::path> texturePathByLowerName;
    const std::filesystem::path timgDir = layoutDir / "timg";
    if (std::filesystem::exists(timgDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(timgDir)) {
            if (entry.is_regular_file()) {
                texturePathByLowerName.emplace(ToLower(entry.path().filename().string()), entry.path());
            }
        }
    }

    std::vector<int> parentStack;
    parentStack.push_back(-1);
    int lastPaneIndex = -1;

    std::size_t offset = 0x10U;
    for (std::uint16_t i = 0; i < blockCount; ++i) {
        if (offset + 8U > data.size()) {
            throw std::runtime_error("BRLYT block header out of range in " + brlytPath.string());
        }

        const std::string kind(reinterpret_cast<const char*>(&data[offset]), 4);
        const std::uint32_t blockSize = ReadU32BE(data, offset + 4U);
        if (blockSize < 8U || offset + blockSize > data.size()) {
            throw std::runtime_error("BRLYT invalid block size in " + brlytPath.string());
        }

        if (kind == "lyt1") {
            layout.mWidth = static_cast<int>(ReadF32BE(data, offset + 12U));
            layout.mHeight = static_cast<int>(ReadF32BE(data, offset + 16U));
        } else if (kind == "txl1") {
            const std::uint16_t textureCount = ReadU16BE(data, offset + 8U);
            const std::size_t texBase = offset + 12U;
            layout.mTextures.reserve(layout.mTextures.size() + textureCount);

            for (std::uint16_t texIndex = 0; texIndex < textureCount; ++texIndex) {
                const std::size_t texEntry = texBase + static_cast<std::size_t>(texIndex) * 8U;
                const std::uint32_t nameOffset = ReadU32BE(data, texEntry + 0U);
                const std::string textureName = ReadCString(data, texBase + nameOffset);
                const std::string lookupName = ToLower(textureName);

                ImageRGBA decoded = MakeFallbackTexture();
                const auto it = texturePathByLowerName.find(lookupName);
                if (it != texturePathByLowerName.end()) {
                    decoded = TplDecoder::DecodeFile(it->second);
                } else {
                    Log(LogLevel::Warn, LogCategory::Layout,
                        "Texture not found in timg: " + textureName + " for " + brlytPath.string());
                }

                layout.mTextures.emplace_back(std::move(decoded));
            }
        } else if (kind == "mat1") {
            const std::uint16_t materialCount = ReadU16BE(data, offset + 8U);
            layout.mMaterials.reserve(layout.mMaterials.size() + materialCount);

            for (std::uint16_t matIndex = 0; matIndex < materialCount; ++matIndex) {
                const std::uint32_t matOffset = ReadU32BE(data, offset + 12U + static_cast<std::size_t>(matIndex) * 4U);
                const std::size_t matStart = offset + matOffset;

                MaterialResource material;
                material.name = ReadCStringBounded(data, matStart, 20U);
                const std::uint32_t resNumBits = ReadU32BE(data, matStart + 60U);
                const std::uint8_t texMapCount = static_cast<std::uint8_t>((resNumBits >> 0U) & 0x0FU);
                const std::uint8_t texSrtCount = static_cast<std::uint8_t>((resNumBits >> 4U) & 0x0FU);
                const std::uint8_t texCoordGenCount = static_cast<std::uint8_t>((resNumBits >> 8U) & 0x0FU);
                const std::uint8_t chanCtrlCount = static_cast<std::uint8_t>((resNumBits >> 25U) & 0x01U);
                const std::uint8_t matColorCount = static_cast<std::uint8_t>((resNumBits >> 27U) & 0x01U);
                material.tevStageCount = static_cast<int>((resNumBits >> 18U) & 0x1FU);

                std::size_t resourceOffset = 64U;
                material.textureIndices.reserve(texMapCount);
                for (std::uint8_t texMapIndex = 0; texMapIndex < texMapCount; ++texMapIndex) {
                    const std::uint16_t texIndex = ReadU16BE(data, matStart + resourceOffset + static_cast<std::size_t>(texMapIndex) * 4U);
                    if (texIndex != 0xFFFFU) {
                        material.textureIndices.push_back(static_cast<int>(texIndex));
                    }
                }
                resourceOffset += static_cast<std::size_t>(texMapCount) * 4U;
                resourceOffset += static_cast<std::size_t>(texSrtCount) * 20U;
                resourceOffset += static_cast<std::size_t>(texCoordGenCount) * 4U;
                resourceOffset += static_cast<std::size_t>(chanCtrlCount) * 4U;

                if (!material.textureIndices.empty()) {
                    material.firstTextureIndex = material.textureIndices.front();
                }

                if (matColorCount > 0U) {
                    const std::uint32_t matColor = ReadU32BE(data, matStart + resourceOffset);
                    material.matColor = {
                        static_cast<std::uint8_t>((matColor >> 24U) & 0xFFU),
                        static_cast<std::uint8_t>((matColor >> 16U) & 0xFFU),
                        static_cast<std::uint8_t>((matColor >> 8U) & 0xFFU),
                        static_cast<std::uint8_t>(matColor & 0xFFU),
                    };
                }

                layout.mMaterials.emplace_back(std::move(material));
            }
        } else if (kind == "pan1" || kind == "pic1" || kind == "txt1" || kind == "bnd1") {
            PaneResource pane;
            ParsePaneCommon(data, offset, pane);

            if (kind == "pan1") {
                pane.type = PaneType::Pane;
            } else if (kind == "pic1") {
                pane.type = PaneType::Picture;
            } else if (kind == "txt1") {
                pane.type = PaneType::TextBox;
            } else {
                pane.type = PaneType::Bounding;
            }

            pane.uv = {0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F};

            if (pane.type == PaneType::Picture) {
                const std::size_t pictureOffset = offset + 8U + 68U;
                for (int v = 0; v < 4; ++v) {
                    const std::uint32_t color = ReadU32BE(data, pictureOffset + static_cast<std::size_t>(v) * 4U);
                    pane.base.vertexColor[static_cast<std::size_t>(v) * 4U + 0U] = static_cast<std::uint8_t>((color >> 24U) & 0xFFU);
                    pane.base.vertexColor[static_cast<std::size_t>(v) * 4U + 1U] = static_cast<std::uint8_t>((color >> 16U) & 0xFFU);
                    pane.base.vertexColor[static_cast<std::size_t>(v) * 4U + 2U] = static_cast<std::uint8_t>((color >> 8U) & 0xFFU);
                    pane.base.vertexColor[static_cast<std::size_t>(v) * 4U + 3U] = static_cast<std::uint8_t>(color & 0xFFU);
                }
                pane.current.vertexColor = pane.base.vertexColor;

                pane.materialIndex = static_cast<int>(ReadU16BE(data, pictureOffset + 16U));
                const std::uint8_t texCoordCount = data.at(pictureOffset + 18U);
                if (texCoordCount > 0U) {
                    const std::size_t uvBase = offset + 96U;
                    pane.uv = {
                        ReadF32BE(data, uvBase + 0U),  ReadF32BE(data, uvBase + 4U),  ReadF32BE(data, uvBase + 8U),
                        ReadF32BE(data, uvBase + 12U), ReadF32BE(data, uvBase + 16U), ReadF32BE(data, uvBase + 20U),
                        ReadF32BE(data, uvBase + 24U), ReadF32BE(data, uvBase + 28U),
                    };
                }
            } else if (pane.type == PaneType::TextBox) {
                pane.materialIndex = static_cast<int>(ReadU16BE(data, offset + 8U + 72U));
            }

            pane.parent = parentStack.back();
            const int paneIndex = static_cast<int>(layout.mPanes.size());
            layout.mPanes.emplace_back(std::move(pane));
            layout.mPaneByName.emplace(layout.mPanes.back().name, paneIndex);
            if (layout.mPanes.back().parent >= 0) {
                layout.mPanes[layout.mPanes.back().parent].children.push_back(paneIndex);
            } else if (layout.mRootPaneIndex < 0) {
                layout.mRootPaneIndex = paneIndex;
            }

            lastPaneIndex = paneIndex;
        } else if (kind == "pas1") {
            if (lastPaneIndex >= 0) {
                parentStack.push_back(lastPaneIndex);
            }
        } else if (kind == "pae1") {
            if (parentStack.size() > 1U) {
                parentStack.pop_back();
            }
        }

        offset += blockSize;
    }

    if (layout.mRootPaneIndex < 0 && !layout.mPanes.empty()) {
        layout.mRootPaneIndex = 0;
    }

    Log(LogLevel::Info, LogCategory::Layout,
        "Loaded BRLYT panes=" + std::to_string(layout.mPanes.size()) + " materials=" + std::to_string(layout.mMaterials.size()) +
            " textures=" + std::to_string(layout.mTextures.size()));

    return layout;
}

void BrlytLayout::ResetAnimationState() {
    for (auto& pane : mPanes) {
        pane.current = pane.base;
    }
}

PaneResource* BrlytLayout::FindPane(std::string_view name) {
    const auto it = mPaneByName.find(std::string(name));
    if (it == mPaneByName.end()) {
        return nullptr;
    }
    return &mPanes[it->second];
}

const PaneResource* BrlytLayout::FindPane(std::string_view name) const {
    const auto it = mPaneByName.find(std::string(name));
    if (it == mPaneByName.end()) {
        return nullptr;
    }
    return &mPanes[it->second];
}

const MaterialResource* BrlytLayout::GetMaterial(int index) const {
    if (index < 0 || index >= static_cast<int>(mMaterials.size())) {
        return nullptr;
    }
    return &mMaterials[index];
}

const ImageRGBA* BrlytLayout::GetTexture(int index) const {
    if (index < 0 || index >= static_cast<int>(mTextures.size())) {
        return nullptr;
    }
    return &mTextures[index];
}

int BrlytLayout::GetWidth() const {
    return mWidth;
}

int BrlytLayout::GetHeight() const {
    return mHeight;
}

int BrlytLayout::GetRootPaneIndex() const {
    return mRootPaneIndex;
}

const std::vector<PaneResource>& BrlytLayout::GetPanes() const {
    return mPanes;
}

}  // namespace pcport
