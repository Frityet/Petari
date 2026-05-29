#include "RendererService.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <aurora/aurora.h>
#include <aurora/event.h>
#include <aurora/gfx.h>
#include <dolphin/gx/GXAurora.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

namespace smgpc::render {
    namespace {

        constexpr auto kLogicalFramebuffer = FramebufferInfo {
            .width = core::kWiiLogicalFramebufferWidth,
            .height = core::kWiiLogicalFramebufferHeight,
        };

        thread_local AuroraRenderer *s_current_renderer = nullptr;

        [[nodiscard]] bool environment_flag_enabled(const char *name, bool fallback) {
            const auto *value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return fallback;
            }

            const auto text = std::string_view(value);
            if (text == "0" || text == "false" || text == "False" || text == "off" || text == "OFF" || text == "no" || text == "NO") {
                return false;
            }
            if (text == "1" || text == "true" || text == "True" || text == "on" || text == "ON" || text == "yes" || text == "YES") {
                return true;
            }
            return fallback;
        }

        [[nodiscard]] std::uint16_t clamp_framebuffer_dimension(std::uint32_t value) {
            return static_cast<std::uint16_t>(std::clamp<std::uint32_t>(value, 1U, std::numeric_limits<std::uint16_t>::max()));
        }

        void append_be32(std::vector<std::uint8_t> &bytes, std::uint32_t value) {
            bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
            bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
        }

        void append_le16(std::vector<std::uint8_t> &bytes, std::uint16_t value) {
            bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
        }

        [[nodiscard]] constexpr std::array<std::uint32_t, 256U> make_crc32_table() {
            auto table = std::array<std::uint32_t, 256U> {};
            for (auto i = 0U; i < table.size(); ++i) {
                auto value = static_cast<std::uint32_t>(i);
                for (auto bit = 0U; bit < 8U; ++bit) {
                    value = (value >> 1U) ^ (0xedb88320U & (0U - (value & 1U)));
                }
                table[i] = value;
            }
            return table;
        }

        [[nodiscard]] std::uint32_t update_crc32(std::uint32_t crc, std::span<const std::uint8_t> bytes) {
            static constexpr auto table = make_crc32_table();
            auto value = crc;
            for (const auto byte : bytes) {
                value = (value >> 8U) ^ table[(value ^ byte) & 0xffU];
            }
            return value;
        }

