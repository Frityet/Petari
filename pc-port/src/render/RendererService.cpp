#include "RendererService.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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
#include <dolphin/gx.h>

namespace smgpc::render {
    namespace {

        constexpr auto kLogicalFramebuffer = FramebufferInfo {
            .width = core::kWiiLogicalFramebufferWidth,
            .height = core::kWiiLogicalFramebufferHeight,
        };

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

        [[nodiscard]] FramebufferInfo framebuffer_from_window_size(const AuroraWindowSize &size) {
            return {
                .width = clamp_framebuffer_dimension(size.fb_width == 0U ? size.native_fb_width : size.fb_width),
                .height = clamp_framebuffer_dimension(size.fb_height == 0U ? size.native_fb_height : size.fb_height),
            };
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

        void configure_copy_clear() {
            GXSetCopyClear(GXColor {.r = 0U, .g = 0U, .b = 0U, .a = 255U}, GX_MAX_Z24);
            GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
        }

        void configure_2d_projection() {
            const auto width = static_cast<float>(kLogicalFramebuffer.width);
            const auto height = static_cast<float>(kLogicalFramebuffer.height);
            const auto half_width = width * 0.5F;
            const auto half_height = height * 0.5F;
            const float projection[4][4] = {
                {1.0F / half_width, 0.0F, 0.0F, 0.0F},
                {0.0F, -1.0F / half_height, 0.0F, 0.0F},
                {0.0F, 0.0F, 1.0F, 0.0F},
                {0.0F, 0.0F, 0.0F, 1.0F},
            };
            const float position[3][4] = {
                {1.0F, 0.0F, 0.0F, 0.0F},
                {0.0F, 1.0F, 0.0F, 0.0F},
                {0.0F, 0.0F, 1.0F, 0.0F},
            };

            GXSetProjection(projection, GX_ORTHOGRAPHIC);
            GXLoadPosMtxImm(position, GX_PNMTX0);
            GXSetCurrentMtx(GX_PNMTX0);
            GXSetViewport(0.0F, 0.0F, width, height, 0.0F, 1.0F);
            GXSetScissor(0U, 0U, kLogicalFramebuffer.width, kLogicalFramebuffer.height);
            GXSetViewportRender(0.0F, 0.0F, width, height, 0.0F, 1.0F);
            GXSetScissorRender(0U, 0U, kLogicalFramebuffer.width, kLogicalFramebuffer.height);
        }

        void configure_channel_state() {
            GXSetNumChans(1U);
            GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
            GXSetChanMatColor(GX_COLOR0A0, GXColor {.r = 255U, .g = 255U, .b = 255U, .a = 255U});
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
                GXTexCoord2f32(vertex.tex_coords[index][0], vertex.tex_coords[index][1]);
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
            AuroraSetViewportPolicy(AURORA_VIEWPORT_FIT);
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
                pointer.valid = true;
                pointer.x = event.button.x * static_cast<float>(core::kWiiLogicalFramebufferWidth) / std::max<float>(1.0F, static_cast<float>(size.width));
                pointer.y = event.button.y * static_cast<float>(core::kWiiLogicalFramebufferHeight) / std::max<float>(1.0F, static_cast<float>(size.height));
                break;
            case SDL_EVENT_MOUSE_MOTION:
                pointer.valid = true;
                pointer.x = event.motion.x * static_cast<float>(core::kWiiLogicalFramebufferWidth) / std::max<float>(1.0F, static_cast<float>(size.width));
                pointer.y = event.motion.y * static_cast<float>(core::kWiiLogicalFramebufferHeight) / std::max<float>(1.0F, static_cast<float>(size.height));
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

    AuroraWindow::AuroraWindow() = default;

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
        struct TextureRecord {
            std::uint16_t width = 0U;
            std::uint16_t height = 0U;
            std::vector<std::uint8_t> rgba;
            GXTexObj object {};
            bool alive = false;
        };

        explicit Impl(AuroraWindow &window_) : window(window_) {
        }

        [[nodiscard]] TextureRecord *texture(TextureHandle handle) {
            if (!handle.is_valid() || handle.value >= textures.size() || !textures[handle.value].alive) {
                return nullptr;
            }
            return &textures[handle.value];
        }

        [[nodiscard]] const TextureRecord *texture(TextureHandle handle) const {
            if (!handle.is_valid() || handle.value >= textures.size() || !textures[handle.value].alive) {
                return nullptr;
            }
            return &textures[handle.value];
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

        void configure_textured_state(TextureHandle texture_handle, const TexturedTriangleBatch2D &batch) {
            configure_2d_projection();
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

        void configure_material_state(const GxMaterialTriangleBatch2D &batch) {
            configure_2d_projection();
            configure_channel_state();
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
                GXSetTexCoordGen(gx_tex_coord_id(index), GX_TG_MTX2x4, static_cast<GXTexGenSrc>(GX_TG_TEX0 + index), GX_IDENTITY);
                load_texture(stage.texture, gx_tex_map_id(index), stage.wrap_u, stage.wrap_v, stage.min_filter, stage.mag_filter);
            }
            GXSetNumTexGens(static_cast<u8>(texture_count));

            for (auto index = std::size_t {}; index < batch.initial_tev_registers.size(); ++index) {
                const auto &color = batch.initial_tev_registers[index];
                GXSetTevColorS10(static_cast<GXTevRegID>(GX_TEVREG0 + index),
                                 GXColorS10 {.r = color[0], .g = color[1], .b = color[2], .a = color[3]});
            }

            GXSetNumTevStages(static_cast<u8>(tev_count));
            for (auto index = std::size_t {}; index < tev_count; ++index) {
                const auto stage_id = static_cast<GXTevStageID>(GX_TEVSTAGE0 + index);
                if (index >= batch.tev_stages.size()) {
                    GXSetTevOrder(stage_id, texture_count == 0U ? GX_TEXCOORD_NULL : GX_TEXCOORD0,
                                  texture_count == 0U ? GX_TEXMAP_NULL : GX_TEXMAP0, GX_COLOR0A0);
                    GXSetTevOp(stage_id, texture_count == 0U ? GX_PASSCLR : GX_MODULATE);
                    continue;
                }

                const auto &stage = batch.tev_stages[index];
                const auto has_texture = stage.texture_stage < texture_count;
                const auto texture_stage = has_texture ? stage.texture_stage : 0U;
                const auto &konst = stage.konst_color;
                GXSetTevKColor(gx_k_color_id(index), GXColor {.r = konst[0], .g = konst[1], .b = konst[2], .a = konst[3]});
                GXSetTevKColorSel(stage_id, gx_k_color_sel(index));
                GXSetTevKAlphaSel(stage_id, gx_k_alpha_sel(index));
                GXSetTevSwapMode(stage_id, GX_TEV_SWAP0, GX_TEV_SWAP0);
                GXSetTevOrder(stage_id, has_texture ? gx_tex_coord_id(texture_stage) : GX_TEXCOORD_NULL,
                              has_texture ? gx_tex_map_id(texture_stage) : GX_TEXMAP_NULL, GX_COLOR0A0);
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
        }

        AuroraWindow &window;
        std::vector<TextureRecord> textures;
        std::vector<std::uint32_t> free_texture_slots;
        std::uint64_t frame_index = 0U;
        double frame_time = 0.0;
        std::uint64_t frame_created_textures = 0U;
        std::uint64_t frame_textured_submits = 0U;
        std::uint64_t frame_material_submits = 0U;
        std::uint64_t frame_submitted_vertices = 0U;
        bool frame_open = false;
    };

    AuroraRenderer::AuroraRenderer() = default;

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

    void AuroraRenderer::request_screenshot_png(const std::filesystem::path &) {
        // Aurora readback support is intentionally not mirrored through the old screenshot queue.
    }

    TextureHandle AuroraRenderer::create_rgba8_texture(std::uint16_t width, std::uint16_t height, std::span<const std::uint8_t> rgba) {
        if (width == 0U || height == 0U || rgba.size() < static_cast<std::size_t>(width) * height * 4U) {
            return invalid_texture();
        }

        auto handle = TextureHandle {};
        if (!_impl->free_texture_slots.empty()) {
            handle.value = _impl->free_texture_slots.back();
            _impl->free_texture_slots.pop_back();
        } else {
            handle.value = static_cast<std::uint32_t>(_impl->textures.size());
            _impl->textures.emplace_back();
        }

        auto &record = _impl->textures[handle.value];
        record.width = width;
        record.height = height;
        record.rgba.assign(rgba.begin(), rgba.begin() + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(width) * height * 4U));
        record.alive = true;
        GXInitTexObj(&record.object, record.rgba.data(), width, height, GX_TF_RGBA8_PC, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GXInitTexObjLOD(&record.object, GX_LINEAR, GX_LINEAR, 0.0F, 0.0F, 0.0F, GX_FALSE, GX_FALSE, GX_ANISO_1);
        ++_impl->frame_created_textures;
        return handle;
    }

    void AuroraRenderer::destroy_texture(TextureHandle texture) {
        auto *record = _impl->texture(texture);
        if (record == nullptr) {
            return;
        }
        record->alive = false;
        record->rgba.clear();
        _impl->free_texture_slots.push_back(texture.value);
    }

    void AuroraRenderer::submit_textured_quad(TextureHandle texture, const TexturedQuad2D &quad) {
        const auto indices = std::array<std::uint16_t, 6U> {0U, 1U, 2U, 0U, 2U, 3U};
        submit_textured_triangles(texture, TexturedTriangleBatch2D {
                                               .vertices = std::span<const TexturedVertex2D>(quad.vertices),
                                               .indices = std::span<const std::uint16_t>(indices),
                                               .primitive_topology = PrimitiveTopology::Triangles,
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

    FramebufferInfo AuroraRenderer::framebuffer_size() const {
        return _impl->window.framebuffer_size();
    }

    FramebufferInfo AuroraRenderer::logical_framebuffer_size() const {
        return kLogicalFramebuffer;
    }

}  // namespace smgpc::render
