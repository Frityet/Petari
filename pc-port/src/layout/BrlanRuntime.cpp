#include "layout/BrlanRuntime.hpp"

#include "core/Logger.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace pcport {
namespace {

std::uint16_t ReadU16BE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data.at(offset)) << 8U) | static_cast<std::uint16_t>(data.at(offset + 1U)));
}

std::int16_t ReadS16BE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::int16_t>(ReadU16BE(data, offset));
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
        const char c = static_cast<char>(data.at(i));
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
        throw std::runtime_error("Failed to open BRLAN: " + path.string());
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

float ClampToU8Float(float value) {
    if (value < 0.0F) {
        return 0.0F;
    }
    if (value > 255.0F) {
        return 255.0F;
    }
    return value;
}

std::uint8_t ClampToU8(float value) {
    const float clamped = ClampToU8Float(value);
    return static_cast<std::uint8_t>(std::lround(clamped));
}

void ApplyPaneTransform(PaneResource& pane, std::uint16_t target, float value) {
    switch (target) {
    case 0:
        pane.current.tx = value;
        return;
    case 1:
        pane.current.ty = value;
        return;
    case 2:
        pane.current.tz = value;
        return;
    case 6:
        pane.current.sx = value;
        return;
    case 7:
        pane.current.sy = value;
        return;
    default:
        return;
    }
}

void ApplyPaneColor(PaneResource& pane, std::uint16_t target, float value) {
    if (target == 16U) {
        pane.current.alpha = ClampToU8(value);
        return;
    }

    if (target >= 16U) {
        return;
    }

    const int vertex = static_cast<int>(target / 4U);
    const int component = static_cast<int>(target % 4U);
    const std::size_t index = static_cast<std::size_t>(vertex * 4 + component);
    pane.current.vertexColor[index] = ClampToU8(value);
}

void ApplyPaneMaterialColorApprox(PaneResource& pane, std::uint16_t target, float value) {
    // RLMC animates material color channels. We do not emulate full TEV here, so map to pane tint as a practical approximation.
    if (target == 3U || target == 7U || target == 11U || target == 15U) {
        pane.current.alpha = ClampToU8(value);
        return;
    }

    const int component = static_cast<int>(target % 4U);
    for (int vertex = 0; vertex < 4; ++vertex) {
        const std::size_t index = static_cast<std::size_t>(vertex * 4 + component);
        pane.current.vertexColor[index] = ClampToU8(value);
    }
}

void ApplyPaneTextureSrt(PaneResource& pane, std::uint16_t target, float value) {
    switch (target) {
    case 0:
        pane.current.texOffsetU = value;
        return;
    case 1:
        pane.current.texOffsetV = value;
        return;
    case 3:
        pane.current.texScaleU = value;
        return;
    case 4:
        pane.current.texScaleV = value;
        return;
    default:
        return;
    }
}

}  // namespace

float BrlanChannel::Sample(float frame) const {
    if (keys.empty()) {
        return 0.0F;
    }

    if (keys.size() == 1U || frame <= keys.front().frame) {
        return keys.front().value;
    }

    if (frame >= keys.back().frame) {
        return keys.back().value;
    }

    for (std::size_t i = 0; i + 1U < keys.size(); ++i) {
        const BrlanKey& k0 = keys[i];
        const BrlanKey& k1 = keys[i + 1U];
        if (frame < k0.frame || frame > k1.frame) {
            continue;
        }

        if (curveType == BrlanCurveType::Step) {
            return k0.value;
        }

        const float span = k1.frame - k0.frame;
        if (std::fabs(span) < 0.00001F) {
            return k1.value;
        }

        const float t = (frame - k0.frame) / span;
        const float t2 = t * t;
        const float t3 = t2 * t;

        const float h00 = 2.0F * t3 - 3.0F * t2 + 1.0F;
        const float h10 = t3 - 2.0F * t2 + t;
        const float h01 = -2.0F * t3 + 3.0F * t2;
        const float h11 = t3 - t2;

        return h00 * k0.value + h10 * span * k0.slope + h01 * k1.value + h11 * span * k1.slope;
    }

    return keys.back().value;
}

