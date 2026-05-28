#include "revolution.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>

#include "runtime/RuntimeContext.hpp"

namespace {

    struct CopySourceState {
        u16 left = 0U;
        u16 top = 0U;
        u16 width = smgpc::render::core::kWiiLogicalFramebufferWidth;
        u16 height = smgpc::render::core::kWiiLogicalFramebufferHeight;
    };

    struct DisplayCopyState {
        CopySourceState source{};
        u16 width = smgpc::render::core::kWiiLogicalFramebufferWidth;
        u16 height = smgpc::render::core::kWiiLogicalFramebufferHeight;
        u32 dest_stride = smgpc::render::core::kWiiLogicalFramebufferWidth * 2U;
        GXCopyMode frame_to_field = GX_COPY_PROGRESSIVE;
    };

    struct TextureCopyState {
        CopySourceState source{};
        u16 width = smgpc::render::core::kWiiLogicalFramebufferWidth;
        u16 height = smgpc::render::core::kWiiLogicalFramebufferHeight;
        GXTexFmt format = GX_TF_RGB565;
        GXBool mipmap = GX_FALSE;
        u32 dest_stride = smgpc::render::core::kWiiLogicalFramebufferWidth * 2U;
    };

    struct GxCopyState {
        DisplayCopyState display{};
        TextureCopyState texture{};
        bool clamp_top = true;
        bool clamp_bottom = true;
        GXGamma gamma = GX_GM_1_0;
        GXColor clear_color{};
        u32 clear_depth = 0U;
        bool copy_filter_aa = false;
        bool copy_filter_vertical = false;
        std::array<std::array<u8, 2U>, 12U> copy_filter_sample_pattern{};
        std::array<u8, 7U> copy_filter_vfilter{};
        f32 y_scale = 1.0F;
        u32 display_copy_y_scale = 256U;
    };

    GxCopyState s_copy_state{};

    [[nodiscard]] smgpc::render::CopyRect copy_rect(const CopySourceState &source) {
        return smgpc::render::CopyRect{
            .left = source.left,
            .top = source.top,
            .right = static_cast<std::int32_t>(source.left + source.width),
            .bottom = static_cast<std::int32_t>(source.top + source.height),
            .width = source.width,
            .height = source.height,
        };
    }

    [[nodiscard]] smgpc::render::CopyRect output_rect(u16 width, u16 height) {
        return smgpc::render::CopyRect{
            .left = 0,
            .top = 0,
            .right = width,
            .bottom = height,
            .width = width,
            .height = height,
        };
    }

    [[nodiscard]] smgpc::render::CopyViewport copy_viewport(const CopySourceState &source) {
        return smgpc::render::CopyViewport{
            .left = static_cast<float>(source.left),
            .right = static_cast<float>(source.left + source.width),
            .top = static_cast<float>(source.top),
            .bottom = static_cast<float>(source.top + source.height),
            .near_depth = 0.0F,
            .far_depth = 1.0F,
        };
    }

    [[nodiscard]] f32 gamma_value(GXGamma gamma) {
        switch (gamma) {
        case GX_GM_1_0:
            return 1.0F;
        case GX_GM_1_7:
            return 1.7F;
        case GX_GM_2_2:
            return 2.2F;
        }

        return 1.0F;
    }

    [[nodiscard]] u32 real_texture_format(GXTexFmt format) {
        if (format == GX_TF_Z16) {
            return 0xBU;
        }

        return static_cast<u32>(format) & 0xFU;
    }

    [[nodiscard]] u32 target_pixel_format(GXTexFmt format) {
        const auto real_format = real_texture_format(format);
        return ((real_format & 0x7U) << 1U) | ((real_format & 0x8U) >> 3U);
    }

