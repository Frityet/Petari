#include "OriginalStageResourceProcessFixture.hpp"
#include "OriginalSceneControllerFixture.hpp"
#include "SceneExecutionFixture.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/Map/LightDirector.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/LightPointCtrl.hpp"
#include "Game/Camera/CameraContext.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/Color.hpp"
#include "Game/Util/LightUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void requireNear(f32 actual, f32 expected, std::string_view message) {
        require(std::isfinite(actual) &&
                    std::fabs(actual - expected) < 0.00001F,
                message);
    }

    void requirePosition(const TVec3f &actual, const TVec3f &expected,
                         std::string_view message) {
        requireNear(actual.x, expected.x, message);
        requireNear(actual.y, expected.y, message);
        requireNear(actual.z, expected.z, message);
    }

    void requireColor(const _GXColor &actual, const _GXColor &expected,
                      std::string_view message) {
        require(actual.r == expected.r && actual.g == expected.g &&
                    actual.b == expected.b && actual.a == expected.a,
                message);
    }

    void requireAbsent(const PointLightInfo &info,
                       std::string_view message) {
        requirePosition(info.mPosition, TVec3f{}, message);
        requireColor(info.mColor, _GXColor{0U, 0U, 0U, 255U}, message);
        requireNear(info.mRadius, 15.0F, message);
        requireNear(info.mBrightness, 0.001F, message);
        require(info.mDistAttnFn == GX_DA_STEEP, message);
    }

    void testOriginalPlayerLightOwnership() {
        using namespace smgpc;
        const auto heaps = compat::JkrHeapRuntime::create(16U << 20);
        test::OriginalSceneControllerFixture original(heaps);
        runtime::SceneScheduler scheduler;
        runtime::SceneSchedulerBinding active(scheduler);
        for (unsigned generation = 0; generation < 8; ++generation) {
            const auto domain = compat::JkrAllocationDomain::create(heaps, 1U << 20);
            test::SceneExecutionFixture scene(scheduler, domain,
                                              &original.scene);
            alignas(32) std::array<u8, 4096> commands{};
            GXBeginDisplayList(commands.data(), commands.size());
            auto* director = static_cast<LightDirector*>(MR::createSceneObj(SceneObj_LightDirector));
            GXEndDisplayList();
            require(director && !director->_1C, "each real scene starts without a borrowed player controller");
            MR::createSceneObj(SceneObj_ClippingDirector);
            {
                LiveActor first("first original player light"), second("second original player light");
                compat::replace_actor_light_ctrl(&first);
                compat::replace_actor_light_ctrl(&second);
                LightFunction::registerPlayerLightCtrl(first.mActorLightCtrl);
                require(director->_1C == first.mActorLightCtrl &&
                            first.mActorLightCtrl->mRegisteredLightDirector == director,
                        "registration uses the original director's actual borrowed pointer");
                LightFunction::registerPlayerLightCtrl(second.mActorLightCtrl);
                compat::replace_actor_light_ctrl(&first);
                require(director->_1C == second.mActorLightCtrl,
                        "replacing an unregistered controller preserves the active player");
                compat::replace_actor_light_ctrl(&second);
                require(!director->_1C, "replacing the registered controller clears its borrowed pointer");
                LightFunction::registerPlayerLightCtrl(first.mActorLightCtrl);
            }
            require(!director->_1C, "actor retirement clears the original director before the controller is freed");
        }
        require(MR::getSceneObjHolder() == nullptr,
                "retired original scenes expose no player light controller");
    }

    void testClampAndCandidateContract(LiveActor &player) {
        auto farActor = LiveActor("point-light far candidate");
        auto tieActor = LiveActor("point-light tie candidate");
        auto nearActor = LiveActor("point-light near candidate");
        player.mPosition.zero();
        farActor.mPosition.set(100.0F, 0.0F, 0.0F);
        tieActor.mPosition.set(-100.0F, 0.0F, 0.0F);
        nearActor.mPosition.set(25.0F, 0.0F, 0.0F);

        auto controller = LightPointCtrl{};
        require(controller._0 == -1 && controller._4 == 30 &&
                    controller._8 == nullptr && controller._C == nullptr &&
                    controller._10 == nullptr && controller._14 != nullptr &&
                    controller._18 != nullptr && controller._1C != nullptr,
                "LightPointCtrl must begin in the retail idle/empty state");
        requireAbsent(*controller._14,
                      "the loaded point-light record must begin absent");
        requireAbsent(*controller._18,
                      "the request point-light record must begin absent");
        requireAbsent(*controller._1C,
                      "the transition point-light record must begin absent");

        const auto nan = std::numeric_limits<f32>::quiet_NaN();
        controller.requestPointLight(
            &farActor, TVec3f{1.0F, 2.0F, 3.0F},
            Color8(20U, 40U, 60U, 255U), nan, -1);
        require(controller._10 == &farActor &&
                    std::isnan(controller._18->mBrightness) &&
                    controller._4 == 30,
                "the first request must win and PPC unordered brightness must preserve NaN");

        controller.requestPointLight(
            &tieActor, TVec3f{4.0F, 5.0F, 6.0F},
            Color8(1U, 2U, 3U, 255U), 0.98F, 4);
        require(controller._10 == &farActor &&
                    std::isnan(controller._18->mBrightness),
                "an equal-distance request must not replace the first candidate");

        controller.requestPointLight(
            &nearActor, TVec3f{7.0F, 8.0F, 9.0F},
            Color8(200U, 100U, 50U, 255U), 2.0F, 2);
        require(controller._10 == &nearActor && controller._4 == 2 &&
                    controller._18->mBrightness == 0.999999F &&
                    controller._18->mRadius == 15.0F &&
                    controller._18->mDistAttnFn == GX_DA_STEEP,
                "a strictly nearer request must replace the candidate and clamp high");

        auto lowController = LightPointCtrl{};
        lowController.requestPointLight(
            &nearActor, TVec3f{}, Color8(255U, 255U, 255U, 255U),
            -std::numeric_limits<f32>::infinity(), 1);
        require(lowController._18->mBrightness == 0.95F,
                "finite and infinite brightness below the retail range must clamp low");
    }

    void testStaleAndAbaCandidateSafety() {
        alignas(LiveActor) auto storage =
            std::array<std::byte, sizeof(LiveActor)>{};
        auto *first =
            new (storage.data()) LiveActor("point-light first generation");
        auto controller = LightPointCtrl{};
        controller.requestPointLight(
            first, TVec3f{10.0F, 20.0F, 30.0F},
            Color8(255U, 0U, 0U, 255U), 0.98F, 5);
        const auto firstGeneration = controller._10Generation;
        first->~LiveActor();

        auto *second = new (storage.data()) LiveActor("point-light second generation");
        require(smgpc::compat::name_obj_runtime_generation(second) !=
                    firstGeneration,
                "the ABA fixture must reuse the pointer with a new generation");
        controller.update();
        require(controller._8 == nullptr && controller._C == nullptr &&
                    controller._10 == nullptr && controller._0 == -1 &&
                    controller._4 == 30,
                "a stale or ABA-reused request must become absent before dereference");
        requireAbsent(*controller._14,
                      "a stale request must leave the published light absent");
        requireAbsent(*controller._18,
                      "stale invalidation must clear the request record before the idle footer copies it");

        controller.requestPointLight(
            second, TVec3f{40.0F, 50.0F, 60.0F},
            Color8(20U, 80U, 40U, 255U), 0.98F, 1);
        controller.update();
        require(controller._0 == 1 && controller._8 == second,
                "the valid generation after a stale request must begin a normal fade-in");
        requirePosition(controller._14->mPosition,
                        TVec3f{40.0F, 50.0F, 60.0F},
                        "a valid post-stale fade-in must use its own target position");
        requireColor(controller._14->mColor,
                     _GXColor{0U, 0U, 0U, 255U},
                     "a valid post-stale fade-in must begin at black without stale color bleed");
        requireNear(controller._14->mBrightness, 0.95F,
                    "a valid post-stale fade-in must begin at the present endpoint brightness");
        controller.update();
        require(controller._0 == -1 && controller._C == second &&
                    controller._CGeneration ==
                        smgpc::compat::name_obj_runtime_generation(second),
                "the post-stale fade-in endpoint must track the valid generation");
        requireColor(controller._14->mColor,
                     _GXColor{20U, 80U, 40U, 255U},
                     "the post-stale fade-in endpoint must reach its own color");

        const auto secondGeneration = controller._CGeneration;
        second->~LiveActor();
        auto *third =
            new (storage.data()) LiveActor("point-light third generation");
        require(smgpc::compat::name_obj_runtime_generation(third) !=
                    secondGeneration,
                "the tracked-actor ABA fixture must reuse the pointer with a new generation");
        controller.requestPointLight(
            third, TVec3f{-10.0F, -20.0F, -30.0F},
            Color8(180U, 30U, 90U, 255U), 0.97F, 2);
        controller.update();
        require(controller._0 == 1 && controller._8 == third &&
                    controller._8Generation != secondGeneration,
                "pointer reuse for a tracked actor must start an actor-switch blend");
        requirePosition(controller._14->mPosition,
                        TVec3f{40.0F, 50.0F, 60.0F},
                        "tracked-actor ABA step zero must retain the old generation position");
        requireColor(controller._14->mColor,
                     _GXColor{20U, 80U, 40U, 255U},
                     "tracked-actor ABA must not take the same-actor immediate path");
        third->~LiveActor();
    }

    void testZeroDurationIsNotNormalized(LiveActor &actor) {
        auto controller = LightPointCtrl{};
        controller.requestPointLight(
            &actor, TVec3f{3.0F, 4.0F, 5.0F},
            Color8(200U, 100U, 50U, 255U), 0.98F, 0);
        controller.update();
        require(controller._4 == 0 && controller._0 == -1 &&
                    controller._C == &actor,
                "a zero-duration transition must execute one deterministic retail step without normalization");
        requirePosition(controller._14->mPosition,
                        TVec3f{3.0F, 4.0F, 5.0F},
                        "fade-in position must use the target even for duration zero");
        requireNear(controller._14->mBrightness, 0.95F,
                    "duration zero must use the retail cosine rate-zero brightness endpoint");
        requireColor(controller._14->mColor,
                     _GXColor{0U, 0U, 0U, 255U},
                     "duration zero must use the retail cosine rate-zero black color endpoint");
        requireNear(controller._14->mRadius, 15.0F,
                    "duration zero must retain the retail point-light radius");
    }

    void testOriginalGxLightSubmission() {
        require(smgpc::runtime::RuntimeContext::try_instance() == nullptr,
                "direct light proof must not install the host showcase runtime");
        alignas(32) std::array<u8, 4096> commands{};
        auto decode = [&commands](u32 size) {
            auto registers = std::map<u16, u32>{};
            auto word = [&commands](std::size_t offset) {
                return u32(commands[offset]) << 24 | u32(commands[offset + 1]) << 16 |
                       u32(commands[offset + 2]) << 8 | commands[offset + 3];
            };
            for (std::size_t offset = 0; offset < size;) {
                const auto opcode = commands[offset++];
                if (opcode == 0) continue;
                require(opcode == 0x10 && offset + 4 <= size,
                        "light APIs must encode actual XF commands");
                const auto header = word(offset);
                offset += 4;
                const auto count = (header >> 16) + 1;
                require(offset + count * 4 <= size, "complete XF light register write");
                for (u32 index = 0; index < count; ++index) {
                    registers[static_cast<u16>((header & 0xffff) + index)] = word(offset);
                    offset += 4;
                }
            }
            return registers;
        };
        ActorLightInfo actor;
        actor.mInfo0 = LightInfo{GXColor{17, 34, 51, 68}, TVec3f{10, 20, 30}, true};
        actor.mInfo1 = LightInfo{GXColor{81, 82, 83, 84}, TVec3f{-40, 50, -60}, true};
        actor.mAlpha2 = 137;
        actor.mColor = GXColor{64, 75, 85, 110};
        GXBeginDisplayList(commands.data(), commands.size());
        LightFunction::loadActorLightInfo(&actor);
        const auto actorRegisters = decode(GXEndDisplayList());
        require(actorRegisters.at(0x603) == 0x11223344 && actorRegisters.at(0x613) == 0x51525354 &&
                    actorRegisters.at(0x623) == 137 && actorRegisters.at(0x100a) == 0x404b556e,
                "authored distinct-channel diffuse, alpha and ambient reach GX without RuntimeContext");
        require(actorRegisters.at(0x60a) == std::bit_cast<u32>(10.0f) &&
                    actorRegisters.at(0x61c) == std::bit_cast<u32>(-60.0f) &&
                    actorRegisters.at(0x604) == std::bit_cast<u32>(1.0f) &&
                    actorRegisters.at(0x607) == std::bit_cast<u32>(1.0f),
                "follow-camera positions and diffuse attenuation retain original GX registers");
        GXBeginDisplayList(commands.data(), commands.size());
        LightFunction::loadAllLightWhite();
        const auto whiteRegisters = decode(GXEndDisplayList());
        for (u16 light = 0; light < 8; ++light) {
            require(whiteRegisters.at(0x603 + light * 16) == 0xffffffff &&
                        whiteRegisters.at(0x60a + light * 16) == 0 &&
                        whiteRegisters.at(0x60b + light * 16) == 0 &&
                        whiteRegisters.at(0x60c + light * 16) == 0,
                    "retail all-white initializer writes eight white lights at origin");
        }
        LightInfoCoin coin;
        coin.base = actor.mInfo0;
        coin._14 = {3, 5, 7, 11}; coin._18 = 65.0f;
        GXBeginDisplayList(commands.data(), commands.size());
        LightFunction::loadLightInfoCoin(&coin);
        const auto coinRegisters = decode(GXEndDisplayList());
        require(coinRegisters.at(0x603) == 0x11223344 && coinRegisters.at(0x633) == 0x0305070b &&
                    coinRegisters.at(0x637) == std::bit_cast<u32>(32.5f) &&
                    coinRegisters.at(0x639) == std::bit_cast<u32>(-31.5f),
                "coin uses original diffuse slot zero and GX_LIGHT3 specular attenuation");
    }
}

