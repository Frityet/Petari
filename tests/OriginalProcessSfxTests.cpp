#include "Game/AudioLib/AudSystem.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "JSystem/JAudio2/JAIStreamMgr.hpp"
#include "JSystem/JAudio2/JASAiCtrl.hpp"
#include "JSystem/JAudio2/JASChannel.hpp"
#include "JSystem/JAudio2/JAUAudible.hpp"
#include "NativeHeapFixture.hpp"
#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "runtime/JasAudioDriver.hpp"
#include <array>
#include <aurora/allocation.hpp>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <unistd.h>

static_assert(OSNanosecondsToTicks(6666667) == 270000);
static_assert(OSSecondsToTicks(600) == 24300000000LL);

namespace {
    void require(bool condition, const char *message) {
        if (!condition)
            throw std::runtime_error(message);
    }
    void check_native_audio_boundaries() {
        JASChannel::MixConfig routing{};
        routing.whole = 0x0285;
        require(routing.parts.upper == 2 && routing.parts.lower0 == 8 && routing.parts.lower1 == 5,
                "DSP bus and volume-source fields retain PPC bit positions");
        const JAUAudibleParam audible(0xfac0, 0x1234);
        require(static_cast<u32>(audible) == 0xfac01234 && audible.getDoppler() == 15 && audible.calcDolby(),
                "Audible halfwords and Doppler fields retain PPC ordering");
        auto root = smgpc::test::create_native_root_heap(1024 * 1024);
        JASGenericMemPool pool;
        for (unsigned cycle = 0; cycle < 2; ++cycle) {
            auto arena = smgpc::test::create_native_solid_heap(root, 64 * 1024);
            JASDram = static_cast<JKRSolidHeap *>(arena.get());
            pool.newMemPool(32, 3);
            auto *slot = pool.alloc(32);
            require(slot && pool.getFreeMemCount() == 2, "Original pool slots allocate from the current audio arena");
            pool.free(slot, 32);
            JASDram = nullptr;
            arena.reset();
            require(pool.getFreeMemCount() == 0 && pool.getTotalMemCount() == 0 && !pool.alloc(32),
                    "Arena retirement unlinks static audio pools before their storage is reused");
        }
    }
#ifndef NDEBUG
    struct Probe {
        unsigned readyFrames = 0, accepted = 0;
        std::uint64_t nonzero = 0;
        unsigned streamPauseFrame = 0, streamResumeFrame = 0;
        float pausedPosition = 0.0f;
        bool streamVerified = false;
        void after_frame(GameSystem &system) {
            const aurora::allocation::HostAllocationScope host;
            auto *controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene || !AudSystem::msBasic)
                return;
            ++readyFrames;
            constexpr std::array names{"SE_SY_BUTTON_CURSOR_ON", "SE_SY_TALK_OK", "SE_SY_GALAXY_DECIDE_CANCEL"};
            if (readyFrames >= 60 && readyFrames <= 180 && readyFrames % 60 == 0) {
                const char *name = names[readyFrames / 60 - 1];
                auto *handle = MR::startSystemSE(name);
                require(handle && handle->getSound(), "An original UI sound must acquire a real JAI handle");
                require(AudSystem::msBasic->getSeMgr().getNumActiveSe() > 0, "Original SE manager owns the requested sound");
                ++accepted;
                std::fprintf(stderr, "[sfx-probe] accepted %s through original sound manager\n", name);
            }
            if (readyFrames == 70) {
                const auto before = JASDriver::getSubFrameCounter();
                {
                    const aurora::os::GuestThreadWaitScope waiting;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                require(JASDriver::getSubFrameCounter() - before >= 8,
                        "Audio subframes continue while the render caller waits");
                std::fprintf(stderr, "[sfx-probe] audio advanced during 100ms render wait\n");
            }
            auto *streams = AudSystem::msBasic->getStreamMgr().getStreamList();
            for (auto *link = streams->getFirst(); link && !streamVerified; link = link->getNext()) {
                auto *stream = link->getObject();
                auto &transport = stream->inner_.aramStream;
                if (!stream->isPlaying() || transport._0C8 < 0.5f || transport.mChannelNum < 2)
                    continue;
                require(transport.mChannels[0] && transport.mChannels[1] && transport.mBlock > 1,
                        "Original JAS streaming loads multiple DVD blocks and owns stereo DSP channels");
                if (!streamPauseFrame) {
                    stream->pause(true);
                    streamPauseFrame = readyFrames;
                } else if (!streamResumeFrame) {
                    if (readyFrames == streamPauseFrame + 5)
                        pausedPosition = transport._0C8;
                    if (readyFrames == streamPauseFrame + 35) {
                        require(transport._0AE != 0 && std::abs(transport._0C8 - pausedPosition) < 0.01f,
                                "Original stream position remains still while paused");
                        stream->pause(false);
                        streamResumeFrame = readyFrames;
                    }
                } else if (readyFrames >= streamResumeFrame + 35) {
                    require(transport._0AE == 0 && transport._0C8 > pausedPosition + 0.2f,
                            "Original stream position advances again after resume");
                    streamVerified = true;
                    std::fprintf(stderr, "[sfx-probe] original stereo DVD/ARAM stream paused and resumed at %.3fs\n", transport._0C8);
                }
            }
            nonzero = smgpc::audio::nonzero_dsp_samples();
        }
    };
#endif
}  // namespace
int main() {
#ifdef NDEBUG
    return 1;
#else
    try {
        check_native_audio_boundaries();
        const char *disc = std::getenv("SMGPC_REAL_DISC");
        const char *source = std::getenv("SMGPC_TEST_SAVE_DIR");
        require(disc && *disc && source && *source, "Set SMGPC_REAL_DISC and SMGPC_TEST_SAVE_DIR");
        const auto save = std::filesystem::temp_directory_path() / ("petari-sfx-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic save directory must be new");
        std::filesystem::copy(source, save, std::filesystem::copy_options::recursive);
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto *name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                 "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"})
            unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640,
            .window_height = 456,
            .window_title = "Original JAudio SFX diagnostic",
            .arguments = {"sfx-probe", "--stage", "EggStarGalaxy", "--scenario", "1", "--save-slot", "1", "--max-frames", "1200"},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct Disc {
            ~Disc() {
                smgpc::app::close_disc_image();
            }
        } disc_lifetime;
        Probe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void *context, GameSystem &system, std::uint64_t) {
                static_cast<Probe *>(context)->after_frame(system);
            },
        };
        const int result = smgpc::app::run_original_game(configuration, *logger, observer);
        require(result == 0 && probe.accepted == 3, "Original audio initializes and three real SE requests complete the probe");
        require(probe.streamVerified, "Original music streaming must prepare, play, pause and resume through JAS");
        require(probe.nonzero > 0, "The shared original DSP output contains actual samples");
        std::fprintf(stderr, "[sfx-probe] PASS accepted=%u DSP nonzero samples=%llu; game owner retired\n",
                     probe.accepted, static_cast<unsigned long long>(probe.nonzero));
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "FAIL original SFX: %s\n", error.what());
        return 1;
    }
#endif
}
