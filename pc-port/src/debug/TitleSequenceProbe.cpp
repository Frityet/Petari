#include "Game/Screen/TitleSequenceProduct.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

#include <aurora/dvd.h>

#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>

namespace {
    [[nodiscard]] bool has_magic(std::span<const std::uint8_t> data, const char (&magic)[5]) {
        return data.size() >= 4U && data[0] == static_cast<std::uint8_t>(magic[0]) && data[1] == static_cast<std::uint8_t>(magic[1]) && data[2] == static_cast<std::uint8_t>(magic[2]) && data[3] == static_cast<std::uint8_t>(magic[3]);
    }

    [[nodiscard]] int run_probe() {
        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 800,
            .height = 600,
            .title = "SMG PC Port title sequence probe",
        });

        const auto* disc_image = std::getenv("SMGPC_DISC_IMAGE");
        if (disc_image == nullptr || disc_image[0] == '\0') {
            throw std::runtime_error("SMGPC_DISC_IMAGE must name a real disc image");
        }
        if (!aurora_dvd_open(disc_image)) {
            throw std::runtime_error("Aurora could not open the requested disc image");
        }

        auto resource_runtime = smgpc::resource::GameResourceRuntime{};

        auto runtime = smgpc::runtime::RuntimeContext(*logger, window, resource_runtime);

        const auto title_logo = runtime.find_layout_archive("TitleLogo");
        const auto press_start = runtime.find_layout_archive("PressStart");
        if (!title_logo.has_value() || !press_start.has_value()) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message {"Title sequence probe could not resolve TitleLogo/PressStart original archives"});
            return 1;
        }

        const auto title_logo_archive = smgpc::resource::RarcArchive::from_bytes(runtime.dvd().read_file(title_logo->generic_string()));
        const auto press_start_archive = smgpc::resource::RarcArchive::from_bytes(runtime.dvd().read_file(press_start->generic_string()));
        if (!title_logo_archive.contains("blyt/titlelogo.brlyt") || !title_logo_archive.contains("anim/appear.brlan") || !title_logo_archive.contains("anim/wait.brlan") || !title_logo_archive.contains("anim/decide.brlan")) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message {"TitleLogo.arc does not contain the expected original title layout/animation files"});
            return 1;
        }
        if (!press_start_archive.contains("blyt/pressstart.brlyt") || !press_start_archive.contains("anim/appear.brlan") || !press_start_archive.contains("anim/wait.brlan") || !press_start_archive.contains("anim/end.brlan")) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message {"PressStart.arc does not contain the expected original prompt layout/animation files"});
            return 1;
        }
        if (!has_magic(title_logo_archive.file_data("blyt/titlelogo.brlyt"), "RLYT") || !has_magic(title_logo_archive.file_data("anim/appear.brlan"), "RLAN")) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message {"TitleLogo.arc contains unexpected layout/animation magic"});
            return 1;
        }
        if (!has_magic(press_start_archive.file_data("blyt/pressstart.brlyt"), "RLYT") || !has_magic(press_start_archive.file_data("anim/appear.brlan"), "RLAN")) {
            logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message {"PressStart.arc contains unexpected layout/animation magic"});
            return 1;
        }
        logger->info(smgpc::logging::Category::APP, smgpc::logging::Message {"Decoded TitleLogo.arc entries: {}, PressStart.arc entries: {}"}, title_logo_archive.entries().size(), press_start_archive.entries().size());

        auto title_sequence = TitleSequenceProduct();
        title_sequence.appear();

        constexpr auto kMaxFrames = 450U;
        constexpr auto kPressComboFrame = 270U;

        for (auto frame = 0U; frame < kMaxFrames; ++frame) {
            runtime.begin_frame(smgpc::render::FrameContext {
                .frame_index = frame,
                .frame_time_seconds = static_cast<double>(frame) / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 800U, .height = 600U},
                .has_focus = true,
                .is_minimized = false,
            });

            runtime.wpad().set_button_mask(
                WPAD_CHAN0,
                frame >= kPressComboFrame ? static_cast<u32>(WPAD_BUTTON_A | WPAD_BUTTON_B) : 0U);
            title_sequence.updateNerve();

            if (!title_sequence.isActive()) {
                logger->info(smgpc::logging::Category::APP, smgpc::logging::Message {"Title sequence probe completed at frame {}"}, frame);
                return 0;
            }
        }

        logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message {"Title sequence probe did not reach dead state within {} frames"}, kMaxFrames);
        return 1;
    }

}  // namespace

int main() try {
    return run_probe();
} catch (const std::exception &e) {
    auto logger = smgpc::logging::create_default_logger();
    logger->fatal(smgpc::logging::Category::APP, smgpc::logging::Message {"Title sequence probe failed: {}"}, e.what());
    return 1;
}
