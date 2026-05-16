#include "LayoutRuntimeActor.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compat/RuntimeContext.hpp"
#include "layout/Bmg.hpp"
#include "layout/Binary.hpp"

namespace smgpc::game::layout {
    namespace {

        [[nodiscard]] std::string baseWithoutExtension(std::string text) {
            const auto slash = text.find_last_of("/\\");
            if (slash != std::string::npos) {
                text = text.substr(slash + 1U);
            }

            const auto dot = text.find_last_of('.');
            if (dot != std::string::npos) {
                text = text.substr(0U, dot);
            }

            return text;
        }

        [[nodiscard]] std::string texturePatternName(std::string baseName, std::int32_t patternIndex) {
            if (baseName.empty()) {
                return {};
            }

            const auto digitEnd = baseName.size();
            auto digitBegin = digitEnd;
            while (digitBegin > 0U && std::isdigit(static_cast< unsigned char >(baseName[digitBegin - 1U]))) {
                --digitBegin;
            }

            if (digitBegin == digitEnd) {
                return baseName + std::to_string(patternIndex);
            }

            const auto numberText = baseName.substr(digitBegin);
            std::int32_t number = 0;
            for (const char digit : numberText) {
                number = number * 10 + static_cast< std::int32_t >(digit - '0');
            }
            if (number + patternIndex < 0) {
                return baseName;
            }
            const auto replacement = std::to_string(number + patternIndex);
            baseName.replace(digitBegin, digitEnd - digitBegin, replacement);
            return baseName;
        }