    [[nodiscard]] u16 block_width_for_real_format(u32 real_format) {
        switch (real_format) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x7:
        case 0x8:
        case 0x9:
        case 0xA:
            return 8U;
        default:
            return 4U;
        }
    }

    [[nodiscard]] u16 block_height_for_real_format(u32 real_format) {
        switch (real_format) {
        case 0x0:
        case 0xE:
            return 8U;
        default:
            return 4U;
        }
    }

    [[nodiscard]] u32 bytes_per_block_for_real_format(u32 real_format) {
        switch (real_format) {
        case 0x6:
            return 64U;
        default:
            return 32U;
        }
    }

    [[nodiscard]] u32 texture_buffer_size_for_level(u16 width, u16 height, GXTexFmt format) {
        const auto real_format = real_texture_format(format);
        const auto block_width = block_width_for_real_format(real_format);
        const auto block_height = block_height_for_real_format(real_format);
        const auto row_blocks = (static_cast<u32>(std::max<u16>(width, 1U)) + block_width - 1U) / block_width;
        const auto column_blocks = (static_cast<u32>(std::max<u16>(height, 1U)) + block_height - 1U) / block_height;
        return row_blocks * column_blocks * bytes_per_block_for_real_format(real_format);
    }

    [[nodiscard]] u32 texture_copy_stride(u16 width, GXTexFmt format) {
        const auto block_width = block_width_for_real_format(real_texture_format(format));
        const auto row_blocks = (static_cast<u32>(width) + block_width - 1U) / block_width;
        return row_blocks * 32U;
    }

    [[nodiscard]] u32 texture_copy_stride(u16 width, u16 height, GXTexFmt format) {
        const auto base_stride = texture_copy_stride(width, format);
        if (format == GX_TF_CMPR && height > 4U) {
            return base_stride / 2U;
        }

        return base_stride;
    }

    [[nodiscard]] u32 scaled_copy_lines(u32 efb_height, u32 i_scale) {
        auto count = (efb_height - 1U) * 256U;
        auto real_height = (count / std::max(i_scale, 1U)) + 1U;
        auto scale_denominator = i_scale;
        if (scale_denominator > 0x80U && scale_denominator < 0x100U) {
            while ((scale_denominator & 0x01U) == 0U) {
                scale_denominator >>= 1U;
            }
            if (efb_height % scale_denominator == 0U) {
                ++real_height;
            }
        }

        return std::min(real_height, 1024U);
    }

    [[nodiscard]] u32 display_copy_y_scale(f32 vertical_scale) {
        if (vertical_scale <= 0.0F || !std::isfinite(vertical_scale)) {
            return 256U;
        }

        return static_cast<u32>(256.0F / vertical_scale) & 0x1FFU;
    }

    [[nodiscard]] std::array<std::uint8_t, 4U> clear_color_value(GXColor color) {
        return {color.r, color.g, color.b, color.a};
    }

    void record_copy_event(smgpc::render::CopyEvent event) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->record_copy_event(std::move(event));
        }
    }

}  // namespace

void GXSetDispCopySrc(u16 left, u16 top, u16 width, u16 height) {
    s_copy_state.display.source = CopySourceState{.left = left, .top = top, .width = width, .height = height};
}

void GXSetTexCopySrc(u16 left, u16 top, u16 width, u16 height) {
    s_copy_state.texture.source = CopySourceState{.left = left, .top = top, .width = width, .height = height};
}

void GXSetDispCopyDst(u16 width, u16 height) {
    s_copy_state.display.width = width;
    s_copy_state.display.height = height;
    s_copy_state.display.dest_stride = static_cast<u32>(width) * 2U;
}

void GXSetTexCopyDst(u16 width, u16 height, GXTexFmt format, GXBool mipmap) {
    s_copy_state.texture.width = width;
    s_copy_state.texture.height = height;
    s_copy_state.texture.format = format;
    s_copy_state.texture.mipmap = mipmap;
    s_copy_state.texture.dest_stride = texture_copy_stride(width, height, format);
}

void GXSetDispCopyFrame2Field(GXCopyMode mode) {
    s_copy_state.display.frame_to_field = mode;
}

void GXSetCopyClamp(GXFBClamp clamp) {
    s_copy_state.clamp_top = (static_cast<u32>(clamp) & GX_CLAMP_TOP) == GX_CLAMP_TOP;
    s_copy_state.clamp_bottom = (static_cast<u32>(clamp) & GX_CLAMP_BOTTOM) == GX_CLAMP_BOTTOM;
}

u16 GXGetNumXfbLines(u16 efb_height, f32 y_scale) {
    return static_cast<u16>(scaled_copy_lines(efb_height, display_copy_y_scale(y_scale)));
}