BrlanAnimation BrlanAnimation::LoadFromFile(const std::filesystem::path& path) {
    BrlanAnimation animation;
    animation.mName = path.stem().string();

    const std::vector<std::uint8_t> data = ReadAll(path);
    if (data.size() < 0x10U) {
        throw std::runtime_error("BRLAN too small: " + path.string());
    }

    if (std::string(reinterpret_cast<const char*>(data.data()), 4) != "RLAN") {
        throw std::runtime_error("Invalid BRLAN signature: " + path.string());
    }

    const std::uint16_t blockCount = ReadU16BE(data, 0x0EU);

    std::size_t offset = 0x10U;
    for (std::uint16_t blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
        if (offset + 8U > data.size()) {
            throw std::runtime_error("BRLAN block header out of range: " + path.string());
        }

        const std::string kind(reinterpret_cast<const char*>(&data[offset]), 4);
        const std::uint32_t blockSize = ReadU32BE(data, offset + 4U);
        if (blockSize < 8U || offset + blockSize > data.size()) {
            throw std::runtime_error("BRLAN block size invalid: " + path.string());
        }

        if (kind == "pat1") {
            const std::uint32_t nameOffset = ReadU32BE(data, offset + 12U);
            if (nameOffset > 0U) {
                animation.mName = ReadCString(data, offset + nameOffset);
            }

            const std::int16_t startFrame = ReadS16BE(data, offset + 20U);
            const std::int16_t endFrame = ReadS16BE(data, offset + 22U);
            if (animation.mFrameSize == 0) {
                const int frameDelta = static_cast<int>(endFrame) - static_cast<int>(startFrame);
                animation.mFrameSize = std::max(frameDelta, 0);
            }
        } else if (kind == "pai1") {
            animation.mFrameSize = static_cast<int>(ReadU16BE(data, offset + 8U));
            animation.mLoop = data.at(offset + 10U) != 0U;

            const std::uint16_t animContNum = ReadU16BE(data, offset + 14U);
            const std::uint32_t animContOffsetsOffset = ReadU32BE(data, offset + 16U);

            for (std::uint16_t contentIndex = 0; contentIndex < animContNum; ++contentIndex) {
                const std::uint32_t contentOffsetRel = ReadU32BE(data, offset + animContOffsetsOffset + static_cast<std::size_t>(contentIndex) * 4U);
                const std::size_t contentOffset = offset + contentOffsetRel;

                const std::string paneName = ReadCStringBounded(data, contentOffset, 20U);
                const std::uint8_t animTypeCount = data.at(contentOffset + 20U);

                for (std::uint8_t animTypeIndex = 0; animTypeIndex < animTypeCount; ++animTypeIndex) {
                    const std::uint32_t animTypeOffsetRel =
                        ReadU32BE(data, contentOffset + 24U + static_cast<std::size_t>(animTypeIndex) * 4U);
                    const std::size_t animTypeOffset = contentOffset + animTypeOffsetRel;

                    const std::string animKind(reinterpret_cast<const char*>(&data[animTypeOffset]), 4);
                    const std::uint8_t channelCount = data.at(animTypeOffset + 4U);

                    for (std::uint8_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
                        const std::uint32_t channelOffsetRel =
                            ReadU32BE(data, animTypeOffset + 8U + static_cast<std::size_t>(channelIndex) * 4U);
                        const std::size_t channelOffset = animTypeOffset + channelOffsetRel;

                        BrlanChannel channel;
                        channel.paneName = paneName;
                        channel.kind = animKind;
                        channel.target = ReadU16BE(data, channelOffset + 0U);
                        channel.curveType = static_cast<BrlanCurveType>(data.at(channelOffset + 2U));
                        const std::uint16_t keyCount = ReadU16BE(data, channelOffset + 4U);
                        const std::uint32_t keyOffsetRel = ReadU32BE(data, channelOffset + 8U);
                        const std::size_t keyOffset = channelOffset + keyOffsetRel;

                        channel.keys.reserve(keyCount);
                        for (std::uint16_t keyIndex = 0; keyIndex < keyCount; ++keyIndex) {
                            BrlanKey key;
                            if (channel.curveType == BrlanCurveType::Step) {
                                const std::size_t keyStart = keyOffset + static_cast<std::size_t>(keyIndex) * 8U;
                                key.frame = ReadF32BE(data, keyStart + 0U);
                                key.value = static_cast<float>(ReadU16BE(data, keyStart + 4U));
                                key.slope = 0.0F;
                            } else if (channel.curveType == BrlanCurveType::Hermite) {
                                const std::size_t keyStart = keyOffset + static_cast<std::size_t>(keyIndex) * 12U;
                                key.frame = ReadF32BE(data, keyStart + 0U);
                                key.value = ReadF32BE(data, keyStart + 4U);
                                key.slope = ReadF32BE(data, keyStart + 8U);
                            } else {
                                throw std::runtime_error("Unsupported BRLAN curve type " + std::to_string(static_cast<int>(channel.curveType)) +
                                                         " in " + path.string());
                            }
                            channel.keys.push_back(key);
                        }

                        animation.mChannels.push_back(std::move(channel));
                    }
                }
            }
        }

        offset += blockSize;
    }

    Log(LogLevel::Info, LogCategory::Layout,
        "Loaded BRLAN " + path.string() + " name=" + animation.mName + " channels=" + std::to_string(animation.mChannels.size()));

    return animation;
}

