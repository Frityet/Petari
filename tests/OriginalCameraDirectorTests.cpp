#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
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
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/ClippingJudge.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"

#include <aurora/exception.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <unistd.h>

namespace {
    void require(bool value, const char* message) {
        if (!value) aurora::throw_host_exception<std::runtime_error>(message);
    }
    void near(float actual, float expected, const char* message) {
        require(std::isfinite(actual) && std::abs(actual - expected) < 0.01F, message);
    }
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
            // Original PSVECNormalize refines frsqrte and rounds an axis to the float below one.
            require(std::bit_cast<u32>(pattern->mDirection.x) == 0x3f7fffff &&
                    std::bit_cast<u32>(pattern->mDirection.y) == 0,
                    "all seven original transient patterns preserve exact retail horizontal normalization");
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

    void original_clipping_judge(CameraDirector& director) {
        auto* clipping = static_cast<ClippingDirector*>(MR::createSceneObj(SceneObj_ClippingDirector));
        auto& judge = *MR::getClippingJudge();
        require(&judge == clipping->mJudge, "clipping utility exposes the actual scene Director child");
        const auto original_subjective_frame = director.mSubjectiveFrame;
        TPos3f view;
        view.identity();
        MR::setCameraViewMtx(view, false, false, TVec3f(0, 0, 0));
        MR::setNearZ(8); MR::setFovy(90);
        director.mSubjectiveFrame = 0;
        clipping->movement();
        require(!judge.isJudgedToClipFrustum(TVec3f(0, 0, -500), 0) &&
                judge.isJudgedToClipFrustum(TVec3f(0, 0, -499), 0),
                "original clipping near plane is 500 independently of the projection near plane");
        require(!judge.isJudgedToClipFrustum(TVec3f(0, 0, -400), 100) &&
                judge.isJudgedToClipFrustum(TVec3f(0, 0, -399), 100),
                "original near-plane sphere tangency is visible");
        for (s32 level = 0; level < 8; ++level) {
            const f32 distance = level ? judge.mClipDistances[level] : MR::getFarZ();
            require(!judge.isJudgedToClipFrustum(TVec3f(0, 0, -distance), 0, level) &&
                    judge.isJudgedToClipFrustum(TVec3f(0, 0, -distance - 2), 0, level),
                    "the camera far plane and all seven original distance planes remain distinct");
        }
        const f32 right = 1000 * MR::getAspect();
        require(!judge.isJudgedToClipFrustum(TVec3f(right - 1, 0, -1000), 0) &&
                judge.isJudgedToClipFrustum(TVec3f(right + 1, 0, -1000), 0) &&
                !judge.isJudgedToClipFrustum(TVec3f(0, 999, -1000), 0) &&
                judge.isJudgedToClipFrustum(TVec3f(0, 1001, -1000), 0),
                "the original side planes use current fovy and aspect");
        director.mSubjectiveFrame = 1;
        clipping->movement();
        require(!judge.isJudgedToClipFrustum(TVec3f(0, 0, -100), 0) &&
                judge.isJudgedToClipFrustum(TVec3f(0, 0, -99), 0),
                "the actual CameraDirector subjective frame selects the original 100-unit near plane");
        director.mSubjectiveFrame = 0;
        view.setPositionFromLookAt(TVec3f(100, 200, 300), TVec3f(0, 1, 0), TVec3f(-900, 200, 300));
        MR::setCameraViewMtx(view, false, false, TVec3f(0, 0, 0));
        clipping->movement();
        require(!judge.isJudgedToClipFrustum(TVec3f(-500, 200, 300), 0) &&
                judge.isJudgedToClipFrustum(TVec3f(700, 200, 300), 0),
                "translated and rotated actual camera matrices determine the clipping half-spaces");
        director.mSubjectiveFrame = original_subjective_frame;
    }


#ifndef NDEBUG
    constexpr std::uint64_t frame_count = 120;

