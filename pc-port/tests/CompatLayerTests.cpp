#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/FileSelectItem.hpp"
#include "Game/Map/FileSelectSky.hpp"
#include "Game/Map/FileSelector.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/compat/BcsvTable.hpp"
#include "Game/compat/BrfntFont.hpp"
#include "Game/compat/BrlanAnimation.hpp"
#include "Game/compat/BrlytLayout.hpp"
#include "Game/compat/CameraParam.hpp"
#include "Game/compat/CameraPose.hpp"
#include "Game/compat/FileSelectSkyRuntime.hpp"
#include "Game/compat/J3dAnimation.hpp"
#include "Game/compat/J3dMaterialRuntime.hpp"
#include "Game/compat/J3dModel.hpp"
#include "Game/compat/J3dModelRenderer.hpp"
#include "Game/compat/J3dTexture.hpp"
#include "Game/compat/JMathTrig.hpp"
#include "Game/compat/ParityTrace.hpp"
#include "Game/compat/RarcArchive.hpp"
#include "Game/compat/RuntimeContext.hpp"
#include "Game/compat/RuntimeServices.hpp"
#include "Game/compat/TplTexture.hpp"
#include "Game/compat/Yaz0.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "capture/ScreenshotService.hpp"
#include "core/RenderTypes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    class NullLogger final : public smgpc::logging::ILogger {
    public:
        void write(std::FILE *, std::source_location, smgpc::logging::Level, smgpc::logging::Category, std::string_view) override {
        }
    };

    class TestWindowService final : public smgpc::render::IWindowService {
    public:
        bool poll_events() override {
            return true;
        }

        [[nodiscard]] bool should_close() const override {
            return false;
        }

        [[nodiscard]] bool is_focused() const override {
            return true;
        }

        [[nodiscard]] bool is_minimized() const override {
            return false;
        }

        [[nodiscard]] smgpc::render::FramebufferInfo framebuffer_size() const override {
            return {.width = 640U, .height = 456U};
        }

        [[nodiscard]] smgpc::render::NativeWindowHandle native_handle() const override {
            return {};
        }

        [[nodiscard]] bool is_input_pressed(smgpc::render::InputButton button) const override {
            switch (button) {
            case smgpc::render::InputButton::CORE_PAD_A:
            case smgpc::render::InputButton::CORE_PAD_B:
                return _hold_title_combo;
            }

            return false;
        }

        void set_title_combo(bool is_pressed) {
            _hold_title_combo = is_pressed;
        }

    private:
        bool _hold_title_combo = false;
    };

    class RecordingRenderer final : public smgpc::render::IRendererEngine {
    public:
        [[nodiscard]] smgpc::render::FrameContext begin_frame() override {
            return {
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = framebuffer_size(),
                .has_focus = true,
                .is_minimized = false,
            };
        }

        void end_frame() override {
        }

        void shutdown() override {
        }

        void request_screenshot_png(const std::filesystem::path &) override {
        }

        [[nodiscard]] smgpc::render::TextureHandle create_rgba8_texture(std::uint16_t width, std::uint16_t height,
                                                                        std::span<const std::uint8_t> rgba) override {
            if (width == 0U || height == 0U) {
                throw std::runtime_error("recording renderer should receive non-empty textures");
            }
            if (rgba.size() != static_cast<std::size_t>(width) * height * 4U) {
                throw std::runtime_error("recording renderer texture upload size mismatch");
            }
            ++texture_count;
            return {.value = next_texture++};
        }

        void destroy_texture(smgpc::render::TextureHandle) override {
        }

        void submit_textured_quad(smgpc::render::TextureHandle texture, const smgpc::render::TexturedQuad2D &) override {
            if (!texture.is_valid()) {
                throw std::runtime_error("recording renderer should receive valid quad texture handles");
            }
            ++quad_count;
        }

        void submit_textured_triangles(smgpc::render::TextureHandle texture, const smgpc::render::TexturedTriangleBatch2D &batch) override {
            if (!texture.is_valid()) {
                throw std::runtime_error("recording renderer should receive valid triangle texture handles");
            }
            ++triangle_batch_count;
            submitted_vertices += batch.vertices.size();
            submitted_indices += batch.indices.size();
            last_triangle_cull_mode = batch.cull_mode;
        }

        void submit_gx_material_triangles(const smgpc::render::GxMaterialTriangleBatch2D &batch) override {
            if (batch.texture_stages.empty()) {
                throw std::runtime_error("recording renderer should receive GX material texture stages");
            }
            if (batch.tev_stages.empty()) {
                throw std::runtime_error("recording renderer should receive GX TEV stages");
            }
            for (const auto &stage : batch.texture_stages) {
                if (!stage.texture.is_valid()) {
                    throw std::runtime_error("recording renderer should receive valid GX material texture handles");
                }
            }
            ++gx_material_batch_count;
            submitted_vertices += batch.vertices.size();
            submitted_indices += batch.indices.size();
            last_gx_material_stage_count = batch.texture_stages.size();
            last_gx_material_tev_stage_count = batch.tev_stages.size();
            last_gx_material_blend = batch.blend;
            last_gx_material_alpha_compare = batch.alpha_compare;
            last_gx_material_initial_tev_registers = batch.initial_tev_registers;
            last_gx_material_fog = batch.fog;
            last_gx_material_depth_test = batch.depth_test;
            last_gx_material_depth_write = batch.depth_write;
            last_gx_material_depth_compare = batch.depth_compare;
            last_gx_material_color_inputs.fill({});
            last_gx_material_stage_konst_colors.fill({});
            for (auto i = std::size_t{}; i < batch.tev_stages.size() && i < last_gx_material_color_inputs.size(); ++i) {
                last_gx_material_color_inputs[i] = batch.tev_stages[i].color_in;
                last_gx_material_stage_konst_colors[i] = batch.tev_stages[i].konst_color;
            }
            if (batch.tev_stages.size() == 2U) {
                saw_gx_material_two_stage_batch = true;
                saw_gx_material_texture_stage_one = saw_gx_material_texture_stage_one || batch.tev_stages[1U].texture_stage == 1U;
                last_two_stage_gx_material_blend = batch.blend;
                saw_gx_material_nonzero_initial_register =
                    saw_gx_material_nonzero_initial_register ||
                    std::ranges::any_of(batch.initial_tev_registers, [](const auto &color) {
                        return std::ranges::any_of(color, [](auto channel) { return channel != 0; });
                    });
            }
            last_gx_material_saw_projective_q = std::ranges::any_of(batch.vertices, [](const auto &vertex) {
                return std::ranges::any_of(vertex.tex_coords, [](const auto &coord) { return std::abs(coord[2U] - 1.0F) > 0.001F; });
            });
            last_gx_material_saw_clip_w = std::ranges::any_of(batch.vertices, [](const auto &vertex) { return vertex.clip_w > 1.001F; });
            last_gx_material_alpha_compare_enabled = batch.alpha_compare.enabled;
            last_triangle_cull_mode = batch.cull_mode;
        }

        [[nodiscard]] smgpc::render::FramebufferInfo framebuffer_size() const override {
            return {.width = 640U, .height = 456U};
        }

        [[nodiscard]] smgpc::render::FramebufferInfo logical_framebuffer_size() const override {
            return {.width = 640U, .height = 456U};
        }

        std::uint32_t next_texture = 1U;
        std::size_t texture_count = 0U;
        std::size_t quad_count = 0U;
        std::size_t triangle_batch_count = 0U;
        std::size_t gx_material_batch_count = 0U;
        std::size_t last_gx_material_stage_count = 0U;
        std::size_t last_gx_material_tev_stage_count = 0U;
        std::size_t submitted_vertices = 0U;
        std::size_t submitted_indices = 0U;
        bool last_gx_material_saw_projective_q = false;
        bool last_gx_material_saw_clip_w = false;
        bool last_gx_material_alpha_compare_enabled = false;
        smgpc::render::GxAlphaCompare2D last_gx_material_alpha_compare{};
        smgpc::render::GxBlendMode2D last_gx_material_blend{};
        smgpc::render::GxBlendMode2D last_two_stage_gx_material_blend{};
        smgpc::render::GxFog2D last_gx_material_fog{};
        std::array<smgpc::render::GxTevRegisterColor2D, 4U> last_gx_material_initial_tev_registers{};
        bool last_gx_material_depth_test = false;
        bool last_gx_material_depth_write = false;
        smgpc::render::DepthCompare last_gx_material_depth_compare = smgpc::render::DepthCompare::LessEqual;
        bool saw_gx_material_two_stage_batch = false;
        bool saw_gx_material_texture_stage_one = false;
        bool saw_gx_material_nonzero_initial_register = false;
        std::array<std::array<std::uint8_t, 4U>, smgpc::render::core::kMaxGxMaterialTevStages2D> last_gx_material_color_inputs{};
        std::array<std::array<std::uint8_t, 4U>, smgpc::render::core::kMaxGxMaterialTevStages2D> last_gx_material_stage_konst_colors{};
        smgpc::render::CullMode last_triangle_cull_mode = smgpc::render::CullMode::None;
    };

    class SchedulerProbeObj final : public NameObj {
    public:
        explicit SchedulerProbeObj(const char *name) : NameObj(name) {
        }

        void movement() override {
            ++movement_count;
        }

        void calcAnim() override {
            ++calc_anim_count;
        }

        void calcViewAndEntry() override {
            ++calc_view_count;
        }

        int movement_count = 0;
        int calc_anim_count = 0;
        int calc_view_count = 0;
    };

    class SchedulerProbeActor final : public LiveActor {
    public:
        explicit SchedulerProbeActor(const char *name) : LiveActor(name) {
        }

        void calcAnim() override {
            ++calc_anim_count;
        }

        void control() override {
            ++control_count;
        }

        void calcAndSetBaseMtx() override {
            ++calc_view_count;
        }

        int calc_anim_count = 0;
        int control_count = 0;
        int calc_view_count = 0;
    };

    [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
        if (offset + 4U > data.size()) {
            throw std::runtime_error("read_be32 out of range");
        }

        return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
               (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
    }

    [[nodiscard]] std::vector<std::uint8_t> read_file(const std::filesystem::path &path) {
        auto file = std::ifstream(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("cannot open " + path.string());
        }

        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        if (size < 0) {
            throw std::runtime_error("cannot determine size for " + path.string());
        }

        auto bytes = std::vector<std::uint8_t>(static_cast<std::size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!file) {
            throw std::runtime_error("cannot read " + path.string());
        }

        return bytes;
    }

    [[nodiscard]] std::filesystem::path disc_files_root() {
        const auto cwd = std::filesystem::current_path();
        const std::filesystem::path candidates[]{
            cwd / "orig" / "RMGK01" / "files",
            cwd.parent_path() / "orig" / "RMGK01" / "files",
        };

        for (const auto &candidate : candidates) {
            std::error_code error{};
            const auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        throw std::runtime_error("could not locate orig/RMGK01/files from " + cwd.string());
    }

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, float tolerance, std::string_view message) {
        if (std::abs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_tev_stage(const smgpc::game::J3dTevStageSummary &stage, std::array<std::uint8_t, 4U> color_in, std::uint8_t k_color_sel,
                           std::array<std::uint8_t, 4U> alpha_in, std::uint8_t alpha_clamp, std::uint8_t k_alpha_sel, std::string_view message) {
        require(stage.color_in == color_in, message);
        require(stage.color_op == 0U && stage.color_bias == 0U && stage.color_scale == 0U && stage.color_clamp == 1U && stage.color_out == 0U,
                message);
        require(stage.k_color_sel == k_color_sel, message);
        require(stage.alpha_in == alpha_in, message);
        require(stage.alpha_op == 0U && stage.alpha_bias == 0U && stage.alpha_scale == 0U && stage.alpha_clamp == alpha_clamp &&
                    stage.alpha_out == 0U,
                message);
        require(stage.k_alpha_sel == k_alpha_sel, message);
    }

    void require_magic(std::span<const std::uint8_t> data, std::string_view magic) {
        require(data.size() >= magic.size(), "data too short for magic");
        for (std::size_t i = 0; i < magic.size(); ++i) {
            require(data[i] == static_cast<std::uint8_t>(magic[i]), "unexpected magic");
        }
    }

    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] std::string base_name(std::string_view path) {
        const auto slash = path.find_last_of('/');
        if (slash == std::string_view::npos) {
            return std::string(path);
        }

        return std::string(path.substr(slash + 1U));
    }

    [[nodiscard]] const smgpc::game::RarcEntry *find_entry_by_basename(const smgpc::game::RarcArchive &archive, std::string_view name) {
        const auto requested = lower_copy(name);
        const auto it =
            std::ranges::find_if(archive.entries(), [&requested](const auto &entry) { return lower_copy(base_name(entry.path)) == requested; });

        return it == archive.entries().end() ? nullptr : &(*it);
    }

    void test_yaz0_decompression(const std::filesystem::path &title_logo_path) {
        const auto compressed = read_file(title_logo_path);
        require(smgpc::game::is_yaz0(compressed), "TitleLogo.arc should be Yaz0-compressed");

        const auto decompressed = smgpc::game::decompress_yaz0(compressed);
        require_magic(decompressed, "RARC");
        require(read_be32(decompressed, 0x04U) == decompressed.size(), "RARC header file size should match decompressed size");
    }

    void test_rarc_title_archives(const std::filesystem::path &root) {
        const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
        const auto press_start = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "PressStart.arc");

        require(title_logo.entries().size() == 18U, "TitleLogo.arc entry count changed");
        require(press_start.entries().size() == 5U, "PressStart.arc entry count changed");

        require(title_logo.contains("blyt/titlelogo.brlyt"), "TitleLogo.arc missing titlelogo.brlyt");
        require(title_logo.contains("anim/appear.brlan"), "TitleLogo.arc missing appear.brlan");
        require(title_logo.contains("anim/wait.brlan"), "TitleLogo.arc missing wait.brlan");
        require(title_logo.contains("anim/decide.brlan"), "TitleLogo.arc missing decide.brlan");
        require(title_logo.contains("timg/mytitlelogokor.tpl"), "TitleLogo.arc missing Korean title logo texture");

        require(press_start.contains("blyt/pressstart.brlyt"), "PressStart.arc missing pressstart.brlyt");
        require(press_start.contains("anim/appear.brlan"), "PressStart.arc missing appear.brlan");
        require(press_start.contains("anim/wait.brlan"), "PressStart.arc missing wait.brlan");
        require(press_start.contains("anim/end.brlan"), "PressStart.arc missing end.brlan");

        require_magic(title_logo.file_data("blyt/titlelogo.brlyt"), "RLYT");
        require_magic(title_logo.file_data("anim/appear.brlan"), "RLAN");
        require_magic(press_start.file_data("blyt/pressstart.brlyt"), "RLYT");
        require_magic(press_start.file_data("anim/appear.brlan"), "RLAN");
    }

    void test_tpl_title_texture_decode(const std::filesystem::path &root) {
        const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
        const auto title_texture = smgpc::game::decode_tpl_texture(title_logo.file_data("timg/mytitlelogokor.tpl"));
        require(title_texture.width == 272U, "mytitlelogokor.tpl width changed");
        require(title_texture.height == 32U, "mytitlelogokor.tpl height changed");
        require(title_texture.format == smgpc::game::TplTextureFormat::I4, "mytitlelogokor.tpl format changed");
        require(title_texture.rgba.size() == static_cast<std::size_t>(title_texture.width) * title_texture.height * 4U,
                "decoded title texture size mismatch");

        auto visible_pixels = 0U;
        for (std::size_t offset = 3U; offset < title_texture.rgba.size(); offset += 4U) {
            if (title_texture.rgba[offset] != 0U) {
                ++visible_pixels;
            }
        }
        require(visible_pixels > 1000U, "decoded title texture should contain visible pixels");
    }

    void test_brlyt_title_picture_parse(const std::filesystem::path &root) {
        const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
        const auto layout = smgpc::game::parse_brlyt_layout(title_logo.file_data("blyt/titlelogo.brlyt"));
        require_near(layout.width, 608.0F, 0.001F, "titlelogo.brlyt layout width changed");
        require_near(layout.height, 456.0F, 0.001F, "titlelogo.brlyt layout height changed");
        require(!layout.panes.empty(), "titlelogo.brlyt should expose pane hierarchy");
        require(!layout.texture_names.empty(), "titlelogo.brlyt should reference textures");
        require(!layout.materials.empty(), "titlelogo.brlyt should expose materials");
        require(!layout.pictures.empty(), "titlelogo.brlyt should contain picture panes");

        const auto title_root = std::ranges::find_if(layout.panes, [](const auto &pane) { return pane.name == "SMGTitleLogo"; });
        require(title_root != layout.panes.end(), "titlelogo.brlyt should contain SMGTitleLogo pane");
        require(title_root->scale_x == 0.0F && title_root->scale_y == 0.0F, "SMGTitleLogo base scale should remain animation-driven");

        const auto title_picture =
            std::ranges::find_if(layout.pictures, [](const auto &picture) { return picture.texture_name == "MyTitleLogoKOR.tpl"; });
        require(title_picture != layout.pictures.end(), "titlelogo.brlyt should reference MyTitleLogoKOR.tpl");
        require(title_picture->material_index == 3U, "title logo picture material index changed");
        require(title_picture->wrap_s == 0U && title_picture->wrap_t == 0U, "title logo picture should preserve clamp wrap modes from BRLYT TexMap");
        require(title_picture->min_filter == 0U && title_picture->mag_filter == 0U, "title logo picture should preserve BRLYT TexMap filter bits");
        require(title_picture->width > 0.0F, "title logo picture width should be positive");
        require(title_picture->height > 0.0F, "title logo picture height should be positive");
        require_near(title_picture->tex_coords[0U].u, 0.0F, 0.001F, "title picture top-left U should come from BRLYT tex coords");
        require_near(title_picture->tex_coords[0U].v, 0.0F, 0.001F, "title picture top-left V should come from BRLYT tex coords");
        require_near(title_picture->tex_coords[2U].u, 1.0F, 0.001F, "title picture bottom-right U should come from BRLYT tex coords");
        require_near(title_picture->tex_coords[2U].v, 1.0F, 0.001F, "title picture bottom-right V should come from BRLYT tex coords");
        require(title_picture->vertex_colors[0U][3U] == 255U, "title picture vertex alpha should come from BRLYT vertex colors");

        const auto galaxy_picture = std::ranges::find_if(layout.pictures, [](const auto &picture) { return picture.name == "PicLogoGalaxy"; });
        require(galaxy_picture != layout.pictures.end(), "titlelogo.brlyt should keep the multi-texture PicLogoGalaxy picture");
        require(galaxy_picture->material_index == 0U, "PicLogoGalaxy material index changed");

        const auto &galaxy_material = layout.materials.at(galaxy_picture->material_index);
        require(galaxy_material.name == "PicLogoGalaxy", "PicLogoGalaxy material name changed");
        require(galaxy_material.textures.size() == 2U, "PicLogoGalaxy should use the original two texture maps");
        require(galaxy_material.textures[0U].texture_name == "MyTitleSpaceKOR.tpl",
                "PicLogoGalaxy first texture should be the scrolling space texture");
        require(galaxy_material.textures[1U].texture_name == "MyTitleMaskKOR.tpl", "PicLogoGalaxy second texture should be the Wii mask texture");
        require(galaxy_material.textures[0U].wrap_s == 1U && galaxy_material.textures[0U].wrap_t == 0U,
                "PicLogoGalaxy space texture should preserve BRLYT wrap modes");
        require(galaxy_material.tex_srts.size() == 2U, "PicLogoGalaxy should expose both texture SRT slots");
        require(galaxy_material.tex_coord_gens.size() == 2U, "PicLogoGalaxy should expose both texture coord generators");
        require(galaxy_material.tex_coord_gens[0U].tex_mtx == 30U && galaxy_material.tex_coord_gens[1U].tex_mtx == 33U,
                "PicLogoGalaxy texture generators should preserve GX_TEXMTX0/GX_TEXMTX1");
        require(galaxy_material.tev_stages.size() == 2U, "PicLogoGalaxy should expose both original TEV stages");
        require(galaxy_material.alpha_compare.enabled, "PicLogoGalaxy should preserve alpha compare state");
        require(galaxy_material.blend_mode.enabled, "PicLogoGalaxy should preserve blend mode state");
        require(galaxy_material.gx_state.source == "BRLYT", "PicLogoGalaxy should populate the shared GX material state as a BRLYT material");
        require(galaxy_material.gx_state.textures.size() == galaxy_material.textures.size(),
                "PicLogoGalaxy GX state should preserve BRLYT texture bindings");
        require(galaxy_material.gx_state.tex_coord_gens.size() == galaxy_material.tex_coord_gens.size(),
                "PicLogoGalaxy GX state should preserve BRLYT texgen bindings");
        require(galaxy_material.gx_state.tev_stages.size() == galaxy_material.tev_stages.size(),
                "PicLogoGalaxy GX state should preserve BRLYT TEV stages");
        require(galaxy_material.gx_state.alpha_compare.enabled && galaxy_material.gx_state.blend.enabled,
                "PicLogoGalaxy GX state should carry alpha compare and blend state");
    }

    void test_simple_layout_title_logo_uses_gx_tev_material_batches() {
        auto logger = NullLogger();
        auto window = TestWindowService();
        auto runtime = smgpc::game::RuntimeContext(logger, window);
        auto renderer = RecordingRenderer();
        auto layout = SimpleLayout("TitleLogoProbe", "TitleLogo", 2U, MR::DrawType_Layout);

        layout.startAnim("Appear", 0U);
        layout.setAnimFrameAndStop(4.0F, 0U);
        layout.startAnim("Wait", 1U);
        layout.setAnimFrameAndStop(5000.0F, 1U);
        layout.appear();
        layout.draw(renderer);

        require(renderer.texture_count > 0U, "TitleLogo SimpleLayout should upload original BRLYT textures");
        require(renderer.gx_material_batch_count > 0U, "TitleLogo SimpleLayout should draw picture panes through GX material batches");
        require(renderer.quad_count == 0U, "TitleLogo SimpleLayout should not use the old textured-quad picture path");
        require(renderer.saw_gx_material_two_stage_batch && renderer.saw_gx_material_texture_stage_one,
                "TitleLogo SimpleLayout should submit PicLogoGalaxy as a two-stage BRLYT TEV batch");
        require(renderer.saw_gx_material_nonzero_initial_register,
                "TitleLogo SimpleLayout should pass BRLYT TEV register colors into the GX material batch");
        require(renderer.last_two_stage_gx_material_blend.enabled && renderer.last_two_stage_gx_material_blend.type == 1U,
                "TitleLogo SimpleLayout should pass PicLogoGalaxy raw BRLYT GX blend state into the GX material batch");
    }

    void test_brlyt_press_start_text_parse(const std::filesystem::path &root) {
        const auto press_start = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "PressStart.arc");
        const auto layout = smgpc::game::parse_brlyt_layout(press_start.file_data("blyt/pressstart.brlyt"));
        require_near(layout.width, 608.0F, 0.001F, "pressstart.brlyt layout width changed");
        require_near(layout.height, 456.0F, 0.001F, "pressstart.brlyt layout height changed");
        require(!layout.font_names.empty(), "pressstart.brlyt should reference font resources");
        require(!layout.panes.empty(), "pressstart.brlyt should expose pane hierarchy");
        require(!layout.text_boxes.empty(), "pressstart.brlyt should contain text boxes");

        const auto shadow = std::ranges::find_if(layout.text_boxes, [](const auto &text_box) { return text_box.name == "ShaStart"; });
        require(shadow != layout.text_boxes.end(), "pressstart.brlyt should contain ShaStart shadow text box");
        require(shadow->material_index == 0U, "ShaStart should use the original shadow material");
        require(shadow->color_mapping_max[0U] == 0U && shadow->color_mapping_max[1U] == 0U && shadow->color_mapping_max[2U] == 0U,
                "ShaStart material should map glyph color to black");
        require(shadow->color_mapping_max[3U] == 100U, "ShaStart material should preserve original shadow alpha mapping");

        const auto prompt = std::ranges::find_if(layout.text_boxes, [](const auto &text_box) { return text_box.name == "TxtStart"; });
        require(prompt != layout.text_boxes.end(), "pressstart.brlyt should contain TxtStart text box");
        require(prompt->material_index == 1U, "TxtStart should use the original foreground text material");
        require(prompt->color_mapping_max[0U] == 255U && prompt->color_mapping_max[1U] == 255U && prompt->color_mapping_max[2U] == 255U,
                "TxtStart material should map glyph color to white");
        require(prompt->font_name == "MessageFont26kor.brfnt", "TxtStart should use the original Korean message font");
        require(prompt->font_width > 0.0F && prompt->font_height > 0.0F, "TxtStart font size should be positive");

        const std::array<std::uint16_t, 11U> expected_text{
            0xff21U,
            0xc640U,
            0x0042U,
            0xb97cU,
            0x0020U,
            0xb20cU,
            0xb7ecU,
            0x0020U,
            0xc8fcU,
            0xc138U,
            0xc694U,
        };
        require(prompt->text.size() == expected_text.size(), "TxtStart text length changed");
        require(std::ranges::equal(prompt->text, expected_text), "TxtStart UTF-16BE text changed");
    }

    void test_brfnt_message_font_decode(const std::filesystem::path &root) {
        const auto font_archive = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "Font.arc");
        const auto *font_entry = find_entry_by_basename(font_archive, "MessageFont26.brfnt");
        require(font_entry != nullptr, "Font.arc should contain MessageFont26.brfnt");

        const auto font = smgpc::game::parse_brfnt_font(font_archive.file_data(*font_entry));
        require(!font.sheets.empty(), "MessageFont26.brfnt should contain decoded glyph sheets");
        require(font.sheet_width > 0U && font.sheet_height > 0U, "MessageFont26.brfnt sheet dimensions should be positive");
        require(font.width > 0U && font.height > 0U, "MessageFont26.brfnt font dimensions should be positive");

        for (const auto code : std::array<std::uint16_t, 4U>{0xff21U, 0xc640U, 0x0042U, 0xb20cU}) {
            require(font.glyph_for(code).has_value(), "MessageFont26.brfnt should map prompt glyphs");
        }
        require(font.glyph_for(0xff21U)->x == font.glyph_for(0x0041U)->x, "fullwidth A should normalize to ASCII A in the BRFNT compatibility layer");
        require(font.glyph_for(0xff21U)->y == font.glyph_for(0x0041U)->y, "fullwidth A should normalize to ASCII A glyph row");

        require(!font.glyph_for_exact(0xff21U).has_value(), "MessageFont26.brfnt should not directly map fullwidth A");
        const auto icon_a = font.glyph_for_exact(0xe000U);
        const auto icon_b = font.glyph_for_exact(0xe00bU);
        require(icon_a.has_value() && icon_b.has_value(), "MessageFont26.brfnt should expose private-use A/B button icon glyphs");
        require(icon_a->sheet_index == 4U && icon_a->x == 190U && icon_a->y == 727U, "A button icon glyph location changed");
        require(icon_b->sheet_index == 4U && icon_b->x == 1U && icon_b->y == 793U, "B button icon glyph location changed");
    }

    void test_bcsv_file_select_camera_parse(const std::filesystem::path &root) {
        const auto file_select = smgpc::game::RarcArchive::from_file(root / "StageData" / "FileSelect.arc");
        const auto camera = smgpc::game::BcsvTable::from_bytes(file_select.file_data("camera/cameraparam.bcam"));

        require(camera.entry_count() == 8U, "FileSelect cameraparam entry count changed");
        require(camera.fields().size() == 32U, "FileSelect cameraparam field count changed");
        require(camera.entry_size() == 128U, "FileSelect cameraparam entry size changed");
        require(camera.field_index("camtype").has_value(), "FileSelect cameraparam should expose camtype field by JMap hash");

        require(camera.get_s32(5U, "version").has_value() && *camera.get_s32(5U, "version") == 196621, "FileSelect start camera version changed");
        require(camera.get_string(5U, "camtype").has_value() && *camera.get_string(5U, "camtype") == "CAM_TYPE_XZ_PARA",
                "FileSelect start camera type changed");
        require_near(*camera.get_float(5U, "angleA"), 1.57693F, 0.00001F, "FileSelect start camera angleA changed");
        require_near(*camera.get_float(5U, "angleB"), 0.473233F, 0.00001F, "FileSelect start camera angleB changed");
        require_near(*camera.get_float(5U, "dist"), 5000.0F, 0.001F, "FileSelect start camera distance changed");
        require_near(*camera.get_float(5U, "fovy"), 45.0F, 0.001F, "FileSelect start camera fovy changed");
        const auto start_world_offset = camera.get_vec3(5U, "woffset");
        require(start_world_offset.has_value(), "FileSelect start camera should expose woffset vector");
        require_near((*start_world_offset)[1U], 100.0F, 0.001F, "FileSelect start camera Y world offset changed");

        require(camera.get_string(6U, "camtype").has_value() && *camera.get_string(6U, "camtype") == "CAM_TYPE_FOLLOW",
                "FileSelect default camera type changed");
        require_near(*camera.get_float(6U, "angleA"), 0.174533F, 0.00001F, "FileSelect default camera angleA changed");
        require_near(*camera.get_float(6U, "angleB"), 0.349066F, 0.00001F, "FileSelect default camera angleB changed");
        require_near(*camera.get_float(6U, "loffset"), 100.0F, 0.001F, "FileSelect default camera local offset changed");
        const auto default_world_offset = camera.get_vec3(6U, "woffset");
        const auto default_axis = camera.get_vec3(6U, "axis");
        require(default_world_offset.has_value() && default_axis.has_value(), "FileSelect default camera should expose vector fields");
        require_near((*default_world_offset)[1U], 170.0F, 0.001F, "FileSelect default camera Y world offset changed");
        require_near((*default_axis)[0U], 1500.0F, 0.001F, "FileSelect default camera X axis changed");
        require_near((*default_axis)[1U], 1000.0F, 0.001F, "FileSelect default camera Y axis changed");

        require(camera.get_string(7U, "id").has_value() && *camera.get_string(7U, "id") == "s:03e7", "FileSelect fallback camera id changed");
    }

    void test_camera_param_file_select_chunk_load(const std::filesystem::path &root) {
        const auto file_select = smgpc::game::RarcArchive::from_file(root / "StageData" / "FileSelect.arc");
        const auto table = smgpc::game::BcsvTable::from_bytes(file_select.file_data("camera/cameraparam.bcam"));
        const auto chunks = smgpc::game::load_camera_param_chunks(table);

        require(chunks.size() == 8U, "FileSelect camera chunk count changed");

        const auto &start = chunks[5U];
        require(start.version == 196621U, "FileSelect start camera chunk version changed");
        require(start.camera_type == "CAM_TYPE_XZ_PARA", "FileSelect start camera chunk type changed");
        require_near(start.general.angle_a, 1.57693F, 0.00001F, "FileSelect start camera chunk angleA changed");
        require_near(start.general.angle_b, 0.473233F, 0.00001F, "FileSelect start camera chunk angleB changed");
        require_near(start.general.dist, 5000.0F, 0.001F, "FileSelect start camera chunk distance changed");
        require(start.general.num1 == 0, "FileSelect start camera chunk num1 changed");
        require_near(start.extra.fovy, 45.0F, 0.001F, "FileSelect start camera chunk fovy changed");
        require_near(start.extra.w_offset.y, 100.0F, 0.001F, "FileSelect start camera chunk world offset changed");
        require(start.extra.flags == 0U, "FileSelect start camera flags changed");
        require(!start.is_on_use_fovy(), "FileSelect start camera should preserve unset nofovy flag");
        require(start.game_thru == 0, "FileSelect start camera game thru flag changed");

        const auto &follow = chunks[6U];
        require(follow.camera_type == "CAM_TYPE_FOLLOW", "FileSelect default follow camera chunk type changed");
        require_near(follow.general.axis.x, 1500.0F, 0.001F, "FileSelect default follow camera axis X changed");
        require_near(follow.general.axis.y, 1000.0F, 0.001F, "FileSelect default follow camera axis Y changed");
        require_near(follow.general.dist, 0.15F, 0.001F, "FileSelect default follow camera dist changed");
        require(follow.general.num1 == 1, "FileSelect default follow camera num1 changed");
        require_near(follow.extra.l_offset, 100.0F, 0.001F, "FileSelect default follow camera local offset changed");
        require_near(follow.extra.w_offset.y, 170.0F, 0.001F, "FileSelect default follow camera world offset changed");

        const auto fallback = smgpc::game::find_camera_param_chunk(chunks, "s:03e7");
        require(fallback.has_value(), "FileSelect fallback camera should be findable by id");
        require(fallback->camera_type == "CAM_TYPE_FOLLOW", "FileSelect fallback camera chunk type changed");
        require_near(fallback->general.axis.x, 900.0F, 0.001F, "FileSelect fallback camera axis X changed");
        require_near(fallback->general.axis.y, 600.0F, 0.001F, "FileSelect fallback camera axis Y changed");
    }

    void test_file_select_title_camera_pose() {
        const auto pose = smgpc::game::file_select_title_camera_pose();
        require_near(pose.eye.x, 0.0F, 0.001F, "FileSelect title camera eye X changed");
        require_near(pose.eye.y, 15800.0F, 0.001F, "FileSelect title camera eye Y should include cFarTarget.Y + 15000 title offset");
        require_near(pose.eye.z, 15000.0F, 0.001F, "FileSelect title camera eye Z changed");
        require_near(pose.watch.x, 0.0F, 0.001F, "FileSelect title camera watch X changed");
        require_near(pose.watch.y, 15800.0F, 0.001F, "FileSelect title camera watch Y should include cFarTarget.Y + 15000 title offset");
        require_near(pose.watch.z, 0.0F, 0.001F, "FileSelect title camera watch Z changed");
        require_near(pose.up.y, 1.0F, 0.001F, "FileSelect title camera up vector changed");
        require_near(pose.fovy_degrees, 60.0F, 0.001F, "FileSelect title camera programmable FOV changed");
        require_near(pose.aspect_ratio, 608.0F / 456.0F, 0.001F, "FileSelect title camera should use original 4:3 CameraContext aspect");
        require_near(pose.near_clip, 100.0F, 0.001F, "FileSelect title camera should use original CameraContext near clip");
        require_near(pose.far_clip, 800000.0F, 0.001F, "FileSelect title camera should use original CameraContext far clip");

        const auto watch = smgpc::game::transform_world_to_camera(pose, pose.watch);
        require_near(watch.x, 0.0F, 0.001F, "FileSelect title watch point should land on camera center X");
        require_near(watch.y, 0.0F, 0.001F, "FileSelect title watch point should land on camera center Y");
        require_near(watch.z, 15000.0F, 0.001F, "FileSelect title watch point depth changed");

        const auto origin = smgpc::game::transform_world_to_camera(pose, {0.0F, 0.0F, 0.0F});
        require_near(origin.y, -15800.0F, 0.001F, "FileSelect title origin Y should match original raised title view");
        require_near(origin.z, 15000.0F, 0.001F, "FileSelect title origin depth changed");
    }

    void test_jmath_short_trig_compat() {
        require(smgpc::game::jmath_sincos_table_index_from_short(0xffffU) == 0x3fffU, "JMath short trig index should use high 14 bits");
        require(smgpc::game::jmath_fctiwz_to_u16(2607.9F) == 2607U, "JMath fctiwz helper should truncate positive values toward zero");
        require(smgpc::game::jmath_fctiwz_to_u16(-1.9F) == 0xffffU, "JMath fctiwz helper should preserve low 16 bits for negative values");
        require_near(smgpc::game::jmath_cos_short(0x0000U), 1.0F, 0.000001F, "JMath cosShort(0) changed");
        require_near(smgpc::game::jmath_sin_short(0x4000U), 1.0F, 0.000001F, "JMath sinShort(0x4000) changed");
        require_near(smgpc::game::jmath_cos_short(0x8000U), -1.0F, 0.000001F, "JMath cosShort(0x8000) changed");
    }

    void test_file_select_sky_runtime() {
        require_near(smgpc::game::file_select_sky_yaw(2300U), 2.3F, 0.000001F, "FileSelectSky yaw update rate changed");
        require_near(smgpc::game::file_select_sky_pitch(0U), 0.0F, 0.000001F, "FileSelectSky pitch frame 0 changed");
        require_near(smgpc::game::file_select_sky_pitch(100U), 0.000400535F, 0.000001F,
                     "FileSelectSky pitch frame 100 should use JMath cosShort conversion");
        require_near(smgpc::game::file_select_sky_pitch(500U), 0.010059165F, 0.000001F,
                     "FileSelectSky pitch frame 500 should use JMath cosShort conversion");
        require_near(smgpc::game::file_select_sky_pitch(1500U), 0.089677349F, 0.000001F,
                     "FileSelectSky pitch frame 1500 should use JMath cosShort conversion");
        require_near(smgpc::game::file_select_sky_pitch(2300U), 0.207162336F, 0.000001F,
                     "FileSelectSky pitch frame 2300 should use JMath cosShort conversion");
        require_near(smgpc::game::file_select_sky_pitch(3000U), 0.345056713F, 0.000001F,
                     "FileSelectSky pitch frame 3000 should use JMath cosShort conversion");

        const auto identity_scaled = smgpc::game::file_select_sky_actor_matrix(0U);
        require_near(identity_scaled.m[0U], 0.8F, 0.000001F, "FileSelectSky frame 0 matrix X scale changed");
        require_near(identity_scaled.m[5U], 0.8F, 0.000001F, "FileSelectSky frame 0 matrix Y scale changed");
        require_near(identity_scaled.m[10U], 0.8F, 0.000001F, "FileSelectSky frame 0 matrix Z scale changed");

        const auto matrix = smgpc::game::file_select_sky_actor_matrix(2300U);
        require_near(matrix.m[0U], -0.533020794F, 0.000001F, "FileSelectSky frame 2300 matrix[0] changed");
        require_near(matrix.m[2U], -0.596564233F, 0.000001F, "FileSelectSky frame 2300 matrix[2] changed");
        require_near(matrix.m[4U], 0.122554213F, 0.000001F, "FileSelectSky frame 2300 matrix[4] changed");
        require_near(matrix.m[5U], 0.782936871F, 0.000001F, "FileSelectSky frame 2300 matrix[5] changed");
        require_near(matrix.m[6U], -0.109500274F, 0.000001F, "FileSelectSky frame 2300 matrix[6] changed");
        require_near(matrix.m[8U], 0.583840132F, 0.000001F, "FileSelectSky frame 2300 matrix[8] changed");
        require_near(matrix.m[9U], -0.164346725F, 0.000001F, "FileSelectSky frame 2300 matrix[9] changed");
        require_near(matrix.m[10U], -0.521652043F, 0.000001F, "FileSelectSky frame 2300 matrix[10] changed");
    }

    void test_brlan_title_animation_parse(const std::filesystem::path &root) {
        const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
        const auto appear = smgpc::game::parse_brlan_animation(title_logo.file_data("anim/appear.brlan"));
        require(appear.frame_size == 201U, "TitleLogo appear frame size changed");
        require(!appear.loop, "TitleLogo appear should not loop");
        require(!appear.contents.empty(), "TitleLogo appear should contain animation content");

        const auto first_frame = appear.pane_frame("SMGTitleLogo", 0.0F);
        require(first_frame.translate_y.has_value() && *first_frame.translate_y == -26.0F, "TitleLogo appear should animate initial root Y");
        require(first_frame.scale_x.has_value() && *first_frame.scale_x == 0.0F, "TitleLogo appear should animate initial root X scale");
        require(first_frame.scale_y.has_value() && *first_frame.scale_y == 0.0F, "TitleLogo appear should animate initial root Y scale");

        const auto visible_frame = appear.pane_frame("SMGTitleLogo", 4.0F);
        require(visible_frame.scale_x.has_value() && *visible_frame.scale_x == 1.0F, "TitleLogo appear should animate root X scale to 1");
        require(visible_frame.scale_y.has_value() && *visible_frame.scale_y == 1.0F, "TitleLogo appear should animate root Y scale to 1");

        const auto wait = smgpc::game::parse_brlan_animation(title_logo.file_data("anim/wait.brlan"));
        require(wait.frame_size == 10000U, "TitleLogo wait frame size changed");
        require(wait.loop, "TitleLogo wait should loop");
        const auto galaxy_texture_middle = wait.texture_frame("PicLogoGalaxy", 5000.0F);
        require(galaxy_texture_middle.translate_s.has_value(), "TitleLogo wait should animate PicLogoGalaxy texture S translation");
        require_near(*galaxy_texture_middle.translate_s, 0.5F, 0.001F, "TitleLogo wait should scroll PicLogoGalaxy texture S at half-frame");
    }

    void test_brlan_press_start_animation_parse(const std::filesystem::path &root) {
        const auto press_start = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "PressStart.arc");
        const auto appear = smgpc::game::parse_brlan_animation(press_start.file_data("anim/appear.brlan"));
        require(appear.frame_size == 31U, "PressStart appear frame size changed");
        require(!appear.loop, "PressStart appear should not loop");
        const auto faded_in = appear.pane_frame("PressAB", 30.0F);
        require(faded_in.alpha.has_value() && *faded_in.alpha == 255.0F, "PressStart appear should fade prompt alpha to 255");

        const auto wait = smgpc::game::parse_brlan_animation(press_start.file_data("anim/wait.brlan"));
        require(wait.frame_size == 120U, "PressStart wait frame size changed");
        require(wait.loop, "PressStart wait should loop");
        const auto middle = wait.pane_frame("PressAB", 60.0F);
        require(middle.translate_y.has_value(), "PressStart wait should animate prompt Y");
        require_near(*middle.translate_y, 3.0F, 0.001F, "PressStart wait should bob prompt Y at mid-frame");
    }

    void test_j3d_comet_near_orbit_sky_textures(const std::filesystem::path &root) {
        const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
        const auto textures = smgpc::game::extract_j3d_textures(sky_archive.file_data("cometnearorbitsky.bdl"));
        require(textures.size() == 12U, "CometNearOrbitSky.bdl TEX1 texture count changed");

        const auto find_texture = [&textures](std::string_view name) -> const smgpc::game::J3dTexture * {
            const auto it = std::ranges::find_if(textures, [name](const auto &texture) { return texture.name == name; });
            return it == textures.end() ? nullptr : &*it;
        };

        const auto *orbit_universe = find_texture("OrbitUniverseL");
        require(orbit_universe != nullptr, "CometNearOrbitSky should contain OrbitUniverseL");
        require(orbit_universe->image.width == 1024U && orbit_universe->image.height == 512U, "OrbitUniverseL dimensions changed");
        require(orbit_universe->image.format == smgpc::game::TplTextureFormat::I4, "OrbitUniverseL should use GX I4");

        const auto *earth = find_texture("EarthKsMM");
        require(earth != nullptr, "CometNearOrbitSky should contain EarthKsMM");
        require(earth->image.width == 256U && earth->image.height == 256U, "EarthKsMM dimensions changed");

        const auto *galaxy = find_texture("Galaxy");
        require(galaxy != nullptr, "CometNearOrbitSky should contain Galaxy");
        require(galaxy->image.width == 64U && galaxy->image.height == 64U, "Galaxy dimensions changed");
        require(galaxy->image.format == smgpc::game::TplTextureFormat::CMPR, "Galaxy should exercise GX CMPR decoding");
        require(std::ranges::any_of(galaxy->image.rgba, [](std::uint8_t value) { return value != 0U; }),
                "CMPR decoded Galaxy texture should not be blank");
    }

    void test_gx_state_decodes_effective_mdl3_material_state() {
        const auto append_bp = [](std::vector<std::uint8_t> &bytes, std::uint8_t address, std::uint32_t value) {
            bytes.push_back(0x61U);
            bytes.push_back(address);
            bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
            bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
        };

        auto display_list = std::vector<std::uint8_t>{};
        append_bp(display_list, 0x00U, ((3U - 1U) << 10U) | (2U << 16U));
        append_bp(display_list, 0x06U, 100U | ((0x7ecU & 0x7ffU) << 11U) | (1U << 22U));
        append_bp(display_list, 0x07U, 30U | (40U << 11U) | (2U << 22U));
        append_bp(display_list, 0x08U, (0x7fbU & 0x7ffU) | (60U << 11U) | (1U << 22U));
        append_bp(display_list, 0x10U,
                  1U | (2U << 2U) | (3U << 4U) | (2U << 7U) | (1U << 9U) | (2U << 11U) | (4U << 13U) | (5U << 16U) |
                      (1U << 19U) | (1U << 20U));
        append_bp(display_list, 0x25U, 1U | (2U << 4U) | (3U << 8U) | (4U << 12U));
        append_bp(display_list, 0x27U, 2U | (3U << 3U) | (4U << 6U) | (5U << 9U));
        append_bp(display_list, 0x28U, 2U | (3U << 3U) | (1U << 6U) | (4U << 7U));
        append_bp(display_list, 0x40U, 1U | (4U << 1U));
        append_bp(display_list, 0xc0U, (4U << 12U) | (14U << 8U) | (8U << 4U) | (1U << 19U) | (2U << 22U));
        append_bp(display_list, 0xc1U, (5U << 13U) | (6U << 10U) | (7U << 7U) | (1U << 19U) | (2U << 22U));
        append_bp(display_list, 0xe2U, 100U | (0x7fbU << 12U));
        append_bp(display_list, 0xe3U, 30U | (40U << 12U));
        append_bp(display_list, 0xe8U, 400U | (1U << 10U));
        append_bp(display_list, 0xe9U, 0x100U | (0x200U << 12U));
        append_bp(display_list, 0xeeU, 127U << 11U);
        append_bp(display_list, 0xefU, 123456U);
        append_bp(display_list, 0xf0U, 5U);
        append_bp(display_list, 0xf1U, (126U << 11U) | (1U << 20U) | (2U << 21U));
        append_bp(display_list, 0xf2U, (17U << 16U) | (34U << 8U) | 51U);
        append_bp(display_list, 0xf3U, (2U << 16U) | 10U | (1U << 22U) | (5U << 19U) | (20U << 8U));
        append_bp(display_list, 0xf6U, (14U << 4U) | (28U << 9U));

        auto state = smgpc::game::GXMaterialState{};
        smgpc::game::gx_apply_mdl3_display_list(state, display_list);

        require(state.tev_stage_count == 3U, "MDL3 gen-mode BP load should update effective TEV stage count");
        require(state.indirect.stage_count == 2U, "MDL3 gen-mode BP load should update effective indirect stage count");
        require(state.indirect.texture_matrices.size() == 1U && state.indirect.texture_matrices[0U].matrix == 0U &&
                    state.indirect.texture_matrices[0U].ma == 100 && state.indirect.texture_matrices[0U].mb == -20 &&
                    state.indirect.texture_matrices[0U].mc == 30 && state.indirect.texture_matrices[0U].md == 40 &&
                    state.indirect.texture_matrices[0U].me == -5 && state.indirect.texture_matrices[0U].mf == 60 &&
                    state.indirect.texture_matrices[0U].scale == 25U,
                "MDL3 indirect matrix BP loads should preserve signed 2x3 matrix rows and scale bits");
        require(state.indirect.tev_stages.size() == 1U && state.indirect.tev_stages[0U].tev_stage == 0U &&
                    state.indirect.tev_stages[0U].ind_stage == 1U && state.indirect.tev_stages[0U].format == 2U &&
                    state.indirect.tev_stages[0U].bias == 3U && state.indirect.tev_stages[0U].bump_alpha == 2U &&
                    state.indirect.tev_stages[0U].matrix_index == 1U && state.indirect.tev_stages[0U].matrix_id == 2U &&
                    state.indirect.tev_stages[0U].wrap_s == 4U && state.indirect.tev_stages[0U].wrap_t == 5U &&
                    state.indirect.tev_stages[0U].use_original_lod && state.indirect.tev_stages[0U].add_previous &&
                    state.indirect.tev_stages[0U].active,
                "MDL3 indirect TEV BP loads should preserve stage command semantics");
        require(state.indirect.texture_coord_scales.size() == 2U && state.indirect.texture_coord_scales[0U].stage == 0U &&
                    state.indirect.texture_coord_scales[0U].scale_s == 1U && state.indirect.texture_coord_scales[0U].scale_t == 2U &&
                    state.indirect.texture_coord_scales[1U].stage == 1U && state.indirect.texture_coord_scales[1U].scale_s == 3U &&
                    state.indirect.texture_coord_scales[1U].scale_t == 4U,
                "MDL3 indirect texture scale BP loads should preserve paired stage scale exponents");
        require(state.indirect.texture_orders.size() == 4U && state.indirect.texture_orders[0U].stage == 0U &&
                    state.indirect.texture_orders[0U].tex_map == 2U && state.indirect.texture_orders[0U].tex_coord == 3U &&
                    state.indirect.texture_orders[1U].stage == 1U && state.indirect.texture_orders[1U].tex_map == 4U &&
                    state.indirect.texture_orders[1U].tex_coord == 5U,
                "MDL3 indirect texture order BP load should preserve indirect texture map and coordinate selectors");
        require(state.tev_orders.size() >= 2U && state.tev_orders[0U].tex_map == 2U && state.tev_orders[0U].tex_coord == 3U &&
                    state.tev_orders[0U].color_channel == 4U,
                "MDL3 TEV order BP load should update effective texture order state");
        require(state.z_mode.enabled && state.z_mode.compare_enable == 1U && state.z_mode.function == 4U && state.z_mode.update_enable == 0U,
                "MDL3 z-mode BP load should update effective depth state");
        require(state.tev_stages.size() >= 1U && state.tev_stages[0U].color_in == std::array<std::uint8_t, 4U>{4U, 14U, 8U, 0U} &&
                    state.tev_stages[0U].alpha_in == std::array<std::uint8_t, 4U>{5U, 6U, 7U, 0U} &&
                    state.tev_stages[0U].color_out == 2U && state.tev_stages[0U].alpha_out == 2U &&
                    state.tev_stages[0U].k_color_sel == 14U && state.tev_stages[0U].k_alpha_sel == 28U,
                "MDL3 TEV BP loads should update effective stage and konst selector state");
        require(state.tev_registers[1U] == smgpc::game::GXTevRegisterColor{100, 40, 30, -5},
                "MDL3 TEV color BP loads should update signed effective TEV registers");
        require(state.alpha_compare.enabled && state.alpha_compare.comp0 == 2U && state.alpha_compare.ref0 == 10U &&
                    state.alpha_compare.op == 1U && state.alpha_compare.comp1 == 5U && state.alpha_compare.ref1 == 20U,
                "MDL3 alpha-compare BP load should update effective pixel-engine state");
        require(state.fog.enabled && state.fog.type == 2U && state.fog.projection == 1U && state.fog.range_adjust_enabled &&
                    state.fog.range_center == 400U && state.fog.b_magnitude == 123456U && state.fog.b_shift == 5U &&
                    state.fog.color == std::array<std::uint8_t, 4U>{17U, 34U, 51U, 255U},
                "MDL3 fog BP loads should preserve typed fog selector, projection, range, B, and color state");
        require_near(state.fog.a, 1.0F, 0.0001F, "MDL3 fog A parameter should decode from GX float bits");
        require_near(state.fog.c, 0.5F, 0.0001F, "MDL3 fog C parameter should decode from GX float bits");
        require_near(state.fog.range_k[0U], 1.0F, 0.0001F, "MDL3 fog range K low sample should decode using Dolphin scale");
        require_near(state.fog.range_k[1U], 2.0F, 0.0001F, "MDL3 fog range K high sample should decode using Dolphin scale");
    }

    void test_j3d_comet_near_orbit_sky_model_probe(const std::filesystem::path &root) {
        const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
        const auto model = smgpc::game::inspect_j3d_model(sky_archive.file_data("cometnearorbitsky.bdl"));

        require(model.section_count == 9U, "CometNearOrbitSky.bdl section count changed");
        require(model.info.has_value(), "CometNearOrbitSky.bdl should expose INF1");
        require(model.vertices.has_value(), "CometNearOrbitSky.bdl should expose VTX1");
        require(model.joints.has_value(), "CometNearOrbitSky.bdl should expose JNT1");
        require(model.envelopes.has_value(), "CometNearOrbitSky.bdl should expose EVP1 envelope metadata");
        require(model.draw_matrices.has_value(), "CometNearOrbitSky.bdl should expose DRW1 draw matrix metadata");
        require(model.shapes.has_value(), "CometNearOrbitSky.bdl should expose SHP1");
        require(model.materials.has_value(), "CometNearOrbitSky.bdl should expose MAT3");
        require(model.mdl3.has_value(), "CometNearOrbitSky.bdl should expose MDL3 material display-list metadata");
        require(model.textures.size() == 12U, "CometNearOrbitSky.bdl should expose TEX1 textures through model probe");

        require(model.info->packet_count == 9U, "CometNearOrbitSky packet count changed");
        require(model.info->vertex_count == 1029U, "CometNearOrbitSky vertex count changed");
        require(model.info->hierarchy.size() == 71U, "CometNearOrbitSky hierarchy size changed");
        require(model.vertices->formats.size() == 4U, "CometNearOrbitSky VTX1 format count changed");
        require(model.joints->joint_count == 8U, "CometNearOrbitSky JNT1 joint count changed");
        require(model.joints->joints.size() == 8U, "CometNearOrbitSky JNT1 joints should be decoded");
        require(model.shapes->shape_count == 9U, "CometNearOrbitSky shape count changed");
        require(model.materials->material_count == 9U, "CometNearOrbitSky material count changed");
        require(model.joints->joints[0U].name == "world_root", "CometNearOrbitSky root joint name changed");
        require(model.joints->joints[7U].name == "Obit", "CometNearOrbitSky orbit joint name changed");
        require(model.joints->parent_indices.size() == 8U, "CometNearOrbitSky joint hierarchy parent map should be decoded");
        require(model.joints->parent_indices[0U] == 0xffffU && model.joints->parent_indices[1U] == 0U && model.joints->parent_indices[2U] == 1U &&
                    model.joints->parent_indices[3U] == 2U && model.joints->parent_indices[7U] == 0U,
                "CometNearOrbitSky joint hierarchy parent map changed");
        require(model.envelopes->matrix_count == 0U, "CometNearOrbitSky EVP1 should be present but have no weighted envelopes");
        require(model.draw_matrices->matrices.size() == model.draw_matrices->matrix_count,
                "CometNearOrbitSky DRW1 draw matrix table should parse to the declared entry count");
        require_near(model.joints->joints[7U].radius, 793869.0F, 0.5F, "CometNearOrbitSky orbit joint radius changed");
        require(model.mdl3->material_count == 9U && model.mdl3->packets.size() == 9U, "CometNearOrbitSky MDL3 packet count changed");
        require(std::ranges::all_of(model.mdl3->packets, [](const auto &packet) { return packet.size > 0U && !packet.bytes.empty(); }),
                "CometNearOrbitSky MDL3 packets should preserve raw GX display-list bytes");

        const auto find_material = [&model](std::string_view name) -> const smgpc::game::J3dMaterialSummary * {
            const auto it = std::ranges::find_if(model.materials->materials, [name](const auto &material) { return material.name == name; });
            return it == model.materials->materials.end() ? nullptr : &*it;
        };

        const auto *space = find_material("Space_Mat_v");
        require(space != nullptr, "CometNearOrbitSky should expose Space_Mat_v");
        require(space->gx_state.source == "J3D", "Space_Mat_v should populate shared GX state as a J3D material");
        require(space->gx_state.textures.size() == space->textures.size(), "Space_Mat_v GX state should preserve texture bindings");
        require(space->gx_state.tev_orders.size() >= space->tev_orders.size(), "Space_Mat_v GX state should preserve TEV orders");
        require(space->gx_state.tev_stages.size() >= space->tev_stages.size(), "Space_Mat_v GX state should preserve TEV stages");
        require(std::ranges::any_of(space->gx_state.mdl3_register_loads, [](const auto &load) {
                    return load.space == smgpc::game::GXRegisterSpace::BP && load.address >= 0xe0U && load.address <= 0xe7U;
                }),
                "Space_Mat_v GX state should retain and apply MDL3 TEV register BP loads");
        require(!space->gx_state.mdl3_display_list.empty(), "Space_Mat_v GX state should preserve its MDL3 material packet");
        require(space->gx_state.mdl3_stats.bp_load_count > 0U && space->gx_state.mdl3_stats.xf_load_count > 0U,
                "Space_Mat_v GX state should decode BP/XF loads from its MDL3 material packet");
        require(space->gx_state.mdl3_stats.parsed_bytes == space->gx_state.mdl3_display_list.size(),
                "Space_Mat_v GX state should parse the full MDL3 material packet");
        require(space->gx_state.mdl3_stats.unknown_opcode_count == 0U, "Space_Mat_v GX state should recognize all MDL3 material-packet opcodes");
        require(std::ranges::any_of(space->gx_state.mdl3_register_loads, [](const auto &load) {
                    return load.space == smgpc::game::GXRegisterSpace::BP && load.address == 0x00U;
                }),
                "Space_Mat_v GX state should retain MDL3 gen-mode BP loads");
        require(space->textures.size() == 3U, "Space_Mat_v should bind the original three textures");
        require(space->textures[0U].texture_index == 5U, "Space_Mat_v first texture should be OrbitUniverseL");
        require(space->textures[1U].texture_index == 6U, "Space_Mat_v second texture should be Galaxy");
        require(space->textures[2U].texture_index == 7U, "Space_Mat_v third texture should be GalaxyRiverK");
        require(space->tex_coord_gens.size() == 3U, "Space_Mat_v should expose the original three texture coordinate generators");
        require(space->tex_coord_gens[0U].matrix == 30U && space->tex_coord_gens[1U].matrix == 33U && space->tex_coord_gens[2U].matrix == 36U,
                "Space_Mat_v texture coordinate generators should preserve GX texture matrix slots");
        require(space->tex_matrices.size() == 3U, "Space_Mat_v should expose the original three texture matrices");
        require_near(space->tex_matrices[0U].scale_s, 0.5F, 0.001F, "Space_Mat_v matrix 0 S scale changed");
        require_near(space->tex_matrices[1U].scale_t, 0.288086F, 0.001F, "Space_Mat_v matrix 1 T scale changed");
        require(space->tev_orders.size() == 3U, "Space_Mat_v should expose the original three TEV orders");
        require(space->tev_orders[0U].tex_coord == 1U && space->tev_orders[0U].tex_map == 1U,
                "Space_Mat_v stage 0 should sample Galaxy through texture coordinate slot 1");
        require(space->tev_orders[1U].tex_coord == 0U && space->tev_orders[1U].tex_map == 0U,
                "Space_Mat_v stage 1 should sample OrbitUniverseL through texture coordinate slot 0");
        require(space->tev_orders[2U].tex_coord == 2U && space->tev_orders[2U].tex_map == 2U,
                "Space_Mat_v stage 2 should sample GalaxyRiverK through texture coordinate slot 2");
        require(space->tev_stages.size() == 3U, "Space_Mat_v should expose the original three raw TEV stages");
        require_tev_stage(space->tev_stages[0U], {15U, 8U, 10U, 14U}, 12U, {7U, 4U, 5U, 7U}, 1U, 28U,
                          "Space_Mat_v TEV stage 0 semantic decode changed");
        require_tev_stage(space->tev_stages[1U], {15U, 10U, 8U, 0U}, 12U, {5U, 7U, 7U, 7U}, 0U, 28U,
                          "Space_Mat_v TEV stage 1 semantic decode changed");
        require(space->alpha_compare.enabled, "Space_Mat_v should preserve alpha compare state");
        require(space->blend.enabled && space->blend.type == 0U && space->blend.src_factor == 1U && space->blend.dst_factor == 0U,
                "Space_Mat_v should preserve original no-blend state");
        require(space->cull_mode == 0U, "Space_Mat_v should preserve original GX_CULL_NONE state");
        require(space->gx_state.cull_mode == 1U, "Space_Mat_v GX state should preserve MDL3 hardware back-face cull state");
        require(space->z_mode.enabled && space->z_mode.compare_enable == 1U && space->z_mode.function == 3U && space->z_mode.update_enable == 0U,
                "Space_Mat_v should preserve original test-only GX_LEQUAL Z mode");
        require(!space->gx_state.fog.enabled && space->gx_state.fog.type == 0U &&
                    space->gx_state.fog.color == std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U},
                "Space_Mat_v MDL3 fog registers should decode to original GX_FOG_NONE state instead of a raw-load marker");
        const auto space_passes = smgpc::game::j3d_material_texture_passes(*space);
        require(space_passes.size() == 3U, "Space_Mat_v should build three runtime texture passes from TEV order");
        require(space_passes[0U].texture_index == 6U && space_passes[0U].tex_coord_slot == 1U,
                "Space_Mat_v pass 0 should sample Galaxy through tex coord 1");
        require(space_passes[1U].texture_index == 5U && space_passes[1U].tex_coord_slot == 0U,
                "Space_Mat_v pass 1 should sample OrbitUniverseL through tex coord 0");
        require(space_passes[2U].texture_index == 7U && space_passes[2U].tex_coord_slot == 2U,
                "Space_Mat_v pass 2 should sample GalaxyRiverK through tex coord 2");
        require(space_passes[0U].tex_matrix.has_value() && space_passes[0U].tex_matrix->slot == 1U,
                "Space_Mat_v pass 0 should resolve GX_TEXMTX1-compatible slot 1");
        const auto composed_space = smgpc::game::j3d_try_compose_material_texture(*space, model.textures, space_passes, space->material_colors[0U]);
        require(composed_space.has_value(), "Space_Mat_v should compose from its original material texture passes");
        require(composed_space->raster_color_baked, "Space_Mat_v pass composition should bake raster color into the texture");
        require(composed_space->image.width == 1024U && composed_space->image.height == 512U,
                "Space_Mat_v pass composition should use the largest source texture dimensions");
        const auto representative_space_pass = smgpc::game::j3d_representative_texture_pass(*space);
        require(representative_space_pass.has_value(), "Space_Mat_v should expose a representative runtime texture pass");
        require(representative_space_pass->texture_index == 5U && representative_space_pass->tex_map_slot == 0U,
                "Space_Mat_v representative pass should use the original base starfield texture map");
        require(representative_space_pass->tex_matrix.has_value() && representative_space_pass->tex_matrix->slot == 0U,
                "Space_Mat_v representative pass should resolve its base texture matrix");
        const auto transformed_space_coord = smgpc::game::j3d_transform_tex_coord(smgpc::game::J3dMeshVertex{.u = 0.75F, .v = 0.25F},
                                                                                  &space->tex_coord_gens[0U], &space->tex_matrices[0U]);
        require_near(transformed_space_coord.u, 0.625F, 0.001F, "J3D texture matrix transform should apply centered S scale");
        require_near(transformed_space_coord.v, 0.25F, 0.001F, "J3D texture matrix transform should preserve V without SRT changes");

        const auto *core_rock = find_material("CoreRock");
        require(core_rock != nullptr, "CometNearOrbitSky should expose CoreRock");
        require(smgpc::game::j3d_material_texture_passes(*core_rock).empty(), "CoreRock should be an untextured material");
        const auto composed_core_rock = smgpc::game::j3d_try_compose_material_constant(*core_rock, core_rock->material_colors[0U]);
        require(composed_core_rock.has_value(), "CoreRock should compose to a constant material texture");
        require(composed_core_rock->raster_color_baked, "CoreRock constant material composition should bake raster color");
        require(composed_core_rock->image.width == 1U && composed_core_rock->image.height == 1U,
                "CoreRock constant material composition should produce one RGBA texel");

        const auto *comet_halo = find_material("CometHalo_v");
        require(comet_halo != nullptr, "CometNearOrbitSky should expose CometHalo_v");
        require(comet_halo->gx_state.indirect.stage_count == 1U, "CometHalo_v MDL3 GX state should preserve declared indirect stage count");
        require(!comet_halo->gx_state.indirect.texture_orders.empty(), "CometHalo_v MDL3 GX state should preserve indirect texture order registers");
        require(std::ranges::any_of(comet_halo->gx_state.indirect.tev_stages, [](const auto &stage) {
                    return stage.active && stage.tev_stage == 0U && stage.ind_stage == 0U;
                }),
                "CometHalo_v MDL3 GX state should preserve its active indirect TEV command");
        require(std::ranges::any_of(comet_halo->gx_state.indirect.texture_orders, [](const auto &order) {
                    return order.stage == 0U && order.tex_map == 1U && order.tex_coord == 1U;
                }),
                "CometHalo_v indirect stage 0 should sample the original texture-map and texcoord slots");
        const auto comet_halo_passes = smgpc::game::j3d_material_texture_passes(*comet_halo);
        require(comet_halo_passes.size() == 1U && comet_halo_passes[0U].texture_index == 2U && comet_halo_passes[0U].stage == 0U,
                "CometHalo_v regular TEV order should still expose one base texture pass");
        const auto indirect_source = smgpc::game::J3dMeshVertex{
            .u = 3.0F / 8.0F,
            .v = 5.0F / 8.0F,
        };
        const auto indirect_trace =
            smgpc::game::j3d_trace_indirect_texture_transform(*comet_halo, model.textures, indirect_source, comet_halo_passes[0U]);
        require(indirect_trace.has_value(), "CometHalo_v should expose a traceable active indirect texture transform");
        require(indirect_trace->tev_stage == 0U && indirect_trace->indirect_stage == 0U && indirect_trace->indirect_tex_map == 1U &&
                    indirect_trace->indirect_tex_coord == 1U,
                "CometHalo_v indirect trace should preserve Dolphin/GX stage and indirect order selectors");
        const auto base_coord = smgpc::game::j3d_transform_tex_coord(
            indirect_source, comet_halo_passes[0U].tex_coord_gen.has_value() ? &*comet_halo_passes[0U].tex_coord_gen : nullptr,
            comet_halo_passes[0U].tex_matrix.has_value() ? &*comet_halo_passes[0U].tex_matrix : nullptr);
        require_near(indirect_trace->base_coord.u, base_coord.u, 0.00001F,
                     "CometHalo_v indirect trace should use the regular pass texture coordinate as its base S coordinate");
        require_near(indirect_trace->base_coord.v, base_coord.v, 0.00001F,
                     "CometHalo_v indirect trace should use the regular pass texture coordinate as its base T coordinate");
        const auto indirect_binding = std::ranges::find_if(comet_halo->textures, [&indirect_trace](const auto &binding) {
            return binding.slot == indirect_trace->indirect_tex_map;
        });
        require(indirect_binding != comet_halo->textures.end() && indirect_binding->texture_index < model.textures.size(),
                "CometHalo_v indirect trace should reference a valid indirect texture binding");
        const auto &base_texture = model.textures[comet_halo_passes[0U].texture_index].image;
        const auto &indirect_texture = model.textures[indirect_binding->texture_index].image;
        const auto shift_indirect_value = [](std::int64_t value, int shift) {
            if (shift >= 0) {
                return value / (std::int64_t{1} << std::min(shift, 30));
            }

            return value * (std::int64_t{1} << std::min(-shift, 30));
        };
        const auto expected_base_ind_s = static_cast< std::int64_t >(
            std::llround(indirect_trace->indirect_coord.u * static_cast< float >(indirect_texture.width) * 128.0F));
        const auto expected_base_ind_t = static_cast< std::int64_t >(
            std::llround(indirect_trace->indirect_coord.v * static_cast< float >(indirect_texture.height) * 128.0F));
        require(indirect_trace->base_indirect_s == expected_base_ind_s && indirect_trace->base_indirect_t == expected_base_ind_t,
                "CometHalo_v indirect trace should preserve Dolphin's 1/128 indirect texture coordinate basis");
        const auto scale = std::ranges::find_if(comet_halo->gx_state.indirect.texture_coord_scales, [&indirect_trace](const auto &entry) {
            return entry.stage == indirect_trace->indirect_stage;
        });
        const auto scale_s = scale == comet_halo->gx_state.indirect.texture_coord_scales.end() ? 0 : scale->scale_s;
        const auto scale_t = scale == comet_halo->gx_state.indirect.texture_coord_scales.end() ? 0 : scale->scale_t;
        require(indirect_trace->scaled_indirect_s == shift_indirect_value(indirect_trace->base_indirect_s, scale_s) &&
                    indirect_trace->scaled_indirect_t == shift_indirect_value(indirect_trace->base_indirect_t, scale_t),
                "CometHalo_v indirect trace should apply GXSetIndTexCoordScale exponents before sampling");
        constexpr auto format_shifts = std::array< std::uint8_t, 4U >{0U, 3U, 4U, 5U};
        const auto format_shift = format_shifts[std::min< std::size_t >(indirect_trace->format, format_shifts.size() - 1U)];
        const auto bias_value = indirect_trace->format == 0U ? -128 : 1;
        const auto expected_biased = std::array< std::int32_t, 3U >{
            static_cast< std::int32_t >((indirect_trace->sampled_indirect_color[3U] >> format_shift) +
                                        ((indirect_trace->bias & 0x1U) != 0U ? bias_value : 0)),
            static_cast< std::int32_t >((indirect_trace->sampled_indirect_color[2U] >> format_shift) +
                                        ((indirect_trace->bias & 0x2U) != 0U ? bias_value : 0)),
            static_cast< std::int32_t >((indirect_trace->sampled_indirect_color[1U] >> format_shift) +
                                        ((indirect_trace->bias & 0x4U) != 0U ? bias_value : 0)),
        };
        require(indirect_trace->biased_indirect_coord == expected_biased,
                "CometHalo_v indirect trace should apply Dolphin's ALP/BLU/GRN format and bias decode");
        const auto matrix = std::ranges::find_if(comet_halo->gx_state.indirect.texture_matrices, [&indirect_trace](const auto &entry) {
            return indirect_trace->matrix_index != 0U && entry.matrix == indirect_trace->matrix_index - 1U;
        });
        auto expected_translation = std::array< std::int64_t, 2U >{0, 0};
        if (matrix != comet_halo->gx_state.indirect.texture_matrices.end()) {
            switch (indirect_trace->matrix_id) {
            case 0U:
                expected_translation[0U] = (static_cast< std::int64_t >(matrix->ma) * indirect_trace->biased_indirect_coord[0U] +
                                            static_cast< std::int64_t >(matrix->mc) * indirect_trace->biased_indirect_coord[1U] +
                                            static_cast< std::int64_t >(matrix->me) * indirect_trace->biased_indirect_coord[2U]) /
                                           8;
                expected_translation[1U] = (static_cast< std::int64_t >(matrix->mb) * indirect_trace->biased_indirect_coord[0U] +
                                            static_cast< std::int64_t >(matrix->md) * indirect_trace->biased_indirect_coord[1U] +
                                            static_cast< std::int64_t >(matrix->mf) * indirect_trace->biased_indirect_coord[2U]) /
                                           8;
                break;
            case 1U:
                expected_translation[0U] = (indirect_trace->base_s * indirect_trace->biased_indirect_coord[0U]) / 256;
                expected_translation[1U] = (indirect_trace->base_t * indirect_trace->biased_indirect_coord[0U]) / 256;
                break;
            case 2U:
                expected_translation[0U] = (indirect_trace->base_s * indirect_trace->biased_indirect_coord[1U]) / 256;
                expected_translation[1U] = (indirect_trace->base_t * indirect_trace->biased_indirect_coord[1U]) / 256;
                break;
            default:
                break;
            }
            const auto matrix_shift = 17 - static_cast< int >(matrix->scale);
            expected_translation[0U] = shift_indirect_value(expected_translation[0U], matrix_shift);
            expected_translation[1U] = shift_indirect_value(expected_translation[1U], matrix_shift);
        }
        require(indirect_trace->translation == expected_translation,
                "CometHalo_v indirect trace should match Dolphin's indirect matrix translation math");
        const auto wrap_indirect_coordinate = [](std::int64_t coord, std::uint8_t wrap) {
            switch (wrap) {
            case 0U:
                return coord;
            case 1U:
                return coord & ((std::int64_t{256} << 7U) - 1);
            case 2U:
                return coord & ((std::int64_t{128} << 7U) - 1);
            case 3U:
                return coord & ((std::int64_t{64} << 7U) - 1);
            case 4U:
                return coord & ((std::int64_t{32} << 7U) - 1);
            case 5U:
                return coord & ((std::int64_t{16} << 7U) - 1);
            default:
                return std::int64_t{0};
            }
        };
        auto expected_transformed_s = wrap_indirect_coordinate(indirect_trace->base_s, indirect_trace->wrap_s) + indirect_trace->translation[0U];
        auto expected_transformed_t = wrap_indirect_coordinate(indirect_trace->base_t, indirect_trace->wrap_t) + indirect_trace->translation[1U];
        if (indirect_trace->add_previous) {
            expected_transformed_s += indirect_trace->base_s;
            expected_transformed_t += indirect_trace->base_t;
        }
        require(indirect_trace->transformed_s == expected_transformed_s && indirect_trace->transformed_t == expected_transformed_t,
                "CometHalo_v indirect trace should apply wrap and add-previous exactly like Dolphin software TEV");
        require_near(indirect_trace->transformed_coord.u,
                     static_cast< float >(expected_transformed_s) / (static_cast< float >(base_texture.width) * 128.0F), 0.00001F,
                     "CometHalo_v indirect trace should expose the transformed source S coordinate");
        require_near(indirect_trace->transformed_coord.v,
                     static_cast< float >(expected_transformed_t) / (static_cast< float >(base_texture.height) * 128.0F), 0.00001F,
                     "CometHalo_v indirect trace should expose the transformed source T coordinate");
        auto comet_halo_without_indirect = *comet_halo;
        comet_halo_without_indirect.gx_state.indirect.stage_count = 0U;
        comet_halo_without_indirect.gx_state.indirect.tev_stages.clear();
        auto indirect_changes_sample = false;
        for (auto y = 1U; y < 8U && !indirect_changes_sample; ++y) {
            for (auto x = 1U; x < 8U; ++x) {
                const auto source = smgpc::game::J3dMeshVertex{
                    .u = static_cast<float>(x) / 8.0F,
                    .v = static_cast<float>(y) / 8.0F,
                };
                const auto with_indirect = smgpc::game::j3d_evaluate_material_color(*comet_halo, model.textures, comet_halo_passes, source,
                                                                                    comet_halo->material_colors[0U]);
                const auto without_indirect =
                    smgpc::game::j3d_evaluate_material_color(comet_halo_without_indirect, model.textures, comet_halo_passes, source,
                                                             comet_halo->material_colors[0U]);
                require(with_indirect.has_value() && without_indirect.has_value(),
                        "CometHalo_v material evaluator should sample both regular and indirect textures");
                if (*with_indirect != *without_indirect) {
                    indirect_changes_sample = true;
                    break;
                }
            }
        }
        require(indirect_changes_sample, "CometHalo_v material evaluation should apply decoded indirect texture coordinates");

        const auto *earth_far = find_material("EarthFar_v");
        require(earth_far != nullptr, "CometNearOrbitSky should expose EarthFar_v");
        require(earth_far->textures.size() == 3U, "EarthFar_v should bind the original earth/cloud texture stack");
        require(earth_far->tex_coord_gens.size() == 3U, "EarthFar_v should expose the original three texture coordinate generators");
        require(earth_far->tex_coord_gens[0U].type == 0U && earth_far->tex_coord_gens[0U].source == 0U,
                "EarthFar_v base texture coordinate generator should be GX_TG_MTX3x4 from position");
        require(earth_far->tex_matrices.size() == 3U, "EarthFar_v should expose the original three texture matrices");
        require_near(earth_far->tex_matrices[1U].translate_s, 0.332031F, 0.001F, "EarthFar_v far texture S translation changed");
        require_near(earth_far->tex_matrices[2U].scale_s, 0.1F, 0.001F, "EarthFar_v cloud matrix S scale changed");
        require_near(earth_far->tex_matrices[2U].scale_t, 0.5F, 0.001F, "EarthFar_v cloud matrix T scale changed");
        const auto projected_earth_coord =
            smgpc::game::j3d_transform_tex_coord(smgpc::game::J3dMeshVertex{.x = 10.0F, .y = 20.0F, .z = 40.0F, .u = 0.75F, .v = 0.25F},
                                                 &earth_far->tex_coord_gens[0U], &earth_far->tex_matrices[0U]);
        require_near(projected_earth_coord.u, 0.49848F, 0.001F,
                     "J3D GX_TG_POS texture generation should apply projected texture matrix before Q divide");
        require_near(projected_earth_coord.v, 0.50253F, 0.001F,
                     "J3D GX_TG_POS texture generation should apply projected texture matrix before Q divide");
        const auto scaled_actor_matrix = smgpc::game::J3dMatrix3x4{
            .m =
                {
                    0.8F,
                    0.0F,
                    0.0F,
                    0.0F,
                    0.0F,
                    0.8F,
                    0.0F,
                    0.0F,
                    0.0F,
                    0.0F,
                    0.8F,
                    0.0F,
                },
        };
        const auto projected_scaled_earth_coord =
            smgpc::game::j3d_transform_tex_coord(smgpc::game::J3dMeshVertex{.x = 10.0F, .y = 20.0F, .z = 40.0F, .u = 0.75F, .v = 0.25F},
                                                 &earth_far->tex_coord_gens[0U], &earth_far->tex_matrices[0U], &scaled_actor_matrix);
        require_near(projected_scaled_earth_coord.u, 0.49879F, 0.001F,
                     "J3D projected texture generation should include the actor/model matrix passed to J3DTexMtx::calc");
        require_near(projected_scaled_earth_coord.v, 0.50202F, 0.001F,
                     "J3D projected texture generation should include the actor/model matrix passed to J3DTexMtx::calc");
        require(earth_far->tev_orders.size() == 3U, "EarthFar_v should expose the original three TEV orders");
        require(earth_far->tev_stages.size() == 3U, "EarthFar_v should expose the original three raw TEV stages");
        require_tev_stage(earth_far->tev_stages[0U], {15U, 10U, 8U, 15U}, 12U, {7U, 4U, 5U, 7U}, 1U, 28U,
                          "EarthFar_v TEV stage 0 semantic decode changed");
        require_tev_stage(earth_far->tev_stages[1U], {4U, 14U, 8U, 0U}, 13U, {5U, 7U, 7U, 7U}, 0U, 28U,
                          "EarthFar_v TEV stage 1 semantic decode changed");
        require_tev_stage(earth_far->tev_stages[2U], {15U, 10U, 8U, 0U}, 14U, {5U, 7U, 7U, 7U}, 0U, 28U,
                          "EarthFar_v TEV stage 2 semantic decode changed");
        require(earth_far->blend.enabled && earth_far->blend.type == 0U && earth_far->blend.src_factor == 1U && earth_far->blend.dst_factor == 0U,
                "EarthFar_v should preserve original no-blend state");
        require(earth_far->cull_mode == 0U, "EarthFar_v should preserve original GX_CULL_NONE state");
        require(earth_far->z_mode.enabled && earth_far->z_mode.compare_enable == 1U && earth_far->z_mode.function == 3U &&
                    earth_far->z_mode.update_enable == 1U,
                "EarthFar_v should preserve original GX_LEQUAL write-enabled Z mode");
        const auto earth_passes = smgpc::game::j3d_material_texture_passes(*earth_far);
        require(earth_passes.size() == 3U, "EarthFar_v should build three runtime texture passes from TEV order");
        require(earth_passes[0U].texture_index == 9U && earth_passes[1U].texture_index == 11U && earth_passes[2U].texture_index == 10U,
                "EarthFar_v runtime passes should preserve earth/far/cloud texture order");
        require(!smgpc::game::j3d_try_compose_material_texture(*earth_far, model.textures, earth_passes, earth_far->material_colors[0U]).has_value(),
                "EarthFar_v should not compose because it uses position/projected texture generation");
        const auto representative_earth_pass = smgpc::game::j3d_representative_texture_pass(*earth_far);
        require(representative_earth_pass.has_value() && representative_earth_pass->texture_index == 9U,
                "EarthFar_v representative pass should use the original base earth texture map");
        const auto evaluated_earth_color = smgpc::game::j3d_evaluate_material_color(
            *earth_far, model.textures, earth_passes, smgpc::game::J3dMeshVertex{.x = 10.0F, .y = 20.0F, .z = 40.0F, .u = 0.75F, .v = 0.25F},
            earth_far->material_colors[0U]);
        require(evaluated_earth_color.has_value(), "EarthFar_v projected/POS material should evaluate through the shared J3D runtime");
        require(std::ranges::any_of(*evaluated_earth_color, [](std::uint8_t channel) { return channel != 0U; }),
                "EarthFar_v projected/POS material evaluation should produce a non-empty RGBA result");

        const auto *sun = find_material("Sun_Mat_v");
        require(sun != nullptr, "CometNearOrbitSky should expose Sun_Mat_v");
        require(sun->textures.size() == 1U && sun->textures[0U].texture_index == 4U, "Sun_Mat_v should bind PlanetSun");
        require(sun->tev_stages.size() == 1U, "Sun_Mat_v should expose its original single TEV stage");
        require_tev_stage(sun->tev_stages[0U], {15U, 8U, 10U, 15U}, 12U, {7U, 4U, 5U, 7U}, 1U, 28U, "Sun_Mat_v TEV stage 0 semantic decode changed");
        require(sun->blend.enabled && sun->blend.type == 1U && sun->blend.src_factor == 4U && sun->blend.dst_factor == 1U,
                "Sun_Mat_v should preserve original additive blend state");

        const auto &space_shape = model.shapes->shapes.at(7U);
        require(space_shape.material_index == 7U, "CometNearOrbitSky shape 7 should use Space_Mat_v");
        require(space_shape.joint_index == 7U, "CometNearOrbitSky Space_Mat_v shape should be attached to Obit joint");
        require(space_shape.draw_order == 7U, "CometNearOrbitSky Space_Mat_v should keep INF1 draw order");
        require(space_shape.matrix_groups.size() == 1U && space_shape.matrix_groups[0U].use_matrix_index != 0xffffU,
                "CometNearOrbitSky Space_Mat_v should preserve its SHP1 matrix group draw matrix index");
        require(space_shape.matrix_groups[0U].display_list_size == space_shape.display_list_bytes,
                "CometNearOrbitSky Space_Mat_v matrix group should own the shape display packet");
        require(space_shape.display_list_bytes == 3232U, "Space_Mat_v shape display list size changed");
        require(space_shape.parsed_display_list_bytes == space_shape.display_list_bytes, "Space_Mat_v shape display list should parse fully");
        require(space_shape.triangle_count == 480U, "Space_Mat_v triangle count changed");

        const auto &sky_shape = model.shapes->shapes.at(8U);
        require(sky_shape.material_index == 6U, "CometNearOrbitSky shape 8 should use Sky_Mat_v");
        require(sky_shape.draw_order == 6U, "CometNearOrbitSky Sky_Mat_v should draw before Space_Mat_v per INF1");

        const auto &sun_shape = model.shapes->shapes.at(6U);
        require(sun_shape.material_index == 8U, "CometNearOrbitSky shape 6 should use Sun_Mat_v");
        require(sun_shape.joint_index == 7U, "CometNearOrbitSky Sun_Mat_v shape should be attached to Obit joint");
        require(sun_shape.draw_order == 8U, "CometNearOrbitSky Sun_Mat_v should keep INF1 draw order");
        require(sun_shape.triangle_count == 16U, "Sun_Mat_v triangle count changed");
    }

    void test_j3d_comet_near_orbit_sky_animation_probe(const std::filesystem::path &root) {
        const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
        const auto bck = smgpc::game::inspect_j3d_animation(sky_archive.file_data("cometnearorbitsky.bck"));
        require(bck.type == "bck1", "CometNearOrbitSky BCK file type changed");
        require(bck.sections.size() == 1U && bck.sections[0U].tag == "ANK1", "CometNearOrbitSky BCK should contain one ANK1 section");
        require(bck.bck.has_value(), "CometNearOrbitSky BCK should expose ANK1 summary");
        require(bck.bck->frame_max == 3000, "CometNearOrbitSky BCK frame max changed");
        require(bck.bck->joint_count == 8U, "CometNearOrbitSky BCK joint count changed");
        require(bck.bck->rotation_fraction == 1U, "CometNearOrbitSky BCK rotation fraction changed");
        require(bck.bck->scale_count == 1U, "CometNearOrbitSky BCK scale value count changed");
        require(bck.bck->rotation_count == 16U, "CometNearOrbitSky BCK rotation value count changed");
        require(bck.bck->translation_count == 10U, "CometNearOrbitSky BCK translation value count changed");
        require(bck.bck->scale_values.size() == 1U, "CometNearOrbitSky BCK scale values should be decoded");
        require(bck.bck->rotation_values.size() == 16U, "CometNearOrbitSky BCK rotation values should be decoded");
        require(bck.bck->translation_values.size() == 10U, "CometNearOrbitSky BCK translation values should be decoded");

        const auto root_joint = smgpc::game::j3d_evaluate_bck_joint_transform(*bck.bck, 0U, 1500.0F);
        require(root_joint.has_value(), "CometNearOrbitSky BCK should evaluate root joint transform");
        require_near(root_joint->scale[0U], 1.0F, 0.001F, "CometNearOrbitSky BCK root X scale changed");
        require(root_joint->rotation[0U] == 0 && root_joint->rotation[1U] == 0 && root_joint->rotation[2U] == 0,
                "CometNearOrbitSky BCK root rotation should remain identity");

        const auto orbit_joint = smgpc::game::j3d_evaluate_bck_joint_transform(*bck.bck, 3U, 1500.0F);
        require(orbit_joint.has_value(), "CometNearOrbitSky BCK should evaluate animated orbit joint transform");
        require(orbit_joint->rotation[0U] == 32686 && orbit_joint->rotation[2U] == 32686,
                "CometNearOrbitSky BCK orbit joint half-frame rotation changed");
        require_near(orbit_joint->translation[0U], 518043.0F, 0.5F, "CometNearOrbitSky BCK orbit joint X translation changed");
        require(!smgpc::game::j3d_evaluate_bck_joint_transform(*bck.bck, 8U, 0.0F).has_value(),
                "CometNearOrbitSky BCK should reject out-of-range joint indices");

        const auto btk = smgpc::game::inspect_j3d_animation(sky_archive.file_data("cometnearorbitsky.btk"));
        require(btk.type == "btk1", "CometNearOrbitSky BTK file type changed");
        require(btk.sections.size() == 1U && btk.sections[0U].tag == "TTK1", "CometNearOrbitSky BTK should contain one TTK1 section");
        require(btk.btk.has_value(), "CometNearOrbitSky BTK should expose TTK1 summary");
        require(btk.btk->frame_max == 10000, "CometNearOrbitSky BTK frame max changed");
        require(btk.btk->track_count == 15U, "CometNearOrbitSky BTK track count changed");
        require(btk.btk->scale_count == 10U, "CometNearOrbitSky BTK scale value count changed");
        require(btk.btk->rotation_count == 3U, "CometNearOrbitSky BTK rotation value count changed");
        require(btk.btk->translation_count == 284U, "CometNearOrbitSky BTK translation value count changed");
        require(btk.btk->scale_values.size() == 10U, "CometNearOrbitSky BTK scale values should be decoded");
        require(btk.btk->rotation_values.size() == 3U, "CometNearOrbitSky BTK rotation values should be decoded");
        require(btk.btk->translation_values.size() == 284U, "CometNearOrbitSky BTK translation values should be decoded");
        require(btk.btk->materials.size() == 5U, "CometNearOrbitSky BTK material update count changed");
        require(btk.btk->materials[3U].material_name == "EarthFar_v", "CometNearOrbitSky BTK should animate EarthFar_v");
        require(btk.btk->materials[3U].tex_matrix_id == 2U, "CometNearOrbitSky BTK EarthFar_v texture matrix id changed");
        require_near(btk.btk->materials[3U].center[0U], 0.5F, 0.001F, "CometNearOrbitSky BTK SRT center X changed");

        const auto earth_start = smgpc::game::j3d_evaluate_btk_texture_srt(*btk.btk, "EarthFar_v", 2U, 0.0F);
        require(earth_start.has_value(), "CometNearOrbitSky BTK should evaluate EarthFar_v matrix 2 at frame 0");
        require_near(earth_start->scale_s, 0.1F, 0.001F, "CometNearOrbitSky BTK EarthFar_v initial S scale changed");
        require_near(earth_start->scale_t, 0.5F, 0.001F, "CometNearOrbitSky BTK EarthFar_v initial T scale changed");
        require_near(earth_start->translate_s, 0.0F, 0.001F, "CometNearOrbitSky BTK EarthFar_v initial S translation changed");
        require_near(earth_start->translate_t, 0.0F, 0.001F, "CometNearOrbitSky BTK EarthFar_v initial T translation changed");

        const auto earth_middle = smgpc::game::j3d_evaluate_btk_texture_srt(*btk.btk, "EarthFar_v", 2U, 5000.0F);
        require(earth_middle.has_value(), "CometNearOrbitSky BTK should evaluate EarthFar_v matrix 2 at half-frame");
        require_near(earth_middle->translate_s, 0.5F, 0.001F, "CometNearOrbitSky BTK EarthFar_v half-frame S translation changed");
        require_near(earth_middle->translate_t, 0.5F, 0.001F, "CometNearOrbitSky BTK EarthFar_v half-frame T translation changed");
        require(!smgpc::game::j3d_evaluate_btk_texture_srt(*btk.btk, "EarthFar_v", 0U, 5000.0F).has_value(),
                "CometNearOrbitSky BTK should only match the material's animated texture matrix id");
    }

    void test_j3d_model_renderer_comet_near_orbit_sky_keeps_shape_meshes(const std::filesystem::path &root) {
        const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
        require(sky_archive.contains("cometnearorbitsky.bdl"), "CometNearOrbitSky.arc should contain the original BDL");
        const auto sky_model = smgpc::game::inspect_j3d_model(sky_archive.file_data("cometnearorbitsky.bdl"));
        require(sky_model.materials.has_value(), "CometNearOrbitSky renderer test should expose source materials");
        const auto find_material_summary = [&sky_model](std::string_view name) -> const smgpc::game::J3dMaterialSummary * {
            const auto it = std::ranges::find_if(sky_model.materials->materials, [name](const auto &material) { return material.name == name; });
            return it == sky_model.materials->materials.end() ? nullptr : &*it;
        };
        const auto *space_summary = find_material_summary("Space_Mat_v");
        const auto *earth_far_summary = find_material_summary("EarthFar_v");
        require(space_summary != nullptr && earth_far_summary != nullptr, "CometNearOrbitSky renderer test should resolve source material state");

        auto renderer = RecordingRenderer();
        auto model_renderer = smgpc::game::J3dModelRenderer();
        model_renderer.load(renderer, sky_archive.file_data("cometnearorbitsky.bdl"));
        require(model_renderer.is_loaded(), "J3dModelRenderer should load CometNearOrbitSky original geometry");
        require(model_renderer.mesh_count() == 9U,
                "J3dModelRenderer should keep one CometNearOrbitSky renderable mesh per original shape with default load options");
        const auto packets = model_renderer.render_packets();
        require(packets.size() == 9U, "J3dModelRenderer should expose one state packet per CometNearOrbitSky renderable mesh");
        for (auto packet_index = std::size_t{1U}; packet_index < packets.size(); ++packet_index) {
            const auto previous = std::pair<std::uint16_t, std::uint8_t>{packets[packet_index - 1U].shape_draw_order,
                                                                         packets[packet_index - 1U].pass_order};
            const auto current = std::pair<std::uint16_t, std::uint8_t>{packets[packet_index].shape_draw_order, packets[packet_index].pass_order};
            require(previous <= current, "J3dModelRenderer packet evidence should preserve draw-order then material-pass ordering");
        }
        const auto earth_far_packet = std::ranges::find_if(packets, [](const auto &packet) {
            return packet.material_name == "EarthFar_v";
        });
        require(earth_far_packet != packets.end() && earth_far_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev &&
                    !earth_far_packet->evaluate_material_per_vertex && earth_far_packet->material_pass_count == 3U &&
                    earth_far_packet->shader_texture_stage_count == 3U,
                "J3dModelRenderer packet evidence should identify projected multi-TEV shader packets");
        const auto earth_night_packet = std::ranges::find_if(packets, [](const auto &packet) {
            return packet.material_name == "EarthNightMat_v";
        });
        require(earth_night_packet != packets.end() &&
                    earth_night_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev &&
                    earth_night_packet->shader_texture_stage_count == 2U,
                "J3dModelRenderer packet evidence should identify projected two-stage TEV shader packets");
        const auto comet_halo_packet = std::ranges::find_if(packets, [](const auto &packet) {
            return packet.material_name == "CometHalo_v";
        });
        require(comet_halo_packet != packets.end() && comet_halo_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ComposedMaterial &&
                    !comet_halo_packet->evaluate_material_per_vertex && comet_halo_packet->indirect_stage_count == 1U &&
                    comet_halo_packet->indirect_texture_order_count > 0U && comet_halo_packet->declared_tev_stage_count == 1U &&
                    comet_halo_packet->active_tev_stage_count == 1U,
                "J3dModelRenderer packet evidence should route CometHalo_v active-indirect TEV through compat material evaluation");
        const auto comet_halo_batch_count_before_draw = renderer.triangle_batch_count;
        const auto comet_halo_vertices_before_draw = renderer.submitted_vertices;
        model_renderer.draw(renderer, smgpc::game::file_select_title_camera_pose(), smgpc::game::J3dMatrix3x4{}, 0U,
                            {.material_filter = "CometHalo_v"});
        require(renderer.triangle_batch_count > comet_halo_batch_count_before_draw &&
                    renderer.submitted_vertices > comet_halo_vertices_before_draw,
                "J3dModelRenderer should submit visible CometHalo_v active-indirect geometry to the renderer");
        const auto sky_packet = std::ranges::find_if(packets, [](const auto &packet) {
            return packet.material_name == "Sky_Mat_v";
        });
        require(sky_packet != packets.end() && sky_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev &&
                    sky_packet->shader_texture_stage_count == 1U,
                "J3dModelRenderer packet evidence should route single-texture TEV packets through the GX shader path");
        const auto space_packet = std::ranges::find_if(packets, [](const auto &packet) {
            return packet.material_name == "Space_Mat_v";
        });
        require(space_packet != packets.end() && space_packet->shape_draw_order == 7U && space_packet->joint_index == 7U &&
                    space_packet->tev_stage_count > 0U && space_packet->mdl3_packet_bytes > 0U && space_packet->mdl3_bp_load_count > 0U &&
                    space_packet->mdl3_xf_load_count > 0U && space_packet->cull_mode == smgpc::render::CullMode::Back &&
                    space_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev && space_packet->shader_texture_stage_count == 3U &&
                    space_packet->gx_blend.enabled && space_packet->gx_blend.type == 0U && space_packet->gx_blend.src_factor == 1U &&
                    space_packet->gx_blend.dst_factor == 0U && space_packet->fog_type == space_summary->gx_state.fog.type &&
                    space_packet->fog_color == space_summary->gx_state.fog.color,
                "J3dModelRenderer packet evidence should preserve GX state and shader-backed material context for Space_Mat_v");
        const auto sky_bck = smgpc::game::inspect_j3d_animation(sky_archive.file_data("cometnearorbitsky.bck"));
        const auto sky_btk = smgpc::game::inspect_j3d_animation(sky_archive.file_data("cometnearorbitsky.btk"));
        require(sky_bck.bck.has_value() && sky_btk.btk.has_value(), "CometNearOrbitSky renderer test should resolve BCK and BTK state");
        model_renderer.set_bck_animation(*sky_bck.bck);
        model_renderer.set_btk_animation(*sky_btk.btk);
        const auto animated_packets = model_renderer.render_packets(3001U);
        const auto animated_space_packet = std::ranges::find_if(animated_packets, [](const auto &packet) {
            return packet.material_name == "Space_Mat_v";
        });
        require(animated_space_packet != animated_packets.end() && animated_space_packet->bck_active && animated_space_packet->btk_active &&
                    animated_space_packet->bck_frame_max == 3000 && animated_space_packet->bck_joint_count == 8U &&
                    animated_space_packet->btk_frame_max == 10000 && animated_space_packet->btk_material_count == 5U,
                "J3dModelRenderer packet evidence should include active BCK and BTK metadata for runtime comparisons");
        require_near(animated_space_packet->bck_frame, 3001.0F, 0.001F,
                     "J3dModelRenderer packet evidence should preserve the submitted BCK frame");
        require_near(animated_space_packet->bck_normalized_frame, 1.0F, 0.001F,
                     "J3dModelRenderer packet evidence should expose looped BCK frame state");
        require_near(animated_space_packet->btk_normalized_frame, 3001.0F, 0.001F,
                     "J3dModelRenderer packet evidence should expose looped BTK frame state");
        const auto batch_count_before_draw = renderer.gx_material_batch_count;
        model_renderer.draw(renderer, smgpc::game::file_select_title_camera_pose(), smgpc::game::J3dMatrix3x4{}, 0U,
                            {.material_filter = "Space_Mat_v"});
        require(renderer.gx_material_batch_count > batch_count_before_draw,
                "J3dModelRenderer should submit filtered Space_Mat_v geometry through the GX material shader path");
        require(renderer.last_gx_material_stage_count == 3U && renderer.last_gx_material_tev_stage_count == 3U,
                "J3dModelRenderer should submit Space_Mat_v as a three-texture GX TEV material batch");
        require(renderer.last_gx_material_color_inputs[0U] == std::array<std::uint8_t, 4U>{15U, 8U, 10U, 14U} &&
                    renderer.last_gx_material_color_inputs[1U] == std::array<std::uint8_t, 4U>{15U, 10U, 8U, 0U} &&
                    renderer.last_gx_material_color_inputs[2U] == std::array<std::uint8_t, 4U>{15U, 10U, 8U, 0U} &&
                    renderer.last_gx_material_alpha_compare_enabled && renderer.last_gx_material_blend.enabled &&
                    renderer.last_gx_material_blend.type == 0U && renderer.last_gx_material_blend.src_factor == 1U &&
                    renderer.last_gx_material_blend.dst_factor == 0U,
                "J3dModelRenderer should preserve Space_Mat_v raw TEV shader state");
        require(renderer.last_gx_material_alpha_compare.comp0 == space_summary->gx_state.alpha_compare.comp0 &&
                    renderer.last_gx_material_alpha_compare.ref0 == space_summary->gx_state.alpha_compare.ref0 &&
                    renderer.last_gx_material_alpha_compare.op == space_summary->gx_state.alpha_compare.op &&
                    renderer.last_gx_material_alpha_compare.comp1 == space_summary->gx_state.alpha_compare.comp1 &&
                    renderer.last_gx_material_alpha_compare.ref1 == space_summary->gx_state.alpha_compare.ref1 &&
                    renderer.last_gx_material_initial_tev_registers == space_summary->gx_state.tev_registers,
                "J3dModelRenderer should submit Space_Mat_v alpha compare and initial TEV registers from effective GX state");
        require(renderer.last_gx_material_fog.enabled == space_summary->gx_state.fog.enabled &&
                    renderer.last_gx_material_fog.type == space_summary->gx_state.fog.type &&
                    renderer.last_gx_material_fog.projection == space_summary->gx_state.fog.projection &&
                    renderer.last_gx_material_fog.color == space_summary->gx_state.fog.color,
                "J3dModelRenderer should submit Space_Mat_v typed GX fog state to the renderer batch");
        require(renderer.last_triangle_cull_mode == smgpc::render::CullMode::Back,
                "J3dModelRenderer should submit the decoded MDL3 GX cull mode to the GX material renderer batch");
        const auto earth_batch_count_before_draw = renderer.gx_material_batch_count;
        model_renderer.draw(renderer, smgpc::game::file_select_title_camera_pose(), smgpc::game::J3dMatrix3x4{}, 0U,
                            {.material_filter = "EarthFar_v"});
        require(renderer.gx_material_batch_count > earth_batch_count_before_draw,
                "J3dModelRenderer should submit projected EarthFar_v geometry through the GX material shader path");
        require(renderer.last_gx_material_stage_count == 3U && renderer.last_gx_material_tev_stage_count == 3U &&
                    renderer.last_gx_material_color_inputs[0U] == std::array<std::uint8_t, 4U>{15U, 10U, 8U, 15U} &&
                    renderer.last_gx_material_color_inputs[1U] == std::array<std::uint8_t, 4U>{4U, 14U, 8U, 0U} &&
                    renderer.last_gx_material_color_inputs[2U] == std::array<std::uint8_t, 4U>{15U, 10U, 8U, 0U},
                "J3dModelRenderer should preserve EarthFar_v projected TEV shader state");
        require(renderer.last_gx_material_saw_projective_q && renderer.last_gx_material_saw_clip_w,
                "J3dModelRenderer should submit projected texture q and clip w for EarthFar_v shader interpolation");
        require(renderer.last_gx_material_blend.enabled && renderer.last_gx_material_blend.type == 0U &&
                    renderer.last_gx_material_blend.src_factor == 1U && renderer.last_gx_material_blend.dst_factor == 0U,
                "J3dModelRenderer should submit EarthFar_v raw GX no-blend state to the GX material renderer batch");
        require(renderer.last_gx_material_initial_tev_registers == earth_far_summary->gx_state.tev_registers &&
                    renderer.last_gx_material_depth_test == (earth_far_summary->gx_state.z_mode.compare_enable != 0U) &&
                    renderer.last_gx_material_depth_write == (earth_far_summary->gx_state.z_mode.update_enable != 0U) &&
                    renderer.last_gx_material_depth_compare == smgpc::render::DepthCompare::LessEqual,
                "J3dModelRenderer should submit EarthFar_v initial TEV registers and depth state from effective GX state");
        require(renderer.last_gx_material_fog.enabled == earth_far_summary->gx_state.fog.enabled &&
                    renderer.last_gx_material_fog.type == earth_far_summary->gx_state.fog.type &&
                    renderer.last_gx_material_fog.color == earth_far_summary->gx_state.fog.color,
                "J3dModelRenderer should submit EarthFar_v typed GX fog state to the renderer batch");
        const auto sun_batch_count_before_draw = renderer.gx_material_batch_count;
        model_renderer.draw(renderer, smgpc::game::file_select_title_camera_pose(), smgpc::game::J3dMatrix3x4{}, 0U,
                            {.material_filter = "Sun_Mat_v"});
        require(renderer.gx_material_batch_count > sun_batch_count_before_draw,
                "J3dModelRenderer should submit filtered Sun_Mat_v geometry through the GX material shader path");
        require(renderer.last_gx_material_blend.enabled && renderer.last_gx_material_blend.type == 1U &&
                    renderer.last_gx_material_blend.src_factor == 4U && renderer.last_gx_material_blend.dst_factor == 1U,
                "J3dModelRenderer should submit Sun_Mat_v raw GX additive blend state to the GX material renderer batch");
        require(renderer.texture_count > 0U, "J3dModelRenderer should upload CometNearOrbitSky textures");

        auto shader_only_renderer = RecordingRenderer();
        auto shader_only_options = smgpc::game::J3dModelRendererLoadOptions{};
        shader_only_options.use_cpu_tev = false;
        auto shader_only_model = smgpc::game::J3dModelRenderer();
        shader_only_model.load(shader_only_renderer, sky_archive.file_data("cometnearorbitsky.bdl"), shader_only_options);
        const auto shader_only_packets = shader_only_model.render_packets();
        const auto shader_only_space = std::ranges::find_if(shader_only_packets, [](const auto &packet) {
            return packet.material_name == "Space_Mat_v";
        });
        require(shader_only_space != shader_only_packets.end() &&
                    shader_only_space->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev,
                "J3dModelRenderer should keep shader-backed Space_Mat_v even when CPU TEV fallbacks are disabled");
        const auto shader_only_earth_far = std::ranges::find_if(shader_only_packets, [](const auto &packet) {
            return packet.material_name == "EarthFar_v";
        });
        require(shader_only_earth_far != shader_only_packets.end() &&
                    shader_only_earth_far->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev,
                "J3dModelRenderer should keep shader-backed EarthFar_v even when CPU TEV fallbacks are disabled");
    }

    void test_j3d_model_renderer_file_select_planet(const std::filesystem::path &root) {
        const auto planet_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "FileSelectDataPlanet.arc");
        require(planet_archive.contains("fileselectdataplanet.bdl"), "FileSelectDataPlanet.arc should contain the original BDL");

        auto renderer = RecordingRenderer();
        auto model_renderer = smgpc::game::J3dModelRenderer();
        model_renderer.load(renderer, planet_archive.file_data("fileselectdataplanet.bdl"));
        require(model_renderer.is_loaded(), "J3dModelRenderer should load FileSelectDataPlanet original geometry");
        require(model_renderer.mesh_count() > 0U, "J3dModelRenderer should expose renderable FileSelectDataPlanet meshes");
        require(renderer.texture_count > 0U, "J3dModelRenderer should upload FileSelectDataPlanet textures");

        model_renderer.draw(renderer, smgpc::game::file_select_far_camera_pose(),
                            smgpc::game::j3d_matrix_from_translation_scale({0.0F, 800.0F, 0.0F}, 30.0F), 0U);
        require(renderer.triangle_batch_count > 0U, "J3dModelRenderer should submit projected FileSelectDataPlanet triangles");
        require(renderer.submitted_vertices > 0U && renderer.submitted_indices > 0U,
                "J3dModelRenderer should submit non-empty FileSelectDataPlanet batches");
    }

    void test_file_select_item_draws_original_planet_model() {
        auto logger = NullLogger();
        auto window = TestWindowService();
        auto runtime = smgpc::game::RuntimeContext(logger, window);
        auto renderer = RecordingRenderer();
        auto item = FileSelectItem(1, true);

        item.appear();
        item.update({0.0F, 800.0F, 0.0F});
        item.draw(renderer, smgpc::game::file_select_far_camera_pose());

        require(renderer.texture_count > 0U, "FileSelectItem should load original FileSelectDataPlanet textures when drawn");
        require(renderer.triangle_batch_count > 0U, "FileSelectItem should draw the original FileSelectDataPlanet model");
        require(renderer.submitted_indices > 0U, "FileSelectItem should submit original FileSelectDataPlanet geometry");
    }

    void test_file_select_sky_draws_through_live_actor_model_compat() {
        auto logger = NullLogger();
        auto window = TestWindowService();
        auto runtime = smgpc::game::RuntimeContext(logger, window);
        auto renderer = RecordingRenderer();
        auto sky = FileSelectSky("ファイル選択空");

        sky.initWithoutIter();
        sky.appear();
        runtime.begin_frame(smgpc::render::FrameContext{
            .frame_index = 0U,
            .frame_time_seconds = 0.0,
            .frame_delta_seconds = 1.0 / 60.0,
            .framebuffer = {.width = 640U, .height = 456U},
            .has_focus = true,
            .is_minimized = false,
        });
        runtime.draw_3d_normal(renderer, smgpc::game::file_select_title_camera_pose());

        require(sky.getNerveStep() == 1, "FileSelectSky should be updated by the scene sky actor compatibility list");
        require(renderer.texture_count > 0U, "FileSelectSky should load original CometNearOrbitSky textures through LiveActor model compat");
        require(renderer.triangle_batch_count > 0U, "FileSelectSky should draw original CometNearOrbitSky model geometry");
        require(renderer.submitted_indices > 0U, "FileSelectSky should submit original sky J3D geometry");
    }

    void test_png_screenshot_service() {
        const std::array<std::uint8_t, 16U> pixels{
            255U,
            0U,
            0U,
            255U,
            0U,
            255U,
            0U,
            255U,
            0U,
            0U,
            255U,
            255U,
            255U,
            255U,
            255U,
            255U,
        };

        const auto output = std::filesystem::temp_directory_path() / "smg-pc-png-screenshot-service-test.png";
        const auto screenshot_service = smgpc::render::capture::create_png_screenshot_service();
        screenshot_service->write_png(output, smgpc::render::capture::ScreenshotImageView{
                                                  .width = 2U,
                                                  .height = 2U,
                                                  .pitch = 8U,
                                                  .pixels = std::span<const std::uint8_t>(pixels.data(), pixels.size()),
                                                  .format = smgpc::render::capture::PixelFormat::RGBA8,
                                                  .origin_bottom_left = false,
                                              });

        const auto png = read_file(output);
        require(png.size() > 64U, "PNG service output is too small");
        require(png[0] == 0x89U && png[1] == 0x50U && png[2] == 0x4eU && png[3] == 0x47U, "PNG service output missing signature");
        require(read_be32(png, 8U) == 13U, "PNG service IHDR length mismatch");
        require(png[12] == 'I' && png[13] == 'H' && png[14] == 'D' && png[15] == 'R', "PNG service output missing IHDR");
        require(read_be32(png, 16U) == 2U, "PNG service width mismatch");
        require(read_be32(png, 20U) == 2U, "PNG service height mismatch");
        require(png[24] == 8U, "PNG service bit depth mismatch");
        require(png[25] == 6U, "PNG service color type mismatch");
    }

    void test_j3d_single_texture_tev_composition() {
        auto material = smgpc::game::J3dMaterialSummary{};
        material.name = "synthetic-sky";
        material.material_colors[0U] = {149U, 195U, 165U, 255U};
        material.tev_k_colors[0U] = {0U, 28U, 43U, 255U};
        material.tev_stage_count = 1U;
        material.textures.push_back(smgpc::game::J3dMaterialTextureBinding{
            .slot = 0U,
            .texture_index = 0U,
        });
        material.tev_orders.push_back(smgpc::game::J3dTevOrderSummary{
            .stage = 0U,
            .tex_coord = 0U,
            .tex_map = 0U,
            .color_channel = 4U,
        });
        material.tev_stages.push_back(smgpc::game::J3dTevStageSummary{
            .stage = 0U,
            .color_in = {14U, 10U, 8U, 15U},
            .color_op = 0U,
            .color_bias = 0U,
            .color_scale = 1U,
            .color_clamp = 1U,
            .color_out = 0U,
            .k_color_sel = 12U,
            .alpha_in = {7U, 4U, 5U, 7U},
            .alpha_op = 0U,
            .alpha_bias = 0U,
            .alpha_scale = 0U,
            .alpha_clamp = 1U,
            .alpha_out = 0U,
            .k_alpha_sel = 28U,
        });

        const auto source = smgpc::game::DecodedTexture{
            .width = 2U,
            .height = 1U,
            .format = smgpc::game::TplTextureFormat::I8,
            .rgba = {0U, 0U, 0U, 0U, 255U, 255U, 255U, 255U},
        };
        const auto composed = smgpc::game::j3d_try_compose_material_texture(material, source, material.material_colors[0U], 0U);
        require(composed.has_value(), "single-texture J3D TEV material should compose");
        require(composed->raster_color_baked, "single-texture J3D TEV material should mark raster color as baked");
        require(composed->image.width == 2U && composed->image.height == 1U, "single-texture J3D TEV composition dimensions changed");
        require(composed->image.rgba[0U] == 0U && composed->image.rgba[1U] == 56U && composed->image.rgba[2U] == 86U &&
                    composed->image.rgba[3U] == 0U,
                "single-texture J3D TEV composition did not apply konst-to-raster color ramp at texel 0");
        require(composed->image.rgba[4U] == 255U && composed->image.rgba[5U] == 255U && composed->image.rgba[6U] == 255U &&
                    composed->image.rgba[7U] == 255U,
                "single-texture J3D TEV composition did not clamp raster color at texel 1");
    }

    void test_wii_logical_render_viewport() {
        require(smgpc::render::core::kWiiLogicalFramebufferWidth == 640U, "logical Wii framebuffer width should match Dolphin title captures");
        require(smgpc::render::core::kWiiLogicalFramebufferHeight == 456U, "logical Wii framebuffer height should match Dolphin title captures");
    }

    void test_runtime_services_dvd_archive_cache(const std::filesystem::path &root) {
        auto dvd = smgpc::game::DvdFileSystemService(root);

        const auto object_path = dvd.find_object_archive("CometNearOrbitSky");
        require(object_path.has_value(), "DVD service should resolve original object archives");
        require(dvd.exists("ObjectData/CometNearOrbitSky.arc"), "DVD service should expose disc-relative file existence");

        auto &archive = dvd.archive("ObjectData/CometNearOrbitSky.arc");
        require(archive.contains("cometnearorbitsky.bdl"), "DVD archive cache should mount original RARC contents");
        auto &cached_archive = dvd.archive("/ObjectData/CometNearOrbitSky.arc");
        require(&archive == &cached_archive, "DVD archive cache should reuse equivalent absolute disc paths");
        require(dvd.archive_load_count("ObjectData/CometNearOrbitSky.arc") == 1U, "DVD archive cache should record one physical load");
        require(dvd.cached_archive_count() == 1U, "DVD archive cache should keep one mounted archive entry");

        const auto bytes = dvd.read_file("files/ObjectData/CometNearOrbitSky.arc");
        require(bytes.size() > 0x40U, "DVD service should read disc-relative files with an optional files prefix");

        auto rejected_escape = false;
        try {
            (void)dvd.resolve("../ObjectData/CometNearOrbitSky.arc");
        } catch (const std::runtime_error &) {
            rejected_escape = true;
        }
        require(rejected_escape, "DVD service should reject paths that escape the disc root");
    }

    void test_runtime_services_wpad_save_message_rfl() {
        auto wpad = smgpc::game::WpadService();
        wpad.begin_frame();
        wpad.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_A | WPAD_BUTTON_B);
        require(wpad.is_connected(WPAD_CHAN0), "WPAD service should mark a channel connected when data arrives");
        require(wpad.is_button_held(WPAD_CHAN0, WPAD_BUTTON_A), "WPAD service should expose held core buttons");
        require(wpad.is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A), "WPAD service should expose trigger edges");
        require(!wpad.is_button_released(WPAD_CHAN0, WPAD_BUTTON_A), "WPAD service should not release newly held buttons");

        wpad.begin_frame();
        wpad.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_A);
        require(!wpad.is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A), "WPAD service should suppress repeated trigger edges");
        require(wpad.is_button_released(WPAD_CHAN0, WPAD_BUTTON_B), "WPAD service should expose release edges");
        wpad.set_pointer(WPAD_CHAN0, 320.0F, 228.0F, true);
        wpad.set_pointer(WPAD_CHAN0, 300.0F, 200.0F, true);
        const auto pointer = wpad.pointer(WPAD_CHAN0);
        require(pointer.valid && pointer.x == 300.0F && pointer.y == 200.0F, "WPAD service should preserve pointer position");
        const auto past_pointer = wpad.past_pointer(WPAD_CHAN0, 1U);
        require(wpad.pointer_history_count(WPAD_CHAN0) == 2U && past_pointer.x == 320.0F && past_pointer.y == 228.0F,
                "WPAD service should preserve pointer history");
        wpad.set_sub_stick(WPAD_CHAN0, -0.75F, 0.5F);
        wpad.set_core_acceleration(WPAD_CHAN0, 1.0F, 2.0F, 3.0F);
        wpad.set_sub_acceleration(WPAD_CHAN0, 4.0F, 5.0F, 6.0F);
        wpad.set_swing(WPAD_CHAN0, true, false);
        wpad.set_distance_to_display(WPAD_CHAN0, 2.25F);
        require(wpad.sub_stick(WPAD_CHAN0).x == -0.75F && wpad.sub_stick(WPAD_CHAN0).y == 0.5F,
                "WPAD service should preserve nunchuk stick state");
        require(wpad.core_acceleration(WPAD_CHAN0).z == 3.0F && wpad.sub_acceleration(WPAD_CHAN0).z == 6.0F,
                "WPAD service should preserve acceleration fixtures");
        require(wpad.is_core_swing(WPAD_CHAN0) && wpad.distance_to_display(WPAD_CHAN0) == 2.25F,
                "WPAD service should preserve swing and display-distance fixtures");

        auto save = smgpc::game::SaveDataService();
        const std::array<std::uint8_t, 4U> save_bytes{1U, 2U, 3U, 4U};
        save.write_file("user/slot0.bin", save_bytes);
        require(save.exists("user/slot0.bin"), "save service should report deterministic fixture files");
        const auto loaded_save = save.read_file("user/slot0.bin");
        require(loaded_save.has_value() && *loaded_save == std::vector<std::uint8_t>(save_bytes.begin(), save_bytes.end()),
                "save service should round-trip file bytes");
        require(save.erase("user/slot0.bin") && !save.exists("user/slot0.bin"), "save service should erase fixture files");

        auto messages = smgpc::game::MessageService();
        messages.set_message("FileSelect_NewFile", "New File");
        require(messages.message_or("FileSelect_NewFile", "fallback") == "New File", "message service should resolve fixture messages");
        require(messages.message_or("Missing", "fallback") == "fallback", "message service should provide deterministic fallback text");

        auto rfl = smgpc::game::RflService();
        require(rfl.is_initialized() && !rfl.has_error(), "RFL service should default to a ready deterministic Mii fixture");
        require(!rfl.valid_miis().empty() && rfl.valid_miis()[0].name == "Mario", "RFL service should expose a fixture Mii entry");
        rfl.set_error(true);
        require(rfl.has_error(), "RFL service should allow tests to force Mii errors");
    }

    void test_runtime_context_routes_through_services() {
        auto logger = NullLogger();
        auto window = TestWindowService();
        auto runtime = smgpc::game::RuntimeContext(logger, window);

        runtime.begin_frame(smgpc::render::FrameContext{
            .frame_index = 10U,
            .frame_time_seconds = 10.0 / 60.0,
            .frame_delta_seconds = 1.0 / 60.0,
            .framebuffer = {.width = 640U, .height = 456U},
            .has_focus = true,
            .is_minimized = false,
        });
        require(!runtime.is_core_pad_button_a(WPAD_CHAN0), "runtime should route released host input through WPAD state");

        window.set_title_combo(true);
        runtime.begin_frame(smgpc::render::FrameContext{
            .frame_index = 11U,
            .frame_time_seconds = 11.0 / 60.0,
            .frame_delta_seconds = 1.0 / 60.0,
            .framebuffer = {.width = 640U, .height = 456U},
            .has_focus = true,
            .is_minimized = false,
        });
        require(runtime.is_core_pad_button_a(WPAD_CHAN0) && runtime.is_core_pad_button_b(WPAD_CHAN0),
                "runtime should route host title combo through WPAD core buttons");
        require(runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A), "runtime WPAD service should retain trigger edges");
        runtime.wpad().set_pointer(WPAD_CHAN0, 160.0F, 120.0F, true);
        runtime.wpad().set_sub_stick(WPAD_CHAN0, 0.75F, -0.6F);
        runtime.wpad().set_core_acceleration(WPAD_CHAN0, 0.0F, 1.0F, 2.0F);
        runtime.wpad().set_sub_acceleration(WPAD_CHAN0, 3.0F, 4.0F, 5.0F);
        runtime.wpad().set_distance_to_display(WPAD_CHAN0, 1.5F);
        auto kpad_status = std::array<KPADStatus, 1U>{};
        require(KPADRead(WPAD_CHAN0, kpad_status.data(), static_cast<u32>(kpad_status.size())) == 1,
                "KPADRead should expose runtime WPAD channel state");
        require((kpad_status[0].hold & WPAD_BUTTON_A) != 0U && kpad_status[0].trig == (WPAD_BUTTON_A | WPAD_BUTTON_B) &&
                    kpad_status[0].pos.x == 160.0F && kpad_status[0].wpad_err == WPAD_ERR_NONE && kpad_status[0].dpd_valid_fg == 1,
                "KPADRead should preserve hold bits, pointer coordinates, and success status");
        auto pointing = TVec2f{};
        MR::getCorePadPointingPosBasedOnScreen(&pointing, WPAD_CHAN0);
        require(pointing.x == 160.0F && pointing.y == 120.0F && MR::isCorePadPointInScreen(WPAD_CHAN0),
                "MR GamePadUtil should expose core pointer state");
        auto accel = TVec3f{};
        MR::getCorePadAcceleration(&accel, WPAD_CHAN0);
        require(accel.y == 1.0F && MR::getCorePadDistanceToDisplay(WPAD_CHAN0) == 1.5F,
                "MR GamePadUtil should expose core acceleration and pointer distance fixtures");
        require(MR::testCorePadButtonA(WPAD_CHAN0) && MR::testCorePadTriggerA(WPAD_CHAN0) && MR::testSystemPadTriggerDecide(),
                "MR GamePadUtil should route button held/trigger/system aliases through WPAD state");
        require(MR::getSubPadStickX(WPAD_CHAN0) == 0.75F && MR::getSubPadStickY(WPAD_CHAN0) == -0.6F &&
                    MR::testSubPadStickTriggerRight(WPAD_CHAN0) && MR::testSubPadStickTriggerDown(WPAD_CHAN0),
                "MR GamePadUtil should expose KB&M-backed nunchuk stick fixtures");
        MR::getSubPadAcceleration(&accel, WPAD_CHAN0);
        require(accel.z == 5.0F && MR::getWPadMaxCount() == static_cast<u32>(WPAD_MAX_CONTROLLERS) && MR::isConnectedWPad(WPAD_CHAN0),
                "MR GamePadUtil should expose sub acceleration, controller count, and connected state");

        auto dvd_file = DVDFileInfo{};
        require(DVDConvertPathToEntrynum("/ObjectData/CometNearOrbitSky.arc") >= 0, "DVD path conversion should use the runtime DVD service");
        require(DVDOpen("/ObjectData/CometNearOrbitSky.arc", &dvd_file) != 0, "DVDOpen should resolve disc-relative files");
        require(DVDGetLength(&dvd_file) > 0x40U, "DVDGetLength should report mounted disc file size");
        auto dvd_magic = std::array<std::uint8_t, 4U>{};
        require(DVDReadPrio(&dvd_file, dvd_magic.data(), static_cast<s32>(dvd_magic.size()), 0, 2) == 4,
                "DVDReadPrio should synchronously read from the runtime DVD service");
        require((dvd_magic[0] == 'Y' && dvd_magic[1] == 'a' && dvd_magic[2] == 'z' && dvd_magic[3] == '0') ||
                    (dvd_magic[0] == 'R' && dvd_magic[1] == 'A' && dvd_magic[2] == 'R' && dvd_magic[3] == 'C'),
                "DVDReadPrio should return original archive bytes");
        require(DVDClose(&dvd_file) != 0 && dvd_file.internal == nullptr, "DVDClose should release runtime DVD file handles");

        char object_archive_path[128]{};
        require(MR::makeObjectArchiveFileNameFromPrefix(object_archive_path, sizeof(object_archive_path), "CometNearOrbitSky", false),
                "MR FileUtil should resolve original object archive paths");
        require(std::string_view(object_archive_path) == "/ObjectData/CometNearOrbitSky.arc",
                "MR FileUtil object archive path should stay disc-relative");
        require(MR::isFileExist(object_archive_path, false), "MR FileUtil should test disc-relative file existence");
        require(MR::getFileSize(object_archive_path, false) > 0x40U, "MR FileUtil should report original file sizes");
        require(MR::convertPathToEntrynumConsideringLanguage(object_archive_path) >= 0, "MR FileUtil should convert paths through DVD service");
        require(MR::loadToMainRAM(object_archive_path, nullptr, nullptr, JKRDvdRipper::ALLOC_DIR_TOP) != nullptr,
                "MR FileUtil should synchronously load original files");
        require(MR::isLoadedFile(object_archive_path), "MR FileUtil should retain loaded-file state");
        auto *mounted_archive = MR::mountArchive(object_archive_path, nullptr);
        require(mounted_archive != nullptr && mounted_archive->contains("cometnearorbitsky.bdl"), "MR FileUtil should mount RARC archives");
        require(MR::isMountedArchive(object_archive_path), "MR FileUtil should retain mounted-archive state");
        require(mounted_archive->getResource("cometnearorbitsky.bdl") != nullptr, "JKR archive shim should expose resources by path");
        JKRArchive *archive_out = nullptr;
        JKRHeap *heap_out = reinterpret_cast<JKRHeap *>(std::uintptr_t{1U});
        MR::getMountedArchiveAndHeap(object_archive_path, &archive_out, &heap_out);
        require(archive_out == mounted_archive && heap_out == nullptr, "MR FileUtil should expose mounted archive handles");

        char layout_archive_path[128]{};
        require(MR::makeLayoutArchiveFileNameFromPrefix(layout_archive_path, sizeof(layout_archive_path), "TitleLogo", false),
                "MR FileUtil should resolve localized layout archives");
        require(std::string_view(layout_archive_path) == "/KrKorean/LayoutData/TitleLogo.arc",
                "MR FileUtil should prefer localized layout archives");
        char scenario_path[128]{};
        MR::makeScenarioArchiveFileName(scenario_path, sizeof(scenario_path), "FileSelect");
        require(std::string_view(scenario_path) == "/StageData/FileSelect/FileSelectScenario.arc",
                "MR FileUtil should build original-style scenario archive paths");

        runtime.start_stage_bgm("MBGM_FILE_SELECT");
        require(runtime.current_stage_bgm_name() == "MBGM_FILE_SELECT", "runtime BGM facade should use the audio event service");
        require(!runtime.is_stage_bgm_prepared(), "runtime BGM should not be prepared until the next frame");
        runtime.begin_frame(smgpc::render::FrameContext{
            .frame_index = 12U,
            .frame_time_seconds = 12.0 / 60.0,
            .frame_delta_seconds = 1.0 / 60.0,
            .framebuffer = {.width = 640U, .height = 456U},
            .has_focus = true,
            .is_minimized = false,
        });
        require(runtime.is_stage_bgm_prepared(), "runtime BGM preparation should be frame based");
        runtime.unlock_stage_bgm();
        require(runtime.audio().is_stage_bgm_unlocked(), "runtime should route BGM unlock to the audio service");
        runtime.start_system_sound("SE_SY_CURSOR");
        runtime.start_cs_sound("CS_DECIDE");
        runtime.stop_stage_bgm(30);
        require(runtime.audio().events().size() == 5U, "audio service should keep separate deterministic event records");

        runtime.emit_effect("TitleLogo", "Decision");
        require(runtime.effects().active_effects("TitleLogo").size() == 1U, "runtime should route effect emission to the effect service");
        runtime.delete_effect_all("TitleLogo");
        require(runtime.effects().active_effects("TitleLogo").empty(), "runtime should route effect cleanup to the effect service");
    }

    void test_scene_scheduler_orders_original_style_categories() {
        auto scheduler = smgpc::game::SceneScheduler();
        auto sky = SchedulerProbeObj("sky-probe");
        auto layout = SchedulerProbeObj("layout-probe");
        auto map_obj = SchedulerProbeObj("map-probe");
        auto area_obj = SchedulerProbeObj("area-probe");
        auto movie = SchedulerProbeObj("movie-probe");
        auto npc_actor = SchedulerProbeActor("npc-probe");
        npc_actor.appear();

        scheduler.connect_name_obj(sky, MR::MovementType_Sky, MR::CalcAnimType_MapObj, MR::DrawBufferType_Sky, -1);
        scheduler.connect_name_obj(layout, MR::MovementType_Layout, MR::CalcAnimType_Layout, -1, MR::DrawType_Layout);
        scheduler.connect_name_obj(map_obj, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_MapObj, -1);
        scheduler.connect_name_obj(area_obj, MR::MovementType_AreaObj, -1, -1, -1);
        scheduler.connect_name_obj(movie, MR::MovementType_Movie, -1, -1, MR::DrawType_Movie);
        scheduler.register_live_actor_model(npc_actor, MR::MovementType_NPC, MR::CalcAnimType_NPC, MR::DrawBufferType_NPC, -1);

        const auto snapshot = scheduler.snapshot();
        require(snapshot.size() == 6U, "scene scheduler should retain all connected NameObj and LiveActor entries");
        require(snapshot[0].name == "sky-probe" && snapshot[0].movement_type == MR::MovementType_Sky,
                "scene scheduler snapshot should preserve registration state");

        scheduler.execute_movement();
        const auto movement_trace = scheduler.last_execution_trace();
        require(movement_trace.size() == 6U, "scene scheduler should trace movement execution");
        require(movement_trace[0].name == "map-probe" && movement_trace[0].phase == smgpc::game::SceneSchedulerPhase::Movement &&
                    movement_trace[1].name == "npc-probe" && movement_trace[2].name == "area-probe" &&
                    movement_trace[3].name == "layout-probe" && movement_trace[4].name == "movie-probe" && movement_trace[5].name == "sky-probe",
                "scene scheduler movement should execute by original SceneExecutor movement category order");
        require(sky.movement_count == 1 && layout.movement_count == 1 && map_obj.movement_count == 1 && area_obj.movement_count == 1 &&
                    movie.movement_count == 1 && npc_actor.control_count == 1,
                "scene scheduler should call NameObj movement and LiveActor control once for active entries");

        NameObjFunction::requestMovementOff(&map_obj);
        scheduler.execute_movement();
        const auto suspended_trace = scheduler.last_execution_trace();
        require(suspended_trace.size() == 5U && suspended_trace[0].name == "npc-probe" && suspended_trace[1].name == "area-probe" &&
                    suspended_trace[2].name == "layout-probe" && suspended_trace[3].name == "movie-probe" &&
                    suspended_trace[4].name == "sky-probe" && map_obj.movement_count == 1 && npc_actor.control_count == 2,
                "scene scheduler should honor NameObj movement suspension");

        scheduler.execute_calc_anim();
        const auto calc_trace = scheduler.last_execution_trace();
        require(calc_trace.size() == 8U && calc_trace[5].name == "sky-probe" &&
                    calc_trace[5].phase == smgpc::game::SceneSchedulerPhase::CalcAnim && calc_trace[6].name == "npc-probe" &&
                    calc_trace[7].name == "layout-probe" && layout.calc_anim_count == 1 && sky.calc_anim_count == 1 &&
                    npc_actor.calc_anim_count == 1,
                "scene scheduler should append phase-tagged calcAnim evidence in original SceneExecutor category order");

        scheduler.execute_calc_view_and_entry();
        const auto calc_view_trace = scheduler.last_execution_trace();
        require(calc_view_trace.size() == 12U && calc_view_trace[8].name == "layout-probe" &&
                    calc_view_trace[8].phase == smgpc::game::SceneSchedulerPhase::CalcViewAndEntry &&
                    calc_view_trace[9].name == "movie-probe" && calc_view_trace[10].name == "npc-probe" &&
                    calc_view_trace[11].name == "sky-probe" && sky.calc_view_count == 1 && layout.calc_view_count == 1 &&
                    area_obj.calc_view_count == 0 && movie.calc_view_count == 1 && npc_actor.calc_view_count == 1,
                "scene scheduler should execute calcViewAndEntry as original 2D entry before 3D draw-buffer entry");
    }

    void test_scene_scheduler_executes_original_normal_draw_buffer_passes() {
        auto scheduler = smgpc::game::SceneScheduler();
        auto renderer = RecordingRenderer();
        auto map_actor = SchedulerProbeActor("map-draw-probe");
        auto npc_actor = SchedulerProbeActor("npc-draw-probe");
        auto sky_actor = SchedulerProbeActor("sky-draw-probe");

        map_actor.appear();
        npc_actor.appear();
        sky_actor.appear();
        scheduler.register_live_actor_model(map_actor, -1, -1, MR::DrawBufferType_MapObj, -1);
        scheduler.register_live_actor_model(npc_actor, -1, -1, MR::DrawBufferType_NPC, -1);
        scheduler.register_live_actor_model(sky_actor, -1, -1, MR::DrawBufferType_Sky, -1);

        scheduler.execute_draw_buffer_list_normal(renderer, smgpc::game::file_select_title_camera_pose());
        const auto trace = scheduler.last_execution_trace();
        require(trace.size() == 6U, "normal draw-buffer list should trace opa and xlu buffer passes with active actors");
        require(trace[0].name == "map-draw-probe" && trace[0].phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa &&
                    trace[0].draw_buffer_type == MR::DrawBufferType_MapObj &&
                    trace[0].draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Opaque,
                "normal draw-buffer opa-before-volume-shadow should draw MapObj opaque before later actor buffers");
        require(trace[1].name == "npc-draw-probe" && trace[1].phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa &&
                    trace[1].draw_buffer_type == MR::DrawBufferType_NPC,
                "normal draw-buffer opa list should draw NPC opaque before deferred sky when prior air is off");
        require(trace[2].name == "sky-draw-probe" && trace[2].phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa &&
                    trace[2].draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Opaque && trace[3].name == "sky-draw-probe" &&
                    trace[3].phase == smgpc::game::SceneSchedulerPhase::DrawBufferXlu &&
                    trace[3].draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Translucent,
                "normal draw-buffer opa list should draw deferred Sky opaque and translucent passes together");
        require(trace[4].name == "map-draw-probe" && trace[4].phase == smgpc::game::SceneSchedulerPhase::DrawBufferXlu &&
                    trace[5].name == "npc-draw-probe" && trace[5].phase == smgpc::game::SceneSchedulerPhase::DrawBufferXlu,
                "normal xlu draw-buffer list should revisit map objects before NPC");
    }

    void test_runtime_context_routes_layout_and_sky_through_scene_scheduler() {
        auto logger = NullLogger();
        auto window = TestWindowService();
        auto runtime = smgpc::game::RuntimeContext(logger, window);
        auto renderer = RecordingRenderer();

        {
            auto layout = SimpleLayout("TitleLogoProbe", "TitleLogo", 1U, MR::DrawType_Layout);
            auto sky = FileSelectSky("ファイル選択空");

            sky.initWithoutIter();
            layout.appear();
            layout.startAnim("Appear", 0U);
            sky.appear();

            const auto snapshot = runtime.scheduler().snapshot();
            const auto has_layout = std::ranges::any_of(snapshot, [](const auto &entry) {
                return entry.kind == smgpc::game::SceneEntryKind::Layout && entry.name == "TitleLogoProbe" &&
                       entry.movement_type == MR::MovementType_Layout;
            });
            const auto has_sky = std::ranges::any_of(snapshot, [](const auto &entry) {
                return entry.kind == smgpc::game::SceneEntryKind::LiveActorModel && entry.name == "ファイル選択空" &&
                       entry.draw_buffer_type == MR::DrawBufferType_Sky;
            });
            require(has_layout && has_sky, "runtime should register layouts and sky actors in the central scene scheduler");

            const auto frame_context = smgpc::render::FrameContext{
                .frame_index = 20U,
                .frame_time_seconds = 20.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            };
            runtime.begin_frame(frame_context);
            const auto trace = runtime.scheduler().last_execution_trace();
            require(!trace.empty() && trace.front().kind == smgpc::game::SceneEntryKind::Layout &&
                        trace.front().phase == smgpc::game::SceneSchedulerPhase::Movement,
                    "runtime scene scheduler should execute layout category before sky movement");
            require(std::ranges::any_of(trace, [](const auto &entry) {
                        return entry.name == "ファイル選択空" && entry.phase == smgpc::game::SceneSchedulerPhase::CalcViewAndEntry;
                    }),
                    "runtime scene scheduler should run sky calcViewAndEntry during the frame phase");
            require(sky.getNerveStep() == 1, "runtime scene scheduler should still move the sky actor nerve");
            require(sky.getBaseMatrix().m == smgpc::game::file_select_sky_actor_matrix(0U).m,
                    "runtime scene scheduler should update sky base matrix during calcViewAndEntry");
            const auto layout_runtime = runtime.scheduler().debug_layout_runtime_snapshot();
            const auto title_logo_runtime = std::ranges::find_if(layout_runtime, [](const auto &entry) {
                return entry.name == "TitleLogoProbe" && entry.layout_name == "TitleLogo";
            });
            require(title_logo_runtime != layout_runtime.end() && !title_logo_runtime->dead && !title_logo_runtime->animations.empty() &&
                        title_logo_runtime->animations[0U].name == "Appear" && title_logo_runtime->animations[0U].rate == 1.0F &&
                        !title_logo_runtime->animations[0U].stopped,
                    "runtime scene scheduler should expose debug-only layout animation state for parity traces");
            require_near(title_logo_runtime->animations[0U].frame, 1.0F, 0.001F,
                         "runtime layout animation evidence should reflect scheduler movement updates");

            runtime.draw_3d_normal(renderer, smgpc::game::file_select_title_camera_pose());
            const auto draw_trace = runtime.scheduler().last_execution_trace();
            require(std::ranges::any_of(draw_trace,
                                        [](const auto &entry) {
                                            return entry.name == "ファイル選択空" &&
                                                   entry.phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa &&
                                                   entry.draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Opaque;
                                        }) &&
                        std::ranges::any_of(draw_trace,
                                            [](const auto &entry) {
                                                return entry.name == "ファイル選択空" &&
                                                       entry.phase == smgpc::game::SceneSchedulerPhase::DrawBufferXlu &&
                                                       entry.draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Translucent;
                                            }),
                    "runtime scene scheduler should tag original sky draw-buffer opa/xlu passes in the execution trace");
            require(renderer.triangle_batch_count > 0U, "runtime scene scheduler should draw sky actors through the sky draw buffer");

            runtime.draw_2d_normal(renderer);
            const auto trace_path = std::filesystem::path(".cache/tests/runtime-parity-trace.json");
            smgpc::game::write_runtime_parity_trace(trace_path, frame_context, runtime);
            const auto trace_bytes = read_file(trace_path);
            const auto trace_json = std::string(reinterpret_cast<const char *>(trace_bytes.data()), trace_bytes.size());
            require(trace_json.find("\"schema\":\"smgpc-runtime-parity-trace-v1\"") != std::string::npos,
                    "runtime parity trace should write a stable schema identifier");
            require(trace_json.find("\"camera_pose\"") != std::string::npos && trace_json.find("\"scene_trace\"") != std::string::npos,
                    "runtime parity trace should include camera pose and scene execution evidence");
            require(trace_json.find("\"layout_runtime\"") != std::string::npos &&
                        trace_json.find("\"layout_name\":\"TitleLogo\"") != std::string::npos &&
                        trace_json.find("\"animations\"") != std::string::npos && trace_json.find("\"name\":\"Appear\"") != std::string::npos,
                    "runtime parity trace should include layout animation frame evidence");
            require(trace_json.find("\"DrawBufferOpa\"") != std::string::npos && trace_json.find("\"DrawBufferXlu\"") != std::string::npos &&
                        trace_json.find("\"DrawType\"") != std::string::npos,
                    "runtime parity trace should include original-style draw-buffer and draw-type phases");
            require(trace_json.find("\"render_packets\"") != std::string::npos &&
                        trace_json.find("\"model_name\":\"CometNearOrbitSky\"") != std::string::npos &&
                        trace_json.find("\"material_name\":\"Space_Mat_v\"") != std::string::npos &&
                        trace_json.find("\"material_name\":\"CometHalo_v\"") != std::string::npos &&
                        trace_json.find("\"packet_mode\":\"ShaderGxTev\"") != std::string::npos &&
                        trace_json.find("\"packet_mode\":\"ComposedMaterial\"") != std::string::npos &&
                        trace_json.find("\"indirect_stage_count\":1") != std::string::npos &&
                        trace_json.find("\"fog_type\"") != std::string::npos &&
                        trace_json.find("\"bck_active\":true") != std::string::npos &&
                        trace_json.find("\"bck_frame_max\":3000") != std::string::npos &&
                        trace_json.find("\"btk_active\":true") != std::string::npos &&
                        trace_json.find("\"btk_frame_max\":10000") != std::string::npos &&
                        trace_json.find("\"draw_pass\":\"Opaque\"") != std::string::npos,
                    "runtime parity trace should include submitted J3D material packet sequence, animation frames, and draw state");
        }

        require(runtime.scheduler().snapshot().empty(), "runtime scene scheduler should unregister destroyed layout and sky actor entries");
    }

    void test_file_selector_title_autorush_gate() {
        auto selector = FileSelector();
        require(selector.getSkyStep() == 0U, "FileSelector sky actor step should start at zero");
        require(selector.getItemCount() == 6, "FileSelector should create the six original file-select item slots");
        require(selector.getItemFileNo(0) == 1 && selector.getItemFileNo(1) == 2 && selector.getItemFileNo(2) == 4 &&
                    selector.getItemFileNo(3) == 6 && selector.getItemFileNo(4) == 5 && selector.getItemFileNo(5) == 3,
                "FileSelector should preserve the original file-number order table");
        require(!selector.isTitleStarted(), "FileSelector title should not start before AutoRushBegin");
        require(!selector.isTitleActive(), "FileSelector::createTitle should keep TitleSequenceProduct killed before AutoRushBegin");
        require(!selector.receiveOtherMsg(0U), "FileSelector should reject unrelated messages");

        const auto &item0 = selector.getItemBasePosition(0);
        require_near(item0.x, -1710.1007F, 0.01F, "FileSelector item 0 base X should match original ring math");
        require_near(item0.y, -1606.9690F, 0.01F, "FileSelector item 0 base Y should match original 20-degree ring pitch");
        require_near(item0.z, 4415.1113F, 0.01F, "FileSelector item 0 base Z should match original ring math");
        const auto &item2 = selector.getItemBasePosition(2);
        require_near(item2.x, 5000.0F, 0.01F, "FileSelector item 2 base X should match original ring radius");
        require_near(item2.y, 0.0F, 0.01F, "FileSelector item 2 base Y should match original ring pitch");
        require_near(item2.z, 0.0F, 0.01F, "FileSelector item 2 base Z should match original ring pitch");

        selector.update();
        require(!selector.isTitleStarted(), "FileSelector WaitBind should not start the title by itself");
        const auto &item0_position = selector.getItemPosition(0);
        require_near(item0_position.x, item0.x * 0.05F, 0.01F, "FileSelector item position should ease toward original base X");
        require_near(item0_position.y, item0.y * 0.05F, 0.01F, "FileSelector item position should ease toward original base Y");
        require_near(item0_position.z, item0.z * 0.05F, 0.01F, "FileSelector item position should ease toward original base Z");
        require(selector.receiveOtherMsg(ACTMES_UPDATE_BASEMTX), "FileSelector should accept UpdateBaseMtx messages");
        require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should accept AutoRushBegin while waiting for bind");
        require(!selector.isTitleStarted(), "FileSelector should defer title startup until the Title nerve first step");

        selector.update();
        require(selector.isTitleStarted(), "FileSelector should start TitleSequenceProduct on the first Title nerve step");
        require(selector.isTitleActive(), "FileSelector title should be active after AutoRushBegin");
        require(!selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should not restart title outside WaitBind");
    }

    void test_file_selector_title_end_file_select_transition() {
        auto logger = NullLogger();
        auto window = TestWindowService();
        auto runtime = smgpc::game::RuntimeContext(logger, window);
        auto selector = FileSelector();

        require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should accept AutoRushBegin for title-end integration test");

        auto saw_title_end = false;
        auto saw_camera_transition_before_far_point = false;

        for (std::uint64_t frame = 0; frame < 720U && !selector.isFileSelectStarted(); ++frame) {
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = frame,
                .frame_time_seconds = static_cast<double>(frame) / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });

            window.set_title_combo(frame >= 160U);
            selector.update();

            if (selector.isTitleEnded()) {
                saw_title_end = true;
                saw_camera_transition_before_far_point = saw_camera_transition_before_far_point || !selector.isCameraAtFarPoint();
            }
        }

        require(saw_title_end, "FileSelector should enter TitleEnd after the original title sequence is decided");
        require(saw_camera_transition_before_far_point,
                "FileSelector TitleEnd should wait for camera far-point transition instead of finishing immediately");
        require(selector.didAppearAllItems(), "FileSelector TitleEnd should appear file-select items on first step");
        require(selector.getAppearedItemCount() == 6, "FileSelector TitleEnd should fan out appear to all six items");
        require(selector.didInitAllItems(), "FileSelector TitleEnd should initialize file-select items on first step");
        require(selector.didInvalidateSelectAll(), "FileSelector TitleEnd should invalidate file-select item selection on first step");
        require(selector.getSelectInvalidItemCount() == 6, "FileSelector TitleEnd should fan out select invalidation to all six items");
        require(selector.getMiiValidIndexCollectionCount() == 1, "FileSelector TitleEnd should collect valid Mii indices once");
        require(selector.getBasePosRatio() == 0.0F, "FileSelector TitleEnd should calculate base positions with ratio 0");
        require(selector.isCameraAtFarPoint(), "FileSelector camera should reach far point before FileSelect starts");
        require(selector.didValidateRotateAllItems(), "FileSelector should validate item rotation after the camera reaches far point");
        require(selector.getRotateInvalidItemCount() == 0, "FileSelector should fan out rotate validation to all six items");
        require(selector.isFileSelectStarted(), "FileSelector should enter the FileSelect nerve after TitleEnd camera completion");
        require(runtime.current_stage_bgm_name() == "MBGM_FILE_SELECT", "FileSelector TitleEnd should start the original file-select BGM");
    }

}  // namespace