const std::string& BrlanAnimation::GetName() const {
    return mName;
}

int BrlanAnimation::GetFrameSize() const {
    return mFrameSize;
}

bool BrlanAnimation::IsLooped() const {
    return mLoop;
}

float BrlanAnimation::NormalizeFrame(float frame) const {
    if (!mLoop || mFrameSize <= 0) {
        return frame;
    }

    const float loopFrame = static_cast<float>(mFrameSize);
    const float wrapped = std::fmod(frame, loopFrame);
    if (wrapped < 0.0F) {
        return wrapped + loopFrame;
    }
    return wrapped;
}

void BrlanAnimation::ApplyToLayout(BrlytLayout& layout, float frame) const {
    const float sampleFrame = NormalizeFrame(frame);

    for (const BrlanChannel& channel : mChannels) {
        PaneResource* const pane = layout.FindPane(channel.paneName);
        if (pane == nullptr) {
            throw std::runtime_error("Animation pane not found: " + channel.paneName + " in animation " + mName);
        }

        const float value = channel.Sample(sampleFrame);

        if (channel.kind == "RLPA") {
            ApplyPaneTransform(*pane, channel.target, value);
        } else if (channel.kind == "RLVC") {
            ApplyPaneColor(*pane, channel.target, value);
        } else if (channel.kind == "RLVI") {
            if (channel.target == 0U) {
                pane->current.visible = value > 0.5F;
            }
        } else if (channel.kind == "RLMC") {
            ApplyPaneMaterialColorApprox(*pane, channel.target, value);
        } else if (channel.kind == "RLTS") {
            ApplyPaneTextureSrt(*pane, channel.target, value);
        } else {
            throw std::runtime_error("Unsupported BRLAN channel kind " + channel.kind + " in animation " + mName);
        }
    }
}

BrlanBundle BrlanBundle::LoadFromDirectory(const std::filesystem::path& animDir) {
    if (!std::filesystem::exists(animDir)) {
        throw std::runtime_error("Animation directory missing: " + animDir.string());
    }

    BrlanBundle bundle;
    std::vector<std::filesystem::path> files;

    for (const auto& entry : std::filesystem::directory_iterator(animDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".brlan") {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    for (const auto& file : files) {
        BrlanAnimation animation = BrlanAnimation::LoadFromFile(file);
        const int index = static_cast<int>(bundle.mAnimations.size());
        bundle.mIndexByLowerName.emplace(ToLower(animation.GetName()), index);
        bundle.mIndexByLowerName.emplace(ToLower(file.stem().string()), index);
        bundle.mAnimations.push_back(std::move(animation));
    }

    return bundle;
}

const BrlanAnimation* BrlanBundle::FindByName(std::string_view name) const {
    const auto it = mIndexByLowerName.find(ToLower(std::string(name)));
    if (it == mIndexByLowerName.end()) {
        return nullptr;
    }
    return &mAnimations[it->second];
}

const std::vector<BrlanAnimation>& BrlanBundle::GetAnimations() const {
    return mAnimations;
}

}  // namespace pcport