f32 GXGetYScaleFactor(u16 efb_height, u16 xfb_height) {
    auto target_height = static_cast<u32>(xfb_height);
    auto y_scale = static_cast<f32>(xfb_height) / static_cast<f32>(efb_height);
    auto real_height = scaled_copy_lines(efb_height, display_copy_y_scale(y_scale));

    while (real_height > xfb_height && target_height > 0U) {
        --target_height;
        y_scale = static_cast<f32>(target_height) / static_cast<f32>(efb_height);
        real_height = scaled_copy_lines(efb_height, display_copy_y_scale(y_scale));
    }

    auto final_scale = y_scale;
    while (real_height < xfb_height) {
        final_scale = y_scale;
        ++target_height;
        y_scale = static_cast<f32>(target_height) / static_cast<f32>(efb_height);
        real_height = scaled_copy_lines(efb_height, display_copy_y_scale(y_scale));
    }

    return final_scale;
}

u32 GXSetDispCopyYScale(f32 vertical_scale) {
    s_copy_state.y_scale = vertical_scale;
    s_copy_state.display_copy_y_scale = display_copy_y_scale(vertical_scale);
    return GXGetNumXfbLines(s_copy_state.display.source.height, vertical_scale);
}

void GXSetCopyClear(GXColor clear_color, u32 clear_z) {
    s_copy_state.clear_color = clear_color;
    s_copy_state.clear_depth = clear_z;
}

void GXSetCopyFilter(GXBool aa, const u8 sample_pattern[12][2], GXBool vertical_filter, const u8 vfilter[7]) {
    s_copy_state.copy_filter_aa = aa != GX_FALSE;
    s_copy_state.copy_filter_vertical = vertical_filter != GX_FALSE;
    if (sample_pattern != nullptr) {
        for (auto i = std::size_t{}; i < s_copy_state.copy_filter_sample_pattern.size(); ++i) {
            s_copy_state.copy_filter_sample_pattern[i][0U] = sample_pattern[i][0U];
            s_copy_state.copy_filter_sample_pattern[i][1U] = sample_pattern[i][1U];
        }
    }
    if (vfilter != nullptr) {
        for (auto i = std::size_t{}; i < s_copy_state.copy_filter_vfilter.size(); ++i) {
            s_copy_state.copy_filter_vfilter[i] = vfilter[i];
        }
    }
}

void GXSetDispCopyGamma(GXGamma gamma) {
    s_copy_state.gamma = gamma;
}

u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 max_lod) {
    auto total = 0U;
    auto level_width = std::max<u16>(width, 1U);
    auto level_height = std::max<u16>(height, 1U);
    const auto level_count = mipmap == GX_TRUE ? std::max<u8>(max_lod, 1U) : 1U;

    for (auto level = u8{0U}; level < level_count; ++level) {
        total += texture_buffer_size_for_level(level_width, level_height, static_cast<GXTexFmt>(format));
        if (mipmap != GX_TRUE || (level_width == 1U && level_height == 1U)) {
            break;
        }
        level_width = static_cast<u16>(std::max<u16>(level_width >> 1U, 1U));
        level_height = static_cast<u16>(std::max<u16>(level_height >> 1U, 1U));
    }

    return total;
}

void GXInitTexObj(GXTexObj *obj, void *image_ptr, u16 width, u16 height, GXTexFmt format, GXTexWrapMode wrap_s, GXTexWrapMode wrap_t,
                  GXBool mipmap) {
    if (obj == nullptr) {
        return;
    }

    obj->dummy[0U] = static_cast<u32>(reinterpret_cast<std::uintptr_t>(image_ptr));
    obj->dummy[1U] = (static_cast<u32>(width) << 16U) | height;
    obj->dummy[2U] = static_cast<u32>(format);
    obj->dummy[3U] = (static_cast<u32>(wrap_s) << 16U) | static_cast<u32>(wrap_t);
    obj->dummy[4U] = mipmap != GX_FALSE ? 1U : 0U;
}

void GXInitTexObjLOD(GXTexObj *obj, GXTexFilter min_filter, GXTexFilter mag_filter, f32 min_lod, f32 max_lod, f32 lod_bias,
                     GXBool bias_clamp, GXBool do_edge_lod, GXAnisotropy max_aniso) {
    if (obj == nullptr) {
        return;
    }

    obj->dummy[5U] = (static_cast<u32>(min_filter) << 16U) | static_cast<u32>(mag_filter);
    obj->dummy[6U] = (static_cast<u32>(bias_clamp != GX_FALSE) << 24U) | (static_cast<u32>(do_edge_lod != GX_FALSE) << 16U) |
                     static_cast<u32>(max_aniso);
    obj->dummy[7U] = static_cast<u32>((min_lod + max_lod + lod_bias) * 256.0F);
}

