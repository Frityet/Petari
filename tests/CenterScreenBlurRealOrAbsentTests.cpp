#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/CenterScreenBlur.hpp"
#include "Game/Screen/CaptureScreenDirector.hpp"
#include "Game/Screen/FullScreenBlur.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Demo/DemoDirector.hpp"
#include "Game/Demo/DemoSimpleCastHolder.hpp"
#include "NativeHeapFixture.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

#include <dolphin/gx/GXAurora.h>
#include <aurora/exception.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) aurora::throw_host_exception<std::runtime_error>(std::string(message));
    }

    void verify_absent_owner() {
        require(MR::getSceneObjHolder() == nullptr,
                "a process without a scene has no original actor");
        MR::createCenterScreenBlur();
        require(MR::getSceneObjHolder() == nullptr &&
                    !MR::isExistSceneObj(SceneObj_CenterScreenBlur),
                "creation without a scene leaves the original actor absent");
        // Original start/draw require the actual scene and player owners.
        // There is no exception contract for an absent original owner.
        const std::array<unsigned char, 32> arbitrary_address{};
        require(!AuroraIsFrameActive() && !AuroraHasTextureCopy(arbitrary_address.data()),
                "outside a renderer frame no arbitrary address is a completed GPU copy");
    }

#ifndef NDEBUG
    constexpr std::uint64_t frame_count = 120;

    struct Probe {
        const CenterScreenBlur* identity = nullptr;
        const void* history_image = nullptr;
        std::weak_ptr<JKRHeap> domain;
        std::uint64_t observations = 0;
        bool checked = false;

        void after_frame(GameSystem& system, std::uint64_t frame) {
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            const aurora::allocation::HostAllocationScope host;
            auto* holder = MR::getSceneObjHolder();
            auto* blur = dynamic_cast<CenterScreenBlur*>(holder->getObj(SceneObj_CenterScreenBlur));
            auto* history = MR::getFullScreenBlurTexture();
            auto* demo = MR::getSceneObj<DemoDirector>(SceneObj_DemoDirector);
            require(blur && blur->mSpine && history && demo,
                    "original initialization constructs the actual blur actor, nerve, player history texture and DemoDirector");
            require(!identity || identity == blur, "ordinary scene frames retain one original blur identity");
            identity = blur;
            domain = MR::getSceneObjHolder()->nativeAllocationHeap();
            require(MR::createSceneObj(SceneObj_CenterScreenBlur) == blur &&
                        demo->_20->nativeRegistrationCount(blur) == 1,
                    "the exact actor is unique and registered once with the real original DemoDirector");
            require(MR::isDead(blur) && history->getWidth() == 128 && history->getHeight() == 64 &&
                        history->getFormat() == GX_TF_RGBA8 && history->mImage,
                    "the original opening retains an inactive blur and Mario's allocated RGBA8 history texture");
            ++observations;
            if (frame != frame_count - 1) return;
            require(observations >= 30, "blur owner survives at least thirty ordinary original scene frames");
            const auto allocation_domain = domain.lock();
            require(allocation_domain != nullptr, "blur checks borrow the actual original scene heap");
            const JKRHeap::CurrentHeapScope allocation(*(allocation_domain));
            const aurora::allocation::ClientAllocationScope allocationRouting({true, true});
            const J3DSys::ContextScope commands;
            require(AuroraIsFrameActive(), "original process observer executes inside its active GX frame");
            require(system.mObjHolder && system.mObjHolder->mCaptureScreenDirector &&
                        MR::getScreenResTIMG() == system.mObjHolder->mCaptureScreenDirector->getResTIMG(),
                    "screen helpers borrow the real GameSystem capture owner");
            require(MR::getScreenTexImage() && AuroraHasTextureCopy(MR::getScreenTexImage()),
                    "ordinary original rendering completed a real screen texture copy");
            // Inject the public action only after the last normal game frame.
            // Follow its actual nerve and draws, then retire the normal scene.
            MR::startCenterScreenBlur(6, 30.0f, 180, 2, 2);
            require(!MR::isDead(blur) && blur->mTime == 6 && blur->mFadeIn == 2 && blur->mFadeOut == 2 &&
                        blur->mOffset == 30.0f && blur->mAlpha == 180 && blur->mBlendRate == 0.0f,
                    "public start preserves the exact retail parameters and appearance contract");
            blur->mSpine->update();
            require(!MR::isDead(blur) && blur->mBlendRate == 0.0f,
                    "the original FadeIn nerve starts at zero blend");
            blur->mSpine->update();
            require(std::fabs(blur->mBlendRate - 0.5f) < 0.00001f,
                    "the original FadeIn nerve computes its linear half blend");
            blur->draw();
            require(history == MR::getFullScreenBlurTexture() && history->mImage && AuroraHasTextureCopy(history->mImage),
                    "the original blur draw samples the real screen copy and captures one real history texture");
            history_image = history->mImage;
            blur->draw();
            require(MR::getFullScreenBlurTexture() == history && history->mImage == history_image &&
                        AuroraHasTextureCopy(history_image),
                    "the second original blur draw reuses its retained history allocation with a new GPU copy");
            for (unsigned step = 0; step < 16 && !MR::isDead(blur); ++step) blur->mSpine->update();
            require(MR::isDead(blur), "the original FadeIn/Keep/FadeOut sequence kills itself");
            checked = true;
            std::fprintf(stderr, "PASS original blur actor/demo/nerve and two real GPU history captures; ordinary_frames=%llu\n",
                         static_cast<unsigned long long>(observations));
        }
    };
#endif
}

int main() {
#ifdef NDEBUG
    std::fprintf(stderr, "Original blur owner diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        verify_absent_owner();
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
        const auto save = std::filesystem::temp_directory_path() /
                          ("petari-original-blur-owner-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "blur diagnostic starts with a fresh console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE", "SMGPC_STRICT_PLACEMENT"})
            unsetenv(name);
        const smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original captured-frame blur regression",
            .arguments = {"original-blur-owner-test", "--stage", "HeavensDoorGalaxy", "--scenario", "1",
                          "--max-frames", std::to_string(frame_count)},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct Disc { ~Disc() { smgpc::app::close_disc_image(); } } disc_lifetime;
        Probe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<Probe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.checked,
                "the actual original process completes the retained blur checks and bounded frame loop");
        require(probe.identity && !NameObj::nativeGeneration(probe.identity) && probe.domain.expired() &&
                    probe.history_image && !AuroraHasTextureCopy(probe.history_image),
                "normal process retirement releases the blur actor, scene heap, history owner and GPU copy");
        verify_absent_owner();
        std::fprintf(stderr, "PASS original blur ownership, public action, nerve lifecycle, real capture history and process retirement\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original blur owner: %s\n", error.what());
        return 1;
    }
#endif
}