namespace {
    void testOriginalCatalog() {
        auto* director = MR::getSceneObj<LightDirector>(SceneObj_LightDirector);
        require(director && director->mResourceHolder && director->mDataHolder && director->mZoneDataHolder,
                "original scene owns the archive, light records and zone records");
        require(director->mDataHolder->mLightCount > 0 && director->mZoneDataHolder->mCount == MR::getZoneNum(),
                "original data owners load every light row and actual catalog zone");
        require(director->_C && director->_1C && director->_1C->mRegisteredLightDirector == director,
                "actual area and player controllers register with the original director");
        auto* root = LightFunction::getAreaLightInfo(ZoneLightID{});
        require(root == director->mDefaultAreaLight &&
                    smgpc::resource::decode_cp932(root->mAreaLightName) == "[共通]宇宙の星",
                "clear ZoneLightID resolves the authored root-zone default");
        require(smgpc::resource::decode_cp932(LightFunction::getDefaultAreaLightName()) == "デフォルト",
                "original default catalog name is distinct from the stage default");
        s32 childZone = -1;
        for (s32 i = 0; i < MR::getZoneNum(); ++i) {
            if (std::string_view(MR::getZoneNameFromZoneId(i)) == "HeavensDoorMysteriousZone") childZone = i;
        }
        require(childZone >= 0, "actual scenario catalog contains the authored child zone");
        ZoneLightID id;
        id._0 = childZone;
        id.mLightID = 0;
        const auto* child = LightFunction::getAreaLightInfo(id);
        require(child && child != root && smgpc::resource::decode_cp932(child->mAreaLightName) == "ロゼッタ出会い",
                "child-zone light zero resolves the Rosetta meeting row");
        require(child->mPlayerLight.mInfo0.mColor.r == 90 && child->mPlayerLight.mInfo0.mColor.g == 90 &&
                    child->mPlayerLight.mInfo0.mColor.b == 90,
                "Rosetta diffuse color comes from original CSV parsing");
        require(!child->mPlayerLight.mInfo0.mIsFollowCamera && child->mPlayerLight.mInfo1.mIsFollowCamera,
                "authored world-space and camera-space flags survive the original parser");
        requireColor(child->mPlayerLight.mColor, GXColor{90, 90, 115, 60},
                     "Rosetta ambient bytes come from original CSV parsing");
        id.mLightID = 1;
        require(smgpc::resource::decode_cp932(LightFunction::getAreaLightInfo(id)->mAreaLightName) == "天文台（ロゼッタ）",
                "child-zone second light row is distinct");
        id.mLightID = 999;
        require(LightFunction::getAreaLightInfo(id) == director->mDataHolder->findAreaLight("\x83\x66\x83\x74\x83\x48\x83\x8b\x83\x67"),
                "missing child ID uses the original literal default lookup");
        requireColor(director->mDataHolder->_8.base.mColor, GXColor{255, 255, 0, 0},
                     "coin light defaults belong to the original data holder");
        require(director->mDataHolder->_8.base.mIsFollowCamera && director->mDataHolder->_8._18 == 65.0f,
                "coin camera and specular defaults retain the original values");
    }