        [[nodiscard]] std::optional< std::string > localizedPaneBaseName(std::string_view normalizedName) {
            static constexpr std::array< std::string_view, 9 > suffixes {
                "jpja",
                "krko",
                "cnsi",
                "usen",
                "euen",
                "eufr",
                "eues",
                "eude",
                "euit",
            };

            for (const auto suffix : suffixes) {
                if (normalizedName.size() > suffix.size() && normalizedName.ends_with(suffix)) {
                    return std::string(normalizedName.substr(0U, normalizedName.size() - suffix.size()));
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional< char16_t > staticTextPictureFontTag(char16_t codepoint) {
            switch (codepoint) {
            case u'\uFF21':
            case u'A':
                return assets::layout::make_bmg_picture_font_tag(0x0000U);
            case u'\uFF22':
            case u'B':
                return assets::layout::make_bmg_picture_font_tag(0x0001U);
            default:
                return std::nullopt;
            }
        }

        [[nodiscard]] std::optional< char16_t > staticTextPictureFontTag(std::u16string_view text, std::size_t index) {
            if (index >= text.size()) {
                return std::nullopt;
            }

            if (text[index] == u'\uFF21') {
                return assets::layout::make_bmg_picture_font_tag(0x0000U);
            }

            if (text[index] != u'B' && text[index] != u'\uFF22') {
                return std::nullopt;
            }

            if (index + 1U < text.size() && (text[index + 1U] == u'\ub97c' || text[index + 1U] == u'\uc744')) {
                return assets::layout::make_bmg_picture_font_tag(0x0001U);
            }
            if (index > 0U && (text[index - 1U] == u'\uc640' || text[index - 1U] == u'\uacfc')) {
                return assets::layout::make_bmg_picture_font_tag(0x0001U);
            }

            return std::nullopt;
        }

        [[nodiscard]] bool has_non_opaque_alpha(const assets::layout::tpl::DecodedImage& image) {
            if (image.empty() || image.rgba8.size() < 4U) {
                return false;
            }

            const std::size_t pixel_count = static_cast< std::size_t >(image.width) * image.height;
            const std::size_t sample_count = std::min< std::size_t >(pixel_count, 2048U);
            const std::size_t sample_stride = std::max< std::size_t >(1U, pixel_count / sample_count);
            std::size_t non_opaque = 0U;
            std::size_t sampled = 0U;

            for (std::size_t pixel_index = 0U; pixel_index < pixel_count; pixel_index += sample_stride) {
                const std::size_t rgba_index = pixel_index * 4U;
                if (rgba_index + 3U >= image.rgba8.size()) {
                    break;
                }
                if (image.rgba8[rgba_index + 3U] != 255U) {
                    ++non_opaque;
                }
                ++sampled;
            }

            if (sampled == 0U) {
                return false;
            }
            return (static_cast< float >(non_opaque) / static_cast< float >(sampled)) > 0.05F;
        }

        struct MaskChannelStats {
            float minimum{1.0F};
            float maximum{};
            float average{};
        };

        [[nodiscard]] MaskChannelStats estimate_mask_channel_stats(const assets::layout::tpl::DecodedImage& image, bool use_alpha_channel) {
            MaskChannelStats stats{};
            if (image.empty() || image.rgba8.size() < 4U) {
                return stats;
            }

            const std::size_t pixel_count = static_cast< std::size_t >(image.width) * image.height;
            const std::size_t sample_count = std::min< std::size_t >(pixel_count, 4096U);
            const std::size_t sample_stride = std::max< std::size_t >(1U, pixel_count / sample_count);
            float sum = 0.0F;
            std::size_t sampled = 0U;

            for (std::size_t pixel_index = 0U; pixel_index < pixel_count; pixel_index += sample_stride) {
                const std::size_t rgba_index = pixel_index * 4U;
                if (rgba_index + 3U >= image.rgba8.size()) {
                    break;
                }

                const float channel = use_alpha_channel ?
                                          static_cast< float >(image.rgba8[rgba_index + 3U]) / 255.0F :
                                          (static_cast< float >(image.rgba8[rgba_index + 0U]) + static_cast< float >(image.rgba8[rgba_index + 1U]) +
                                           static_cast< float >(image.rgba8[rgba_index + 2U])) /
                                              (255.0F * 3.0F);

                stats.minimum = std::min(stats.minimum, channel);
                stats.maximum = std::max(stats.maximum, channel);
                sum += channel;
                ++sampled;
            }

            if (sampled > 0U) {
                stats.average = sum / static_cast< float >(sampled);
            }
            return stats;
        }

        [[nodiscard]] float estimate_colorfulness(const assets::layout::tpl::DecodedImage& image) {
            if (image.empty() || image.rgba8.size() < 4U) {
                return 0.0F;
            }

            const std::size_t pixel_count = static_cast< std::size_t >(image.width) * image.height;
            const std::size_t sample_count = std::min< std::size_t >(pixel_count, 4096U);
            const std::size_t sample_stride = std::max< std::size_t >(1U, pixel_count / sample_count);
            float diff_sum = 0.0F;
            std::size_t sampled = 0U;

            for (std::size_t pixel_index = 0U; pixel_index < pixel_count; pixel_index += sample_stride) {
                const std::size_t rgba_index = pixel_index * 4U;
                if (rgba_index + 2U >= image.rgba8.size()) {
                    break;
                }

                const float r = static_cast< float >(image.rgba8[rgba_index + 0U]) / 255.0F;
                const float g = static_cast< float >(image.rgba8[rgba_index + 1U]) / 255.0F;
                const float b = static_cast< float >(image.rgba8[rgba_index + 2U]) / 255.0F;
                diff_sum += (std::fabs(r - g) + std::fabs(g - b) + std::fabs(b - r)) / 3.0F;
                ++sampled;
            }

            if (sampled == 0U) {
                return 0.0F;
            }
            return diff_sum / static_cast< float >(sampled);
        }

        [[nodiscard]] bool isWhiteColorRegister(const std::array< std::uint8_t, 4 >& color) {
            return color[0U] == 255U && color[1U] == 255U && color[2U] == 255U;
        }

        [[nodiscard]] bool colorRegistersDiffer(const std::array< std::uint8_t, 4 >& a, const std::array< std::uint8_t, 4 >& b) {
            return a[0U] != b[0U] || a[1U] != b[1U] || a[2U] != b[2U] || a[3U] != b[3U];
        }

        [[nodiscard]] float explicitTevStageColorScale(const assets::layout::MaterialDefinition* material) {
            if (material == nullptr || material->tev_stages.size() < 2U) {
                return 1.0F;
            }

            const auto scale = static_cast< std::uint8_t >((material->tev_stages[1U].raw[6U] >> 6U) & 0x3U);
            switch (scale) {
            case 1U:
                return 2.0F;
            case 2U:
                return 4.0F;
            case 3U:
                return 0.5F;
            default:
                return 1.0F;
            }
        }

        [[nodiscard]] std::uint32_t packMaterialColor(const std::array< std::uint8_t, 4 >& color) {
            return render::layout::pack_abgr(color[0U], color[1U], color[2U], color[3U]);
        }

        [[nodiscard]] std::optional< std::size_t > texturePatternStageFromTarget(std::uint16_t target) {
            if (target < 0x100U) {
                return static_cast< std::size_t >(target);
            }

            if ((target & 0x00FFU) == 0U) {
                return static_cast< std::size_t >(target >> 8U);
            }

            return std::nullopt;
        }

        [[nodiscard]] bool should_invert_mask_alpha(const assets::layout::tpl::DecodedImage& image, bool use_alpha_channel) {
            if (image.empty() || image.rgba8.size() < 4U) {
                return false;
            }

            const std::size_t pixel_count = static_cast< std::size_t >(image.width) * image.height;
            const std::size_t sample_count = std::min< std::size_t >(pixel_count, 4096U);
            const std::size_t sample_stride = std::max< std::size_t >(1U, pixel_count / sample_count);
            std::uint64_t alpha_sum = 0U;
            std::size_t sampled = 0U;

            for (std::size_t pixel_index = 0U; pixel_index < pixel_count; pixel_index += sample_stride) {
                const std::size_t rgba_index = pixel_index * 4U;
                if (rgba_index + 3U >= image.rgba8.size()) {
                    break;
                }

                const std::uint8_t alpha = use_alpha_channel ?
                                               image.rgba8[rgba_index + 3U] :
                                               static_cast< std::uint8_t >((static_cast< std::uint32_t >(image.rgba8[rgba_index + 0U]) +
                                                                            static_cast< std::uint32_t >(image.rgba8[rgba_index + 1U]) +
                                                                            static_cast< std::uint32_t >(image.rgba8[rgba_index + 2U])) /
                                                                           3U);
                alpha_sum += alpha;
                ++sampled;
            }

            if (sampled == 0U) {
                return false;
            }

            const float average_alpha = static_cast< float >(alpha_sum) / static_cast< float >(sampled) / 255.0F;
            return average_alpha > 0.5F;
        }

        [[nodiscard]] float location_adjust_scale_x(const assets::layout::PaneDefinition& pane) {
            if (pane.location_adjust && compat::runtime_context().is_widescreen) {
                return 0.75F;
            }
            return 1.0F;
        }

        enum class WindowFrameSection : std::uint8_t {
            LeftTop,
            RightTop,
            RightBottom,
            LeftBottom,
        };

        struct TextureFlipInfo {
            std::array< std::array< float, 2 >, 4 > coords;
            std::array< std::uint8_t, 2 > axis;
        };

        [[nodiscard]] const TextureFlipInfo& texture_flip_info(std::uint8_t textureFlip) {
            static const std::array< TextureFlipInfo, 6 > FLIP_INFO {{
                TextureFlipInfo {{{{0.0F, 0.0F}, {1.0F, 0.0F}, {0.0F, 1.0F}, {1.0F, 1.0F}}}, {{0U, 1U}}},
                TextureFlipInfo {{{{1.0F, 0.0F}, {0.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 1.0F}}}, {{0U, 1U}}},
                TextureFlipInfo {{{{0.0F, 1.0F}, {1.0F, 1.0F}, {0.0F, 0.0F}, {1.0F, 0.0F}}}, {{0U, 1U}}},
                TextureFlipInfo {{{{0.0F, 1.0F}, {0.0F, 0.0F}, {1.0F, 1.0F}, {1.0F, 0.0F}}}, {{1U, 0U}}},
                TextureFlipInfo {{{{1.0F, 1.0F}, {0.0F, 1.0F}, {1.0F, 0.0F}, {0.0F, 0.0F}}}, {{0U, 1U}}},
                TextureFlipInfo {{{{1.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 0.0F}, {0.0F, 1.0F}}}, {{1U, 0U}}},
            }};

            if (textureFlip >= FLIP_INFO.size()) {
                return FLIP_INFO[0U];
            }
            return FLIP_INFO[textureFlip];
        }

        [[nodiscard]] std::array< std::uint8_t, 16 > solid_vertex_color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
            return {
                r, g, b, a,
                r, g, b, a,
                r, g, b, a,
                r, g, b, a,
            };
        }

        [[nodiscard]] std::array< float, 8 > window_frame_tex_coords(
            WindowFrameSection section,
            std::uint8_t textureFlip,
            float width,
            float height,
            const assets::layout::tpl::DecodedImage* texture) {
            if (texture == nullptr || texture->width == 0U || texture->height == 0U) {
                return {0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F};
            }

            const auto& flip = texture_flip_info(textureFlip);
            const auto ix = static_cast< std::size_t >(flip.axis[0U]);
            const auto iy = static_cast< std::size_t >(flip.axis[1U]);
            const std::array< float, 2 > textureSize {
                static_cast< float >(texture->width),
                static_cast< float >(texture->height),
            };
            std::array< std::array< float, 2 >, 4 > coords {};

            switch (section) {
            case WindowFrameSection::LeftTop:
                coords[0U][ix] = coords[2U][ix] = flip.coords[0U][ix];
                coords[0U][iy] = coords[1U][iy] = flip.coords[0U][iy];
                coords[3U][ix] = coords[1U][ix] =
                    flip.coords[0U][ix] + width / ((flip.coords[1U][ix] - flip.coords[0U][ix]) * textureSize[ix]);
                coords[3U][iy] = coords[2U][iy] =
                    flip.coords[0U][iy] + height / ((flip.coords[2U][iy] - flip.coords[0U][iy]) * textureSize[iy]);
                break;
            case WindowFrameSection::RightTop:
                coords[1U][ix] = coords[3U][ix] = flip.coords[1U][ix];
                coords[1U][iy] = coords[0U][iy] = flip.coords[1U][iy];
                coords[2U][ix] = coords[0U][ix] =
                    flip.coords[1U][ix] + width / ((flip.coords[0U][ix] - flip.coords[1U][ix]) * textureSize[ix]);
                coords[2U][iy] = coords[3U][iy] =
                    flip.coords[1U][iy] + height / ((flip.coords[3U][iy] - flip.coords[1U][iy]) * textureSize[iy]);
                break;
            case WindowFrameSection::RightBottom:
                coords[3U][ix] = coords[1U][ix] = flip.coords[3U][ix];
                coords[3U][iy] = coords[2U][iy] = flip.coords[3U][iy];
                coords[0U][ix] = coords[2U][ix] =
                    flip.coords[3U][ix] + width / ((flip.coords[2U][ix] - flip.coords[3U][ix]) * textureSize[ix]);
                coords[0U][iy] = coords[1U][iy] =
                    flip.coords[3U][iy] + height / ((flip.coords[1U][iy] - flip.coords[3U][iy]) * textureSize[iy]);
                break;
            case WindowFrameSection::LeftBottom:
                coords[2U][ix] = coords[0U][ix] = flip.coords[2U][ix];
                coords[2U][iy] = coords[3U][iy] = flip.coords[2U][iy];
                coords[1U][ix] = coords[3U][ix] =
                    flip.coords[2U][ix] + width / ((flip.coords[3U][ix] - flip.coords[2U][ix]) * textureSize[ix]);
                coords[1U][iy] = coords[0U][iy] =
                    flip.coords[2U][iy] + height / ((flip.coords[0U][iy] - flip.coords[2U][iy]) * textureSize[iy]);
                break;
            default:
                break;
            }

            return {
                coords[0U][0U], coords[0U][1U],
                coords[1U][0U], coords[1U][1U],
                coords[2U][0U], coords[2U][1U],
                coords[3U][0U], coords[3U][1U],
            };
        }

    }  // namespace

    LayoutRuntimeActor::LayoutRuntimeActor(std::shared_ptr< const LayoutArchiveData > resource) : mResource(std::move(resource)) {
        if (mResource == nullptr) {
            return;
        }

        mBaseStates.reserve(mResource->layout.panes.size());
        mCommittedStates.reserve(mResource->layout.panes.size());
        mCurrentStates.reserve(mResource->layout.panes.size());

        for (std::size_t paneIndex = 0; paneIndex < mResource->layout.panes.size(); ++paneIndex) {
            const auto& pane = mResource->layout.panes[paneIndex];

            RuntimePaneState state{};
            state.visible = pane.visible;
            state.alpha = pane.alpha;
            state.tx = pane.translate.x;
            state.ty = pane.translate.y;
            state.tz = pane.translate.z;
            state.rz = pane.rotate.z;
            state.sx = pane.scale.x;
            state.sy = pane.scale.y;
            state.width = pane.size.x;
            state.height = pane.size.y;
            state.textPosition = pane.text_position;

            for (std::size_t i = 0; i < pane.vertex_colors.size(); ++i) {
                const auto& color = pane.vertex_colors[i];
                state.vertexColor[i * 4U + 0U] = color.r;
                state.vertexColor[i * 4U + 1U] = color.g;
                state.vertexColor[i * 4U + 2U] = color.b;
                state.vertexColor[i * 4U + 3U] = color.a;
            }

            mBaseStates.push_back(state);
            mCommittedStates.push_back(state);
            mCurrentStates.push_back(state);
            mPaneTextureCache.emplace_back();
            mLanguageVisibleStates.push_back(state.visible);
            mTextOverrides.emplace_back(std::nullopt);

            mPaneIndexByName.emplace(normalizeName(pane.name), paneIndex);
        }

        mIsAlive = false;
    }

    std::string LayoutRuntimeActor::normalizeName(std::string name) {
        return assets::layout::binary::to_lower_ascii(std::move(name));
    }

    float LayoutRuntimeActor::anchorOffsetX(std::uint8_t basePosition, float width) {
        switch (basePosition % 3U) {
        case 0U:
            return 0.0F;
        case 1U:
            return -width * 0.5F;
        case 2U:
            return -width;
        default:
            return 0.0F;
        }
    }

    float LayoutRuntimeActor::anchorOffsetY(std::uint8_t basePosition, float height) {
        const auto row = static_cast< std::uint8_t >((basePosition / 3U) % 3U);
        switch (row) {
        case 0U:
            return 0.0F;
        case 1U:
            return -height * 0.5F;
        case 2U:
            return -height;
        default:
            return 0.0F;
        }
    }

    std::uint8_t LayoutRuntimeActor::clampU8(float value) {
        if (value <= 0.0F) {
            return 0U;
        }
        if (value >= 255.0F) {
            return 255U;
        }
        return static_cast< std::uint8_t >(std::lround(value));
    }

    const assets::layout::PaneDefinition* LayoutRuntimeActor::paneDefinition(std::size_t index) const {
        if (mResource == nullptr || index >= mResource->layout.panes.size()) {
            return nullptr;
        }
        return &mResource->layout.panes[index];
    }

    const LayoutRuntimeActor::RuntimePaneState* LayoutRuntimeActor::paneState(std::size_t index) const {
        if (index >= mCurrentStates.size()) {
            return nullptr;
        }
        return &mCurrentStates[index];
    }

    LayoutRuntimeActor::FollowPosition LayoutRuntimeActor::followPosition(std::size_t paneIndex, const assets::layout::PaneDefinition& pane) const {
        if (mResource != nullptr && static_cast< std::size_t >(mResource->layout.root_pane) == paneIndex) {
            const auto root_follow = mFollowPositions.find({});
            if (root_follow != mFollowPositions.end()) {
                return root_follow->second;
            }
        }

        const auto pane_follow = mFollowPositions.find(normalizeName(pane.name));
        if (pane_follow != mFollowPositions.end()) {
            return pane_follow->second;
        }

        return {};
    }

    LayoutRuntimeActor::PaneAnimationSlot* LayoutRuntimeActor::findPaneAnimationSlot(const std::string& paneName, std::uint32_t slotIndex) {
        for (auto& slot : mPaneAnimationSlots) {
            if (slot.pane_name == paneName && slot.slot_index == slotIndex) {
                return &slot;
            }
        }
        return nullptr;
    }

    const LayoutRuntimeActor::PaneAnimationSlot* LayoutRuntimeActor::findPaneAnimationSlot(const std::string& paneName,
                                                                                           std::uint32_t slotIndex) const {
        for (const auto& slot : mPaneAnimationSlots) {
            if (slot.pane_name == paneName && slot.slot_index == slotIndex) {
                return &slot;
            }
        }
        return nullptr;
    }

    std::vector< std::size_t > LayoutRuntimeActor::paneIndicesForVisibilityName(const char* pPaneName) const {
        std::vector< std::size_t > indices {};
        if (mResource == nullptr || pPaneName == nullptr) {
            return indices;
        }

        const auto pane_name = normalizeName(pPaneName);
        for (std::size_t pane_index = 0U; pane_index < mResource->layout.panes.size(); ++pane_index) {
            const auto normalized_pane = normalizeName(mResource->layout.panes[pane_index].name);
            if (normalized_pane == pane_name) {
                indices.push_back(pane_index);
                continue;
            }

            const auto localized_base = localizedPaneBaseName(normalized_pane);
            if (localized_base.has_value() && *localized_base == pane_name) {
                indices.push_back(pane_index);
            }
        }

        return indices;
    }

    bool LayoutRuntimeActor::hasLocalizedVisibilityVariant(const std::string& paneName) const {
        if (mResource == nullptr) {
            return false;
        }

        for (const auto& pane : mResource->layout.panes) {
            const auto localized_base = localizedPaneBaseName(normalizeName(pane.name));
            if (localized_base.has_value() && *localized_base == paneName) {
                return true;
            }
        }

        return false;
    }

    bool LayoutRuntimeActor::shouldRespectLanguageVisibility(std::size_t paneIndex) const {
        if (mResource == nullptr || paneIndex >= mResource->layout.panes.size()) {
            return false;
        }

        const auto pane_name = normalizeName(mResource->layout.panes[paneIndex].name);
        if (localizedPaneBaseName(pane_name).has_value()) {
            return true;
        }

        return hasLocalizedVisibilityVariant(pane_name);
    }

    bool LayoutRuntimeActor::paneIsInSubtree(std::size_t paneIndex, std::size_t rootPaneIndex) const {
        if (paneIndex == rootPaneIndex) {
            return true;
        }
        if (mResource == nullptr || paneIndex >= mResource->layout.panes.size() || rootPaneIndex >= mResource->layout.panes.size()) {
            return false;
        }

        auto parent = mResource->layout.panes[paneIndex].parent;
        while (parent >= 0) {
            if (static_cast< std::size_t >(parent) == rootPaneIndex) {
                return true;
            }
            if (static_cast< std::size_t >(parent) >= mResource->layout.panes.size()) {
                return false;
            }
            parent = mResource->layout.panes[static_cast< std::size_t >(parent)].parent;
        }

        return false;
    }

    void LayoutRuntimeActor::setPaneVisibleAtIndex(std::size_t paneIndex, bool visible, bool respectLanguageVisibility) {
        if (paneIndex >= mBaseStates.size() || paneIndex >= mCommittedStates.size() || paneIndex >= mCurrentStates.size()) {
            return;
        }

        const bool language_visible = paneIndex < mLanguageVisibleStates.size() ? mLanguageVisibleStates[paneIndex] : true;
        const bool final_visible = visible && (!(respectLanguageVisibility || shouldRespectLanguageVisibility(paneIndex)) || language_visible);
        mBaseStates[paneIndex].visible = final_visible;
        mCommittedStates[paneIndex].visible = final_visible;
        mCurrentStates[paneIndex].visible = final_visible;
    }

    void LayoutRuntimeActor::setPaneVisibleInSubtree(std::size_t paneIndex, bool visible, bool respectLanguageVisibility) {
        if (mResource == nullptr || paneIndex >= mResource->layout.panes.size()) {
            return;
        }

        setPaneVisibleAtIndex(paneIndex, visible, respectLanguageVisibility);

        const auto& pane = mResource->layout.panes[paneIndex];
        for (const auto child_index : pane.children) {
            if (child_index < 0) {
                continue;
            }
            setPaneVisibleInSubtree(static_cast< std::size_t >(child_index), visible, respectLanguageVisibility);
        }
    }

    void LayoutRuntimeActor::setTextBoxTextInSubtree(std::size_t paneIndex, const std::u16string& text, bool* pChanged) {
        if (mResource == nullptr || paneIndex >= mResource->layout.panes.size() || paneIndex >= mTextOverrides.size()) {
            return;
        }

        const auto& pane = mResource->layout.panes[paneIndex];
        if (pane.type == assets::layout::PaneType::Text) {
            mTextOverrides[paneIndex] = text;
            if (pChanged != nullptr) {
                *pChanged = true;
            }
        }

        for (const auto child_index : pane.children) {
            if (child_index < 0) {
                continue;
            }
            setTextBoxTextInSubtree(static_cast< std::size_t >(child_index), text, pChanged);
        }
    }

    void LayoutRuntimeActor::setTextBoxVerticalPositionInSubtree(std::size_t paneIndex, std::uint8_t position, bool* pChanged) {
        if (mResource == nullptr || paneIndex >= mResource->layout.panes.size() || paneIndex >= mBaseStates.size() ||
            paneIndex >= mCommittedStates.size() || paneIndex >= mCurrentStates.size()) {
            return;
        }

        const auto& pane = mResource->layout.panes[paneIndex];
        if (pane.type == assets::layout::PaneType::Text) {
            const auto clamped_position = static_cast< std::uint8_t >(std::min< std::uint8_t >(position, 2U));
            const auto current_column = static_cast< std::uint8_t >(mBaseStates[paneIndex].textPosition % 3U);
            const auto text_position = static_cast< std::uint8_t >(clamped_position * 3U + current_column);
            mBaseStates[paneIndex].textPosition = text_position;
            mCommittedStates[paneIndex].textPosition = text_position;
            mCurrentStates[paneIndex].textPosition = text_position;
            if (pChanged != nullptr) {
                *pChanged = true;
            }
        }

        for (const auto child_index : pane.children) {
            if (child_index < 0) {
                continue;
            }
            setTextBoxVerticalPositionInSubtree(static_cast< std::size_t >(child_index), position, pChanged);
        }
    }

    void LayoutRuntimeActor::resetPose() {
        mCurrentStates = mCommittedStates;
    }

    void LayoutRuntimeActor::rebuildPose() {
        resetPose();

        for (const auto& slot : mAnimationLayers) {
            if (slot.animation == nullptr) {
                continue;
            }

            applyAnimation(*slot.animation, slot.frame, nullptr);
        }

        for (const auto& slot : mPaneAnimationSlots) {
            if (slot.animation == nullptr) {
                continue;
            }

            applyAnimation(*slot.animation, slot.frame, &slot.pane_name);
        }
    }

    void LayoutRuntimeActor::applyAnimation(const assets::layout::BrlanAnimation& animation, float frame, const std::string* rootPaneName) {
        applyAnimationToStates(animation, frame, rootPaneName, &mCurrentStates);
    }

    void LayoutRuntimeActor::applyAnimationToStates(
        const assets::layout::BrlanAnimation& animation,
        float frame,
        const std::string* rootPaneName,
        std::vector< RuntimePaneState >* pStates) const {
        if (pStates == nullptr) {
            return;
        }

        const auto sampled_frame = animation.normalize_frame(frame);
        std::size_t rootPaneIndex{};
        bool hasRootPane = false;
        if (rootPaneName != nullptr) {
            const auto root_found = mPaneIndexByName.find(*rootPaneName);
            if (root_found == mPaneIndexByName.end()) {
                return;
            }
            rootPaneIndex = root_found->second;
            hasRootPane = true;
        }

        for (const auto& track : animation.tracks) {
            const auto pane_found = mPaneIndexByName.find(normalizeName(track.pane_name));
            if (pane_found == mPaneIndexByName.end()) {
                continue;
            }

            const auto paneIndex = pane_found->second;
            if (hasRootPane && not paneIsInSubtree(paneIndex, rootPaneIndex)) {
                continue;
            }
            if (paneIndex >= pStates->size()) {
                continue;
            }

            auto& state = (*pStates)[paneIndex];
            const auto value = track.sample(sampled_frame);

            if (track.kind == "RLPA") {
                switch (track.target) {
                case 0U:
                    state.tx = value;
                    break;
                case 1U:
                    state.ty = value;
                    break;
                case 2U:
                    state.tz = value;
                    break;
                case 5U:
                    state.rz = value;
                    break;
                case 6U:
                    state.sx = value;
                    break;
                case 7U:
                    state.sy = value;
                    break;
                case 8U:
                    state.width = value;
                    break;
                case 9U:
                    state.height = value;
                    break;
                default:
                    break;
                }
                continue;
            }

            if (track.kind == "RLVC") {
                if (track.target == 16U) {
                    state.alpha = clampU8(value);
                    continue;
                }

                if (track.target < 16U) {
                    state.vertexColor[track.target] = clampU8(value);
                }
                continue;
            }

            if (track.kind == "RLMC") {
                if (track.target == 3U || track.target == 7U || track.target == 11U || track.target == 15U) {
                    state.alpha = clampU8(value);
                    continue;
                }

                const auto component = static_cast< std::size_t >(track.target % 4U);
                for (std::size_t vertex = 0; vertex < 4U; ++vertex) {
                    state.vertexColor[vertex * 4U + component] = clampU8(value);
                }
                continue;
            }

            if (track.kind == "RLTS") {
                const std::size_t texture_stage = static_cast< std::size_t >(track.target / 5U);
                const auto stage_target = static_cast< std::uint16_t >(track.target % 5U);
                if (texture_stage < state.texOffsetU.size()) {
                    switch (stage_target) {
                    case 0U:
                        state.texOffsetU[texture_stage] = value;
                        break;
                    case 1U:
                        state.texOffsetV[texture_stage] = value;
                        break;
                    case 2U:
                        state.texRotate[texture_stage] = value;
                        break;
                    case 3U:
                        state.texScaleU[texture_stage] = value;
                        break;
                    case 4U:
                        state.texScaleV[texture_stage] = value;
                        break;
                    default:
                        break;
                    }
                }
                continue;
            }

            if (track.kind == "RLTP") {
                const auto texture_stage = texturePatternStageFromTarget(track.target);
                if (texture_stage.has_value() && *texture_stage < state.texturePattern.size()) {
                    const auto patternIndex = static_cast< std::int32_t >(std::lround(value));
                    state.texturePattern[*texture_stage] = patternIndex;
                    if (patternIndex >= 0 && static_cast< std::size_t >(patternIndex) < animation.texture_names.size()) {
                        state.texturePatternName[*texture_stage] = animation.texture_names[static_cast< std::size_t >(patternIndex)];
                    } else {
                        state.texturePatternName[*texture_stage].reset();
                    }
                }
                continue;
            }

            if (track.kind == "RLVI") {
                if (track.target == 0U) {
                    const bool language_visible = paneIndex < mLanguageVisibleStates.size() ? mLanguageVisibleStates[paneIndex] : true;
                    state.visible = value > 0.5F && (!shouldRespectLanguageVisibility(paneIndex) || language_visible);
                }
                continue;
            }
        }
    }

    void LayoutRuntimeActor::commitAnimationSlot(const AnimationSlot& slot) {
        if (slot.animation == nullptr) {
            return;
        }

        applyAnimationToStates(*slot.animation, slot.frame, nullptr, &mCommittedStates);
    }

    void LayoutRuntimeActor::commitPaneAnimationSlot(const PaneAnimationSlot& slot) {
        if (slot.animation == nullptr) {
            return;
        }

        applyAnimationToStates(*slot.animation, slot.frame, &slot.pane_name, &mCommittedStates);
    }

    void LayoutRuntimeActor::appear() {
        mIsAlive = true;
        for (auto& slot : mAnimationLayers) {
            slot = AnimationSlot{};
        }
        mPaneAnimationSlots.clear();
        mCommittedStates = mBaseStates;
        resetPose();
    }

    void LayoutRuntimeActor::kill() {
        mIsAlive = false;
        for (auto& slot : mAnimationLayers) {
            slot = AnimationSlot{};
        }
        mPaneAnimationSlots.clear();
        mCommittedStates = mBaseStates;
        resetPose();
    }

    bool LayoutRuntimeActor::isDead() const {
        return not mIsAlive;
    }

    void LayoutRuntimeActor::startAnim(const char* pAnimName, std::uint32_t layer) {
        if (mResource == nullptr || pAnimName == nullptr || layer >= mAnimationLayers.size()) {
            return;
        }

        const auto key = normalizeName(pAnimName);
        const auto found = mResource->animations_by_name.find(key);
        if (found == mResource->animations_by_name.end()) {
            return;
        }

        auto& slot = mAnimationLayers[layer];
        commitAnimationSlot(slot);
        slot.animation = &found->second;
        slot.frame = 0.0F;
        slot.rate = 1.0F;

        rebuildPose();
    }

    bool LayoutRuntimeActor::isAnimStopped(std::uint32_t layer) const {
        if (layer >= mAnimationLayers.size()) {
            return true;
        }

        const auto& slot = mAnimationLayers[layer];
        if (slot.animation == nullptr) {
            return true;
        }
        if (slot.animation->loop) {
            return false;
        }

        return slot.frame >= static_cast< float >(slot.animation->frame_size);
    }

    void LayoutRuntimeActor::setAnimFrameAndStop(float frame, std::uint32_t layer) {
        if (layer >= mAnimationLayers.size()) {
            return;
        }

        auto& slot = mAnimationLayers[layer];
        if (slot.animation == nullptr) {
            return;
        }

        slot.frame = frame;
        slot.rate = 0.0F;

        rebuildPose();
    }

    void LayoutRuntimeActor::setAnimFrame(float frame, std::uint32_t layer) {
        if (layer >= mAnimationLayers.size()) {
            return;
        }

        auto& slot = mAnimationLayers[layer];
        if (slot.animation == nullptr) {
            return;
        }

        slot.frame = frame;
        rebuildPose();
    }

    void LayoutRuntimeActor::setAnimRate(float rate, std::uint32_t layer) {
        if (layer >= mAnimationLayers.size()) {
            return;
        }

        auto& slot = mAnimationLayers[layer];
        if (slot.animation == nullptr) {
            return;
        }

        slot.rate = rate;
    }

    float LayoutRuntimeActor::getAnimFrame(std::uint32_t layer) const {
        if (layer >= mAnimationLayers.size()) {
            return 0.0F;
        }
        return mAnimationLayers[layer].frame;
    }

    float LayoutRuntimeActor::getAnimRate(std::uint32_t layer) const {
        if (layer >= mAnimationLayers.size()) {
            return 0.0F;
        }
        return mAnimationLayers[layer].rate;
    }

    float LayoutRuntimeActor::getAnimFrameMax(std::uint32_t layer) const {
        if (layer >= mAnimationLayers.size()) {
            return 0.0F;
        }

        const auto* animation = mAnimationLayers[layer].animation;
        if (animation == nullptr) {
            return 0.0F;
        }

        return static_cast< float >(animation->frame_size);
    }

    float LayoutRuntimeActor::getAnimFrameMax(const char* pAnimName) const {
        if (mResource == nullptr || pAnimName == nullptr) {
            return 0.0F;
        }

        const auto found = mResource->animations_by_name.find(normalizeName(pAnimName));
        if (found == mResource->animations_by_name.end()) {
            return 0.0F;
        }

        return static_cast< float >(found->second.frame_size);
    }

    void LayoutRuntimeActor::update(float deltaFrames) {
        if (not mIsAlive) {
            return;
        }

        if (deltaFrames < 0.0F) {
            deltaFrames = 0.0F;
        }

        for (auto& slot : mAnimationLayers) {
            if (slot.animation == nullptr || slot.rate == 0.0F || deltaFrames <= 0.0F) {
                continue;
            }

            slot.frame += deltaFrames * slot.rate;
            if (not slot.animation->loop) {
                const auto end_frame = static_cast< float >(slot.animation->frame_size);
                if (slot.frame > end_frame) {
                    slot.frame = end_frame;
                }
                if (slot.frame < 0.0F) {
                    slot.frame = 0.0F;
                }
            }
        }

        for (auto& slot : mPaneAnimationSlots) {
            if (slot.animation == nullptr || slot.rate == 0.0F || deltaFrames <= 0.0F) {
                continue;
            }

            slot.frame += deltaFrames * slot.rate;
            if (not slot.animation->loop) {
                const auto end_frame = static_cast< float >(slot.animation->frame_size);
                if (slot.frame > end_frame) {
                    slot.frame = end_frame;
                }
                if (slot.frame < 0.0F) {
                    slot.frame = 0.0F;
                }
            }
        }

        rebuildPose();
    }

    void LayoutRuntimeActor::emitEffect(const char*) {
    }

    void LayoutRuntimeActor::deleteEffectAll() {
    }

    void LayoutRuntimeActor::setRootTranslation(float x, float y) {
        mRootTx = x;
        mRootTy = y;
    }

    void LayoutRuntimeActor::setPaneFollowPosition(const char* pPaneName, const float* pX, const float* pY) {
        const std::string pane_name = pPaneName != nullptr ? normalizeName(pPaneName) : std::string {};
        if (pX == nullptr || pY == nullptr) {
            mFollowPositions.erase(pane_name);
            return;
        }

        mFollowPositions[pane_name] = FollowPosition {
            .x = pX,
            .y = pY,
        };
    }

    void LayoutRuntimeActor::replacePaneTexture(const char* pPaneName, const nw4r::lyt::TexMap* pTexMap, std::uint8_t slot) {
        if (pPaneName == nullptr || slot >= 4U) {
            return;
        }

        const auto indices = paneIndicesForVisibilityName(pPaneName);
        for (const auto pane_index : indices) {
            if (pane_index >= mBaseStates.size() || pane_index >= mCommittedStates.size() || pane_index >= mCurrentStates.size()) {
                continue;
            }
            mBaseStates[pane_index].textureOverrides[slot] = pTexMap;
            mCommittedStates[pane_index].textureOverrides[slot] = pTexMap;
            mCurrentStates[pane_index].textureOverrides[slot] = pTexMap;
            if (pane_index < mPaneTextureCache.size()) {
                mPaneTextureCache[pane_index][slot].reset();
            }
        }
    }

    nw4r::lyt::TexMap* LayoutRuntimeActor::getPaneTexture(const char* pPaneName, std::uint8_t slot) const {
        if (mResource == nullptr || pPaneName == nullptr || slot >= 4U) {
            return nullptr;
        }

        const auto found = mPaneIndexByName.find(normalizeName(pPaneName));
        if (found == mPaneIndexByName.end()) {
            return nullptr;
        }

        const auto pane_index = found->second;
        if (pane_index >= mResource->layout.panes.size() || pane_index >= mCurrentStates.size() || pane_index >= mPaneTextureCache.size()) {
            return nullptr;
        }

        const auto* override_texture = mCurrentStates[pane_index].textureOverrides[slot];
        if (override_texture != nullptr) {
            return const_cast<nw4r::lyt::TexMap*>(override_texture);
        }

        const auto* image = resolveTextureForMaterialStage(mResource->layout.panes[pane_index].material_index, slot, mCurrentStates[pane_index]);
        if (image == nullptr) {
            return nullptr;
        }

        u8 wrap_s = 0U;
        u8 wrap_t = 0U;
        const auto material_index = mResource->layout.panes[pane_index].material_index;
        if (material_index >= 0 && static_cast<std::size_t>(material_index) < mResource->layout.materials.size()) {
            const auto& material = mResource->layout.materials[static_cast<std::size_t>(material_index)];
            if (slot < material.texture_maps.size()) {
                wrap_s = material.texture_maps[slot].wrap_s;
                wrap_t = material.texture_maps[slot].wrap_t;
            }
        }

        auto& cached = mPaneTextureCache[pane_index][slot];
        if (cached == nullptr || cached->decodedImage() != image || cached->wrapS() != wrap_s || cached->wrapT() != wrap_t) {
            cached = std::make_unique<nw4r::lyt::TexMap>(image, wrap_s, wrap_t);
        }
        return cached.get();
    }

    void LayoutRuntimeActor::setPaneVisible(const char* pPaneName, bool visible) {
        if (pPaneName == nullptr) {
            return;
        }

        const auto pane_name = normalizeName(pPaneName);
        const auto indices = paneIndicesForVisibilityName(pPaneName);
        if (indices.empty()) {
            return;
        }

        const bool respect_language_visibility = hasLocalizedVisibilityVariant(pane_name);
        for (const auto pane_index : indices) {
            setPaneVisibleAtIndex(pane_index, visible, respect_language_visibility);
        }
    }

    void LayoutRuntimeActor::setPaneVisibleRecursive(const char* pPaneName, bool visible) {
        if (pPaneName == nullptr) {
            return;
        }

        const auto pane_name = normalizeName(pPaneName);
        const auto indices = paneIndicesForVisibilityName(pPaneName);
        if (indices.empty()) {
            return;
        }

        const bool respect_language_visibility = hasLocalizedVisibilityVariant(pane_name);
        for (const auto pane_index : indices) {
            setPaneVisibleInSubtree(pane_index, visible, respect_language_visibility);
        }
    }

    void LayoutRuntimeActor::setTextBoxTextRecursive(const char* pPaneName, std::u16string text) {
        if (mResource == nullptr) {
            return;
        }

        if (pPaneName == nullptr) {
            bool changed = false;
            const auto root_index = mResource->layout.root_pane;
            if (root_index >= 0 && static_cast< std::size_t >(root_index) < mResource->layout.panes.size()) {
                setTextBoxTextInSubtree(static_cast< std::size_t >(root_index), text, &changed);
            }
            if (!changed) {
                for (std::size_t pane_index = 0U; pane_index < mResource->layout.panes.size() && pane_index < mTextOverrides.size(); ++pane_index) {
                    if (mResource->layout.panes[pane_index].type == assets::layout::PaneType::Text) {
                        mTextOverrides[pane_index] = text;
                    }
                }
            }
            return;
        }

        const auto found = mPaneIndexByName.find(normalizeName(pPaneName));
        if (found == mPaneIndexByName.end()) {
            return;
        }

        bool changed = false;
        setTextBoxTextInSubtree(found->second, text, &changed);
        if (!changed && found->second < mTextOverrides.size()) {
            mTextOverrides[found->second] = std::move(text);
        }
    }

    void LayoutRuntimeActor::clearTextBoxTextRecursive(const char* pPaneName) {
        setTextBoxTextRecursive(pPaneName, {});
    }

    void LayoutRuntimeActor::setTextBoxVerticalPositionRecursive(const char* pPaneName, std::uint8_t position) {
        if (mResource == nullptr) {
            return;
        }

        if (pPaneName == nullptr) {
            bool changed = false;
            const auto root_index = mResource->layout.root_pane;
            if (root_index >= 0 && static_cast< std::size_t >(root_index) < mResource->layout.panes.size()) {
                setTextBoxVerticalPositionInSubtree(static_cast< std::size_t >(root_index), position, &changed);
            }
            if (!changed) {
                for (std::size_t pane_index = 0U; pane_index < mResource->layout.panes.size(); ++pane_index) {
                    setTextBoxVerticalPositionInSubtree(pane_index, position, nullptr);
                }
            }
            return;
        }

        const auto found = mPaneIndexByName.find(normalizeName(pPaneName));
        if (found == mPaneIndexByName.end()) {
            return;
        }

        setTextBoxVerticalPositionInSubtree(found->second, position, nullptr);
    }

    bool LayoutRuntimeActor::hasPane(const char* pPaneName) const {
        if (pPaneName == nullptr) {
            return false;
        }

        return mPaneIndexByName.find(normalizeName(pPaneName)) != mPaneIndexByName.end();
    }

    void LayoutRuntimeActor::startPaneAnim(const char* pPaneName, const char* pAnimName, std::uint32_t slotIndex) {
        if (mResource == nullptr || pPaneName == nullptr || pAnimName == nullptr) {
            return;
        }

        const auto pane_name = normalizeName(pPaneName);
        if (mPaneIndexByName.find(pane_name) == mPaneIndexByName.end()) {
            return;
        }

        const auto animation_name = normalizeName(pAnimName);
        const auto animation_found = mResource->animations_by_name.find(animation_name);
        if (animation_found == mResource->animations_by_name.end()) {
            return;
        }

        auto* slot = findPaneAnimationSlot(pane_name, slotIndex);
        if (slot == nullptr) {
            mPaneAnimationSlots.push_back(PaneAnimationSlot{
                .pane_name = pane_name,
                .slot_index = slotIndex,
            });
            slot = &mPaneAnimationSlots.back();
        } else {
            commitPaneAnimationSlot(*slot);
        }

        slot->animation = &animation_found->second;
        slot->frame = 0.0F;
        slot->rate = 1.0F;

        rebuildPose();
    }

    bool LayoutRuntimeActor::isPaneAnimStopped(const char* pPaneName, std::uint32_t slotIndex) const {
        if (pPaneName == nullptr) {
            return true;
        }

        const auto* slot = findPaneAnimationSlot(normalizeName(pPaneName), slotIndex);
        if (slot == nullptr || slot->animation == nullptr) {
            return true;
        }
        if (slot->animation->loop) {
            return false;
        }

        return slot->frame >= static_cast< float >(slot->animation->frame_size);
    }

    void LayoutRuntimeActor::setPaneAnimFrame(const char* pPaneName, float frame, std::uint32_t slotIndex) {
        if (pPaneName == nullptr) {
            return;
        }

        auto* slot = findPaneAnimationSlot(normalizeName(pPaneName), slotIndex);
        if (slot == nullptr || slot->animation == nullptr) {
            return;
        }

        slot->frame = frame;
        rebuildPose();
    }

    float LayoutRuntimeActor::getPaneAnimFrame(const char* pPaneName, std::uint32_t slotIndex) const {
        if (pPaneName == nullptr) {
            return 0.0F;
        }

        const auto* slot = findPaneAnimationSlot(normalizeName(pPaneName), slotIndex);
        if (slot == nullptr) {
            return 0.0F;
        }

        return slot->frame;
    }

    void LayoutRuntimeActor::setPaneAnimRate(const char* pPaneName, float rate, std::uint32_t slotIndex) {
        if (pPaneName == nullptr) {
            return;
        }

        auto* slot = findPaneAnimationSlot(normalizeName(pPaneName), slotIndex);
        if (slot == nullptr) {
            return;
        }

        slot->rate = rate;
    }

    const assets::layout::tpl::DecodedImage* LayoutRuntimeActor::resolveTextureForMaterial(std::int32_t materialIndex) const {
        if (mResource == nullptr) {
            return nullptr;
        }

        if (materialIndex < 0 || static_cast< std::size_t >(materialIndex) >= mResource->layout.materials.size()) {
            return nullptr;
        }

        const auto& material = mResource->layout.materials[static_cast< std::size_t >(materialIndex)];
        return resolveTextureByLayoutIndex(material.texture_index);
    }

    const assets::layout::tpl::DecodedImage* LayoutRuntimeActor::resolveTexturePatternByLayoutIndex(
        std::int32_t textureIndex,
        std::int32_t patternIndex) const {
        if (mResource == nullptr || textureIndex < 0 || patternIndex < 0) {
            return nullptr;
        }
        if (static_cast< std::size_t >(textureIndex) >= mResource->layout.texture_names.size()) {
            return nullptr;
        }

        const auto baseName = baseWithoutExtension(mResource->layout.texture_names[static_cast< std::size_t >(textureIndex)]);
        const auto patternName = texturePatternName(baseName, patternIndex);
        const auto found = mResource->textures_by_name.find(normalizeName(patternName));
        if (found != mResource->textures_by_name.end()) {
            return &found->second;
        }

        const auto indexedTexture = textureIndex + patternIndex;
        if (indexedTexture != textureIndex) {
            return resolveTextureByLayoutIndex(indexedTexture);
        }

        return nullptr;
    }

    const assets::layout::tpl::DecodedImage* LayoutRuntimeActor::resolveTextureByName(std::string_view textureName) const {
        if (mResource == nullptr || textureName.empty()) {
            return nullptr;
        }

        const auto baseName = baseWithoutExtension(std::string(textureName));
        const auto found = mResource->textures_by_name.find(normalizeName(baseName));
        if (found == mResource->textures_by_name.end()) {
            return nullptr;
        }

        return &found->second;
    }

    const assets::layout::tpl::DecodedImage* LayoutRuntimeActor::resolveTextureForMaterialStage(
        std::int32_t materialIndex,
        std::size_t stage,
        const RuntimePaneState& state) const {
        if (mResource == nullptr) {
            return nullptr;
        }

        if (materialIndex < 0 || static_cast< std::size_t >(materialIndex) >= mResource->layout.materials.size()) {
            return nullptr;
        }

        const auto& material = mResource->layout.materials[static_cast< std::size_t >(materialIndex)];
        if (stage >= material.texture_indices.size()) {
            return nullptr;
        }

        if (stage < state.textureOverrides.size() && state.textureOverrides[stage] != nullptr) {
            return state.textureOverrides[stage]->decodedImage();
        }

        const auto textureIndex = material.texture_indices[stage];
        if (stage < state.texturePatternName.size() && state.texturePatternName[stage].has_value()) {
            if (const auto* patternTexture = resolveTextureByName(*state.texturePatternName[stage])) {
                return patternTexture;
            }
        }

        if (stage < state.texturePattern.size() && state.texturePattern[stage].has_value()) {
            if (const auto* patternTexture = resolveTexturePatternByLayoutIndex(textureIndex, *state.texturePattern[stage])) {
                return patternTexture;
            }
        }

        return resolveTextureByLayoutIndex(textureIndex);
    }

    const std::u16string& LayoutRuntimeActor::textForPane(std::size_t paneIndex) const {
        static const std::u16string sEmptyText {};
        if (mResource == nullptr || paneIndex >= mResource->layout.panes.size()) {
            return sEmptyText;
        }

        if (paneIndex < mTextOverrides.size() && mTextOverrides[paneIndex].has_value()) {
            return *mTextOverrides[paneIndex];
        }

        return mResource->layout.panes[paneIndex].text;
    }

    const assets::layout::tpl::DecodedImage* LayoutRuntimeActor::resolveTextureByLayoutIndex(std::int32_t textureIndex) const {
        if (mResource == nullptr) {
            return nullptr;
        }
        if (textureIndex < 0 || static_cast< std::size_t >(textureIndex) >= mResource->layout.texture_names.size()) {
            return nullptr;
        }

        const auto textureName = baseWithoutExtension(mResource->layout.texture_names[static_cast< std::size_t >(textureIndex)]);
        const auto found = mResource->textures_by_name.find(normalizeName(textureName));
        if (found == mResource->textures_by_name.end()) {
            return nullptr;
        }
        return &found->second;
    }

    bool LayoutRuntimeActor::chooseColorAndMaskTextures(std::int32_t materialIndex, const RuntimePaneState& state,
                                                        const assets::layout::tpl::DecodedImage** colorTexture,
                                                        std::size_t* colorStage, const assets::layout::tpl::DecodedImage** maskTexture,
                                                        std::size_t* maskStage, bool* invertMask, bool* maskUsesAlpha) const {
        if (colorTexture == nullptr || colorStage == nullptr || maskTexture == nullptr || maskStage == nullptr || invertMask == nullptr ||
            maskUsesAlpha == nullptr) {
            return false;
        }

        // Keep the caller-provided color texture as the fallback when mask selection is rejected.
        *colorStage = 0U;
        *maskTexture = nullptr;
        *maskStage = 0U;
        *invertMask = false;
        *maskUsesAlpha = false;

        if (mResource == nullptr) {
            return false;
        }
        if (materialIndex < 0 || static_cast< std::size_t >(materialIndex) >= mResource->layout.materials.size()) {
            return false;
        }

        const auto& material = mResource->layout.materials[static_cast< std::size_t >(materialIndex)];
        if (material.texture_indices.size() < 2U) {
            return false;
        }

        const auto* stage0 = resolveTextureForMaterialStage(materialIndex, 0U, state);
        const auto* stage1 = resolveTextureForMaterialStage(materialIndex, 1U, state);
        if (stage0 == nullptr || stage1 == nullptr || stage0->empty() || stage1->empty()) {
            return false;
        }

        const bool stage0_uses_alpha = has_non_opaque_alpha(*stage0);
        const bool stage1_uses_alpha = has_non_opaque_alpha(*stage1);
        const float colorfulness0 = estimate_colorfulness(*stage0);
        const float colorfulness1 = estimate_colorfulness(*stage1);

        bool stage0_is_mask = (stage0_uses_alpha != stage1_uses_alpha) ? stage0_uses_alpha : (colorfulness0 <= colorfulness1);
        const bool stage0_is_narrow_lookup = stage0->width * 2U <= stage1->width && stage0->height <= stage1->height;
        const bool stage1_is_narrow_lookup = stage1->width * 2U <= stage0->width && stage1->height <= stage0->height;
        const bool forced_narrow_lookup_pair = stage0_is_narrow_lookup != stage1_is_narrow_lookup;
        if (forced_narrow_lookup_pair) {
            stage0_is_mask = !stage0_is_narrow_lookup;
        }
        const auto* selected_mask = stage0_is_mask ? stage0 : stage1;
        const auto* selected_color = stage0_is_mask ? stage1 : stage0;
        const std::size_t selected_mask_stage = stage0_is_mask ? 0U : 1U;
        const std::size_t selected_color_stage = stage0_is_mask ? 1U : 0U;

        const bool use_alpha_channel = has_non_opaque_alpha(*selected_mask);
        const float mask_colorfulness = estimate_colorfulness(*selected_mask);
        const float color_texture_colorfulness = estimate_colorfulness(*selected_color);
        const auto channel_stats = estimate_mask_channel_stats(*selected_mask, use_alpha_channel);

        // Keep mask usage conservative to avoid accidentally zeroing unrelated panes.
        if ((channel_stats.maximum - channel_stats.minimum) < 0.10F) {
            return false;
        }
        if (not use_alpha_channel && mask_colorfulness > 0.12F) {
            return false;
        }
        if (not forced_narrow_lookup_pair && not use_alpha_channel && std::fabs(color_texture_colorfulness - mask_colorfulness) < 0.015F) {
            return false;
        }
        if (color_texture_colorfulness + 0.01F < mask_colorfulness && not use_alpha_channel) {
            return false;
        }

        *colorTexture = selected_color;
        *colorStage = selected_color_stage;
        *maskTexture = selected_mask;
        *maskStage = selected_mask_stage;
        *maskUsesAlpha = use_alpha_channel;
        *invertMask = should_invert_mask_alpha(*selected_mask, use_alpha_channel);
        return true;
    }

    void LayoutRuntimeActor::applyPicture(
        std::size_t paneIndex,
        const TransformContext& parent,
        render::layout::LayoutDrawList* pDrawList,
        std::int32_t materialIndexOverride) const {
        const auto* pane = paneDefinition(paneIndex);
        const auto* state = paneState(paneIndex);
        if (pane == nullptr || state == nullptr) {
            return;
        }

        const auto material_index = materialIndexOverride >= 0 ? materialIndexOverride : pane->material_index;
        applyMaterialQuad(
            paneIndex,
            parent,
            pDrawList,
            material_index,
            0.0F,
            0.0F,
            state->width,
            state->height,
            pane->tex_coords,
            state->vertexColor,
            true);
    }

    void LayoutRuntimeActor::applyMaterialQuad(
        std::size_t paneIndex,
        const TransformContext& parent,
        render::layout::LayoutDrawList* pDrawList,
        std::int32_t materialIndex,
        float localX,
        float localY,
        float localWidth,
        float localHeight,
        const std::array< float, 8 >& texCoords,
        const std::array< std::uint8_t, 16 >& vertexColor,
        bool usePaneTextureTransform) const {
        if (mResource == nullptr || pDrawList == nullptr) {
            return;
        }

        const auto* pane = paneDefinition(paneIndex);
        const auto* state = paneState(paneIndex);
        if (pane == nullptr || state == nullptr) {
            return;
        }

        const float location_scale_x = location_adjust_scale_x(*pane);
        const float world_scale_x = parent.scaleX * state->sx * location_scale_x;
        const float world_scale_y = parent.scaleY * state->sy;

        float pane_origin_x = parent.originX + state->tx * parent.scaleX;
        float pane_origin_y = parent.originY - state->ty * parent.scaleY;
        const auto follow_position = followPosition(paneIndex, *pane);
        if (follow_position.x != nullptr && follow_position.y != nullptr) {
            pane_origin_x = *follow_position.x;
            pane_origin_y = *follow_position.y;
        }

        const float draw_x = pane_origin_x + (anchorOffsetX(pane->base_position, state->width) + localX) * world_scale_x;
        const float draw_y = pane_origin_y + (anchorOffsetY(pane->base_position, state->height) + localY) * world_scale_y;
        const float draw_w = localWidth * world_scale_x;
        const float draw_h = localHeight * world_scale_y;

        if (std::fabs(draw_w) < 0.00001F || std::fabs(draw_h) < 0.00001F) {
            return;
        }

        const float alpha = parent.alpha * (static_cast< float >(state->alpha) / 255.0F);
        const float rotation_z = parent.rotationZ + state->rz;
        const bool use_custom_vertices = std::fabs(rotation_z) > 0.0001F;

        float x_tl = draw_x;
        float y_tl = draw_y;
        float x_tr = draw_x + draw_w;
        float y_tr = draw_y;
        float x_bl = draw_x;
        float y_bl = draw_y + draw_h;
        float x_br = draw_x + draw_w;
        float y_br = draw_y + draw_h;

        if (use_custom_vertices) {
            constexpr float DEG_TO_RAD = 0.017453292519943295F;
            const float radians = rotation_z * DEG_TO_RAD;
            const float c = std::cos(radians);
            const float s = std::sin(radians);
            const float local_x0 = (anchorOffsetX(pane->base_position, state->width) + localX) * world_scale_x;
            const float local_y0 = (anchorOffsetY(pane->base_position, state->height) + localY) * world_scale_y;
            const float local_x1 = local_x0 + draw_w;
            const float local_y1 = local_y0 + draw_h;
            const auto rotate_point = [&](float x, float y, float* pX, float* pY) {
                *pX = pane_origin_x + c * x - s * y;
                *pY = pane_origin_y + s * x + c * y;
            };

            rotate_point(local_x0, local_y0, &x_tl, &y_tl);
            rotate_point(local_x1, local_y0, &x_tr, &y_tr);
            rotate_point(local_x0, local_y1, &x_bl, &y_bl);
            rotate_point(local_x1, local_y1, &x_br, &y_br);
        }

        const auto* material = (materialIndex >= 0 && static_cast< std::size_t >(materialIndex) < mResource->layout.materials.size()) ?
                                   &mResource->layout.materials[static_cast< std::size_t >(materialIndex)] :
                                   nullptr;
        const auto* texture = resolveTextureForMaterialStage(materialIndex, 0U, *state);

        const assets::layout::tpl::DecodedImage* colorTexture = texture;
        const assets::layout::tpl::DecodedImage* maskTexture = nullptr;
        std::size_t colorStage = 0U;
        std::size_t maskStage = 0U;
        bool invertMask = false;
        bool maskUsesAlpha = false;
        bool hasMask =
            chooseColorAndMaskTextures(materialIndex, *state, &colorTexture, &colorStage, &maskTexture, &maskStage, &invertMask, &maskUsesAlpha);
        if (not hasMask && colorTexture == nullptr) {
            colorTexture = texture;
            colorStage = 0U;
        }

        bool useNarrowLookupAsAuxiliaryTexture = false;
        if (material != nullptr && material->texture_indices.size() == 2U) {
            const auto* stage0 = resolveTextureForMaterialStage(materialIndex, 0U, *state);
            const auto* stage1 = resolveTextureForMaterialStage(materialIndex, 1U, *state);
            if (stage0 != nullptr && stage1 != nullptr) {
                const bool stage0_is_narrow_lookup = stage0->width * 2U <= stage1->width && stage0->height <= stage1->height;
                const bool stage1_is_narrow_lookup = stage1->width * 2U <= stage0->width && stage1->height <= stage0->height;
                const bool has_narrow_auxiliary = stage0_is_narrow_lookup != stage1_is_narrow_lookup;
                const auto* visible_texture = stage0_is_narrow_lookup ? stage1 : stage0;
                const std::size_t visible_stage = stage0_is_narrow_lookup ? 1U : 0U;
                if (has_narrow_auxiliary && has_non_opaque_alpha(*visible_texture)) {
                    colorTexture = visible_texture;
                    colorStage = visible_stage;
                    maskTexture = nullptr;
                    maskStage = 0U;
                    hasMask = false;
                    invertMask = false;
                    maskUsesAlpha = false;
                    useNarrowLookupAsAuxiliaryTexture = true;
                }
            }
        }

        const auto blendMode = material != nullptr && material->blend_mode == assets::layout::MaterialBlendMode::Additive ?
                                   render::layout::BlendMode::Additive :
                                   render::layout::BlendMode::Alpha;
        const bool useTwoColorIaMaterial = material != nullptr && material->tev_stage_count > 1 && !useNarrowLookupAsAuxiliaryTexture;
        const bool useOneTextureColorLerp =
            material != nullptr && material->tev_stage_count == 0 && material->texture_indices.size() == 1U && colorTexture != nullptr &&
            estimate_colorfulness(*colorTexture) <= 0.01F && isWhiteColorRegister(material->texture_color) &&
            colorRegistersDiffer(material->texture_color, material->font_color);

        const auto color_for_vertex = [&](std::size_t vertex_index) {
            const auto base = vertex_index * 4U;
            const auto source_r = static_cast< float >(vertexColor[base + 0U]) / 255.0F;
            const auto source_g = static_cast< float >(vertexColor[base + 1U]) / 255.0F;
            const auto source_b = static_cast< float >(vertexColor[base + 2U]) / 255.0F;
            const auto source_a = static_cast< float >(vertexColor[base + 3U]) / 255.0F;

            const auto mat_r = material != nullptr ? static_cast< float >(material->mat_color[0U]) / 255.0F : 1.0F;
            const auto mat_g = material != nullptr ? static_cast< float >(material->mat_color[1U]) / 255.0F : 1.0F;
            const auto mat_b = material != nullptr ? static_cast< float >(material->mat_color[2U]) / 255.0F : 1.0F;
            const auto mat_a = material != nullptr ? static_cast< float >(material->mat_color[3U]) / 255.0F : 1.0F;
            const auto* tev_color = [&]() -> const std::array< std::uint8_t, 4 >* {
                if (material == nullptr) {
                    return nullptr;
                }
                if (material->tev_stage_count > 1) {
                    return nullptr;
                }
                if (colorTexture == nullptr) {
                    return &material->texture_color;
                }
                const bool texture_color_is_black =
                    material->texture_color[0U] <= 64U && material->texture_color[1U] <= 64U && material->texture_color[2U] <= 64U;
                const bool font_color_has_rgb =
                    material->font_color[0U] != 0U || material->font_color[1U] != 0U || material->font_color[2U] != 0U;
                if (texture_color_is_black && font_color_has_rgb) {
                    return &material->font_color;
                }
                if (useOneTextureColorLerp) {
                    return &material->font_color;
                }
                return &material->texture_color;
            }();
            const bool use_simple_tev_color = tev_color != nullptr;
            const auto tev_r = use_simple_tev_color && tev_color != nullptr ? static_cast< float >((*tev_color)[0U]) / 255.0F : 1.0F;
            const auto tev_g = use_simple_tev_color && tev_color != nullptr ? static_cast< float >((*tev_color)[1U]) / 255.0F : 1.0F;
            const auto tev_b = use_simple_tev_color && tev_color != nullptr ? static_cast< float >((*tev_color)[2U]) / 255.0F : 1.0F;
            const auto tev_alpha = use_simple_tev_color && tev_color != nullptr && (*tev_color)[3U] == 0U && material != nullptr &&
                                       material->font_color[3U] != 0U ?
                                       material->font_color[3U] :
                                       (use_simple_tev_color && tev_color != nullptr ? (*tev_color)[3U] : 255U);
            const auto tev_a = static_cast< float >(tev_alpha) / 255.0F;

            return render::layout::pack_abgr(
                clampU8(source_r * mat_r * tev_r * 255.0F),
                clampU8(source_g * mat_g * tev_g * 255.0F),
                clampU8(source_b * mat_b * tev_b * 255.0F),
                clampU8(source_a * mat_a * tev_a * alpha * 255.0F));
        };

        const auto apply_tex_srt = [](float u, float v, float translate_u, float translate_v, float rotate, float scale_u, float scale_v) {
            constexpr float DEG_TO_RAD = 0.017453292519943295F;
            const float radians = rotate * DEG_TO_RAD;
            const float c = std::cos(radians);
            const float s = std::sin(radians);
            const float centered_u = u - 0.5F;
            const float centered_v = v - 0.5F;
            const float a0 = c * scale_u;
            const float a1 = -s * scale_v;
            const float b0 = s * scale_u;
            const float b1 = c * scale_v;
            return std::pair< float, float >{
                translate_u + 0.5F + a0 * centered_u + a1 * centered_v,
                translate_v + 0.5F + b0 * centered_u + b1 * centered_v,
            };
        };

        const auto sample_tex_coord = [&](std::size_t stage, float u, float v) -> std::pair< float, float > {
            if (usePaneTextureTransform) {
                const auto pane_srt = apply_tex_srt(u, v, state->texOffsetU[stage], state->texOffsetV[stage], state->texRotate[stage],
                                                    state->texScaleU[stage], state->texScaleV[stage]);
                u = pane_srt.first;
                v = pane_srt.second;
            }

            if (material != nullptr && stage < material->texture_srts.size()) {
                const auto& srt = material->texture_srts[stage];
                const auto material_srt = apply_tex_srt(u, v, srt.translate.x, srt.translate.y, srt.rotate, srt.scale.x, srt.scale.y);
                u = material_srt.first;
                v = material_srt.second;
            }

            return {u, v};
        };

        const auto make_texture_ref = [&](const assets::layout::tpl::DecodedImage* image, std::size_t stage) {
            if (stage < state->textureOverrides.size() && state->textureOverrides[stage] != nullptr) {
                const auto* override_texture = state->textureOverrides[stage];
                const auto* override_image = override_texture->decodedImage();
                return render::layout::TextureRef{
                    .id = override_texture->textureId(),
                    .rgba8 = override_image != nullptr && not override_image->rgba8.empty() ? override_image->rgba8.data() : nullptr,
                    .width = override_image != nullptr ? override_image->width : static_cast<std::uint16_t>(0U),
                    .height = override_image != nullptr ? override_image->height : static_cast<std::uint16_t>(0U),
                    .wrap_s = override_texture->wrapS(),
                    .wrap_t = override_texture->wrapT(),
                };
            }

            std::uint8_t wrap_s = 0U;
            std::uint8_t wrap_t = 0U;
            if (material != nullptr && stage < material->texture_maps.size()) {
                wrap_s = material->texture_maps[stage].wrap_s;
                wrap_t = material->texture_maps[stage].wrap_t;
            }

            return render::layout::TextureRef{
                .id = image != nullptr ? static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(image)) : 0U,
                .rgba8 = image != nullptr && not image->rgba8.empty() ? image->rgba8.data() : nullptr,
                .width = image != nullptr ? image->width : static_cast< std::uint16_t >(0U),
                .height = image != nullptr ? image->height : static_cast< std::uint16_t >(0U),
                .wrap_s = wrap_s,
                .wrap_t = wrap_t,
            };
        };

        const std::size_t primaryStage = std::min< std::size_t >(colorStage, state->texOffsetU.size() - 1U);
        const std::size_t secondaryStage = std::min< std::size_t >(maskStage, state->texOffsetU.size() - 1U);
        const auto primary_tl = sample_tex_coord(primaryStage, texCoords[0U], texCoords[1U]);
        const auto primary_tr = sample_tex_coord(primaryStage, texCoords[2U], texCoords[3U]);
        const auto primary_bl = sample_tex_coord(primaryStage, texCoords[4U], texCoords[5U]);
        const auto primary_br = sample_tex_coord(primaryStage, texCoords[6U], texCoords[7U]);
        const auto secondary_tl = sample_tex_coord(secondaryStage, texCoords[0U], texCoords[1U]);
        const auto secondary_tr = sample_tex_coord(secondaryStage, texCoords[2U], texCoords[3U]);
        const auto secondary_bl = sample_tex_coord(secondaryStage, texCoords[4U], texCoords[5U]);
        const auto secondary_br = sample_tex_coord(secondaryStage, texCoords[6U], texCoords[7U]);

        pDrawList->push_quad(render::layout::QuadCommand{
            .x0 = use_custom_vertices ? std::min({x_tl, x_tr, x_bl, x_br}) : draw_x,
            .y0 = use_custom_vertices ? std::min({y_tl, y_tr, y_bl, y_br}) : draw_y,
            .x1 = use_custom_vertices ? std::max({x_tl, x_tr, x_bl, x_br}) : draw_x + draw_w,
            .y1 = use_custom_vertices ? std::max({y_tl, y_tr, y_bl, y_br}) : draw_y + draw_h,
            .coordinate_width = mResource->layout.size.x,
            .coordinate_height = mResource->layout.size.y,
            .use_custom_vertices = use_custom_vertices,
            .x_tl = x_tl,
            .y_tl = y_tl,
            .x_tr = x_tr,
            .y_tr = y_tr,
            .x_bl = x_bl,
            .y_bl = y_bl,
            .x_br = x_br,
            .y_br = y_br,
            .u0 = primary_tl.first,
            .v0 = primary_tl.second,
            .u1 = primary_br.first,
            .v1 = primary_br.second,
            .u0_secondary = secondary_tl.first,
            .v0_secondary = secondary_tl.second,
            .u1_secondary = secondary_br.first,
            .v1_secondary = secondary_br.second,
            .use_custom_tex_coords = true,
            .u_tl = primary_tl.first,
            .v_tl = primary_tl.second,
            .u_tr = primary_tr.first,
            .v_tr = primary_tr.second,
            .u_bl = primary_bl.first,
            .v_bl = primary_bl.second,
            .u_br = primary_br.first,
            .v_br = primary_br.second,
            .u_tl_secondary = secondary_tl.first,
            .v_tl_secondary = secondary_tl.second,
            .u_tr_secondary = secondary_tr.first,
            .v_tr_secondary = secondary_tr.second,
            .u_bl_secondary = secondary_bl.first,
            .v_bl_secondary = secondary_bl.second,
            .u_br_secondary = secondary_br.first,
            .v_br_secondary = secondary_br.second,
            .color_tl = color_for_vertex(0U),
            .color_tr = color_for_vertex(1U),
            .color_bl = color_for_vertex(2U),
            .color_br = color_for_vertex(3U),
            .blend_mode = blendMode,
            .use_mask_texture = hasMask,
            .invert_mask = invertMask,
            .mask_uses_alpha = maskUsesAlpha,
            .texture_alpha_only = useTwoColorIaMaterial,
            .texture_color_lerp = useOneTextureColorLerp,
            .tev_color0 = material != nullptr ? packMaterialColor(material->texture_color) : render::layout::pack_abgr(0U, 0U, 0U, 0U),
            .tev_color1 = material != nullptr ? packMaterialColor(material->font_color) : render::layout::pack_abgr(255U, 255U, 255U, 255U),
            .tev_color_scale = explicitTevStageColorScale(material),
            .texture = make_texture_ref(colorTexture, primaryStage),
            .mask_texture = make_texture_ref(maskTexture, secondaryStage),
        });
    }