        [[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> type, std::span<const std::uint8_t> data) {
            auto value = 0xffffffffU;
            value = update_crc32(value, type);
            value = update_crc32(value, data);
            return value ^ 0xffffffffU;
        }

        [[nodiscard]] std::uint32_t adler32(std::span<const std::uint8_t> bytes) {
            constexpr auto kMod = 65521U;
            constexpr auto kMaxChunk = 5552U;
            auto a = 1U;
            auto b = 0U;

            auto offset = std::size_t {};
            while (offset < bytes.size()) {
                const auto chunk_size = std::min<std::size_t>(bytes.size() - offset, kMaxChunk);
                for (auto i = 0U; i < chunk_size; ++i) {
                    a += bytes[offset + i];
                    b += a;
                }
                a %= kMod;
                b %= kMod;
                offset += chunk_size;
            }

            return (b << 16U) | a;
        }

        void append_png_chunk(std::vector<std::uint8_t> &png, std::array<char, 4U> type, std::span<const std::uint8_t> data) {
            append_be32(png, static_cast<std::uint32_t>(data.size()));
            const auto type_offset = png.size();
            for (const auto c : type) {
                png.push_back(static_cast<std::uint8_t>(c));
            }
            png.insert(png.end(), data.begin(), data.end());
            append_be32(png, crc32(std::span<const std::uint8_t>(png.data() + type_offset, type.size()), data));
        }

        [[nodiscard]] std::vector<std::uint8_t> make_zlib_stored_stream(std::span<const std::uint8_t> payload) {
            auto stream = std::vector<std::uint8_t> {};
            stream.reserve(payload.size() + (payload.size() / 65535U + 1U) * 5U + 6U);
            stream.push_back(0x78U);
            stream.push_back(0x01U);

            auto offset = std::size_t {};
            while (offset < payload.size()) {
                const auto remaining = payload.size() - offset;
                const auto block_size = static_cast<std::uint16_t>(std::min<std::size_t>(remaining, 65535U));
                const auto final_block = (offset + block_size) == payload.size();
                stream.push_back(final_block ? 0x01U : 0x00U);
                append_le16(stream, block_size);
                append_le16(stream, static_cast<std::uint16_t>(~block_size));
                stream.insert(stream.end(), payload.begin() + static_cast<std::ptrdiff_t>(offset),
                              payload.begin() + static_cast<std::ptrdiff_t>(offset + block_size));
                offset += block_size;
            }

            append_be32(stream, adler32(payload));
            return stream;
        }

        void write_rgba8_png(const std::filesystem::path &path, std::uint32_t width, std::uint32_t height, std::uint32_t pitch,
                             std::span<const std::uint8_t> rgba) {
            if (width == 0U || height == 0U || pitch < width * 4U || rgba.size() < static_cast<std::size_t>(pitch) * height) {
                throw std::runtime_error("Cannot write Aurora screenshot: invalid readback dimensions");
            }

            auto rows = std::vector<std::uint8_t> {};
            rows.resize((static_cast<std::size_t>(width) * 4U + 1U) * height);
            for (auto y = 0U; y < height; ++y) {
                const auto *source = rgba.data() + static_cast<std::size_t>(y) * pitch;
                auto *destination = rows.data() + static_cast<std::size_t>(y) * (static_cast<std::size_t>(width) * 4U + 1U);
                destination[0] = 0U;
                std::memcpy(destination + 1U, source, static_cast<std::size_t>(width) * 4U);
            }

            auto ihdr = std::vector<std::uint8_t> {};
            ihdr.reserve(13U);
            append_be32(ihdr, width);
            append_be32(ihdr, height);
            ihdr.push_back(8U);
            ihdr.push_back(6U);
            ihdr.push_back(0U);
            ihdr.push_back(0U);
            ihdr.push_back(0U);

            static constexpr auto kPngSignature = std::array<std::uint8_t, 8U> {
                0x89U, 0x50U, 0x4eU, 0x47U, 0x0dU, 0x0aU, 0x1aU, 0x0aU,
            };
            auto png = std::vector<std::uint8_t> {};
            const auto idat = make_zlib_stored_stream(rows);
            png.reserve(kPngSignature.size() + ihdr.size() + idat.size() + 64U);
            png.insert(png.end(), kPngSignature.begin(), kPngSignature.end());
            append_png_chunk(png, {'I', 'H', 'D', 'R'}, ihdr);
            append_png_chunk(png, {'I', 'D', 'A', 'T'}, idat);
            append_png_chunk(png, {'I', 'E', 'N', 'D'}, {});

            const auto parent = path.parent_path();
            if (!parent.empty()) {
                std::filesystem::create_directories(parent);
            }
            auto file = std::ofstream(path, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Cannot write Aurora screenshot: " + path.string());
            }
            file.write(reinterpret_cast<const char *>(png.data()), static_cast<std::streamsize>(png.size()));
            if (!file) {
                throw std::runtime_error("Failed while writing Aurora screenshot: " + path.string());
            }
        }

        [[nodiscard]] FramebufferInfo framebuffer_from_window_size(const AuroraWindowSize &size) {
            return {
                .width = clamp_framebuffer_dimension(size.fb_width == 0U ? size.native_fb_width : size.fb_width),
                .height = clamp_framebuffer_dimension(size.fb_height == 0U ? size.native_fb_height : size.fb_height),
            };
        }

        [[nodiscard]] InputPointerState logical_pointer_from_window_point(float x, float y, const AuroraWindowSize &size) {
            const auto window_width = std::max(1.0F, static_cast<float>(size.width));
            const auto window_height = std::max(1.0F, static_cast<float>(size.height));
            const auto logical_x = x * static_cast<float>(core::kWiiLogicalFramebufferWidth) / window_width;
            const auto logical_y = y * static_cast<float>(core::kWiiLogicalFramebufferHeight) / window_height;
            return {
                .x = logical_x,
                .y = logical_y,
                .valid = logical_x >= 0.0F && logical_x <= static_cast<float>(core::kWiiLogicalFramebufferWidth) &&
                         logical_y >= 0.0F && logical_y <= static_cast<float>(core::kWiiLogicalFramebufferHeight),
            };
        }

        [[nodiscard]] std::uint32_t rgba8_mip_count(std::uint16_t width, std::uint16_t height, float max_lod) {
            auto levels = std::uint32_t {1U};
            auto mip_width = static_cast<std::uint32_t>(std::max<std::uint16_t>(width, 1U));
            auto mip_height = static_cast<std::uint32_t>(std::max<std::uint16_t>(height, 1U));
            while (mip_width > 1U || mip_height > 1U) {
                mip_width = std::max(1U, mip_width >> 1U);
                mip_height = std::max(1U, mip_height >> 1U);
                ++levels;
            }

            const auto requested = std::max(1U, static_cast<std::uint32_t>(std::max(0.0F, max_lod)) + 1U);
            return std::min(levels, requested);
        }

        void append_downsampled_rgba8_mip(std::vector<std::uint8_t> &mips, const std::vector<std::uint8_t> &previous,
                                          std::uint32_t previous_width, std::uint32_t previous_height,
                                          std::uint32_t next_width, std::uint32_t next_height) {
            const auto first = mips.size();
            mips.resize(first + static_cast<std::size_t>(next_width) * next_height * 4U);

            const auto source_pixel = [&](std::uint32_t x, std::uint32_t y, std::size_t component) -> std::uint8_t {
                x = std::min(x, previous_width - 1U);
                y = std::min(y, previous_height - 1U);
                return previous[(static_cast<std::size_t>(y) * previous_width + x) * 4U + component];
            };

            for (auto y = 0U; y < next_height; ++y) {
                for (auto x = 0U; x < next_width; ++x) {
                    auto alpha_sum = 0U;
                    auto premul = std::array<std::uint32_t, 3U> {};
                    for (auto oy = 0U; oy < 2U; ++oy) {
                        for (auto ox = 0U; ox < 2U; ++ox) {
                            const auto sx = std::min(previous_width - 1U, x * 2U + ox);
                            const auto sy = std::min(previous_height - 1U, y * 2U + oy);
                            const auto alpha = static_cast<std::uint32_t>(source_pixel(sx, sy, 3U));
                            alpha_sum += alpha;
                            for (auto component = 0U; component < 3U; ++component) {
                                premul[component] += static_cast<std::uint32_t>(source_pixel(sx, sy, component)) * alpha;
                            }
                        }
                    }

                    const auto dst = first + (static_cast<std::size_t>(y) * next_width + x) * 4U;
                    if (alpha_sum == 0U) {
                        mips[dst + 0U] = 0U;
                        mips[dst + 1U] = 0U;
                        mips[dst + 2U] = 0U;
                    } else {
                        for (auto component = 0U; component < 3U; ++component) {
                            mips[dst + component] =
                                static_cast<std::uint8_t>(std::min<std::uint32_t>((premul[component] + alpha_sum / 2U) / alpha_sum, 255U));
                        }
                    }
                    mips[dst + 3U] = static_cast<std::uint8_t>((alpha_sum + 2U) / 4U);
                }
            }
        }

        [[nodiscard]] std::vector<std::uint8_t> build_rgba8_mips(std::uint16_t width, std::uint16_t height,
                                                                 std::span<const std::uint8_t> rgba, std::uint32_t mip_count) {
            const auto base_width = static_cast<std::uint32_t>(std::max<std::uint16_t>(width, 1U));
            const auto base_height = static_cast<std::uint32_t>(std::max<std::uint16_t>(height, 1U));
            auto previous = std::vector<std::uint8_t>(rgba.begin(), rgba.begin() + static_cast<std::ptrdiff_t>(base_width * base_height * 4U));
            auto mips = previous;
            auto previous_width = base_width;
            auto previous_height = base_height;

            for (auto level = 1U; level < mip_count; ++level) {
                const auto next_width = std::max(1U, previous_width >> 1U);
                const auto next_height = std::max(1U, previous_height >> 1U);
                append_downsampled_rgba8_mip(mips, previous, previous_width, previous_height, next_width, next_height);
                previous.assign(mips.end() - static_cast<std::ptrdiff_t>(next_width * next_height * 4U), mips.end());
                previous_width = next_width;
                previous_height = next_height;
            }

            return mips;
        }

        void aurora_log_callback(AuroraLogLevel level, const char *module, const char *message, unsigned int length) {
            auto *out = level >= LOG_ERROR ? stderr : stdout;
            const auto *level_name = "info";
            switch (level) {
            case LOG_DEBUG:
                level_name = "debug";
                break;
            case LOG_INFO:
                level_name = "info";
                break;
            case LOG_WARNING:
                level_name = "warning";
                break;
            case LOG_ERROR:
                level_name = "error";
                break;
            case LOG_FATAL:
                level_name = "fatal";
                break;
            }
            std::fprintf(out, "[aurora:%s:%s] %.*s\n", level_name, module == nullptr ? "unknown" : module,
                         static_cast<int>(length), message == nullptr ? "" : message);
            std::fflush(out);
            if (level == LOG_FATAL) {
                std::abort();
            }
        }

        [[nodiscard]] std::optional<InputButton> input_button_from_key(SDL_Keycode key) {
            switch (key) {
            case SDLK_A:
            case SDLK_RETURN:
            case SDLK_SPACE:
                return InputButton::CORE_PAD_A;
            case SDLK_B:
            case SDLK_BACKSPACE:
                return InputButton::CORE_PAD_B;
            case SDLK_UP:
                return InputButton::CORE_PAD_UP;
            case SDLK_DOWN:
                return InputButton::CORE_PAD_DOWN;
            case SDLK_LEFT:
                return InputButton::CORE_PAD_LEFT;
            case SDLK_RIGHT:
                return InputButton::CORE_PAD_RIGHT;
            case SDLK_EQUALS:
            case SDLK_KP_PLUS:
                return InputButton::CORE_PAD_PLUS;
            case SDLK_MINUS:
            case SDLK_KP_MINUS:
                return InputButton::CORE_PAD_MINUS;
            case SDLK_HOME:
            case SDLK_H:
                return InputButton::CORE_PAD_HOME;
            case SDLK_C:
                return InputButton::CORE_PAD_C;
            case SDLK_Z:
                return InputButton::CORE_PAD_Z;
            default:
                return std::nullopt;
            }
        }

        [[nodiscard]] std::uint8_t gx_wrap_mode(std::uint8_t value) {
            return static_cast<std::uint8_t>(std::min<std::uint8_t>(value, GX_MAX_TEXWRAPMODE - 1));
        }

        [[nodiscard]] std::uint8_t gx_texture_filter(std::uint8_t value) {
            return static_cast<std::uint8_t>(std::min<std::uint8_t>(value, GX_LIN_MIP_LIN));
        }

        [[nodiscard]] GXCompare gx_compare(DepthCompare compare) {
            switch (compare) {
            case DepthCompare::Never:
                return GX_NEVER;
            case DepthCompare::Less:
                return GX_LESS;
            case DepthCompare::Equal:
                return GX_EQUAL;
            case DepthCompare::LessEqual:
                return GX_LEQUAL;
            case DepthCompare::Greater:
                return GX_GREATER;
            case DepthCompare::NotEqual:
                return GX_NEQUAL;
            case DepthCompare::GreaterEqual:
                return GX_GEQUAL;
            case DepthCompare::Always:
                return GX_ALWAYS;
            }
            return GX_ALWAYS;
        }

        [[nodiscard]] GXCullMode gx_cull_mode(CullMode mode) {
            switch (mode) {
            case CullMode::None:
                return GX_CULL_NONE;
            case CullMode::Front:
                return GX_CULL_FRONT;
            case CullMode::Back:
                return GX_CULL_BACK;
            case CullMode::FrontAndBack:
                return GX_CULL_ALL;
            }
            return GX_CULL_NONE;
        }

        [[nodiscard]] GXBlendMode gx_blend_type(const GxBlendMode2D &blend, BlendMode fallback) {
            if (blend.enabled) {
                return static_cast<GXBlendMode>(blend.type);
            }
            switch (fallback) {
            case BlendMode::Opaque:
                return GX_BM_NONE;
            case BlendMode::Alpha:
            case BlendMode::Additive:
                return GX_BM_BLEND;
            }
            return GX_BM_BLEND;
        }

        [[nodiscard]] GXBlendFactor gx_src_factor(const GxBlendMode2D &blend, BlendMode fallback) {
            if (blend.enabled) {
                return static_cast<GXBlendFactor>(blend.src_factor);
            }
            return fallback == BlendMode::Additive ? GX_BL_SRCALPHA : GX_BL_SRCALPHA;
        }

        [[nodiscard]] GXBlendFactor gx_dst_factor(const GxBlendMode2D &blend, BlendMode fallback) {
            if (blend.enabled) {
                return static_cast<GXBlendFactor>(blend.dst_factor);
            }
            return fallback == BlendMode::Additive ? GX_BL_ONE : GX_BL_INVSRCALPHA;
        }

        [[nodiscard]] GXLogicOp gx_logic_op(const GxBlendMode2D &blend) {
            return blend.enabled ? static_cast<GXLogicOp>(blend.op) : GX_LO_CLEAR;
        }

        [[nodiscard]] GXTexCoordID gx_tex_coord_id(std::size_t index) {
            return static_cast<GXTexCoordID>(GX_TEXCOORD0 + index);
        }

        [[nodiscard]] GXTexMapID gx_tex_map_id(std::size_t index) {
            return static_cast<GXTexMapID>(GX_TEXMAP0 + index);
        }

        [[nodiscard]] GXIndTexStageID gx_ind_tex_stage_id(std::uint8_t index) {
            return static_cast<GXIndTexStageID>(GX_INDTEXSTAGE0 + std::min<std::uint8_t>(index, GX_MAX_INDTEXSTAGE - 1U));
        }

        [[nodiscard]] GXIndTexMtxID gx_ind_tex_regular_matrix_id(std::uint8_t index) {
            return static_cast<GXIndTexMtxID>(GX_ITM_0 + std::min<std::uint8_t>(index, core::kMaxGxIndirectMatrices2D - 1U));
        }

        [[nodiscard]] GXIndTexMtxID gx_ind_tex_stage_matrix_id(const core::GxIndirectTevStage2D &stage) {
            if (stage.matrix_index == 0U || stage.matrix_index > core::kMaxGxIndirectMatrices2D || stage.matrix_id > 2U) {
                return GX_ITM_OFF;
            }
            return static_cast<GXIndTexMtxID>(stage.matrix_index + stage.matrix_id * 4U);
        }

        [[nodiscard]] GXIndTexFormat gx_ind_tex_format(std::uint8_t value) {
            return static_cast<GXIndTexFormat>(std::min<std::uint8_t>(value, GX_MAX_ITFORMAT - 1U));
        }

        [[nodiscard]] GXIndTexBiasSel gx_ind_tex_bias(std::uint8_t value) {
            return static_cast<GXIndTexBiasSel>(std::min<std::uint8_t>(value, GX_MAX_ITBIAS - 1U));
        }

        [[nodiscard]] GXIndTexAlphaSel gx_ind_tex_alpha(std::uint8_t value) {
            return static_cast<GXIndTexAlphaSel>(std::min<std::uint8_t>(value, GX_MAX_ITBALPHA - 1U));
        }

        [[nodiscard]] GXIndTexWrap gx_ind_tex_wrap(std::uint8_t value) {
            return static_cast<GXIndTexWrap>(std::min<std::uint8_t>(value, GX_MAX_ITWRAP - 1U));
        }

        [[nodiscard]] GXIndTexScale gx_ind_tex_scale(std::uint8_t value) {
            return static_cast<GXIndTexScale>(std::min<std::uint8_t>(value, GX_MAX_ITSCALE - 1U));
        }

        [[nodiscard]] GXChannelID gx_channel_id(std::uint8_t index) {
            switch (index) {
            case 0U:
                return GX_COLOR0;
            case 1U:
                return GX_COLOR1;
            case 2U:
                return GX_ALPHA0;
            case 3U:
                return GX_ALPHA1;
            case 4U:
                return GX_COLOR0A0;
            case 5U:
                return GX_COLOR1A1;
            case 6U:
                return GX_COLOR_ZERO;
            case 7U:
                return GX_ALPHA_BUMP;
            case 8U:
                return GX_ALPHA_BUMPN;
            case 0xffU:
                return GX_COLOR_NULL;
            default:
                return GX_COLOR0A0;
            }
        }

        [[nodiscard]] GXTevKColorID gx_k_color_id(std::size_t index) {
            return static_cast<GXTevKColorID>(GX_KCOLOR0 + std::min<std::size_t>(index, GX_MAX_KCOLOR - 1U));
        }

        [[nodiscard]] GXTevKColorSel gx_k_color_sel(std::size_t index) {
            constexpr auto selectors = std::array<GXTevKColorSel, GX_MAX_KCOLOR> {
                GX_TEV_KCSEL_K0,
                GX_TEV_KCSEL_K1,
                GX_TEV_KCSEL_K2,
                GX_TEV_KCSEL_K3,
            };
            return selectors[std::min<std::size_t>(index, selectors.size() - 1U)];
        }

        [[nodiscard]] GXTevKAlphaSel gx_k_alpha_sel(std::size_t index) {
            constexpr auto selectors = std::array<GXTevKAlphaSel, GX_MAX_KCOLOR> {
                GX_TEV_KASEL_K0_A,
                GX_TEV_KASEL_K1_A,
                GX_TEV_KASEL_K2_A,
                GX_TEV_KASEL_K3_A,
            };
            return selectors[std::min<std::size_t>(index, selectors.size() - 1U)];
        }

        [[nodiscard]] GXTevKColorSel gx_k_color_sel_for_stage(const GxTevStage2D &stage, std::size_t fallback_index) {
            if (stage.k_color_sel <= 0x1fU) {
                return static_cast<GXTevKColorSel>(stage.k_color_sel);
            }
            return gx_k_color_sel(fallback_index);
        }

        [[nodiscard]] GXTevKAlphaSel gx_k_alpha_sel_for_stage(const GxTevStage2D &stage, std::size_t fallback_index) {
            if (stage.k_alpha_sel <= 0x1fU) {
                return static_cast<GXTevKAlphaSel>(stage.k_alpha_sel);
            }
            return gx_k_alpha_sel(fallback_index);
        }

        [[nodiscard]] GXTevSwapSel gx_tev_swap_sel(std::uint8_t value) {
            return static_cast<GXTevSwapSel>(std::min<std::uint8_t>(value, GX_TEV_SWAP3));
        }

        void configure_copy_clear() {
            GXSetCopyClear(GXColor {.r = 0U, .g = 0U, .b = 0U, .a = 255U}, GX_MAX_Z24);
            GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
            GXSetDispCopySrc(0U, 0U, kLogicalFramebuffer.width, kLogicalFramebuffer.height);
            GXSetDispCopyDst(kLogicalFramebuffer.width, kLogicalFramebuffer.height);
            GXSetDispCopyYScale(1.0F);
        }

        void configure_2d_projection(RenderSpace2D space) {
            const auto width = static_cast<float>(space == RenderSpace2D::Layout ? core::kWiiLayoutWidth : kLogicalFramebuffer.width);
            const auto height = static_cast<float>(kLogicalFramebuffer.height);
            const auto half_width = width * 0.5F;
            const auto half_height = height * 0.5F;
            const auto y_scale = space == RenderSpace2D::Layout ? 1.0F : -1.0F;
            const float projection[4][4] = {
                {1.0F / half_width, 0.0F, 0.0F, 0.0F},
                {0.0F, y_scale / half_height, 0.0F, 0.0F},
                {0.0F, 0.0F, 1.0F, 0.0F},
                {0.0F, 0.0F, 0.0F, 1.0F},
            };
            const auto viewport_width = static_cast<float>(kLogicalFramebuffer.width);
            const auto viewport_height = static_cast<float>(kLogicalFramebuffer.height);
            const float position[3][4] = {
                {1.0F, 0.0F, 0.0F, 0.0F},
                {0.0F, 1.0F, 0.0F, 0.0F},
                {0.0F, 0.0F, 1.0F, 0.0F},
            };

            GXSetProjection(projection, GX_ORTHOGRAPHIC);
            GXLoadPosMtxImm(position, GX_PNMTX0);
            GXSetCurrentMtx(GX_PNMTX0);
            GXSetViewport(0.0F, 0.0F, viewport_width, viewport_height, 0.0F, 1.0F);
            GXSetScissor(0U, 0U, kLogicalFramebuffer.width, kLogicalFramebuffer.height);
        }

        void configure_3d_projection(const smgpc::camera::CameraPose &camera_pose) {
            const auto width = static_cast<float>(kLogicalFramebuffer.width);
            const auto height = static_cast<float>(kLogicalFramebuffer.height);
            Mtx44 projection {};
            Mtx view {};
            const auto eye = Point3d {
                .x = camera_pose.eye.x,
                .y = camera_pose.eye.y,
                .z = camera_pose.eye.z,
            };
            const auto target = Point3d {
                .x = camera_pose.watch.x,
                .y = camera_pose.watch.y,
                .z = camera_pose.watch.z,
            };
            const auto up = Vec {
                .x = camera_pose.up.x,
                .y = camera_pose.up.y,
                .z = camera_pose.up.z,
            };

            C_MTXPerspective(projection, camera_pose.fovy_degrees, camera_pose.aspect_ratio, camera_pose.near_clip, camera_pose.far_clip);
            C_MTXLookAt(view, &eye, &up, &target);

            GXSetProjection(projection, GX_PERSPECTIVE);
            GXLoadPosMtxImm(view, GX_PNMTX0);
            GXSetCurrentMtx(GX_PNMTX0);
            GXSetViewport(0.0F, 0.0F, width, height, 0.0F, 1.0F);
            GXSetScissor(0U, 0U, kLogicalFramebuffer.width, kLogicalFramebuffer.height);
        }

        void configure_channel_state(std::uint8_t color_src = GX_SRC_VTX, std::uint8_t alpha_src = GX_SRC_VTX,
                                     std::array<std::uint8_t, 4U> material_color = {255U, 255U, 255U, 255U}) {
            GXSetNumChans(1U);
            GXSetChanCtrl(GX_COLOR0, GX_FALSE, GX_SRC_REG, static_cast<GXColorSrc>(color_src), GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
            GXSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_REG, static_cast<GXColorSrc>(alpha_src), GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
            GXSetChanMatColor(GX_COLOR0A0, GXColor {.r = material_color[0], .g = material_color[1], .b = material_color[2], .a = material_color[3]});
            GXSetChanAmbColor(GX_COLOR0A0, GXColor {.r = 255U, .g = 255U, .b = 255U, .a = 255U});
        }

        void configure_blend_depth(const GxBlendMode2D &blend, BlendMode fallback_blend, bool depth_test, bool depth_write,
                                   DepthCompare depth_compare, CullMode cull_mode) {
            GXSetBlendMode(gx_blend_type(blend, fallback_blend), gx_src_factor(blend, fallback_blend), gx_dst_factor(blend, fallback_blend),
                           gx_logic_op(blend));
            GXSetColorUpdate(blend.enabled ? blend.color_update : GX_TRUE);
            GXSetAlphaUpdate(blend.enabled ? blend.alpha_update : GX_TRUE);
            GXSetZMode(depth_test ? GX_TRUE : GX_FALSE, gx_compare(depth_compare), depth_write ? GX_TRUE : GX_FALSE);
            GXSetCullMode(gx_cull_mode(cull_mode));
        }

        void configure_alpha_compare(const GxAlphaCompare2D &compare) {
            if (!compare.enabled) {
                GXSetAlphaCompare(GX_ALWAYS, 0U, GX_AOP_AND, GX_ALWAYS, 0U);
                return;
            }
            GXSetAlphaCompare(static_cast<GXCompare>(compare.comp0), compare.ref0, static_cast<GXAlphaOp>(compare.op),
                              static_cast<GXCompare>(compare.comp1), compare.ref1);
        }

        [[nodiscard]] GXPrimitive gx_primitive(PrimitiveTopology topology) {
            return topology == PrimitiveTopology::TriangleStrip ? GX_TRIANGLESTRIP : GX_TRIANGLES;
        }

        void emit_textured_vertex(const TexturedVertex2D &vertex) {
            GXPosition3f32(vertex.x, vertex.y, vertex.z);
            GXColor4u8(vertex.color[0], vertex.color[1], vertex.color[2], vertex.color[3]);
            GXTexCoord2f32(vertex.u, vertex.v);
        }

        void emit_material_vertex(const GxMaterialVertex2D &vertex, std::size_t tex_coord_count) {
            GXPosition3f32(vertex.x, vertex.y, vertex.z);
            GXColor4u8(vertex.color[0], vertex.color[1], vertex.color[2], vertex.color[3]);
            for (auto index = std::size_t {}; index < tex_coord_count; ++index) {
                const auto &coord = vertex.tex_coords[index];
                if (std::abs(coord[2]) > 0.000001F) {
                    GXTexCoord2f32(coord[0] / coord[2], coord[1] / coord[2]);
                } else {
                    GXTexCoord2f32(std::clamp(coord[0] * 0.5F, -1.0F, 1.0F), std::clamp(coord[1] * 0.5F, -1.0F, 1.0F));
                }
            }
        }

        template <typename Vertex>
        void dump_batch_bounds(const char *kind, std::uint64_t frame_index, std::uint64_t submit_index, std::span<const Vertex> vertices,
                               std::span<const std::uint16_t> indices) {
            if (!environment_flag_enabled("SMGPC_AURORA_RENDER_DUMP", false) || submit_index > 48U || vertices.empty()) {
                return;
            }

            auto min_x = std::numeric_limits<float>::max();
            auto min_y = std::numeric_limits<float>::max();
            auto min_z = std::numeric_limits<float>::max();
            auto max_x = std::numeric_limits<float>::lowest();
            auto max_y = std::numeric_limits<float>::lowest();
            auto max_z = std::numeric_limits<float>::lowest();
            auto min_alpha = std::uint8_t {255U};
            auto max_alpha = std::uint8_t {0U};
            for (const auto &vertex : vertices) {
                min_x = std::min(min_x, vertex.x);
                min_y = std::min(min_y, vertex.y);
                min_z = std::min(min_z, vertex.z);
                max_x = std::max(max_x, vertex.x);
                max_y = std::max(max_y, vertex.y);
                max_z = std::max(max_z, vertex.z);
                min_alpha = std::min(min_alpha, vertex.color[3]);
                max_alpha = std::max(max_alpha, vertex.color[3]);
            }

            std::fprintf(stderr,
                         "[smgpc:render-dump] frame=%llu submit=%llu kind=%s vertices=%zu indices=%zu "
                         "x=[%.2f,%.2f] y=[%.2f,%.2f] z=[%.2f,%.2f] alpha=[%u,%u]\n",
                         static_cast<unsigned long long>(frame_index), static_cast<unsigned long long>(submit_index), kind,
                         vertices.size(), indices.size(), min_x, max_x, min_y, max_y, min_z, max_z, min_alpha, max_alpha);
        }

        [[nodiscard]] TextureHandle invalid_texture() {
            return {};
        }

    }  // namespace

    struct AuroraWindow::Impl {
        explicit Impl(const WindowConfiguration &configuration) {
            auto config = AuroraConfig {};
            config.appName = configuration.title.c_str();
            config.windowWidth = static_cast<std::uint32_t>(std::max(configuration.width, 1));
            config.windowHeight = static_cast<std::uint32_t>(std::max(configuration.height, 1));
            config.vsync = environment_flag_enabled("SMGPC_ENABLE_VSYNC", true);
            config.pauseOnFocusLost = false;
            config.allowJoystickBackgroundEvents = true;
            config.allowTextureReplacements = true;
            config.allowTextureDumps = environment_flag_enabled("SMGPC_AURORA_TEXTURE_DUMPS", false);
            config.mem1Size = 24U * 1024U * 1024U;
            config.mem2Size = 64U * 1024U * 1024U;
            config.logCallback = &aurora_log_callback;
            config.logLevel = LOG_INFO;
            config.desiredBackend = BACKEND_AUTO;

            info = aurora_initialize(0, nullptr, &config);
            size = info.windowSize;
            AuroraSetViewportPolicy(AURORA_VIEWPORT_STRETCH);
            GXInit(nullptr, 0U);
            configure_copy_clear();
        }

        ~Impl() {
            shutdown();
        }

        void shutdown() {
            if (!initialized) {
                return;
            }
            aurora_shutdown();
            initialized = false;
        }

        void process_sdl_event(const SDL_Event &event) {
            switch (event.type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                if (const auto button = input_button_from_key(event.key.key); button.has_value()) {
                    pressed[static_cast<std::size_t>(*button)] = event.type == SDL_EVENT_KEY_DOWN;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    pressed[static_cast<std::size_t>(InputButton::CORE_PAD_A)] = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    pressed[static_cast<std::size_t>(InputButton::CORE_PAD_B)] = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                }
                pointer = logical_pointer_from_window_point(event.button.x, event.button.y, size);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                pointer = logical_pointer_from_window_point(event.motion.x, event.motion.y, size);
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                focused = true;
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                focused = false;
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                minimized = true;
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                minimized = false;
                break;
            default:
                break;
            }
        }

        bool poll_events() {
            for (const AuroraEvent *event = aurora_update(); event != nullptr && event->type != AURORA_NONE; ++event) {
                switch (event->type) {
                case AURORA_EXIT:
                    should_close = true;
                    break;
                case AURORA_WINDOW_RESIZED:
                case AURORA_DISPLAY_SCALE_CHANGED:
                    size = event->windowSize;
                    break;
                case AURORA_PAUSED:
                    minimized = true;
                    break;
                case AURORA_UNPAUSED:
                    minimized = false;
                    break;
                case AURORA_SDL_EVENT:
                    process_sdl_event(event->sdl);
                    break;
                default:
                    break;
                }
            }
            return !should_close;
        }

        AuroraInfo info {};
        AuroraWindowSize size {};
        std::array<bool, static_cast<std::size_t>(InputButton::COUNT)> pressed {};
        InputPointerState pointer {};
        bool should_close = false;
        bool focused = true;
        bool minimized = false;
        bool initialized = true;
    };

    AuroraWindow::AuroraWindow(const WindowConfiguration &configuration) : _impl(std::make_unique<Impl>(configuration)) {
    }

    AuroraWindow::~AuroraWindow() = default;

    bool AuroraWindow::poll_events() {
        return _impl->poll_events();
    }

    bool AuroraWindow::should_close() const {
        return _impl->should_close;
    }

    bool AuroraWindow::is_focused() const {
        return _impl->focused;
    }

    bool AuroraWindow::is_minimized() const {
        return _impl->minimized;
    }

    FramebufferInfo AuroraWindow::framebuffer_size() const {
        return framebuffer_from_window_size(_impl->size);
    }

    NativeWindowHandle AuroraWindow::native_handle() const {
        return {};
    }

    bool AuroraWindow::is_input_pressed(InputButton button) const {
        if (button == InputButton::COUNT) {
            return false;
        }
        return _impl->pressed[static_cast<std::size_t>(button)];
    }

    InputPointerState AuroraWindow::input_pointer_state() const {
        return _impl->pointer;
    }

    void AuroraWindow::close() {
        _impl->should_close = true;
    }

    void AuroraWindow::shutdown() {
        _impl->shutdown();
    }

    struct AuroraRenderer::Impl {
        explicit Impl(AuroraWindow &window_) : window(window_) {
        }

        [[nodiscard]] core::AuroraTexture *texture(TextureHandle handle) {
            if (!handle.is_valid()) {
                return nullptr;
            }
            return handle.texture.get();
        }

        [[nodiscard]] const core::AuroraTexture *texture(TextureHandle handle) const {
            if (!handle.is_valid()) {
                return nullptr;
            }
            return handle.texture.get();
        }

        void load_texture(TextureHandle handle, GXTexMapID map_id, std::uint8_t wrap_u, std::uint8_t wrap_v, std::uint8_t min_filter,
                          std::uint8_t mag_filter) {
            auto *record = texture(handle);
            if (record == nullptr) {
                return;
            }
            GXInitTexObjWrapMode(&record->object, static_cast<GXTexWrapMode>(gx_wrap_mode(wrap_u)),
                                 static_cast<GXTexWrapMode>(gx_wrap_mode(wrap_v)));
            GXInitTexObjFilter(&record->object, static_cast<GXTexFilter>(gx_texture_filter(min_filter)),
                               static_cast<GXTexFilter>(gx_texture_filter(mag_filter)));
            GXLoadTexObj(&record->object, map_id);
        }

        void configure_textured_state(TextureHandle texture_handle, const TexturedTriangleBatch2D &batch,
                                      const smgpc::camera::CameraPose *camera_pose = nullptr) {
            if (camera_pose != nullptr) {
                configure_3d_projection(*camera_pose);
            } else {
                configure_2d_projection(batch.space);
            }
            configure_channel_state();
            configure_blend_depth(batch.gx_blend, batch.blend_mode, batch.depth_test, batch.depth_write, batch.depth_compare, batch.cull_mode);
            configure_alpha_compare({});

            GXClearVtxDesc();
            GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
            GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
            GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0U);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0U);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0U);
            GXSetNumTexGens(1U);
            GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
            load_texture(texture_handle, GX_TEXMAP0, batch.wrap_u, batch.wrap_v, batch.min_filter, batch.mag_filter);
            GXSetNumTevStages(1U);
            GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
            GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        }

        void configure_material_state(const GxMaterialTriangleBatch2D &batch,
                                      const smgpc::camera::CameraPose *camera_pose = nullptr) {
            if (camera_pose != nullptr) {
                configure_3d_projection(*camera_pose);
            } else {
                configure_2d_projection(batch.space);
            }
            configure_channel_state(batch.channel_color_src, batch.channel_alpha_src, batch.channel_material_color);
            configure_blend_depth(batch.blend, BlendMode::Alpha, batch.depth_test, batch.depth_write, batch.depth_compare, batch.cull_mode);
            configure_alpha_compare(batch.alpha_compare);

            const auto texture_count = std::min<std::size_t>(batch.texture_stages.size(), core::kMaxGxMaterialTextureStages2D);
            const auto tev_count = std::clamp<std::size_t>(batch.tev_stages.empty() ? 1U : batch.tev_stages.size(), 1U,
                                                           core::kMaxGxMaterialTevStages2D);

            GXClearVtxDesc();
            GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
            GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0U);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0U);
            for (auto index = std::size_t {}; index < texture_count; ++index) {
                const auto attr = static_cast<GXAttr>(GX_VA_TEX0 + index);
                const auto &stage = batch.texture_stages[index];
                GXSetVtxDesc(attr, GX_DIRECT);
                GXSetVtxAttrFmt(GX_VTXFMT0, attr, GX_TEX_ST, GX_F32, 0U);
                const auto texgen_source = stage.has_texgen_matrix ?
                                               stage.texgen_source :
                                               static_cast<GXTexGenSrc>(GX_TG_TEX0 + index);
                const auto texgen_matrix = stage.has_texgen_matrix ? stage.texgen_matrix : GX_IDENTITY;
                if (stage.has_texgen_matrix) {
                    GXLoadTexMtxImm(stage.texgen_matrix_values.data(), texgen_matrix, stage.texgen_matrix_type);
                }
                GXSetTexCoordGen(gx_tex_coord_id(index), stage.texgen_type, texgen_source, texgen_matrix);
                load_texture(stage.texture, gx_tex_map_id(index), stage.wrap_u, stage.wrap_v, stage.min_filter, stage.mag_filter);
            }
            GXSetNumTexGens(static_cast<u8>(texture_count));

            for (auto index = std::size_t {}; index < batch.initial_tev_registers.size(); ++index) {
                const auto &color = batch.initial_tev_registers[index];
                GXSetTevColorS10(static_cast<GXTevRegID>(GX_TEVPREV + index),
                                 GXColorS10 {.r = color[0], .g = color[1], .b = color[2], .a = color[3]});
            }
            if (batch.has_initial_tev_k_colors) {
                for (auto index = std::size_t {}; index < batch.initial_tev_k_colors.size(); ++index) {
                    const auto &color = batch.initial_tev_k_colors[index];
                    GXSetTevKColor(gx_k_color_id(index), GXColor {.r = color[0], .g = color[1], .b = color[2], .a = color[3]});
                }
            }

            GXSetNumTevStages(static_cast<u8>(tev_count));
            GXSetNumIndStages(std::min<u8>(batch.indirect_stage_count, GX_MAX_INDTEXSTAGE));
            for (auto index = std::size_t {}; index < tev_count; ++index) {
                GXSetTevDirect(static_cast<GXTevStageID>(GX_TEVSTAGE0 + index));
            }
            for (auto index = std::size_t {}; index < tev_count; ++index) {
                const auto stage_id = static_cast<GXTevStageID>(GX_TEVSTAGE0 + index);
                if (index >= batch.tev_stages.size()) {
                    GXSetTevOrder(stage_id, texture_count == 0U ? GX_TEXCOORD_NULL : GX_TEXCOORD0,
                                  texture_count == 0U ? GX_TEXMAP_NULL : GX_TEXMAP0, GX_COLOR0A0);
                    GXSetTevOp(stage_id, texture_count == 0U ? GX_PASSCLR : GX_MODULATE);
                    continue;
                }

                const auto &stage = batch.tev_stages[index];
                auto texture_coord_stage = stage.texture_coord_stage;
                auto texture_map_stage = stage.texture_map_stage;
                if ((texture_coord_stage >= texture_count || texture_map_stage >= texture_count) && stage.texture_stage < texture_count) {
                    texture_coord_stage = stage.texture_stage;
                    texture_map_stage = stage.texture_stage;
                }
                const auto has_texture = texture_coord_stage < texture_count && texture_map_stage < texture_count;
                if (!batch.has_initial_tev_k_colors) {
                    const auto &konst = stage.konst_color;
                    GXSetTevKColor(gx_k_color_id(index), GXColor {.r = konst[0], .g = konst[1], .b = konst[2], .a = konst[3]});
                }
                GXSetTevKColorSel(stage_id, gx_k_color_sel_for_stage(stage, index));
                GXSetTevKAlphaSel(stage_id, gx_k_alpha_sel_for_stage(stage, index));
                GXSetTevSwapMode(stage_id, gx_tev_swap_sel(stage.ras_swap), gx_tev_swap_sel(stage.tex_swap));
                GXSetTevOrder(stage_id, has_texture ? gx_tex_coord_id(texture_coord_stage) : GX_TEXCOORD_NULL,
                              has_texture ? gx_tex_map_id(texture_map_stage) : GX_TEXMAP_NULL, gx_channel_id(stage.color_channel));
                GXSetTevColorIn(stage_id, static_cast<GXTevColorArg>(stage.color_in[0]), static_cast<GXTevColorArg>(stage.color_in[1]),
                                static_cast<GXTevColorArg>(stage.color_in[2]), static_cast<GXTevColorArg>(stage.color_in[3]));
                GXSetTevAlphaIn(stage_id, static_cast<GXTevAlphaArg>(stage.alpha_in[0]), static_cast<GXTevAlphaArg>(stage.alpha_in[1]),
                                static_cast<GXTevAlphaArg>(stage.alpha_in[2]), static_cast<GXTevAlphaArg>(stage.alpha_in[3]));
                GXSetTevColorOp(stage_id, static_cast<GXTevOp>(stage.color_op), static_cast<GXTevBias>(stage.color_bias),
                                static_cast<GXTevScale>(stage.color_scale), stage.color_clamp ? GX_TRUE : GX_FALSE,
                                static_cast<GXTevRegID>(stage.color_out));
                GXSetTevAlphaOp(stage_id, static_cast<GXTevOp>(stage.alpha_op), static_cast<GXTevBias>(stage.alpha_bias),
                                static_cast<GXTevScale>(stage.alpha_scale), stage.alpha_clamp ? GX_TRUE : GX_FALSE,
                                static_cast<GXTevRegID>(stage.alpha_out));
            }

            for (const auto &order : batch.indirect_texture_orders) {
                if (order.stage >= batch.indirect_stage_count || order.tex_coord >= texture_count || order.tex_map >= texture_count) {
                    continue;
                }
                GXSetIndTexOrder(gx_ind_tex_stage_id(order.stage), gx_tex_coord_id(order.tex_coord), gx_tex_map_id(order.tex_map));
            }
            for (const auto &scale : batch.indirect_texture_scales) {
                if (scale.stage >= batch.indirect_stage_count) {
                    continue;
                }
                GXSetIndTexCoordScale(gx_ind_tex_stage_id(scale.stage), gx_ind_tex_scale(scale.scale_s), gx_ind_tex_scale(scale.scale_t));
            }
            for (const auto &matrix : batch.indirect_texture_matrices) {
                if (matrix.matrix >= core::kMaxGxIndirectMatrices2D) {
                    continue;
                }
                float gx_matrix[2][3] = {
                    {
                        static_cast<float>(matrix.ma) / 1024.0F,
                        static_cast<float>(matrix.mc) / 1024.0F,
                        static_cast<float>(matrix.me) / 1024.0F,
                    },
                    {
                        static_cast<float>(matrix.mb) / 1024.0F,
                        static_cast<float>(matrix.md) / 1024.0F,
                        static_cast<float>(matrix.mf) / 1024.0F,
                    },
                };
                GXSetIndTexMtx(gx_ind_tex_regular_matrix_id(matrix.matrix), gx_matrix, static_cast<s8>(matrix.scale) - 17);
            }
            for (const auto &stage : batch.indirect_tev_stages) {
                if (!stage.active || stage.tev_stage >= tev_count || stage.ind_stage >= batch.indirect_stage_count) {
                    continue;
                }
                GXSetTevIndirect(static_cast<GXTevStageID>(GX_TEVSTAGE0 + stage.tev_stage), gx_ind_tex_stage_id(stage.ind_stage),
                                 gx_ind_tex_format(stage.format), gx_ind_tex_bias(stage.bias), gx_ind_tex_stage_matrix_id(stage),
                                 gx_ind_tex_wrap(stage.wrap_s), gx_ind_tex_wrap(stage.wrap_t),
                                 stage.add_previous ? GX_TRUE : GX_FALSE, stage.use_original_lod ? GX_TRUE : GX_FALSE,
                                 gx_ind_tex_alpha(stage.bump_alpha));
            }
        }

        AuroraWindow &window;
        std::uint64_t frame_index = 0U;
        double frame_time = 0.0;
        std::uint64_t frame_created_textures = 0U;
        std::uint64_t frame_textured_submits = 0U;
        std::uint64_t frame_material_submits = 0U;
        std::uint64_t frame_submitted_vertices = 0U;
        bool frame_open = false;
    };

    AuroraRenderer::AuroraRenderer(AuroraWindow &window) : _impl(std::make_unique<Impl>(window)) {
    }

    AuroraRenderer::~AuroraRenderer() = default;

    FrameContext AuroraRenderer::begin_frame() {
        ++_impl->frame_index;
        _impl->frame_time += 1.0 / 60.0;
        _impl->frame_open = aurora_begin_frame();
        if (_impl->frame_open) {
            _impl->frame_created_textures = 0U;
            _impl->frame_textured_submits = 0U;
            _impl->frame_material_submits = 0U;
            _impl->frame_submitted_vertices = 0U;
            configure_copy_clear();
        }
        return {
            .frame_index = _impl->frame_index,
            .frame_time_seconds = _impl->frame_time,
            .frame_delta_seconds = 1.0 / 60.0,
            .framebuffer = framebuffer_size(),
            .has_focus = _impl->window.is_focused(),
            .is_minimized = _impl->window.is_minimized(),
        };
    }

    void AuroraRenderer::end_frame() {
        if (_impl->frame_open) {
            GXFlush();
            GXCopyDisp(nullptr, GX_TRUE);
            aurora_end_frame();
            if (environment_flag_enabled("SMGPC_AURORA_RENDER_STATS", false)) {
                const auto *stats = aurora_get_stats();
                std::fprintf(stderr,
                             "[smgpc:render] frame=%llu textures=%llu textured=%llu material=%llu vertices=%llu "
                             "aurora_draws=%u merged=%u verts=%u indices=%u uniforms=%u\n",
                             static_cast<unsigned long long>(_impl->frame_index),
                             static_cast<unsigned long long>(_impl->frame_created_textures),
                             static_cast<unsigned long long>(_impl->frame_textured_submits),
                             static_cast<unsigned long long>(_impl->frame_material_submits),
                             static_cast<unsigned long long>(_impl->frame_submitted_vertices),
                             stats != nullptr ? stats->drawCallCount : 0U, stats != nullptr ? stats->mergedDrawCallCount : 0U,
                             stats != nullptr ? stats->lastVertSize : 0U, stats != nullptr ? stats->lastIndexSize : 0U,
                             stats != nullptr ? stats->lastUniformSize : 0U);
            }
            _impl->frame_open = false;
        }
    }

    void AuroraRenderer::shutdown() {
        _impl->window.shutdown();
    }

    void AuroraRenderer::request_screenshot_png(const std::filesystem::path &path) {
        auto width = 0U;
        auto height = 0U;
        if (path.empty() || AuroraGetDisplayCopySize(&width, &height) == GX_FALSE || width == 0U || height == 0U) {
            return;
        }

        const auto tight_pitch = width * 4U;
        auto row_stride = 0U;
        auto pixels = std::vector<std::uint8_t>(static_cast<std::size_t>(tight_pitch) * height);
        if (AuroraReadDisplayCopyRGBA8(pixels.data(), static_cast<u32>(pixels.size()), &width, &height, &row_stride) == GX_FALSE ||
            row_stride == 0U) {
            return;
        }

        write_rgba8_png(path, width, height, row_stride, pixels);
    }

    TextureHandle AuroraRenderer::create_rgba8_texture(std::uint16_t width, std::uint16_t height, std::span<const std::uint8_t> rgba) {
        if (width == 0U || height == 0U || rgba.size() < static_cast<std::size_t>(width) * height * 4U) {
            return invalid_texture();
        }

        auto handle = TextureHandle {
            .texture = std::make_shared<core::AuroraTexture>(),
        };
        handle.texture->width = width;
        handle.texture->height = height;
        handle.texture->rgba.assign(rgba.begin(), rgba.begin() + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(width) * height * 4U));
        handle.texture->format = GX_TF_RGBA8_PC;
        handle.texture->mipmap = false;
        handle.texture->min_lod = 0.0F;
        handle.texture->max_lod = 0.0F;
        handle.texture->lod_bias = 0.0F;
        handle.texture->bias_clamp = false;
        handle.texture->edge_lod = false;
        handle.texture->max_anisotropy = GX_ANISO_1;
        handle.texture->alive = true;
        GXInitTexObj(&handle.texture->object, handle.texture->rgba.data(), width, height, GX_TF_RGBA8_PC, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GXInitTexObjLOD(&handle.texture->object, GX_LINEAR, GX_LINEAR, 0.0F, 0.0F, 0.0F, GX_FALSE, GX_FALSE, GX_ANISO_1);
        ++_impl->frame_created_textures;
        return handle;
    }

    TextureHandle AuroraRenderer::create_rgba8_mip_texture(std::uint16_t width, std::uint16_t height,
                                                           std::span<const std::uint8_t> rgba, float min_lod,
                                                           float max_lod, float lod_bias, bool bias_clamp,
                                                           bool edge_lod, GXAnisotropy max_anisotropy,
                                                           std::uint8_t min_filter, std::uint8_t mag_filter) {
        if (width == 0U || height == 0U || rgba.size() < static_cast<std::size_t>(width) * height * 4U) {
            return invalid_texture();
        }

        const auto mip_count = rgba8_mip_count(width, height, max_lod);
        if (mip_count <= 1U) {
            return create_rgba8_texture(width, height, rgba);
        }

        auto handle = TextureHandle {
            .texture = std::make_shared<core::AuroraTexture>(),
        };
        handle.texture->width = width;
        handle.texture->height = height;
        handle.texture->rgba = build_rgba8_mips(width, height, rgba, mip_count);
        handle.texture->format = GX_TF_RGBA8_PC;
        handle.texture->mipmap = true;
        handle.texture->min_lod = min_lod;
        handle.texture->max_lod = static_cast<float>(mip_count - 1U);
        handle.texture->lod_bias = lod_bias;
        handle.texture->bias_clamp = bias_clamp;
        handle.texture->edge_lod = edge_lod;
        handle.texture->max_anisotropy = max_anisotropy;
        handle.texture->alive = true;
        GXInitTexObj(&handle.texture->object, handle.texture->rgba.data(), width, height, GX_TF_RGBA8_PC, GX_CLAMP, GX_CLAMP, GX_TRUE);
        GXInitTexObjLOD(&handle.texture->object, static_cast<GXTexFilter>(gx_texture_filter(min_filter)),
                        static_cast<GXTexFilter>(gx_texture_filter(mag_filter)), min_lod, handle.texture->max_lod, lod_bias,
                        bias_clamp ? GX_TRUE : GX_FALSE, edge_lod ? GX_TRUE : GX_FALSE, max_anisotropy);
        ++_impl->frame_created_textures;
        return handle;
    }

    TextureHandle AuroraRenderer::create_gx_texture(std::uint16_t width, std::uint16_t height, GXTexFmt format,
                                                    std::span<const std::uint8_t> image_data, bool mipmap,
                                                    float min_lod, float max_lod, float lod_bias, bool bias_clamp,
                                                    bool edge_lod, GXAnisotropy max_anisotropy,
                                                    std::uint8_t min_filter, std::uint8_t mag_filter) {
        if (width == 0U || height == 0U || image_data.empty()) {
            return invalid_texture();
        }

        auto handle = TextureHandle {
            .texture = std::make_shared<core::AuroraTexture>(),
        };
        handle.texture->width = width;
        handle.texture->height = height;
        handle.texture->rgba.assign(image_data.begin(), image_data.end());
        handle.texture->format = format;
        handle.texture->mipmap = mipmap;
        handle.texture->min_lod = min_lod;
        handle.texture->max_lod = mipmap ? std::max(min_lod, max_lod) : 0.0F;
        handle.texture->lod_bias = lod_bias;
        handle.texture->bias_clamp = bias_clamp;
        handle.texture->edge_lod = edge_lod;
        handle.texture->max_anisotropy = max_anisotropy;
        handle.texture->alive = true;
        GXInitTexObj(&handle.texture->object, handle.texture->rgba.data(), width, height, format, GX_CLAMP, GX_CLAMP,
                     mipmap ? GX_TRUE : GX_FALSE);
        GXInitTexObjLOD(&handle.texture->object, static_cast<GXTexFilter>(gx_texture_filter(min_filter)),
                        static_cast<GXTexFilter>(gx_texture_filter(mag_filter)), min_lod, handle.texture->max_lod, lod_bias,
                        bias_clamp ? GX_TRUE : GX_FALSE, edge_lod ? GX_TRUE : GX_FALSE, max_anisotropy);
        ++_impl->frame_created_textures;
        return handle;
    }

    void AuroraRenderer::submit_textured_quad(TextureHandle texture, const TexturedQuad2D &quad) {
        const auto indices = std::array<std::uint16_t, 6U> {0U, 1U, 2U, 0U, 2U, 3U};
        submit_textured_triangles(texture, TexturedTriangleBatch2D {
                                               .vertices = std::span<const TexturedVertex2D>(quad.vertices),
                                               .indices = std::span<const std::uint16_t>(indices),
                                               .primitive_topology = PrimitiveTopology::Triangles,
                                               .space = quad.space,
                                               .wrap_u = quad.wrap_u,
                                               .wrap_v = quad.wrap_v,
                                               .min_filter = quad.min_filter,
                                               .mag_filter = quad.mag_filter,
                                               .blend = quad.blend,
                                               .blend_mode = quad.blend_mode,
                                               .gx_blend = quad.gx_blend,
                                               .depth_test = quad.depth_test,
                                               .depth_write = quad.depth_write,
                                               .depth_compare = quad.depth_compare,
                                               .cull_mode = quad.cull_mode,
                                               .fog = quad.fog,
                                           });
    }

    void AuroraRenderer::submit_textured_triangles(TextureHandle texture, const TexturedTriangleBatch2D &batch) {
        if (!_impl->frame_open || _impl->texture(texture) == nullptr || batch.vertices.empty() || batch.indices.empty()) {
            return;
        }

        ++_impl->frame_textured_submits;
        _impl->frame_submitted_vertices += std::min<std::size_t>(batch.indices.size(), UINT16_MAX);
        dump_batch_bounds("textured", _impl->frame_index, _impl->frame_textured_submits + _impl->frame_material_submits,
                          batch.vertices, batch.indices);
        _impl->configure_textured_state(texture, batch);
        GXBegin(gx_primitive(batch.primitive_topology), GX_VTXFMT0, static_cast<u16>(std::min<std::size_t>(batch.indices.size(), UINT16_MAX)));
        for (const auto index : batch.indices) {
            if (index < batch.vertices.size()) {
                emit_textured_vertex(batch.vertices[index]);
            }
        }
        GXEnd();
    }

    void AuroraRenderer::submit_textured_triangles_3d(TextureHandle texture, const TexturedTriangleBatch2D &batch,
                                                      const smgpc::camera::CameraPose &camera_pose) {
        if (!_impl->frame_open || _impl->texture(texture) == nullptr || batch.vertices.empty() || batch.indices.empty()) {
            return;
        }

        ++_impl->frame_textured_submits;
        _impl->frame_submitted_vertices += std::min<std::size_t>(batch.indices.size(), UINT16_MAX);
        dump_batch_bounds("textured3d", _impl->frame_index, _impl->frame_textured_submits + _impl->frame_material_submits,
                          batch.vertices, batch.indices);
        _impl->configure_textured_state(texture, batch, &camera_pose);
        GXBegin(gx_primitive(batch.primitive_topology), GX_VTXFMT0, static_cast<u16>(std::min<std::size_t>(batch.indices.size(), UINT16_MAX)));
        for (const auto index : batch.indices) {
            if (index < batch.vertices.size()) {
                emit_textured_vertex(batch.vertices[index]);
            }
        }
        GXEnd();
    }

    void AuroraRenderer::submit_gx_material_triangles(const GxMaterialTriangleBatch2D &batch) {
        if (!_impl->frame_open || batch.vertices.empty() || batch.indices.empty()) {
            return;
        }

        ++_impl->frame_material_submits;
        _impl->frame_submitted_vertices += std::min<std::size_t>(batch.indices.size(), UINT16_MAX);
        dump_batch_bounds("material", _impl->frame_index, _impl->frame_textured_submits + _impl->frame_material_submits,
                          batch.vertices, batch.indices);
        _impl->configure_material_state(batch);
        const auto texture_count = std::min<std::size_t>(batch.texture_stages.size(), core::kMaxGxMaterialTextureStages2D);
        GXBegin(gx_primitive(batch.primitive_topology), GX_VTXFMT0, static_cast<u16>(std::min<std::size_t>(batch.indices.size(), UINT16_MAX)));
        for (const auto index : batch.indices) {
            if (index < batch.vertices.size()) {
                emit_material_vertex(batch.vertices[index], texture_count);
            }
        }
        GXEnd();
    }

    void AuroraRenderer::submit_gx_material_triangles_3d(const GxMaterialTriangleBatch2D &batch,
                                                         const smgpc::camera::CameraPose &camera_pose) {
        if (!_impl->frame_open || batch.vertices.empty() || batch.indices.empty()) {
            return;
        }

        ++_impl->frame_material_submits;
        _impl->frame_submitted_vertices += std::min<std::size_t>(batch.indices.size(), UINT16_MAX);
        dump_batch_bounds("material3d", _impl->frame_index, _impl->frame_textured_submits + _impl->frame_material_submits,
                          batch.vertices, batch.indices);
        _impl->configure_material_state(batch, &camera_pose);
        const auto texture_count = std::min<std::size_t>(batch.texture_stages.size(), core::kMaxGxMaterialTextureStages2D);
        GXBegin(gx_primitive(batch.primitive_topology), GX_VTXFMT0, static_cast<u16>(std::min<std::size_t>(batch.indices.size(), UINT16_MAX)));
        for (const auto index : batch.indices) {
            if (index < batch.vertices.size()) {
                emit_material_vertex(batch.vertices[index], texture_count);
            }
        }
        GXEnd();
    }

    FramebufferInfo AuroraRenderer::framebuffer_size() const {
        return _impl->window.framebuffer_size();
    }

    FramebufferInfo AuroraRenderer::logical_framebuffer_size() const {
        return kLogicalFramebuffer;
    }

    AuroraRenderer &current_aurora_renderer() {
        if (s_current_renderer == nullptr) {
            throw std::runtime_error("Aurora renderer context is not active");
        }
        return *s_current_renderer;
    }

    ScopedAuroraRendererContext::ScopedAuroraRendererContext(AuroraRenderer &renderer) : _previous(s_current_renderer) {
        s_current_renderer = &renderer;
    }

    ScopedAuroraRendererContext::~ScopedAuroraRendererContext() {
        s_current_renderer = _previous;
    }

}  // namespace smgpc::render