    void verify_terminal_camera(smgpc::camera::CameraDirectorRuntime& owner) {
        auto& director = owner.director();
        const auto domain = MR::getSceneObjHolder()->nativeAllocationDomain();
        require(domain != nullptr, "camera checks borrow the actual original scene allocation owner");
        const smgpc::compat::JkrAllocationScope allocation(domain);
        const J3DSys::ContextScope commands;
        require(JKRHeap::findFromRoot(&director) == &domain->heap() &&
                    JKRHeap::findFromRoot(director.mCameraCreator) == &domain->heap(),
                "the naturally initialized director and creator belong to the original scene heap");
        shaker_storage(*director.mShaker, domain);

        auto& context = owner.context();
        const auto view = context.mView;
        const auto inverse = context.mViewInv;
        const auto projection = context.mProjection;
        const auto near_z = context.mNearZ;
        const auto fovy = context.mFovy;
        const auto shake = context.mShakeOffset;
        auto* target_before = director.getTarget();
        auto* target = director.mCameraTargetMtx;
        struct TargetFields {
            TPos3f matrix;
            TVec3f position, last_move, gravity, up, front, side;
            bool invalid_last_move;
            CubeCameraArea* area;
        } saved_target{target->mMatrix, target->mPosition, target->mLastMove, target->mGravityVector,
                       target->mUp, target->mFront, target->mSide, target->mInvalidLastMove, target->mCameraArea};
        struct Restore {
            CameraContext& context;
            TPos3f view, inverse;
            TProj3f projection;
            float near_z, fovy;
            TVec2f shake;
            CameraDirector& director;
            CameraTargetObj* target_before;
            CameraTargetMtx& target;
            const TargetFields& saved_target;
            ~Restore() {
                context.mView = view;
                context.mViewInv = inverse;
                context.mProjection = projection;
                context.mNearZ = near_z;
                context.mFovy = fovy;
                context.mShakeOffset = shake;
                target.mMatrix = saved_target.matrix;
                target.mPosition = saved_target.position;
                target.mLastMove = saved_target.last_move;
                target.mGravityVector = saved_target.gravity;
                target.mUp = saved_target.up;
                target.mFront = saved_target.front;
                target.mSide = saved_target.side;
                target.mInvalidLastMove = saved_target.invalid_last_move;
                target.mCameraArea = saved_target.area;
                director.setTarget(target_before);
            }
        } restore{context, view, inverse, projection, near_z, fovy, shake,
                  director, target_before, *target, saved_target};

        TPos3f matrix;
        matrix.identity();
        matrix.setTrans(TVec3f(100.0f, 200.0f, 300.0f));
        target->setMtx(matrix);
        CameraTargetArg target_arg(target);
        target_arg.setTarget();
        target->movement();
        require(director.getTarget() == target && target->getLastMove().squared() == 0,
                "the original target argument selects its real matrix target and invalidates initial displacement");

        u8* data = nullptr;
        {
            const smgpc::compat::JkrHostAllocationScope host;
            auto raw = animation();
            data = static_cast<u8*>(owner.retain_animation(raw));
            std::fill(raw.begin(), raw.end(), 0xa5);
        }
        require(CameraAnim::getAnimFrame(data) == 2 && JKRHeap::findFromRoot(data) == nullptr,
                "the real camera owner retains host-endian CANM data after source destruction");
        // A controlled original CameraAnim borrows the actual manager/target.
        // Do not append a new event to the already closed/sorted authored
        // chunk table or replace the scene's in-progress opening demo.
        CameraAnim camera("Original retained CANM regression");
        camera.mCameraMan = director.mCameraManGame;
        struct CameraChildren {
            CameraAnim& camera;
            ~CameraChildren() {
                delete camera.mDataAccessor;
                delete camera.mKeyDataAccessor;
                delete camera.mPoseParam;
            }
        } camera_children{camera};
        camera.setParam(data, 1.0f);
        camera.reset();
        camera.calc();
        near(CameraLocalUtil::getPos(&camera).x, 110, "original CANM evaluation applies actual target translation");
        near(CameraLocalUtil::getPos(&camera).z, 900, "original CANM eye depth survives endian decoding");
        matrix.setTrans(TVec3f(150.0f, 200.0f, 300.0f));
        target->setMtx(matrix);
        target->movement();
        camera.calc();
        near(CameraLocalUtil::getPos(&camera).x, 180, "next original CANM frame follows actual target movement");
        near(CameraLocalUtil::getFovy(&camera), 60, "original CANM fovy reaches the next authored key");

        TPos3f test_view;
        test_view.setPositionFromLookAt(TVec3f(120, 400, 600), TVec3f(0, 1, 0), TVec3f(120, 400, 0));
        MR::setCameraViewMtx(test_view, false, false, TVec3f(0, 0, 0));
        MR::setNearZ(75);
        MR::setFovy(55);
        MR::setShakeOffset(0.01F, -0.02F);
        const auto pose = owner.pose();
        require(&MR::getCameraViewMtx() == &context.mView,
                "the renderer adapter reads the actual original CameraContext matrix");
        near(pose.eye.x, 120, "renderer projection receives original eye");
        near(pose.near_clip, 75, "renderer projection receives original near clip");
        near(pose.fovy_degrees, 55, "renderer projection receives original fovy");
        near(pose.projection_offset_y, -0.02F, "renderer projection receives original shake offset");
        // This changes the judge and clipping membership only after the last
        // normal game frame. Original scene retirement is the next operation.
        original_clipping_judge(director);
    }