    void LayoutRuntimeActor::applyWindow(
        std::size_t paneIndex,
        const TransformContext& parent,
        render::layout::LayoutDrawList* pDrawList) const {
        const auto* pane = paneDefinition(paneIndex);
        const auto* state = paneState(paneIndex);
        if (pane == nullptr || state == nullptr || pDrawList == nullptr) {
            return;
        }

        WindowFrameSize frame_size {};
        if (pane->window_frames.size() == 1U) {
            const auto* texture = resolveTextureForMaterial(pane->window_frames[0U].material_index);
            if (texture != nullptr) {
                frame_size.left = static_cast< float >(texture->width);
                frame_size.right = static_cast< float >(texture->width);
                frame_size.top = static_cast< float >(texture->height);
                frame_size.bottom = static_cast< float >(texture->height);
            }
        } else if (pane->window_frames.size() == 4U || pane->window_frames.size() == 8U) {
            const auto* left_top = resolveTextureForMaterial(pane->window_frames[0U].material_index);
            const auto* right_bottom = resolveTextureForMaterial(pane->window_frames[3U].material_index);
            if (left_top != nullptr) {
                frame_size.left = static_cast< float >(left_top->width);
                frame_size.top = static_cast< float >(left_top->height);
            }
            if (right_bottom != nullptr) {
                frame_size.right = static_cast< float >(right_bottom->width);
                frame_size.bottom = static_cast< float >(right_bottom->height);
            }
        }

        applyMaterialQuad(
            paneIndex,
            parent,
            pDrawList,
            pane->material_index,
            frame_size.left - pane->window_content_inflation.left,
            frame_size.top - pane->window_content_inflation.top,
            state->width - frame_size.left - frame_size.right + pane->window_content_inflation.left + pane->window_content_inflation.right,
            state->height - frame_size.top - frame_size.bottom + pane->window_content_inflation.top + pane->window_content_inflation.bottom,
            pane->tex_coords,
            state->vertexColor,
            true);

        if (pane->window_frames.empty()) {
            return;
        }

        const auto white = solid_vertex_color(255U, 255U, 255U, 255U);
        if (pane->window_frames.size() == 1U) {
            const auto& frame = pane->window_frames[0U];
            const auto* texture = resolveTextureForMaterial(frame.material_index);
            const auto draw_frame_strip = [&](WindowFrameSection section, std::uint8_t textureFlip, float x, float y, float width, float height) {
                if (width <= 0.0F || height <= 0.0F) {
                    return;
                }
                applyMaterialQuad(
                    paneIndex,
                    parent,
                    pDrawList,
                    frame.material_index,
                    x,
                    y,
                    width,
                    height,
                    window_frame_tex_coords(section, textureFlip, width, height, texture),
                    white,
                    false);
            };

            draw_frame_strip(WindowFrameSection::LeftTop, 0U, 0.0F, 0.0F, state->width - frame_size.right, frame_size.top);
            draw_frame_strip(WindowFrameSection::RightTop, 1U, state->width - frame_size.right, 0.0F, frame_size.right, state->height - frame_size.bottom);
            draw_frame_strip(WindowFrameSection::RightBottom, 4U, frame_size.left, state->height - frame_size.bottom, state->width - frame_size.left, frame_size.bottom);
            draw_frame_strip(WindowFrameSection::LeftBottom, 2U, 0.0F, frame_size.top, frame_size.left, state->height - frame_size.top);
            return;
        }

        const auto draw_frame_quad = [&](std::size_t frameIndex, WindowFrameSection section, float x, float y, float width, float height) {
            if (frameIndex >= pane->window_frames.size() || width <= 0.0F || height <= 0.0F) {
                return;
            }

            const auto& frame = pane->window_frames[frameIndex];
            const auto* texture = resolveTextureForMaterial(frame.material_index);
            applyMaterialQuad(
                paneIndex,
                parent,
                pDrawList,
                frame.material_index,
                x,
                y,
                width,
                height,
                window_frame_tex_coords(section, frame.texture_flip, width, height, texture),
                white,
                false);
        };

        if (pane->window_frames.size() == 4U) {
            draw_frame_quad(0U, WindowFrameSection::LeftTop, 0.0F, 0.0F, state->width - frame_size.right, frame_size.top);
            draw_frame_quad(1U, WindowFrameSection::RightTop, state->width - frame_size.right, 0.0F, frame_size.right, state->height - frame_size.bottom);
            draw_frame_quad(3U, WindowFrameSection::RightBottom, frame_size.left, state->height - frame_size.bottom, state->width - frame_size.left, frame_size.bottom);
            draw_frame_quad(2U, WindowFrameSection::LeftBottom, 0.0F, frame_size.top, frame_size.left, state->height - frame_size.top);
            return;
        }

        if (pane->window_frames.size() == 8U) {
            const float center_width = state->width - frame_size.left - frame_size.right;
            const float center_height = state->height - frame_size.top - frame_size.bottom;
            draw_frame_quad(0U, WindowFrameSection::LeftTop, 0.0F, 0.0F, frame_size.left, frame_size.top);
            draw_frame_quad(6U, WindowFrameSection::LeftTop, frame_size.left, 0.0F, center_width, frame_size.top);
            draw_frame_quad(1U, WindowFrameSection::RightTop, state->width - frame_size.right, 0.0F, frame_size.right, frame_size.top);
            draw_frame_quad(5U, WindowFrameSection::RightTop, state->width - frame_size.right, frame_size.top, frame_size.right, center_height);
            draw_frame_quad(3U, WindowFrameSection::RightBottom, state->width - frame_size.right, state->height - frame_size.bottom, frame_size.right, frame_size.bottom);
            draw_frame_quad(7U, WindowFrameSection::RightBottom, frame_size.left, state->height - frame_size.bottom, center_width, frame_size.bottom);
            draw_frame_quad(2U, WindowFrameSection::LeftBottom, 0.0F, state->height - frame_size.bottom, frame_size.left, frame_size.bottom);
            draw_frame_quad(4U, WindowFrameSection::LeftBottom, 0.0F, frame_size.top, frame_size.left, center_height);
        }
    }