    void testCoordinateBlend() {
        auto* camera = MR::getSceneObj<CameraContext>(SceneObj_CameraContext);
        const auto savedView = camera->mView;
        const auto savedInverse = camera->mViewInv;
        struct Restore {
            CameraContext* camera;
            TPos3f view, inverse;
            ~Restore() { camera->mView = view; camera->mViewInv = inverse; }
        } restore{camera, savedView, savedInverse};
        camera->mView.identity();
        camera->mView.setTrans(TVec3f(-100, -200, -300));
        camera->mViewInv.identity();
        camera->mViewInv.setTrans(TVec3f(100, 200, 300));

        ActorLightInfo from{}, to{}, result{};
        from.mInfo0 = {{10, 20, 30, 40}, {110, 220, 330}, false};
        to.mInfo0 = {{50, 60, 70, 80}, {30, 40, 50}, true};
        from.mInfo1 = {{10, 20, 30, 40}, {10, 20, 30}, true};
        to.mInfo1 = {{50, 60, 70, 80}, {130, 240, 350}, false};
        from.mAlpha2 = 10;
        to.mAlpha2 = 90;
        result.mInfo0.mIsFollowCamera = true;
        result.mInfo1.mIsFollowCamera = false;
        LightFunction::blendActorLightInfo(&result, from, to, 0.25f);
        requirePosition(result.mInfo0.mPos, TVec3f(15, 25, 35), "world source converts to target camera space before blending");
        requirePosition(result.mInfo1.mPos, TVec3f(115, 225, 335), "camera source converts to target world space before blending");
        require(result.mInfo0.mIsFollowCamera && !result.mInfo1.mIsFollowCamera && result.mAlpha2 == 30,
                "blend preserves destination spaces and original alpha interpolation");
        requireColor(result.mInfo0.mColor, GXColor{20, 30, 40, 50}, "original color interpolation truncates authored bytes");
        LightFunction::blendActorLightInfo(&result, from, to, 1.5f);
        requirePosition(result.mInfo0.mPos, TVec3f(40, 50, 60), "original interpolation extrapolates without clamping the rate");
        requireColor(result.mInfo0.mColor, GXColor{70, 80, 90, 100}, "color extrapolation uses the same unclamped rate");
        TVec3f world;
        LightFunction::calcLightWorldPos(&world, to.mInfo0);
        requirePosition(world, TVec3f(130, 240, 350), "follow-camera light converts through actual inverse view");
    }

