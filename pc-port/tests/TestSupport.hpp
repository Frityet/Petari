#pragma once

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/FileSelectEffect.hpp"
#include "Game/Map/FileSelectItem.hpp"
#include "Game/Map/FileSelectSky.hpp"
#include "Game/Map/FileSelector.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/BackButton.hpp"
#include "Game/Screen/BrosButton.hpp"
#include "Game/Screen/ButtonPaneController.hpp"
#include "Game/Screen/FileSelectButton.hpp"
#include "Game/Screen/FileSelectInfo.hpp"
#include "Game/Screen/FileSelectNumber.hpp"
#include "Game/Screen/InformationMessage.hpp"
#include "Game/Screen/Manual2P.hpp"
#include "Game/Screen/MiiConfirmIcon.hpp"
#include "Game/Screen/MiiSelect.hpp"
#include "Game/Screen/SaveIcon.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Screen/SysInfoWindow.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/System/NANDManager.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/SaveDataHandler.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/LightUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
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
#include <cctype>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace smgpc::tests {

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
                return _hold_core_pad_a;
            case smgpc::render::InputButton::CORE_PAD_B:
                return _hold_core_pad_b;
            }

            return false;
        }

        void set_title_combo(bool is_pressed) {
            set_core_buttons(is_pressed, is_pressed);
        }

        void set_core_buttons(bool hold_a, bool hold_b) {
            _hold_core_pad_a = hold_a;
            _hold_core_pad_b = hold_b;
        }

    private:
        bool _hold_core_pad_a = false;
        bool _hold_core_pad_b = false;
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

        bool receiveOtherMsg(u32 msg, HitSensor *pSender, HitSensor *pReceiver) override {
            ++message_count;
            last_message = msg;
            last_sender = pSender;
            last_receiver = pReceiver;
            last_sender_host = MR::getSensorHost(pSender);
            last_receiver_host = MR::getSensorHost(pReceiver);
            return accept_messages;
        }

        int calc_anim_count = 0;
        int control_count = 0;
        int calc_view_count = 0;
        int message_count = 0;
        u32 last_message = 0U;
        HitSensor *last_sender = nullptr;
        HitSensor *last_receiver = nullptr;
        LiveActor *last_sender_host = nullptr;
        LiveActor *last_receiver_host = nullptr;
        bool accept_messages = false;
    };

    [[nodiscard]] inline std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
        if (offset + 4U > data.size()) {
            throw std::runtime_error("read_be32 out of range");
        }

        return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
               (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
    }

    [[nodiscard]] inline std::vector<std::uint8_t> read_file(const std::filesystem::path &path) {
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

    class ScopedCurrentPath final {
    public:
        explicit ScopedCurrentPath(const std::filesystem::path &path) : _previous(std::filesystem::current_path()) {
            std::filesystem::current_path(path);
        }

        ~ScopedCurrentPath() {
            std::error_code error{};
            std::filesystem::current_path(_previous, error);
        }

        ScopedCurrentPath(const ScopedCurrentPath &) = delete;
        ScopedCurrentPath &operator=(const ScopedCurrentPath &) = delete;

    private:
        std::filesystem::path _previous;
    };

    [[nodiscard]] inline std::filesystem::path weakly_canonical_or_normal(const std::filesystem::path &path) {
        std::error_code error{};
        const auto canonical = std::filesystem::weakly_canonical(path, error);
        if (!error) {
            return canonical;
        }

        return path.lexically_normal();
    }

    inline void append_disc_root_candidates_from_anchor(std::vector<std::filesystem::path> &candidates, const std::filesystem::path &anchor) {
        auto directory = weakly_canonical_or_normal(anchor);
        while (!directory.empty()) {
            candidates.push_back(directory / "orig" / "RMGK01" / "files");
            if (directory == directory.root_path()) {
                break;
            }

            directory = directory.parent_path();
        }
    }

    [[nodiscard]] inline std::filesystem::path disc_files_root() {
        const auto cwd = std::filesystem::current_path();
        auto candidates = std::vector<std::filesystem::path>{};
        append_disc_root_candidates_from_anchor(candidates, cwd);

        for (const auto &candidate : candidates) {
            std::error_code error{};
            const auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        throw std::runtime_error("could not locate orig/RMGK01/files from " + cwd.string());
    }

    inline void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    inline void require_near(float actual, float expected, float tolerance, std::string_view message) {
        if (std::abs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message));
        }
    }

    inline void require_tev_stage(const smgpc::game::J3dTevStageSummary &stage, std::array<std::uint8_t, 4U> color_in, std::uint8_t k_color_sel,
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

    inline void require_magic(std::span<const std::uint8_t> data, std::string_view magic) {
        require(data.size() >= magic.size(), "data too short for magic");
        for (std::size_t i = 0; i < magic.size(); ++i) {
            require(data[i] == static_cast<std::uint8_t>(magic[i]), "unexpected magic");
        }
    }

    [[nodiscard]] inline std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] inline std::string base_name(std::string_view path) {
        const auto slash = path.find_last_of('/');
        if (slash == std::string_view::npos) {
            return std::string(path);
        }

        return std::string(path.substr(slash + 1U));
    }

    [[nodiscard]] inline const smgpc::game::RarcEntry *find_entry_by_basename(const smgpc::game::RarcArchive &archive, std::string_view name) {
        const auto requested = lower_copy(name);
        const auto it =
            std::ranges::find_if(archive.entries(), [&requested](const auto &entry) { return lower_copy(base_name(entry.path)) == requested; });

        return it == archive.entries().end() ? nullptr : &(*it);
    }

    using RegisteredTestFunction = void (*)();

    struct RegisteredTest {
        std::string_view suite;
        std::string_view name;
        std::string_view file;
        int line = 0;
        RegisteredTestFunction run = nullptr;
    };

    [[nodiscard]] inline std::vector<RegisteredTest> &registered_tests() {
        static auto tests = std::vector<RegisteredTest>{};
        return tests;
    }

    class TestRegistrar final {
    public:
        TestRegistrar(std::string_view suite, std::string_view name, std::string_view file, int line, RegisteredTestFunction run) {
            registered_tests().push_back(RegisteredTest{
                .suite = suite,
                .name = name,
                .file = file,
                .line = line,
                .run = run,
            });
        }
    };

    inline void run_registered_tests(std::string_view suite) {
        auto selected = std::vector<const RegisteredTest *>{};
        for (const auto &test : registered_tests()) {
            if (test.suite == suite) {
                selected.push_back(&test);
            }
        }

        std::ranges::stable_sort(selected, [](const auto *left, const auto *right) {
            if (left->file != right->file) {
                return left->file < right->file;
            }

            return left->line < right->line;
        });

        require(!selected.empty(), "test suite did not register any tests");
        for (const auto *test : selected) {
            std::cout << "[test] " << test->suite << " / " << test->name << '\n';
            test->run();
        }
    }

    inline int run_named_test_suite(std::string_view label, void (*run)()) {
        try {
            run();
            std::cout << label << " tests passed\n";
            return 0;
        } catch (const std::exception &e) {
            std::cerr << label << " tests failed: " << e.what() << '\n';
            return 1;
        }
    }

}  // namespace smgpc::tests

#define $test(description)                                                              \
    template <>                                                                         \
    struct TestCase<__LINE__> {                                                         \
        static void run();                                                              \
        [[maybe_unused]] static const ::smgpc::tests::TestRegistrar registrar;          \
    };                                                                                  \
    [[maybe_unused]] const ::smgpc::tests::TestRegistrar TestCase<__LINE__>::registrar{ \
        kTestSuite, description, __FILE__, __LINE__, &TestCase<__LINE__>::run};         \
    void TestCase<__LINE__>::run()