    void LayoutRuntimeActor::applyText(std::size_t paneIndex, const TransformContext& parent, render::layout::LayoutDrawList* pDrawList) const {
        if (mResource == nullptr || pDrawList == nullptr) {
            return;
        }

        const auto* pane = paneDefinition(paneIndex);
        const auto* state = paneState(paneIndex);
        if (pane == nullptr || state == nullptr) {
            return;
        }

        if (pane->font_index < 0 || static_cast< std::size_t >(pane->font_index) >= mResource->layout.font_names.size()) {
            return;
        }

        const auto font_keys =
            LayoutArchiveLoader::make_font_name_lookup_keys(mResource->layout.font_names[static_cast< std::size_t >(pane->font_index)]);
        const assets::layout::BrfntFont* font = nullptr;
        for (const auto& font_key : font_keys) {
            const auto font_found = mResource->fonts_by_name.find(font_key);
            if (font_found != mResource->fonts_by_name.end()) {
                font = &font_found->second;
                break;
            }
        }
        if (font == nullptr) {
            return;
        }

        const assets::layout::BrfntFont* picture_font = nullptr;
        const auto picture_font_keys = LayoutArchiveLoader::make_font_name_lookup_keys("PictureFont.brfnt");
        for (const auto& font_key : picture_font_keys) {
            const auto font_found = mResource->fonts_by_name.find(font_key);
            if (font_found != mResource->fonts_by_name.end()) {
                picture_font = &font_found->second;
                break;
            }
        }

        if (font->sheets().empty() || font->cell_width() == 0U || font->cell_height() == 0U) {
            return;
        }

        const float world_scale_x = parent.scaleX * state->sx * location_adjust_scale_x(*pane);
        const float world_scale_y = parent.scaleY * state->sy;

        float pane_origin_x = parent.originX + state->tx * parent.scaleX;
        float pane_origin_y = parent.originY - state->ty * parent.scaleY;
        const auto follow_position = followPosition(paneIndex, *pane);
        if (follow_position.x != nullptr && follow_position.y != nullptr) {
            pane_origin_x = *follow_position.x;
            pane_origin_y = *follow_position.y;
        }

        const float draw_x = pane_origin_x + anchorOffsetX(pane->base_position, state->width) * world_scale_x;
        const float draw_y = pane_origin_y + anchorOffsetY(pane->base_position, state->height) * world_scale_y;

        const float alpha = parent.alpha * (static_cast< float >(state->alpha) / 255.0F);

        const float text_scale_x =
            pane->text_font_size.x > 0.0F ? pane->text_font_size.x / static_cast< float >(std::max< std::uint8_t >(1U, font->font_width())) : 1.0F;
        const float text_scale_y =
            pane->text_font_size.y > 0.0F ? pane->text_font_size.y / static_cast< float >(std::max< std::uint8_t >(1U, font->font_height())) : 1.0F;

        std::array< std::uint8_t, 4 > material_font_color{255U, 255U, 255U, 255U};
        if (pane->material_index >= 0 && static_cast< std::size_t >(pane->material_index) < mResource->layout.materials.size()) {
            material_font_color = mResource->layout.materials[static_cast< std::size_t >(pane->material_index)].font_color;
        }

        const auto pack_text_color = [&](const assets::layout::Color& color) {
            return render::layout::pack_abgr(
                clampU8((static_cast< float >(color.r) / 255.0F) * static_cast< float >(material_font_color[0U])),
                clampU8((static_cast< float >(color.g) / 255.0F) * static_cast< float >(material_font_color[1U])),
                clampU8((static_cast< float >(color.b) / 255.0F) * static_cast< float >(material_font_color[2U])),
                clampU8((static_cast< float >(color.a) / 255.0F) * static_cast< float >(material_font_color[3U]) * alpha));
        };
        const auto packed_top_color = pack_text_color(pane->text_colors[0U]);
        const auto packed_bottom_color = pack_text_color(pane->text_colors[1U]);
        const auto packed_picture_font_color = render::layout::pack_abgr(255U, 255U, 255U, clampU8(alpha * 255.0F));
        const auto pack_private_icon_color = [&](std::uint8_t r, std::uint8_t g, std::uint8_t b) {
            return render::layout::pack_abgr(r, g, b, clampU8(alpha * 255.0F));
        };
        const auto top_color_for_codepoint = [&](char16_t codepoint) {
            if (assets::layout::is_bmg_picture_font_tag(codepoint)) {
                return packed_picture_font_color;
            }

            switch (codepoint) {
            case u'\uE00C':
                return pack_private_icon_color(255U, 229U, 64U);
            case u'\uE00D':
                return pack_private_icon_color(255U, 238U, 40U);
            case u'\uE016':
                return pack_private_icon_color(255U, 105U, 255U);
            default:
                return packed_top_color;
            }
        };
        const auto bottom_color_for_codepoint = [&](char16_t codepoint) {
            if (assets::layout::is_bmg_picture_font_tag(codepoint)) {
                return packed_picture_font_color;
            }

            switch (codepoint) {
            case u'\uE00C':
                return pack_private_icon_color(255U, 152U, 0U);
            case u'\uE00D':
                return pack_private_icon_color(255U, 185U, 0U);
            case u'\uE016':
                return pack_private_icon_color(178U, 50U, 230U);
            default:
                return packed_bottom_color;
            }
        };
        const auto font_for_codepoint = [&](char16_t codepoint, std::uint16_t* pFontCodepoint) -> const assets::layout::BrfntFont* {
            if (assets::layout::is_bmg_picture_font_tag(codepoint) && picture_font != nullptr) {
                if (pFontCodepoint != nullptr) {
                    *pFontCodepoint = assets::layout::bmg_picture_font_codepoint(assets::layout::bmg_picture_font_tag_payload(codepoint));
                }
                return picture_font;
            }

            if (pFontCodepoint != nullptr) {
                *pFontCodepoint = static_cast< std::uint16_t >(codepoint);
            }
            return font;
        };
        const auto font_line_height = static_cast< float >(font->line_feed()) * text_scale_y * world_scale_y;
        const auto line_advance = font_line_height + pane->text_line_space * world_scale_y;
        const auto pane_width = state->width * world_scale_x;
        const auto pane_height = state->height * world_scale_y;
            const auto glyph_height = std::max(1, static_cast< int >(font->cell_height()));
            const auto line_glyph_height = static_cast< float >(glyph_height) * text_scale_y * world_scale_y;
            const auto line_block_height = std::max(line_glyph_height, font_line_height);
            const auto& text = textForPane(paneIndex);
            const auto make_line_codepoints = [&](std::size_t begin, std::size_t end) {
                std::vector< char16_t > codepoints{};
                codepoints.reserve(end - begin);
                for (std::size_t i = begin; i < end; ++i) {
                    if (picture_font != nullptr && i + 1U < end && text[i + 1U] == u'i') {
                        if (const auto picture_tag = staticTextPictureFontTag(text[i])) {
                            codepoints.push_back(*picture_tag);
                            ++i;
                            continue;
                        }
                    }
                    if (picture_font != nullptr) {
                        if (const auto picture_tag = staticTextPictureFontTag(text, i)) {
                            codepoints.push_back(*picture_tag);
                            continue;
                        }
                    }
                    codepoints.push_back(text[i]);
                }
                return codepoints;
            };

            std::vector< std::pair< std::size_t, std::size_t > > line_ranges{};
            line_ranges.reserve(4U);
        std::size_t line_begin = 0U;
        for (std::size_t i = 0U; i < text.size(); ++i) {
            if (text[i] == u'\n') {
                line_ranges.emplace_back(line_begin, i);
                line_begin = i + 1U;
            }
            }
            line_ranges.emplace_back(line_begin, text.size());

            const auto measure_line_width = [&](const std::vector< char16_t >& codepoints) {
                float width = 0.0F;
                for (std::size_t i = 0U; i < codepoints.size(); ++i) {
                    std::uint16_t font_codepoint = 0U;
                    const auto* glyph_font = font_for_codepoint(codepoints[i], &font_codepoint);
                    assets::layout::BrfntGlyph glyph{};
                    if (glyph_font == nullptr || not glyph_font->get_glyph(font_codepoint, &glyph)) {
                        continue;
                    }
                    width += static_cast< float >(glyph.widths.char_width) * text_scale_x * world_scale_x;
                    if (i + 1U < codepoints.size()) {
                        width += pane->text_char_space * text_scale_x * world_scale_x;
                    }
                }
            return width;
        };

        const auto text_block_height = line_ranges.empty() ? 0.0F : line_block_height + static_cast< float >(line_ranges.size() - 1U) * line_advance;
        float block_origin_y = draw_y;
        const auto text_anchor_row = static_cast< std::uint8_t >((state->textPosition / 3U) % 3U);
        if (text_anchor_row == 1U) {
            block_origin_y += (pane_height - text_block_height) * 0.5F;
        } else if (text_anchor_row == 2U) {
            block_origin_y += pane_height - text_block_height;
        }

            float line_origin_y = block_origin_y;
            const auto text_anchor_col = static_cast< std::uint8_t >(state->textPosition % 3U);
            for (const auto& [begin, end] : line_ranges) {
                const auto line_codepoints = make_line_codepoints(begin, end);
                const auto line_width = measure_line_width(line_codepoints);
                float line_origin_x = draw_x;
                if (text_anchor_col == 1U) {
                    line_origin_x += (pane_width - line_width) * 0.5F;
            } else if (text_anchor_col == 2U) {
                line_origin_x += pane_width - line_width;
                }

                float pen_x = line_origin_x;
                for (std::size_t i = 0U; i < line_codepoints.size(); ++i) {
                    const auto codepoint = line_codepoints[i];
                    std::uint16_t font_codepoint = 0U;
                    const auto* glyph_font = font_for_codepoint(codepoint, &font_codepoint);
                    assets::layout::BrfntGlyph glyph{};
                    if (glyph_font == nullptr || not glyph_font->get_glyph(font_codepoint, &glyph)) {
                        continue;
                }

                if (glyph.sheet_index >= glyph_font->sheets().size()) {
                    continue;
                }

                const auto& sheet = glyph_font->sheets()[glyph.sheet_index];
                if (sheet.empty()) {
                    continue;
                }

                    const auto glyph_width = static_cast< int >(glyph.widths.glyph_width);
                    const auto glyph_advance =
                        (static_cast< float >(glyph.widths.char_width) + (i + 1U < line_codepoints.size() ? pane->text_char_space : 0.0F)) * text_scale_x * world_scale_x;
                    if (glyph_width <= 0) {
                        pen_x += glyph_advance;
                        continue;
                }

                const auto quad_x0 = pen_x + static_cast< float >(glyph.widths.left) * text_scale_x * world_scale_x;
                const auto baseline_offset =
                    (static_cast< float >(font->baseline_position()) - static_cast< float >(glyph_font->baseline_position())) * text_scale_y * world_scale_y;
                const auto quad_y0 = line_origin_y + baseline_offset;
                const auto quad_x1 = quad_x0 + static_cast< float >(glyph_width) * text_scale_x * world_scale_x;
                const auto quad_y1 = quad_y0 + static_cast< float >(std::max(1, static_cast< int >(glyph_font->cell_height()))) * text_scale_y * world_scale_y;
                const auto glyph_texture_width = std::min< int >(glyph_width, static_cast< int >(glyph_font->cell_width()));

                const auto u0 = static_cast< float >(glyph.cell_x) / static_cast< float >(sheet.width);
                const auto v0 = static_cast< float >(glyph.cell_y) / static_cast< float >(sheet.height);
                const auto u1 = static_cast< float >(glyph.cell_x + glyph_texture_width) / static_cast< float >(sheet.width);
                const auto v1 = static_cast< float >(glyph.cell_y + glyph_font->cell_height()) / static_cast< float >(sheet.height);

                pDrawList->push_quad(render::layout::QuadCommand{
                    .x0 = quad_x0,
                    .y0 = quad_y0,
                    .x1 = quad_x1,
                    .y1 = quad_y1,
                    .coordinate_width = mResource->layout.size.x,
                    .coordinate_height = mResource->layout.size.y,
                    .u0 = u0,
                    .v0 = v0,
                    .u1 = u1,
                        .v1 = v1,
                        .color_tl = top_color_for_codepoint(codepoint),
                        .color_tr = top_color_for_codepoint(codepoint),
                        .color_bl = bottom_color_for_codepoint(codepoint),
                        .color_br = bottom_color_for_codepoint(codepoint),
                    .texture =
                        render::layout::TextureRef{
                            .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&sheet)),
                            .rgba8 = sheet.rgba8.data(),
                            .width = sheet.width,
                            .height = sheet.height,
                        },
                });

                pen_x += glyph_advance;
            }

