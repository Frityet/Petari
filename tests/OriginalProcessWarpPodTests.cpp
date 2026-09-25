#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActorGroup.hpp"
#include "Game/MapObj/WarpPod.hpp"
#include "Game/NameObj/NameObjFinder.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "resource/TextEncoding.hpp"
#include "NativeHeapFixture.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/SceneScheduler.hpp"
#include <JSystem/JUtility/JUTTexture.hpp>
#include <aurora/allocation.hpp>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <unistd.h>
#include <vector>

#ifndef NDEBUG
namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

std::vector<JMapInfoIter> original_placements() {
    auto* root = MR::getStageDataHolder();
    require(root, "Original stage holder exists");
    std::vector<JMapInfoIter> result;
    for (s32 zone = 0; zone < MR::getZoneNum(); ++zone) {
        auto* holder = root->getStageDataHolderFromZoneId(zone);
        if (!holder) continue;
        for (const auto& table : holder->mPlacementObjs)
            for (s32 row = 0; row < table.getNumEntries(); ++row) {
                const JMapInfoIter iter(&table, row);
                const char* name = nullptr;
                if (MR::getObjectName(&name, iter) && std::strcmp(name, "WarpPod") == 0)
                    result.push_back(iter);
            }
    }
    return result;
}

struct Probe {
    bool exercised = false;
    bool observed_movement = false;
    std::uint64_t initialized_frame = 0;
    std::array<WarpPod*, 2> actors{};
    WarpPodMgr* manager = nullptr;
    LiveActorGroup* group = nullptr;
    std::array<u16, 2> initial_timers{};

    void initialize() {
        const auto placements = original_placements();
        require(placements.size() == actors.size(), "Exactly two authored WarpPod placements exist");
        require(MR::getSceneObjHolder()->nativeAllocationHeap() != nullptr, "Actual original scene allocation domain exists");
        group = dynamic_cast<LiveActorGroup*>(NameObjFinder::find(CP932("ワープポッド群")));
        require(group && group->getObjNum() == actors.size(), "Factory constructed both original named-group members");
        for (std::size_t i = 0; i < actors.size(); ++i)
            actors[i] = dynamic_cast<WarpPod*>(group->getActor(i));
        manager = MR::getWarpPodManager();
        require(manager && group && group->getObjNum() == actors.size(), "Actual manager and named group retain both actors");
        auto* scheduler = smgpc::runtime::try_active_scene_scheduler();
        require(scheduler != nullptr, "Original scene scheduler exists");
        const auto scheduled = scheduler->snapshot();
        unsigned paths = 0;
        for (std::size_t i = 0; i < actors.size(); ++i) {
            auto* actor = actors[i];
            auto* pair = actors[1 - i];
            require(actor && actor->mPairPod == pair && manager->getPairPod(actor) == pair,
                    "Original pair lookup is mutual and excludes the querying actor");
            require(actor->_8C && actor->_8C->_0 == 0 && actor->_8C->mZoneID == 5 &&
                        actor->mArg1 == 0 && actor->mArg2 == 0 && !actor->_CB,
                    "Actual zone/group and invisible active sensor arguments survive init");
            require(actor->mModelManager && actor->_94 && actor->mEventCameraName && !actor->mFlag.mIsDead,
                    "Original model, camera info/name and live actor state exist");
            const auto name = smgpc::resource::decode_cp932(actor->getName());
            require(std::count_if(scheduled.begin(), scheduled.end(), [&](const auto& entry) {
                return entry.name == name && entry.movement_type == MR::MovementType_MapObj &&
                       entry.calc_anim_type == -1 && entry.draw_buffer_type == -1 && entry.draw_type == -1 &&
                       entry.live_actor_position[0] == actor->mPosition.x &&
                       entry.live_actor_position[1] == actor->mPosition.y &&
                       entry.live_actor_position[2] == actor->mPosition.z;
            }) == 1, "Each invisible authored actor has its exact original movement-only scheduler registration");
            auto* sensor = actor->getSensor("eye");
            require(sensor && sensor->mHost == actor && sensor->mSensorGroup && sensor->mValidByHost &&
                        std::abs(sensor->mRadius - 120.0f * actor->mScale.x) < 0.001f,
                    "Original eye sensor retains real group, host and authored scaled radius");
            if (actor->_CA) {
                ++paths;
                require(actor->_C4 && actor->_C8 == 60 && actor->_D4 && actor->_D8 &&
                            actor->_D4->getTexInfo() && actor->_D8->getTexInfo() &&
                            actor->_D4->getWidth() > 0 && actor->_D8->getHeight() > 0,
                        "Original pair owns the 60-point path and both actual resource textures");
                for (u16 point = 0; point < actor->_C8; ++point)
                    require(std::isfinite(actor->_C4[point].x) && std::isfinite(actor->_C4[point].y) &&
                                std::isfinite(actor->_C4[point].z), "Original path points are finite");
            }
            initial_timers[i] = actor->_A4;
        }
        require(paths == 1, "Original pair selects exactly one authored path owner");
    }

    void after_frame(GameSystem& system, std::uint64_t frame) {
        if (exercised) {
            observed_movement |= actors[0]->_A4 != initial_timers[0] && actors[1]->_A4 != initial_timers[1];
            if (frame == 359) {
                for (std::size_t i = 0; i < actors.size(); ++i)
                    std::fprintf(stderr, "[warp-pod-probe] opening observation actor=%zu timer_initial=%u timer_final=%u name_flags=%u clipped=%d dead=%d\n",
                                 i, initial_timers[i], actors[i]->_A4, actors[i]->getFlag(),
                                 actors[i]->mFlag.mIsClipped, actors[i]->mFlag.mIsDead);
            }
            return;
        }
        auto* controller = system.mSceneController;
        if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
            controller->getCurrentSceneForExecute() != controller->mScene ||
            !dynamic_cast<GameScene*>(controller->mScene)) return;
        const aurora::allocation::HostAllocationScope host;
        initialize();
        initialized_frame = frame;
        exercised = true;
        std::fprintf(stderr, "[warp-pod-probe] PASS actual factory pair/group/resources/sensors/path; frame=%llu\n",
                     static_cast<unsigned long long>(frame));
    }
};
}
#endif

int main() {
#ifdef NDEBUG
    std::fprintf(stderr, "This diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the actual disc image");
        const auto save = std::filesystem::temp_directory_path() / ("petari-original-warp-pod-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic starts with fresh native console settings");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original WarpPod pair integration",
            .arguments = {"original-warp-pod-test", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "360"},
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
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.exercised &&
                    probe.initialized_frame < 300,
                "Actual process completes the pair probe and subsequent normal frames");
        for (auto* actor : probe.actors)
            require(!NameObj::nativeGeneration(actor) && !NameObj::nativeGeneration(actor),
                    "Normal scene teardown retires both actual actor identities");
        require(!NameObj::nativeGeneration(probe.manager) &&
                    !NameObj::nativeGeneration(probe.group),
                "Normal scene teardown retires the actual manager and named group");
        std::fprintf(stderr, "PASS original-process WarpPod: actual factory pair/group/resources/sensors/path/scheduler and scene retirement; observed_both_timers_advance=%d; post-intro movement and player traversal not tested\n", probe.observed_movement);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process WarpPod: %s\n", error.what());
        return 1;
    }
#endif
}
