#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/LiveActor/ShadowVolumeLine.hpp"
#include "Game/LiveActor/ShadowVolumeSphere.hpp"
#include "Game/Map/GroundChecker.hpp"
#include "Game/MapObj/PunchingKinoko.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/Cp932Literal.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <aurora/allocation.hpp>
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

    std::size_t original_placement_count() {
        auto* root = MR::getStageDataHolder();
        require(root != nullptr, "Actual original stage data exists");
        std::size_t count = 0;
        for (s32 zone = 0; zone < MR::getZoneNum(); ++zone) {
            auto* holder = root->getStageDataHolderFromZoneId(zone);
            if (!holder) continue;
            for (const auto& table : holder->mPlacementObjs)
                for (s32 row = 0; row < table.getNumEntries(); ++row) {
                    const JMapInfoIter iter(&table, row);
                    const char* name = nullptr;
                    if (MR::getObjectName(&name, iter) && std::strcmp(name, "PunchingKinoko") == 0) ++count;
                }
        }
        return count;
    }

    struct Probe {
        bool exercised = false;
        std::uint64_t ready_frame = 0;
        const NameObj* holder_identity = nullptr;
        std::vector<const NameObj*> retained_identities;

        void exercise() {
            auto* shadows = MR::getSceneObj<ShadowControllerHolder>(SceneObj_ShadowControllerHolder);
            require(smgpc::scene::current_scene_allocation_domain() && shadows,
                    "Actual original scene heap and shadow holder exist");
            holder_identity = shadows;
            const auto objects = smgpc::compat::snapshot_name_obj_runtime_objects();
            std::vector<PunchingKinoko*> actors;
            std::vector<GroundChecker*> checkers;
            for (auto* object : objects) {
                if (auto* actor = dynamic_cast<PunchingKinoko*>(object)) actors.push_back(actor);
                if (auto* checker = dynamic_cast<GroundChecker*>(object)) checkers.push_back(checker);
            }
            require(original_placement_count() == 14 && actors.size() == 14 && checkers.size() == 14,
                    "Actual Gateway stage data and ordinary factory create all fourteen actors and GroundCheckers");
            std::vector<GroundChecker*> referenced_checkers;
            for (auto* actor : actors) {
                require(actor->mModelManager && actor->mStarPointerTarget && actor->mSpine && !actor->mFlag.mIsDead,
                        "Canonical factory init creates a live real model, pointer target and nerve");
                require(actor->getSensor("Head")->mHost == actor && actor->getSensor("Head")->mSensorGroup &&
                            actor->getSensor("Body")->mHost == actor && actor->getSensor("Body")->mSensorGroup,
                        "Every original head/body sensor belongs to the actual sensor groups");
                auto* list = actor->mShadowControllerList;
                require(list && list->getControllerCount() == 3, "Every original init creates three shadow controllers");
                auto* body = list->getController(CP932("体"));
                auto* head = list->getController(CP932("頭"));
                auto* vine = list->getController(CP932("つた"));
                require(body && head && vine && dynamic_cast<ShadowVolumeSphere*>(body->getShadowDrawer()) &&
                            dynamic_cast<ShadowVolumeSphere*>(head->getShadowDrawer()),
                        "Original CP932 shadow identities resolve the two actual sphere drawers and line controller");
                auto* line = dynamic_cast<ShadowVolumeLine*>(vine->getShadowDrawer());
                const auto found = std::ranges::find_if(checkers, [&](const auto* checker) {
                    return head->mDropPos == &checker->mPosition && head->mDropDir == &checker->mGravity;
                });
                require(line && line->mFromShadowController == body && line->mToShadowController == head &&
                            body->mDropPos == &actor->mPosition && !vine->isCalcCollision() && found != checkers.end(),
                        "Every original shadow line retains its body/head controllers and actual GroundChecker bindings");
                auto* ground = *found;
                require(ground->mBinder && !ground->mFlag.mIsDead &&
                            std::ranges::find(referenced_checkers, ground) == referenced_checkers.end(),
                        "Every actor owns a distinct live GroundChecker with actual world Binder");
                referenced_checkers.push_back(ground);
                for (u32 index = 0; index < list->getControllerCount(); ++index) {
                    auto* controller = list->getController(index);
                    require(std::find(shadows->_C.begin(), shadows->_C.end(), controller) != shadows->_C.end(),
                            "Actual original shadow holder registers all forty-two controllers");
                    retained_identities.push_back(controller->getShadowDrawer());
                }
                const auto matrix = MR::getJointMtx(actor, "Ball");
                require(matrix != nullptr, "Every actual model has its original Ball joint matrix");
                for (unsigned row = 0; row < 3; ++row)
                    for (unsigned column = 0; column < 4; ++column)
                        require(std::isfinite(matrix[row][column]), "Original model joint matrices remain finite during normal frames");
                retained_identities.push_back(actor);
                retained_identities.push_back(ground);
            }
        }

        void after_frame(GameSystem& system, std::uint64_t frame) {
            if (exercised) return;
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            if (!ready_frame) ready_frame = frame;
            // Observe normal execution after placement. Do not register another
            // model after original draw-actor-list allocation or tick actors
            // independently of the original scheduler.
            if (frame < ready_frame + 20) return;
            const aurora::allocation::HostAllocationScope host;
            exercise();
            exercised = true;
            std::fprintf(stderr, "[punching-kinoko-probe] PASS all fourteen real factory placements, models/sensors/binders, forty-two original shadow controllers; frame=%llu\n",
                         static_cast<unsigned long long>(frame));
        }
    };
}
#endif

int main() {
#ifdef NDEBUG
    std::fprintf(stderr, "This original-process diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
        const auto save = std::filesystem::temp_directory_path() / ("petari-original-punching-kinoko-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic starts with a fresh native console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original PunchingKinoko integration",
            .arguments = {"original-punching-kinoko-test", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
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
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.exercised,
                "OriginalProcess completes read-only placement diagnostic and normal bounded frame loop");
        require(!smgpc::compat::has_name_obj_runtime_state(probe.holder_identity) &&
                    smgpc::compat::actor_shadow_runtime_state_count() == 0,
                "Normal scene retirement removes actual shadow holder and all actor shadow owners");
        for (const auto* object : probe.retained_identities)
            require(!smgpc::compat::has_name_obj_runtime_state(object), "Normal original scene teardown retires every actual actor, GroundChecker and shadow drawer");
        std::fprintf(stderr, "PASS original-process PunchingKinoko: all fourteen ordinary factory placements, actual model/sensor/binder/shadow graph, normal frames and scene retirement\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process PunchingKinoko: %s\n", error.what());
        return 1;
    }
#endif
}