            line_origin_y += line_advance;
        }
    }

    void LayoutRuntimeActor::appendPaneRecursive(std::size_t paneIndex, const TransformContext& parent,
                                                 render::layout::LayoutDrawList* pDrawList) const {
        if (mResource == nullptr || pDrawList == nullptr || paneIndex >= mResource->layout.panes.size() || paneIndex >= mCurrentStates.size()) {
            return;
        }

        const auto& pane = mResource->layout.panes[paneIndex];
        const auto& state = mCurrentStates[paneIndex];
        if (not parent.visible || not state.visible) {
            return;
        }

        if (pane.type == assets::layout::PaneType::Picture) {
            applyPicture(paneIndex, parent, pDrawList);
        } else if (pane.type == assets::layout::PaneType::Window) {
            applyWindow(paneIndex, parent, pDrawList);
        } else if (pane.type == assets::layout::PaneType::Text) {
            applyText(paneIndex, parent, pDrawList);
        }

        const float world_scale_x = parent.scaleX * state.sx * location_adjust_scale_x(pane);
        const float world_scale_y = parent.scaleY * state.sy;

        float pane_origin_x = parent.originX + state.tx * parent.scaleX;
        float pane_origin_y = parent.originY - state.ty * parent.scaleY;
        const auto follow_position = followPosition(paneIndex, pane);
        if (follow_position.x != nullptr && follow_position.y != nullptr) {
            pane_origin_x = *follow_position.x;
            pane_origin_y = *follow_position.y;
        }

        const TransformContext child{
            .originX = pane_origin_x,
            .originY = pane_origin_y,
            .scaleX = world_scale_x,
            .scaleY = world_scale_y,
            .rotationZ = parent.rotationZ + state.rz,
            .alpha = parent.alpha * (static_cast< float >(state.alpha) / 255.0F),
            .visible = true,
        };

        for (const auto child_index : pane.children) {
            if (child_index < 0) {
                continue;
            }
            appendPaneRecursive(static_cast< std::size_t >(child_index), child, pDrawList);
        }
    }

    bool LayoutRuntimeActor::getPaneTransRecursive(std::size_t paneIndex, const TransformContext& parent, const std::string& paneName, float* pX,
                                                   float* pY) const {
        if (mResource == nullptr || paneIndex >= mResource->layout.panes.size() || paneIndex >= mCurrentStates.size()) {
            return false;
        }

        const auto& pane = mResource->layout.panes[paneIndex];
        const auto& state = mCurrentStates[paneIndex];
        const float world_scale_x = parent.scaleX * state.sx * location_adjust_scale_x(pane);
        const float world_scale_y = parent.scaleY * state.sy;
        float pane_origin_x = parent.originX + state.tx * parent.scaleX;
        float pane_origin_y = parent.originY - state.ty * parent.scaleY;
        const auto follow_position = followPosition(paneIndex, pane);
        if (follow_position.x != nullptr && follow_position.y != nullptr) {
            pane_origin_x = *follow_position.x;
            pane_origin_y = *follow_position.y;
        }

        if (normalizeName(pane.name) == paneName) {
            if (pX != nullptr) {
                *pX = pane_origin_x;
            }
            if (pY != nullptr) {
                *pY = pane_origin_y;
            }
            return true;
        }

        const TransformContext child{
            .originX = pane_origin_x,
            .originY = pane_origin_y,
            .scaleX = world_scale_x,
            .scaleY = world_scale_y,
            .rotationZ = parent.rotationZ + state.rz,
            .alpha = parent.alpha * (static_cast< float >(state.alpha) / 255.0F),
            .visible = parent.visible && state.visible,
        };

        for (const auto child_index : pane.children) {
            if (child_index < 0) {
                continue;
            }
            if (getPaneTransRecursive(static_cast< std::size_t >(child_index), child, paneName, pX, pY)) {
                return true;
            }
        }

        return false;
    }

    bool LayoutRuntimeActor::getPaneBoundsRecursive(
        std::size_t paneIndex,
        const TransformContext& parent,
        const std::string& paneName,
        float* pX0,
        float* pY0,
        float* pX1,
        float* pY1) const {
        if (mResource == nullptr || paneIndex >= mResource->layout.panes.size() || paneIndex >= mCurrentStates.size()) {
            return false;
        }

        const auto& pane = mResource->layout.panes[paneIndex];
        const auto& state = mCurrentStates[paneIndex];
        if (not parent.visible || not state.visible) {
            return false;
        }

        const float world_scale_x = parent.scaleX * state.sx * location_adjust_scale_x(pane);
        const float world_scale_y = parent.scaleY * state.sy;
        float pane_origin_x = parent.originX + state.tx * parent.scaleX;
        float pane_origin_y = parent.originY - state.ty * parent.scaleY;
        const auto follow_position = followPosition(paneIndex, pane);
        if (follow_position.x != nullptr && follow_position.y != nullptr) {
            pane_origin_x = *follow_position.x;
            pane_origin_y = *follow_position.y;
        }

        if (normalizeName(pane.name) == paneName) {
            const float draw_x = pane_origin_x + anchorOffsetX(pane.base_position, state.width) * world_scale_x;
            const float draw_y = pane_origin_y + anchorOffsetY(pane.base_position, state.height) * world_scale_y;
            const float opposite_x = draw_x + state.width * world_scale_x;
            const float opposite_y = draw_y + state.height * world_scale_y;
            if (pX0 != nullptr) {
                *pX0 = std::min(draw_x, opposite_x);
            }
            if (pY0 != nullptr) {
                *pY0 = std::min(draw_y, opposite_y);
            }
            if (pX1 != nullptr) {
                *pX1 = std::max(draw_x, opposite_x);
            }
            if (pY1 != nullptr) {
                *pY1 = std::max(draw_y, opposite_y);
            }
            return true;
        }

        const TransformContext child{
            .originX = pane_origin_x,
            .originY = pane_origin_y,
            .scaleX = world_scale_x,
            .scaleY = world_scale_y,
            .rotationZ = parent.rotationZ + state.rz,
            .alpha = parent.alpha * (static_cast< float >(state.alpha) / 255.0F),
            .visible = parent.visible && state.visible,
        };

        for (const auto child_index : pane.children) {
            if (child_index < 0) {
                continue;
            }
            if (getPaneBoundsRecursive(static_cast< std::size_t >(child_index), child, paneName, pX0, pY0, pX1, pY1)) {
                return true;
            }
        }

        return false;
    }

    bool LayoutRuntimeActor::getPaneTrans(const char* pPaneName, float* pX, float* pY) const {
        if (mResource == nullptr || pPaneName == nullptr) {
            return false;
        }

        const auto root_index = mResource->layout.root_pane;
        if (root_index < 0 || static_cast< std::size_t >(root_index) >= mResource->layout.panes.size()) {
            return false;
        }

        const TransformContext root{
            .originX = (mResource->layout.center_origin ? (mResource->layout.size.x * 0.5F) : 0.0F) + mRootTx,
            .originY = (mResource->layout.center_origin ? (mResource->layout.size.y * 0.5F) : 0.0F) + mRootTy,
            .scaleX = 1.0F,
            .scaleY = 1.0F,
            .alpha = 1.0F,
            .visible = true,
        };

        return getPaneTransRecursive(static_cast< std::size_t >(root_index), root, normalizeName(pPaneName), pX, pY);
    }

    bool LayoutRuntimeActor::getPaneBounds(const char* pPaneName, float* pX0, float* pY0, float* pX1, float* pY1) const {
        if (mResource == nullptr || pPaneName == nullptr) {
            return false;
        }

        const auto root_index = mResource->layout.root_pane;
        if (root_index < 0 || static_cast< std::size_t >(root_index) >= mResource->layout.panes.size()) {
            return false;
        }

        const TransformContext root{
            .originX = (mResource->layout.center_origin ? (mResource->layout.size.x * 0.5F) : 0.0F) + mRootTx,
            .originY = (mResource->layout.center_origin ? (mResource->layout.size.y * 0.5F) : 0.0F) + mRootTy,
            .scaleX = 1.0F,
            .scaleY = 1.0F,
            .alpha = 1.0F,
            .visible = true,
        };

        return getPaneBoundsRecursive(static_cast< std::size_t >(root_index), root, normalizeName(pPaneName), pX0, pY0, pX1, pY1);
    }

    void LayoutRuntimeActor::appendDrawCommands(render::layout::LayoutDrawList* pDrawList) const {
        if (not mIsAlive || mResource == nullptr || pDrawList == nullptr) {
            return;
        }

        const auto root_index = mResource->layout.root_pane;
        if (root_index < 0 || static_cast< std::size_t >(root_index) >= mResource->layout.panes.size()) {
            return;
        }

        const TransformContext root{
            .originX = (mResource->layout.center_origin ? (mResource->layout.size.x * 0.5F) : 0.0F) + mRootTx,
            .originY = (mResource->layout.center_origin ? (mResource->layout.size.y * 0.5F) : 0.0F) + mRootTy,
            .scaleX = 1.0F,
            .scaleY = 1.0F,
            .alpha = 1.0F,
            .visible = true,
        };

        appendPaneRecursive(static_cast< std::size_t >(root_index), root, pDrawList);
    }

    const LayoutArchiveData* LayoutRuntimeActor::resource() const {
        return mResource.get();
    }

}  // namespace smgpc::game::layout
