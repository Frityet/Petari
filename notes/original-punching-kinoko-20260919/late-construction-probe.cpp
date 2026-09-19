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
#include "compat/J3dCommandScope.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/NameObjChildOwner.hpp"
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

    JMapInfoIter original_placement() {
        auto* root = MR::getStageDataHolder();
        require(root != nullptr, "Actual original stage data exists");
        for (s32 zone = 0; zone < MR::getZoneNum(); ++zone) {
            auto* holder = root->getStageDataHolderFromZoneId(zone);
            if (!holder) continue;
            for (const auto& table : holder->mPlacementObjs)
                for (s32 row = 0; row < table.getNumEntries(); ++row) {
                    const JMapInfoIter iter(&table, row);
                    const char* name = nullptr;
                    if (MR::getObjectName(&name, iter) && std::strcmp(name, "PunchingKinoko") == 0) return iter;
                }
        }
        throw std::runtime_error("The actual stage has no PunchingKinoko placement");
    }

    struct PlacementZone {
        s32 previous = MR::getCurrentPlacementZoneId();
        explicit PlacementZone(s32 zone) { MR::setCurrentPlacementZoneId(zone); }
        ~PlacementZone() { MR::setCurrentPlacementZoneId(previous); }
    };

    struct Probe {
        bool exercised = false;
        const NameObj* holder_identity = nullptr;

        void exercise() {
            const auto domain = smgpc::scene::current_scene_allocation_domain();
            auto* scheduler = smgpc::runtime::try_active_scene_scheduler();
            auto* shadows = MR::getSceneObj<ShadowControllerHolder>(SceneObj_ShadowControllerHolder);
            require(domain && scheduler && shadows, "Actual original scene heap, scheduler and shadow holder exist");
            holder_identity = shadows;
            const auto actors_before = smgpc::compat::actor_runtime_state_count();
            const auto shadow_count = shadows->_C.size();
            const auto iter = original_placement();
            const PlacementZone zone(MR::getPlacedZoneId(iter));
            smgpc::scene::NameObjChildOwner objects;
            PunchingKinoko* actor = nullptr;
            const auto marker = smgpc::compat::mark_name_obj_runtime_registrations();
            objects.capture_construction_children([&] {
                const smgpc::compat::JkrAllocationScope game(domain);
                actor = new PunchingKinoko("PunchingKinokoOriginalProcessProbe");
                actor->init(iter);
            });
            GroundChecker* ground = nullptr;
            for (auto* object : smgpc::compat::snapshot_name_obj_runtime_objects_since(marker)) {
                if (auto* candidate = dynamic_cast<GroundChecker*>(object)) {
                    require(!ground, "Original actor constructs exactly one GroundChecker");
                    ground = candidate;
                }
            }
            require(actor->mModelManager && actor->mStarPointerTarget && actor->mSpine && ground && ground->mBinder &&
                        !actor->mFlag.mIsDead && !ground->mFlag.mIsDead,
                    "Canonical init creates a real model, pointer target, nerve and live bound child");
            require(actor->getSensor("Head")->mHost == actor && actor->getSensor("Head")->mSensorGroup &&
                        actor->getSensor("Body")->mHost == actor && actor->getSensor("Body")->mSensorGroup,
                    "Original head/body sensors enter actual sensor groups");
            auto* list = actor->mShadowControllerList;
            require(list && list->getControllerCount() == 3, "Original init creates exactly three shadow controllers");
            auto* body = list->getController(CP932("体"));
            auto* head = list->getController(CP932("頭"));
            auto* vine = list->getController(CP932("つた"));
            require(body && head && vine && dynamic_cast<ShadowVolumeSphere*>(body->getShadowDrawer()) &&
                        dynamic_cast<ShadowVolumeSphere*>(head->getShadowDrawer()),
                    "Original CP932 names resolve both sphere drawers and the line controller");
            auto* line = dynamic_cast<ShadowVolumeLine*>(vine->getShadowDrawer());
            require(line && line->mFromShadowController == body && line->mToShadowController == head &&
                        head->mDropPos == &ground->mPosition && head->mDropDir == &ground->mGravity &&
                        body->mDropPos == &actor->mPosition && !vine->isCalcCollision(),
                    "Original line and head retain actual body/GroundChecker controller and position/gravity bindings");
            const std::vector<const NameObj*> drawers{body->getShadowDrawer(), head->getShadowDrawer(), line};
            require(shadows->_C.size() == shadow_count + 3, "Actual shadow holder receives the canonical three controllers");
            {
                const smgpc::compat::JkrAllocationScope game(domain);
                objects.init_registration_suffix_after_placement();
                for (u32 index = 0; index < list->getControllerCount(); ++index) {
                    list->getController(index)->updateDirection();
                    list->getController(index)->updateProjection();
                }
                // A bounded real actor update using the unchanged nerve/control
                // and actual world collision, without touching any story switch.
                actor->movement();
                const smgpc::compat::J3dCommandScope commands;
                actor->calcAnim();
            }
            const auto matrix = MR::getJointMtx(actor, "Ball");
            require(matrix != nullptr, "Actual Ball joint matrix exists after original model calculation");
            for (unsigned row = 0; row < 3; ++row)
                for (unsigned column = 0; column < 4; ++column)
                    require(std::isfinite(matrix[row][column]), "Original joint callback produces a finite actual model matrix");
            require(std::abs(matrix[0][3] - ground->mPosition.x) < 0.01F &&
                        std::abs(matrix[1][3] - ground->mPosition.y) < 0.01F &&
                        std::abs(matrix[2][3] - ground->mPosition.z) < 0.01F,
                    "Original Ball callback follows the real GroundChecker world position");
            require(smgpc::compat::actor_runtime_state_count() == actors_before + 2,
                    "Canonical construction owns only its actor and GroundChecker LiveActors");
            objects.clear();
            require(!smgpc::compat::has_actor_runtime_state(actor) && !smgpc::compat::has_actor_runtime_state(ground) &&
                        smgpc::compat::actor_runtime_state_count() == actors_before && shadows->_C.size() == shadow_count,
                    "Capture retirement removes actor, GroundChecker, binder and actual shadow memberships before another frame");
            for (const auto* drawer : drawers)
                require(!smgpc::compat::has_name_obj_runtime_state(drawer), "Actor retirement removes every original shadow drawer");
            const auto scheduled = scheduler->snapshot();
            require(std::ranges::none_of(scheduled, [](const auto& entry) {
                return entry.name == "PunchingKinokoOriginalProcessProbe";
            }), "Actor retirement removes original movement/model scheduler registrations");
        }

        void after_frame(GameSystem& system, std::uint64_t frame) {
            if (exercised) return;
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            const aurora::allocation::HostAllocationScope host;
            exercise();
            exercised = true;
            std::fprintf(stderr, "[punching-kinoko-probe] PASS real placement/model/sensors/shadows, original control/joint callback, actor retirement; frame=%llu\n",
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
                "OriginalProcess completes actor diagnostic and normal bounded frame loop");
        require(!smgpc::compat::has_name_obj_runtime_state(probe.holder_identity) &&
                    smgpc::compat::actor_shadow_runtime_state_count() == 0,
                "Normal scene retirement removes actual shadow holder and all actor shadow owners");
        std::fprintf(stderr, "PASS original-process PunchingKinoko: real placement/model/sensors/shadows, original control/joint callback, actor and scene retirement; factory activation not tested\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process PunchingKinoko: %s\n", error.what());
        return 1;
    }
#endif
}