int main() try {
    const auto root = disc_files_root();
    test_yaz0_decompression(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
    test_rarc_title_archives(root);
    test_tpl_title_texture_decode(root);
    test_brlyt_title_picture_parse(root);
    test_simple_layout_title_logo_uses_gx_tev_material_batches();
    test_brlyt_press_start_text_parse(root);
    test_brfnt_message_font_decode(root);
    test_bcsv_file_select_camera_parse(root);
    test_camera_param_file_select_chunk_load(root);
    test_file_select_title_camera_pose();
    test_jmath_short_trig_compat();
    test_file_select_sky_runtime();
    test_brlan_title_animation_parse(root);
    test_brlan_press_start_animation_parse(root);
    test_j3d_comet_near_orbit_sky_textures(root);
    test_gx_state_decodes_effective_mdl3_material_state();
    test_j3d_comet_near_orbit_sky_model_probe(root);
    test_j3d_comet_near_orbit_sky_animation_probe(root);
    test_j3d_model_renderer_comet_near_orbit_sky_keeps_shape_meshes(root);
    test_j3d_model_renderer_file_select_planet(root);
    test_file_select_item_draws_original_planet_model();
    test_file_select_sky_draws_through_live_actor_model_compat();
    test_png_screenshot_service();
    test_j3d_single_texture_tev_composition();
    test_wii_logical_render_viewport();
    test_runtime_services_dvd_archive_cache(root);
    test_runtime_services_wpad_save_message_rfl();
    test_runtime_context_routes_through_services();
    test_scene_scheduler_orders_original_style_categories();
    test_scene_scheduler_executes_original_normal_draw_buffer_passes();
    test_runtime_context_routes_layout_and_sky_through_scene_scheduler();
    test_file_selector_title_autorush_gate();
    test_file_selector_title_end_file_select_transition();
    std::cout << "compat layer tests passed\n";
    return 0;
} catch (const std::exception &e) {
    std::cerr << "compat layer tests failed: " << e.what() << '\n';
    return 1;
}
