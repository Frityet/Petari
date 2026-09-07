#include "SceneExecutionFixture.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CameraUtilCompat.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/StageResourceBinding.hpp"
#include "compat/StageSessionState.hpp"
#include "compat/StarPointerDepthOwnership.hpp"
#include "compat/StageZoneMatrixRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/SceneExecutionService.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "Game/Camera/CameraAnim.hpp"
#include "Game/Camera/CameraContext.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Camera/CameraHolder.hpp"
#include "Game/Camera/CameraLocalUtil.hpp"
#include "Game/Camera/CameraManEvent.hpp"
#include "Game/Camera/CameraManGame.hpp"
#include "Game/Camera/CameraParamChunk.hpp"
#include "Game/Camera/CameraParamChunkHolder.hpp"
#include "Game/Camera/CameraParamChunkID.hpp"
#include "Game/Camera/CameraShakePatternImpl.hpp"
#include "Game/Camera/CameraShakeTask.hpp"
#include "Game/Camera/CameraShaker.hpp"
#include "Game/Camera/CameraTargetArg.hpp"
#include "Game/Camera/CameraTargetMtx.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Scene/StopSceneController.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"

#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
    void require(bool value, const char* message) {
        if (!value) aurora::throw_host_exception<std::runtime_error>(message);
    }
    void near(float actual, float expected, const char* message) {
        require(std::isfinite(actual) && std::abs(actual - expected) < 0.01F, message);
    }
    class Logger final : public smgpc::logging::ILogger {
        void write(std::FILE*, std::source_location, smgpc::logging::Level,
                   smgpc::logging::Category, std::string_view) override {}
    };
    void put32(std::vector<u8>& bytes, std::size_t offset, u32 value) {
        for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = u8(value >> (24 - 8 * i));
    }
    std::vector<u8> animation() {
        std::vector<u8> bytes(0x20 + 64 + 4 + 16 * 4);
        std::copy_n("ANDOCANM", 8, bytes.begin());
        put32(bytes, 8, 1); put32(bytes, 0x10, 1); put32(bytes, 0x18, 2); put32(bytes, 0x1c, 64);
        for (unsigned i = 0; i < 8; ++i) {
            put32(bytes, 0x20 + i * 8, 2); put32(bytes, 0x24 + i * 8, i * 2);
        }
        put32(bytes, 0x60, 64);
        const std::array<float, 16> values{10, 30, 20, 20, 600, 600, 10, 30, 20, 20, 0, 0, 0, 0, 45, 60};
        for (unsigned i = 0; i < values.size(); ++i) put32(bytes, 0x64 + i * 4, std::bit_cast<u32>(values[i]));
        return bytes;
    }
    void shaker_storage(CameraShaker& shaker, const std::shared_ptr<smgpc::compat::JkrAllocationDomain>& domain) {
        smgpc::compat::JkrAllocationScope game(domain);
        // Invoke the actual constructor helper again to inspect its transient
        // seven-word retail write before createInfinityTask replaces four slots.
        shaker.createSinglyHorizontalTask();
        const std::array<u32, 7> words{0x3e99999a, 0x3f800000, 0x40400000,
                                     0x43300000, 0x80000000, 0x43300000, 0x80000000};
        for (std::size_t i = 0; i < words.size(); ++i) {
            auto* task = i < 3 ? shaker.mHorizontalTasks[i] : shaker.mInfinityTasks[i - 3];
            auto* pattern = static_cast<CameraShakePatternSingly*>(task->mPattern);
            require(std::bit_cast<u32>(pattern->mIntensity) == words[i], "horizontal allocation preserves every adjacent retail word including signed zero");
            require(pattern->mDirection.x == 1 && pattern->mDirection.y == 0,
                    "all seven original transient patterns receive horizontal direction");
            require(JKRHeap::findFromRoot(task) == &domain->heap() &&
                    JKRHeap::findFromRoot(pattern) == &domain->heap(), "transient shaker allocations belong to actual scene Game storage");
        }
        shaker.createInfinityTask();
        for (auto* task : shaker.mInfinityTasks) {
            auto* pattern = static_cast<CameraShakePatternVerticalSin*>(task->mPattern);
            require(pattern->mIntensity == 1 && pattern->mSpeed == 15,
                    "original infinity helper replaces the four overwritten slots");
        }
    }

    void camera_category_precedes_clipping(smgpc::runtime::RuntimeContext& runtime, CameraDirector& director,
                                          smgpc::test::SceneExecutionFixture& execution) {
        auto& scheduler = runtime.scheduler();
        // This is a synthetic scheduler input into the actual CameraContext.
        // Full Director movement needs the separately owned Mario graph; its
        // original resources and manager selection are exercised above.
        struct RestoreFlags {
            CameraDirector& director; u16 flags;
            ~RestoreFlags() { director.mFlag = flags; }
        } restore{director, director.mFlag};
        NameObjFunction::requestMovementOff(&director);
        struct Publisher final : NameObj {
            Publisher() : NameObj("Camera category fixture") {}
            void movement() override {
                MR::setCameraViewMtx(view, false, false, TVec3f(0, 0, 0));
                ++calls;
            }
            TPos3f view;
            u32 calls = 0;
        } publisher;
        struct Subject final : LiveActor {
            Subject() : LiveActor("Clipping category fixture") {}
            void startClipped() override { ++starts; LiveActor::startClipped(); }
            void endClipped() override { ++ends; LiveActor::endClipped(); }
            u32 starts = 0, ends = 0;
        } actor;
        actor.mPosition.set(0, 0, 0);
        // The synthetic subject has no complete actor initializer; publish
        // its explicit readiness and real clipping membership separately.
        actor.mFlag.mIsDead = false;
        smgpc::compat::configure_actor_clipping_sphere(&actor, 10, nullptr);
        scheduler.connect_name_obj(actor, MR::MovementType_Player, -1, -1, -1);
        MR::addToClippingTarget(&actor);
        scheduler.connect_name_obj(publisher, MR::MovementType_Camera, -1, -1, -1);
        struct PhaseProbe final : NameObj {
            PhaseProbe(int value, std::vector<int>& sequence)
                : NameObj("Original frame sequence probe"), value(value), sequence(sequence) {}
            void movement() override { record(value); }
            void calcAnim() override { record(value + 100); }
            void record(int value) {
                const smgpc::compat::JkrHostAllocationScope host;
                sequence.push_back(value);
            }
            int value;
            std::vector<int>& sequence;
        };
        std::vector<int> sequence;
        PhaseProbe clipped(10, sequence), planet(11, sequence), map(12, sequence), enemy(13, sequence);
        PhaseProbe collision(20, sequence), player(21, sequence), shadow(22, sequence);
        scheduler.connect_name_obj(clipped, MR::MovementType_ClippedMapParts, MR::CalcAnimType_ClippedMapParts, -1, -1);
        scheduler.connect_name_obj(planet, MR::MovementType_Planet, MR::CalcAnimType_Planet, -1, -1);
        scheduler.connect_name_obj(map, MR::MovementType_CollisionMapObj, MR::CalcAnimType_CollisionMapObj, -1, -1);
        scheduler.connect_name_obj(enemy, MR::MovementType_CollisionEnemy, MR::CalcAnimType_CollisionEnemy, -1, -1);
        scheduler.connect_name_obj(collision, MR::MovementType_CollisionDirector, -1, -1, -1);
        scheduler.connect_name_obj(player, MR::MovementType_Player, -1, -1, -1);
        scheduler.connect_name_obj(shadow, MR::MovementType_ShadowControllerHolder, -1, -1, -1);
        execution.complete_initialization();
        MR::getSceneNameObjMovementController()->movement();
        TPos3f far_view, near_view;
        far_view.setPositionFromLookAt(TVec3f(100000, 0, 1000), TVec3f(0, 1, 0), TVec3f(100000, 0, 0));
        near_view.setPositionFromLookAt(TVec3f(0, 0, 1000), TVec3f(0, 1, 0), TVec3f(0, 0, 0));
        MR::setCameraViewMtx(far_view, false, false, TVec3f(0, 0, 0));
        runtime.refresh_scene_camera_pose();
        scheduler.execute_movement_category(MR::MovementType_ClippingDirector);
        require(actor.mFlag.mIsClipped && actor.starts == 1, "old actual context view initially clips the subject");
        publisher.view.set(near_view);
        scheduler.begin_frame();
        scheduler.execute_movement_category(MR::MovementType_Camera);
        require(publisher.calls == 1 && runtime.scene_camera_pose().has_value(), "Camera category publishes the actual context view in this frame");
        near(runtime.scene_camera_pose()->eye.x, 0, "Camera callback publication reaches the renderer before clipping");
        require(actor.mFlag.mIsClipped && actor.ends == 0, "Camera category does not evaluate clipping early");
        scheduler.execute_movement_category(MR::MovementType_ClippingDirector);
        require(!actor.mFlag.mIsClipped && actor.ends == 1, "category4 uses the new actual camera view and invokes original endClipped");
        publisher.view.set(far_view);
        scheduler.begin_frame();
        scheduler.execute_movement_category(MR::MovementType_Camera);
        near(runtime.scene_camera_pose()->eye.x, 100000, "next Camera category publishes the next view rather than a cached pose");
        require(!actor.mFlag.mIsClipped, "subject retains prior clipping state until category4");
        scheduler.execute_movement_category(MR::MovementType_ClippingDirector);
        require(actor.mFlag.mIsClipped && actor.starts == 2 && actor.ends == 1,
                "next category4 evaluates the changed view exactly at its original boundary");

        publisher.view.set(near_view);
        runtime.scene_execution().execute_movement();
        require(!actor.mFlag.mIsClipped && actor.ends == 2,
                "the complete original movement list uses its camera before its clipping phase");
        require(sequence == std::vector<int>({10, 11, 12, 13, 110, 111, 112, 113, 20, 21}),
                "all four collision animations precede collision director and player movement in the original frame");
        runtime.scene_execution().execute_calc_anim_and_view();
        require(sequence == std::vector<int>({10, 11, 12, 13, 110, 111, 112, 113, 20, 21, 22}),
                "the original later animation list runs shadow movement without repeating collision animations");

        auto* stop = MR::getSceneObj<StopSceneController>(SceneObj_StopSceneController);
        stop->requestStopScene(3);
        sequence.clear();
        const auto camera_calls = publisher.calls;
        runtime.scene_execution().execute_movement();
        runtime.scene_execution().execute_movement();
        require(sequence.empty() && publisher.calls == camera_calls && stop->isSceneStopped(),
                "original stopped frames skip the complete movement list, including the camera");
        runtime.scene_execution().execute_movement();
        require(!stop->isSceneStopped() && publisher.calls == camera_calls + 1 &&
                sequence == std::vector<int>({10, 11, 12, 13, 110, 111, 112, 113, 20, 21}),
                "the original decrement boundary resumes exactly one complete movement frame");
    }
}