    struct Probe {
        std::weak_ptr<smgpc::compat::JkrAllocationDomain> domain;
        const CameraDirector* identity = nullptr;
        std::uint64_t observed_frames = 0;
        bool original_event_observed = false;
        bool terminal_checked = false;

        void after_frame(GameSystem& system, std::uint64_t frame) {
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            const smgpc::compat::JkrHostAllocationScope host;
            auto* owner = smgpc::camera::current_camera_director_runtime();
            require(owner && owner->ready() && &owner->director() == MR::getCameraDirector(),
                    "normal scene initialization publishes the actual ready CameraDirector");
            auto& director = owner->director();
            require(!identity || identity == &director, "normal frames retain the same original camera owner");
            identity = &director;
            domain = MR::getSceneObjHolder()->nativeAllocationDomain();
            require(director.mHolder->getNum() == 45 && director.getCurrentCameraMan() != nullptr,
                    "the original process constructs all 45 camera controllers and a live manager");
            require(&owner->context() == MR::getSceneObjHolder()->getObj(SceneObj_CameraContext),
                    "camera context is the actual original SceneObj");
            CameraParamChunkID_Tmp start_id;
            start_id.createStartID(MR::getCurrentStartZoneId(), MR::getCurrentStartCameraId());
            require(director.mChunkHolder->getChunk(start_id) != nullptr,
                    "the original close-creation pass retains the authored selected start-camera chunk");
            if (director.getCurrentCameraMan() == director.mCameraManEvent) {
                auto& event = *director.mCameraManEvent;
                // startEvent pushes the manager before its next movement
                // applies the queued chunk and selects the camera. Only the
                // completed selection can be compared with the holder.
                if (event.mChunk && event.mCamera) {
                    require(event.mCamera == director.mHolder->getCameraInner(event.mChunk->mCameraTypeIndex),
                            "completed natural event selection uses the authored controller in the original holder");
                    original_event_observed = true;
                }
            }
            ++observed_frames;
            if (frame == frame_count - 1) {
                require(observed_frames >= 30 && original_event_observed,
                        "the original opening camera ran for at least thirty ordinary frames");
                verify_terminal_camera(*owner);
                terminal_checked = true;
                std::fprintf(stderr, "PASS original camera terminal owner/CANM/projection/clipping; frames=%llu\n",
                             static_cast<unsigned long long>(observed_frames));
            }
        }
    };
#endif
}

int main() {
#ifdef NDEBUG
    std::fprintf(stderr, "Original camera owner diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc for original camera owner validation");
        const auto save = std::filesystem::temp_directory_path() /
                          ("petari-original-camera-owner-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "camera diagnostic starts with a fresh console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE", "SMGPC_STRICT_PLACEMENT"})
            unsetenv(name);
        const smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original camera owner regression",
            .arguments = {"original-camera-owner-test", "--stage", "HeavensDoorGalaxy", "--scenario", "1",
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
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.terminal_checked,
                "the original process completes all camera checks and its requested bounded frame loop");
        require(probe.identity && probe.domain.expired() && !smgpc::camera::current_camera_director_runtime() &&
                    !smgpc::camera::current_original_camera_context(),
                "normal process teardown retires both camera publications and their original scene heap");
        std::fprintf(stderr, "PASS original camera ownership, authored dispatch, CANM retention, projection, clipping and retirement\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original camera owner: %s\n", error.what());
        return 1;
    }
#endif
}