void GXLoadTexObj(GXTexObj *, GXTexMapID) {
}

void GXCopyDisp(void *dest, GXBool clear) {
    const auto &display = s_copy_state.display;
    const auto source_rect = copy_rect(display.source);
    const auto output_size = smgpc::render::FramebufferInfo{.width = display.width, .height = display.height};
    record_copy_event(smgpc::render::CopyEvent{
        .kind = smgpc::render::CopyEventKind::Xfb,
        .copy_to_xfb = true,
        .clear = clear != GX_FALSE,
        .clamp_top = s_copy_state.clamp_top,
        .clamp_bottom = s_copy_state.clamp_bottom,
        .clear_color = clear_color_value(s_copy_state.clear_color),
        .clear_depth = s_copy_state.clear_depth,
        .copy_filter_aa = s_copy_state.copy_filter_aa,
        .copy_filter_vertical = s_copy_state.copy_filter_vertical,
        .copy_filter_sample_pattern = s_copy_state.copy_filter_sample_pattern,
        .copy_filter_vfilter = s_copy_state.copy_filter_vfilter,
        .dest_addr = static_cast<u32>(reinterpret_cast<std::uintptr_t>(dest)),
        .dest_stride = display.dest_stride,
        .source_rect = source_rect,
        .output_size = output_size,
        .frame_to_field = static_cast<u32>(display.frame_to_field),
        .gamma_index = static_cast<u32>(s_copy_state.gamma),
        .gamma_value = gamma_value(s_copy_state.gamma),
        .y_scale = s_copy_state.y_scale,
        .dispcopyyscale = s_copy_state.display_copy_y_scale,
        .scissor = source_rect,
        .viewport = copy_viewport(display.source),
        .backbuffer = output_size,
        .target_rect = output_rect(display.width, display.height),
        .render_pass = "GXCopyDisp",
    });
}

void GXCopyTex(void *dest, GXBool clear) {
    const auto &texture = s_copy_state.texture;
    const auto source_rect = copy_rect(texture.source);
    const auto output_size = smgpc::render::FramebufferInfo{.width = texture.width, .height = texture.height};
    record_copy_event(smgpc::render::CopyEvent{
        .kind = smgpc::render::CopyEventKind::Texture,
        .copy_to_xfb = false,
        .depth_copy = (static_cast<u32>(texture.format) & _GX_TF_ZTF) == _GX_TF_ZTF,
        .clear = clear != GX_FALSE,
        .clamp_top = s_copy_state.clamp_top,
        .clamp_bottom = s_copy_state.clamp_bottom,
        .auto_conversion = true,
        .clear_color = clear_color_value(s_copy_state.clear_color),
        .clear_depth = s_copy_state.clear_depth,
        .copy_filter_aa = s_copy_state.copy_filter_aa,
        .copy_filter_vertical = s_copy_state.copy_filter_vertical,
        .copy_filter_sample_pattern = s_copy_state.copy_filter_sample_pattern,
        .copy_filter_vfilter = s_copy_state.copy_filter_vfilter,
        .dest_addr = static_cast<u32>(reinterpret_cast<std::uintptr_t>(dest)),
        .dest_stride = texture.dest_stride,
        .source_rect = source_rect,
        .output_size = output_size,
        .target_pixel_format = target_pixel_format(texture.format),
        .real_format = real_texture_format(texture.format),
        .gamma_index = static_cast<u32>(s_copy_state.gamma),
        .gamma_value = gamma_value(s_copy_state.gamma),
        .y_scale = s_copy_state.y_scale,
        .dispcopyyscale = s_copy_state.display_copy_y_scale,
        .scissor = source_rect,
        .viewport = copy_viewport(texture.source),
        .backbuffer = output_size,
        .target_rect = output_rect(texture.width, texture.height),
        .render_pass = "GXCopyTex",
    });
}

void GXPixModeSync() {
}

void DCFlushRange(void *, u32) {
}