int main() {
    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    require(disc != nullptr, "SMGPC_REAL_DISC must name the real disc for original camera owner validation");
    smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original camera scene ownership"});
    smgpc::render::AuroraRenderer renderer(window);
    require(aurora_dvd_open(disc), "cannot open requested real disc");
    struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
    DVDInit();
    smgpc::resource::GameResourceRuntime process({96U << 20, 32U << 20, 4U << 20});
    Logger logger;
    smgpc::runtime::RuntimeContext runtime(logger, window, process);
    auto& scheduler = runtime.scheduler();
    smgpc::runtime::SceneSchedulerBinding scheduler_binding(scheduler);
    (void)renderer.begin_frame();
    smgpc::compat::require_star_pointer_depth().initialize_layouts();
    const auto baseline_entries = scheduler.snapshot().size();
    const auto baseline_objects = smgpc::compat::name_obj_runtime_state_count();
    const auto baseline_free = process.host_heaps()->root_heap().getFreeSize();
    for (const char* stage : {"HeavensDoorGalaxy", "EggStarGalaxy"}) {
        std::vector<smgpc::scene::StageHolderOccurrence> holders;
        const auto tables = smgpc::scene::resolve_stage_placement_tables(runtime.dvd(), stage, 1, &holders);
        smgpc::compat::StageResourceBinding resources(runtime.dvd(), holders, tables);
        smgpc::compat::StageZoneMatrixBinding zones(holders, tables);
        smgpc::compat::StageSessionState session("Game", stage, 1, JMapIdInfo(0, 0));
        smgpc::compat::StageSessionBinding session_binding(session);
        std::weak_ptr<smgpc::compat::JkrAllocationDomain> weak_domain;
        {
            smgpc::test::SceneExecutionFixture execution(
                scheduler, smgpc::compat::JkrAllocationDomain::create(process.host_heaps(), 8U << 20));
            auto& binding = execution.objects();
            auto& holder = *smgpc::scene::current_scene_obj_holder();
            holder.create(SceneObj_NameObjGroup);
            holder.create(SceneObj_AreaObjContainer);
            holder.create(SceneObj_PlanetGravityManager);
            binding.initialize_camera_system();
            auto* owner = smgpc::camera::current_camera_director_runtime();
            require(owner && &owner->director() == MR::getCameraDirector(), "native publication references the exact SceneObj CameraDirector");
            auto& director = owner->director();
            require(&owner->context() == holder.getObj(SceneObj_CameraContext), "view owner is the actual CameraContext SceneObj");
            require(director.mHolder->getNum() == 45 && director.getCurrentCameraMan() == director.mCameraManGame,
                    "original constructor owns all 45 controllers and activates CameraManGame");
            const auto domain = scheduler.allocation_domain();
            weak_domain = domain;
            require(JKRHeap::findFromRoot(&director) == &domain->heap() &&
                    JKRHeap::findFromRoot(director.mCameraCreator) == &domain->heap(),
                    "original director and GameCameraCreator belong to the true scene heap");
            shaker_storage(*director.mShaker, domain);
            auto* target = director.mCameraTargetMtx;
            TPos3f matrix;
            matrix.identity(); matrix.setTrans(TVec3f(100, 200, 300));
            target->setMtx(matrix);
            CameraTargetArg target_arg(target);
            target_arg.setTarget();
            target->movement();
            require(director.getTarget() == target && target->getLastMove().squared() == 0,
                    "original target argument selects matrix target and invalidates initial displacement");
            ActorCameraInfo info;
            {
                auto raw = animation();
                smgpc::compat::declare_event_camera_animation(info, "Retained owner animation", raw);
                std::fill(raw.begin(), raw.end(), 0xa5);
            }
            auto* event = director.getEventParameter(0, "Retained owner animation");
            auto* data = reinterpret_cast<u8*>(event->mGeneralParam->mNum1);
            require(CameraAnim::getAnimFrame(data) == 2 && JKRHeap::findFromRoot(data) == nullptr,
                    "actor camera chunk borrows retained host-endian CANM data after raw source destruction");
            binding.init_after_placement();
            binding.complete_initialization();
            require(owner->ready() && director.mCameraManGame->mIsStartPosActive,
                    "original close-creation scan, start/event creation, archive load and sort finish before gameplay");
            CameraParamChunkID_Tmp start_id;
            start_id.createStartID(MR::getCurrentStartZoneId(), MR::getCurrentStartCameraId());
            require(director.mChunkHolder->getChunk(start_id) != nullptr, "original start scan materializes the selected authored start camera ID");
            {
                smgpc::compat::JkrAllocationScope game(domain);
                director.mCameraManGame->selectCameraChunk();
                require(director.mCameraManGame->mChunk != nullptr &&
                        director.mCameraManGame->mCamera == director.mHolder->getCameraInner(director.mCameraManGame->mChunk->mCameraTypeIndex),
                        "actual game manager selects its authored controller through the complete holder");
                MR::startEventCamera(&info, "Retained owner animation", target_arg, 0);
                require(director.getCurrentCameraMan() == director.mCameraManEvent,
                        "original event request switches the director's actual camera-manager stack");
                director.mCameraManEvent->movement();
                near(CameraLocalUtil::getPos(director.mCameraManEvent).x, 110, "original event translator and CANM evaluator apply target translation");
                near(CameraLocalUtil::getPos(director.mCameraManEvent).z, 900, "original CANM eye depth survives endian decoding");
                matrix.setTrans(TVec3f(150, 200, 300)); target->setMtx(matrix); target->movement();
                director.mCameraManEvent->movement();
                near(CameraLocalUtil::getPos(director.mCameraManEvent).x, 180, "next original event frame follows actual live target movement");
                near(CameraLocalUtil::getFovy(director.mCameraManEvent), 60, "original CANM fovy track reaches its next authored key");
            }
            TPos3f view;
            view.setPositionFromLookAt(TVec3f(120, 400, 600), TVec3f(0, 1, 0), TVec3f(120, 400, 0));
            MR::setCameraViewMtx(view, false, false, TVec3f(0, 0, 0));
            MR::setNearZ(75); MR::setFovy(55); MR::setShakeOffset(0.01F, -0.02F);
            runtime.refresh_scene_camera_pose();
            const auto pose = runtime.scene_camera_pose();
            require(pose.has_value() && &MR::getCameraViewMtx() == &owner->context().mView,
                    "renderer camera state is derived from actual original context matrix");
            near(pose->eye.x, 120, "renderer receives exact original eye");
            near(pose->near_clip, 75, "renderer receives original near clip");
            near(pose->fovy_degrees, 55, "renderer receives original fovy");
            near(pose->projection_offset_y, -0.02F, "renderer receives original shaker projection offset");
            camera_category_precedes_clipping(runtime, director, execution);
        }
        require(weak_domain.expired() && !smgpc::camera::current_camera_director_runtime() &&
                !smgpc::camera::current_original_camera_context(), "scene teardown retires camera publication and its entire Game domain");
        require(scheduler.snapshot().size() == baseline_entries &&
                smgpc::compat::name_obj_runtime_state_count() == baseline_objects &&
                process.host_heaps()->root_heap().getFreeSize() == baseline_free,
                "all original camera descendants, capture callbacks and transient allocations retire before next scene");
        std::cout << stage << ": original ownership, authored selection, event dispatch, CANM retention, projection and teardown passed\n";
    }
}
