#include "Game/Screen/TitleSequenceProduct.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cstdlib>
#include <exception>
#include <memory>

namespace {

    class ProbeWindowService final : public smgpc::render::IWindowService {
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
            return {.width = 800U, .height = 600U};
        }

        [[nodiscard]] smgpc::render::NativeWindowHandle native_handle() const override {
            return {};
        }

        [[nodiscard]] bool is_input_pressed(smgpc::render::InputButton button) const override {
            switch (button) {
            case smgpc::render::InputButton::CORE_PAD_A:
            case smgpc::render::InputButton::CORE_PAD_B:
                return _hold_core_ab;
            case smgpc::render::InputButton::CORE_PAD_UP:
            case smgpc::render::InputButton::CORE_PAD_DOWN:
            case smgpc::render::InputButton::CORE_PAD_LEFT:
            case smgpc::render::InputButton::CORE_PAD_RIGHT:
            case smgpc::render::InputButton::CORE_PAD_PLUS:
            case smgpc::render::InputButton::CORE_PAD_MINUS:
            case smgpc::render::InputButton::CORE_PAD_HOME:
            case smgpc::render::InputButton::CORE_PAD_C:
            case smgpc::render::InputButton::CORE_PAD_Z:
            case smgpc::render::InputButton::COUNT:
                return false;
            default:
                return false;
            }
        }

        void set_core_ab_buttons(bool is_pressed) {
            _hold_core_ab = is_pressed;
        }

    private:
        bool _hold_core_ab = false;
    };

    [[nodiscard]] bool has_magic(std::span<const std::uint8_t> data, const char (&magic)[5]) {
        return data.size() >= 4U && data[0] == static_cast<std::uint8_t>(magic[0]) && data[1] == static_cast<std::uint8_t>(magic[1]) && data[2] == static_cast<std::uint8_t>(magic[2]) && data[3] == static_cast<std::uint8_t>(magic[3]);
    }

    [[nodiscard]] int run_probe() {
        auto logger = smgpc::logging::create_default_logger();
        auto window = ProbeWindowService();
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);

        const auto title_logo = runtime.find_layout_archive("TitleLogo");
        const auto press_start = runtime.find_layout_archive("PressStart");
        if (!title_logo.has_value() || !press_start.has_value()) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message{"Title sequence probe could not resolve TitleLogo/PressStart original archives"});
            return 1;
        }

        const auto title_logo_archive = smgpc::resource::RarcArchive::from_file(*title_logo);
        const auto press_start_archive = smgpc::resource::RarcArchive::from_file(*press_start);
        if (!title_logo_archive.contains("blyt/titlelogo.brlyt") || !title_logo_archive.contains("anim/appear.brlan") || !title_logo_archive.contains("anim/wait.brlan") || !title_logo_archive.contains("anim/decide.brlan")) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message{"TitleLogo.arc does not contain the expected original title layout/animation files"});
            return 1;
        }
        if (!press_start_archive.contains("blyt/pressstart.brlyt") || !press_start_archive.contains("anim/appear.brlan") || !press_start_archive.contains("anim/wait.brlan") || !press_start_archive.contains("anim/end.brlan")) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message{"PressStart.arc does not contain the expected original prompt layout/animation files"});
            return 1;
        }
        if (!has_magic(title_logo_archive.file_data("blyt/titlelogo.brlyt"), "RLYT") || !has_magic(title_logo_archive.file_data("anim/appear.brlan"), "RLAN")) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message{"TitleLogo.arc contains unexpected layout/animation magic"});
            return 1;
        }
        if (!has_magic(press_start_archive.file_data("blyt/pressstart.brlyt"), "RLYT") || !has_magic(press_start_archive.file_data("anim/appear.brlan"), "RLAN")) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message{"PressStart.arc contains unexpected layout/animation magic"});
            return 1;
        }
        logger->info(smgpc::logging::Category::APP, smgpc::logging::Message{"Decoded TitleLogo.arc entries: {}, PressStart.arc entries: {}"}, title_logo_archive.entries().size(), press_start_archive.entries().size());

        auto title_sequence = TitleSequenceProduct();
        title_sequence.appear();

        constexpr auto kMaxFrames = 450U;
        constexpr auto kPressComboFrame = 270U;

        for (auto frame = 0U; frame < kMaxFrames; ++frame) {
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = frame,
                .frame_time_seconds = static_cast<double>(frame) / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 800U, .height = 600U},
                .has_focus = true,
                .is_minimized = false,
            });

            window.set_core_ab_buttons(frame >= kPressComboFrame);
            title_sequence.updateNerve();

            if (!title_sequence.isActive()) {
                logger->info(smgpc::logging::Category::APP, smgpc::logging::Message{"Title sequence probe completed at frame {}"}, frame);
                return 0;
            }
        }

        logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message{"Title sequence probe did not reach dead state within {} frames"}, kMaxFrames);
        return 1;
    }

}  // namespace

int main() try {
    return run_probe();
} catch (const std::exception &e) {
    auto logger = smgpc::logging::create_default_logger();
    logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message{"Title sequence probe failed: {}"}, e.what());
    return 1;
}
