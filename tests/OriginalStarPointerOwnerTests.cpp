#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/StageSessionState.hpp"
#include "compat/StarPointerDepthOwnership.hpp"
#include "runtime/RuntimeContext.hpp"
#include "Game/Screen/StarPointerBlur.hpp"
#include "Game/Screen/StarPointerController.hpp"
#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/Screen/StarPointerGuidance.hpp"
#include "Game/Screen/StarPointerLayout.hpp"
#include "Game/System/StarPointerOnOffController.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) aurora::throw_host_exception<std::runtime_error>(message);
}
class Logger final : public smgpc::logging::ILogger {
    void write(std::FILE*, std::source_location, smgpc::logging::Level,
               smgpc::logging::Category, std::string_view) override {}
};
void modes_and_resources(smgpc::runtime::RuntimeContext& runtime) {
    auto& owner = smgpc::compat::require_star_pointer_depth();
    auto& director = owner.director();
    auto& modes = owner.modes();
    require(director.mStarPointerLayouts == nullptr, "actual layouts are created at the process layout boundary, after resources exist");
    smgpc::compat::StageSessionState outer("Game", "HeavensDoorGalaxy", 1, JMapIdInfo(0, 0));
    {
        smgpc::compat::StageSessionBinding outer_binding(outer);
        require(director.mStarPointerLayouts && director.mGuidance == owner.guidance() &&
                director.mControllers == &owner.controller(0) && director.mPeekZ->mInfos[1] == &owner.controller(1).mInfo,
                "the actual original director owns both controllers, layouts, guidance, and callback records");
        auto* guidance = owner.guidance();
        require(guidance->mSpineFrame1P && guidance->mSpineGuidance && guidance->mSpineFrame2P,
                "PointerGuidance.arc creates all three real original guidance state machines");
        for (int port = 0; port < 2; ++port) {
            const auto* layout = director.getStarPointerLayout(port);
            require(layout->mPadChannel == port && layout->mDirector == &director && layout->mCommandStream &&
                    layout->mNumber && layout->mBlur && layout->mBlur->mTexture,
                    "both complete original cursor resource graphs retain their original port and director");
            require(layout->mBlur->mTexture->getWidth() == 64 && layout->mBlur->mTexture->getHeight() == 64,
                    "both blur textures decode the actual 64x64 StarPointerBlur.arc/Blur.bti asset");
        }
        require(modes.mMode == StarPointerMode_Game && modes.mModeCounter[StarPointerMode_Game] == 1 &&
                MR::isStarPointerValid(0) && MR::isStarPointerValid(1) && director.isEnableStarPointerShootStarPiece(),
                "actual Base-to-Game entry enables original layouts and shooting");
        require(!MR::isExistStarPointerGuidance() && !MR::isExistStarPointerGuidanceFrame1P(),
                "initialized EndWait spines report no guidance or frame before requests");
        aurora::wpad_service().set_connected(0, true);
        runtime.messages().set_message("PointerOwnerRetained", u"First line\nSecond line");
        require(guidance->request1PGuidance("PointerOwnerRetained", true), "actual connected and valid layout accepts original guidance request");
        const auto* text = guidance->mGuidanceMessage;
        runtime.messages().set_message("PointerOwnerOther", u"Another message");
        (void)MR::getLayoutMessageDirect("PointerOwnerOther");
        require(text && text == MR::getLayoutMessageDirect("PointerOwnerRetained") && std::wstring(text) == L"First line\nSecond line",
                "guidance borrows per-message retained wide storage across independent lookups");
        require(!MR::isExistStarPointerGuidance(), "a request alone does not fabricate visible guidance before the original spine advances");

        int yes_no_request;
        MR::startStarPointerModeChooseYesNo(&yes_no_request);
        require(modes.mMode == StarPointerMode_ChooseYesNo &&
                director.getStarPointerLayout(0)->mPointerKind == StarPointerKind_FingerPointer &&
                !director.isEnableStarPointerShootStarPiece(), "original priority selection changes actual pointer kind and shooting policy");
        {
            smgpc::compat::StageSessionState inner("Game", "EggStarGalaxy", 1, JMapIdInfo(0, 0));
            smgpc::compat::StageSessionBinding inner_binding(inner);
            require(modes.mModeCounter[StarPointerMode_Game] == 2 && modes.mMode == StarPointerMode_ChooseYesNo,
                    "a nested stage retains the outer high-priority request and adds only its own Game request");
        }
        require(modes.mModeCounter[StarPointerMode_Game] == 1 && modes.mMode == StarPointerMode_ChooseYesNo &&
                director.mIsUpdateTransHolder, "inner teardown removes its own request and restores outer camera-transform ownership");
        MR::endStarPointerMode(&yes_no_request);
        require(modes.mMode == StarPointerMode_Game && director.isEnableStarPointerShootStarPiece(),
                "popping the explicit higher-priority request resumes the original Game setup");
        modes.requestMode(nullptr, StarPointerMode_PauseMenu);
        modes.update();
        require(modes.mMode == StarPointerMode_PauseMenu && !modes.mRequested[StarPointerMode_PauseMenu],
                "one-frame mode requests are selected then cleared by original update");
        modes.update();
        require(modes.mMode == StarPointerMode_Game, "the next original update restores the retained Game request");

        std::array<int, 16> requests{};
        for (auto& request : requests) modes.incModeCounter(&request, StarPointerMode_ChooseYesNo);
        require(modes.mModeCounter[StarPointerMode_ChooseYesNo] == 15,
                "original 16-slot request capacity includes the existing stage request");
        for (int i = 14; i >= 0; --i) modes.popState(&requests[i]);
        require(modes.mModeCounter[StarPointerMode_ChooseYesNo] == 0 && modes.mMode == StarPointerMode_Game,
                "original reverse request compaction preserves the stage request");
        owner.controller(0).mOutScreenTime = 35;
        owner.controller(1).mOutScreenTime = 36;
        require(MR::isStarPointerInScreenAnyPort(nullptr), "original screen hysteresis includes frame 35");
        owner.controller(0).mOutScreenTime = 36;
        require(!MR::isStarPointerInScreenAnyPort(nullptr), "both actual controller counters beyond 35 exclude the pointer");
    }
    require(modes.mModeCounter[StarPointerMode_Game] == 0 && !director.mIsUpdateTransHolder &&
            !MR::isStarPointerValid(0) && !MR::isStarPointerValid(1),
            "last stage teardown resumes original Base mode while retaining process-owned resources");
}
}
int main() {
    try {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc, "SMGPC_REAL_DISC must name the real disc for original pointer owner validation");
        smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original pointer ownership"});
        smgpc::render::AuroraRenderer renderer(window);
        require(aurora_dvd_open(disc), "cannot open requested real disc");
        struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
        DVDInit();
        smgpc::resource::GameResourceRuntime process({96U << 20, 32U << 20, 4U << 20});
        Logger logger;
        const auto baseline_objects = smgpc::compat::name_obj_runtime_state_count();
        for (int cycle = 0; cycle < 2; ++cycle) {
            {
                smgpc::runtime::RuntimeContext runtime(logger, window, process);
                smgpc::runtime::SceneSchedulerBinding scheduler_binding(runtime.scheduler());
                (void)renderer.begin_frame();
                modes_and_resources(runtime);
                renderer.end_frame();
            }
            require(!smgpc::compat::try_star_pointer_depth() &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline_objects,
                    "process shutdown retires actual pointer descendants and publication before a second runtime");
        }
        std::cout << "Original pointer resources, guidance, mode priority/capacity, nested stages, and repeated ownership passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