    void testPointTransition() {
        LiveActor actor("original point transition");
        LightPointCtrl controller;
        controller.requestPointLight(&actor, TVec3f(25, 5, -2), Color8(200, 100, 50, 255), 0.99f, 2);
        controller.update();
        require(controller._0 == 1 && controller._8 == &actor, "fade-in includes step zero");
        requireColor(controller._14->mColor, GXColor{0, 0, 0, 255}, "fade-in begins at black");
        requireNear(controller._14->mBrightness, 0.95f, "fade-in begins at minimum present brightness");
        controller.update();
        requireColor(controller._14->mColor, GXColor{100, 50, 25, 255}, "cosine midpoint blends half color");
        requireNear(controller._14->mBrightness, 0.97f, "cosine midpoint blends brightness");
        controller.update();
        require(controller._0 == -1 && controller._C == &actor, "fade-in includes its endpoint");
        requireColor(controller._14->mColor, GXColor{200, 100, 50, 255}, "fade-in reaches authored color");
        controller.update();
        require(controller._0 == 1 && controller._8 == nullptr && controller._4 == 30, "missed request starts the original thirty-step fade-out");
        for (int i = 0; i < 15; ++i) controller.update();
        requireColor(controller._14->mColor, GXColor{100, 50, 25, 255}, "fade-out retains the cosine midpoint");
        requirePosition(controller._14->mPosition, TVec3f(25, 5, -2), "fade-out freezes the prior actor position");
        for (int i = 0; i < 15; ++i) controller.update();
        require(controller._0 == -1 && controller._C == nullptr, "fade-out includes step thirty");
        controller.update();
        requireAbsent(*controller._14, "idle frame after fade-out clears the real point record");
    }
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--original-owner-only") {
            testOriginalPlayerLightOwnership();
            std::cout << "PASS player-light registration, replacement, retirement and eight scene lifetimes\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--original-gx-only") {
            testOriginalGxLightSubmission();
            std::cout << "PASS original diffuse, ambient, alpha, white and coin GX commands\n";
            return 0;
        }
        return smgpc::test::run_stage_resource_process("original-light-owners", [] {
            testOriginalCatalog();
            testCoordinateBlend();
            auto& player = *MR::getMarioHolder()->getMarioActor();
            const auto position = player.mPosition;
            struct RestorePosition { LiveActor& actor; TVec3f position; ~RestorePosition() { actor.mPosition = position; } } restore{player, position};
            testClampAndCandidateContract(player);
            testStaleAndAbaCandidateSafety();
            LiveActor zeroDuration("original point zero duration");
            testZeroDurationIsNotNormalized(zeroDuration);
            testPointTransition();
            testOriginalGxLightSubmission();
        });
    } catch (const std::exception& error) {
        std::cerr << "FAIL original lights: " << error.what() << '\n';
        return 1;
    }
}
