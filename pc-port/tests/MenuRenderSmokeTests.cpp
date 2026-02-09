#include "menu/MenuDirector.hpp"
#include "render/BgfxRenderer.hpp"
#include "tests/TestCommon.hpp"
#include "tests/TestHarness.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

namespace {

struct MenuRenderRunResult {
    bool sawTitleAppear = false;
    bool sawTitleLoop = false;
    bool sawTitleDecide = false;
    bool sawTitleDone = false;
    std::uint64_t hashTitleAppear = 0U;
    std::uint64_t hashTitleLoop = 0U;
    std::uint64_t hashTitleDecide = 0U;
};

const MenuRenderRunResult& RunMenuRenderScenario() {
    static const MenuRenderRunResult result = [] {
        MenuRenderRunResult run;

        const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();

        pcport::MenuDirectorConfig config;
        config.bootDurationMs = 1;
        config.autoAdvanceMs = 1000;

        pcport::MenuDirector director = pcport::MenuDirector::Create(assets, config);
        pcport::BgfxRenderer renderer(608, 456);

        bool requestedAdvance = false;

        for (int frame = 0; frame < 3000; ++frame) {
            director.SetButtonState(false, false);
            director.Update(1000.0F / 60.0F);
            renderer.BeginFrame();
            renderer.Clear(0U, 0U, 0U, 255U);
            director.Render(renderer);
            renderer.EndFrame();

            if (director.GetState() == pcport::MenuState::TitleAppear) {
                run.sawTitleAppear = true;
                if (run.hashTitleAppear == 0U && director.GetStateFrame() >= 15.0F) {
                    run.hashTitleAppear = renderer.ComputeHash(pcport::CaptureSurface::Internal);
                }
            } else if (director.GetState() == pcport::MenuState::TitleLoop) {
                run.sawTitleLoop = true;
                if (!requestedAdvance && director.GetStateFrame() >= 20.0F) {
                    director.RequestAdvance();
                    requestedAdvance = true;
                }
                if (run.hashTitleLoop == 0U && director.GetStateFrame() >= 15.0F) {
                    run.hashTitleLoop = renderer.ComputeHash(pcport::CaptureSurface::Internal);
                }
            } else if (director.GetState() == pcport::MenuState::TitleDecide) {
                run.sawTitleDecide = true;
                if (run.hashTitleDecide == 0U && director.GetStateFrame() >= 10.0F) {
                    run.hashTitleDecide = renderer.ComputeHash(pcport::CaptureSurface::Internal);
                }
            } else if (director.GetState() == pcport::MenuState::TitleDone) {
                run.sawTitleDone = true;
                break;
            }
        }

        return run;
    }();

    return result;
}

std::filesystem::path MakeCaptureDir(const std::string& suffix) {
    const auto ticks = static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count());
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / ("pc_port_layer_capture_" + suffix + "_" + std::to_string(ticks));
    std::filesystem::create_directories(dir);
    return dir;
}

$pc_port_test(MenuRenderReachesExpectedStates) {
    const MenuRenderRunResult& run = RunMenuRenderScenario();
    $pc_port_require(run.sawTitleAppear);
    $pc_port_require(run.sawTitleLoop);
    $pc_port_require(run.sawTitleDecide);
    $pc_port_require(run.sawTitleDone);
}

$pc_port_test(MenuRenderProducesPhaseHashes) {
    const MenuRenderRunResult& run = RunMenuRenderScenario();
    $pc_port_require(run.hashTitleAppear != 0U);
    $pc_port_require(run.hashTitleLoop != 0U);
    $pc_port_require(run.hashTitleDecide != 0U);
}

$pc_port_test(MenuRenderHashesChangeAcrossPhases) {
    const MenuRenderRunResult& run = RunMenuRenderScenario();
    $pc_port_require(run.hashTitleAppear != run.hashTitleLoop);
    $pc_port_require(run.hashTitleLoop != run.hashTitleDecide);
}

$pc_port_test(MenuRenderCapturesAllLayersAsPngAndPpm) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();

    pcport::MenuDirectorConfig config;
    config.bootDurationMs = 1;
    config.autoAdvanceMs = 1000;
    pcport::MenuDirector director = pcport::MenuDirector::Create(assets, config);
    pcport::BgfxRenderer renderer(608, 456);

    for (int frame = 0; frame < 40; ++frame) {
        director.SetButtonState(false, false);
        director.Update(1000.0F / 60.0F);
        renderer.BeginFrame();
        renderer.Clear(0U, 0U, 0U, 255U);
        director.Render(renderer);
        renderer.EndFrame();
    }

    const int layerCount = renderer.GetCapturedLayerCount();
    $pc_port_require(layerCount >= 1);

    const std::filesystem::path captureDir = MakeCaptureDir("menu");
    const bool pngOk = renderer.SaveAllLayersPng(captureDir, "capture");
    const bool ppmOk = renderer.SaveAllLayersPpm(captureDir, "capture");
    $pc_port_require(pngOk);
    $pc_port_require(ppmOk);

    $pc_port_require(std::filesystem::exists(captureDir / "capture_presented.png"));
    $pc_port_require(std::filesystem::exists(captureDir / "capture_presented.ppm"));

    for (int i = 0; i < layerCount; ++i) {
        $pc_port_require(renderer.ComputeLayerHash(i) != 0U);
    }

    std::error_code ec;
    std::filesystem::remove_all(captureDir, ec);
}

}  // namespace
