#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/LightDirector.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/LightPointCtrl.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/Color.hpp"
#include "Game/Util/LightUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/GXState.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageLightSceneBinding.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template <typename Exception>
    void requireThrows(const std::function<void()> &operation,
                       std::string_view messageFragment,
                       std::string_view message) {
        try {
            operation();
        } catch (const Exception &error) {
            require(std::string_view(error.what()).find(messageFragment) !=
                        std::string_view::npos,
                    message);
            return;
        }
        throw std::runtime_error(std::string(message));
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

    [[nodiscard]] std::array<std::uint8_t, 4U> colorValue(
        const _GXColor &color) {
        return {color.r, color.g, color.b, color.a};
    }

    void requireDiffuseLight(
        const smgpc::render::GXLightState *actual,
        const LightInfo &expected, std::string_view message) {
        require(actual != nullptr && actual->loaded &&
                    actual->coordinate_space ==
                        (expected.mIsFollowCamera ? smgpc::render::GXLightCoordinateSpace::View : smgpc::render::GXLightCoordinateSpace::World) &&
                    actual->color == colorValue(expected.mColor),
                message);
        requireNear(actual->position[0U], expected.mPos.x, message);
        requireNear(actual->position[1U], expected.mPos.y, message);
        requireNear(actual->position[2U], expected.mPos.z, message);
    }

    [[nodiscard]] std::optional<std::filesystem::path> findRealDisc() {
        if (const auto *configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            const auto path = std::filesystem::path(configured);
            if (std::filesystem::is_regular_file(path)) {
                return path;
            }
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        if (error) {
            return std::nullopt;
        }
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) &&
                    !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        return std::nullopt;
    }

    class DiscMount final {
    public:
        DiscMount() {
            const auto path = findRealDisc();
            if (!path.has_value()) {
                return;
            }
            aurora_dvd_close();
            const auto pathString = path->string();
            if (!aurora_dvd_open(pathString.c_str())) {
                throw std::runtime_error(
                    "the point-light fallback proof could not open the real SMG disc");
            }
            DVDInit();
            _mounted = true;
        }

        ~DiscMount() {
            if (_mounted) {
                aurora_dvd_close();
            }
        }

        [[nodiscard]] bool mounted() const {
            return _mounted;
        }

    private:
        bool _mounted = false;
    };

    class NpcPointLightRequester final : public LiveActor {
    public:
        NpcPointLightRequester()
            : LiveActor("point-light NPC requester"),
              color(200U, 100U, 50U, 255U) {
            MR::connectToSceneNpcMovement(this);
        }

        void movement() override {
            ++movementCount;
            if (enabled) {
                MR::requestPointLight(this, lightPosition, color, brightness,
                                      duration);
            }
        }

        TVec3f lightPosition{25.0F, 5.0F, -2.0F};
        Color8 color;
        f32 brightness = 0.99F;
        s32 duration = 2;
        s32 movementCount = 0;
        bool enabled = true;
    };

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

    void testRequestServiceBoundary(
        smgpc::runtime::RuntimeContext &runtime, LiveActor &actor) {
        requireThrows<std::invalid_argument>(
            [&] {
                MR::requestPointLight(nullptr, TVec3f{},
                                      Color8(1U, 2U, 3U, 255U), 0.98F,
                                      2);
            },
            "LiveActor",
            "a null point-light requester must fail separately from scene ownership");
        requireThrows<std::logic_error>(
            [&] {
                MR::requestPointLight(&actor, TVec3f{},
                                      Color8(1U, 2U, 3U, 255U), 0.98F,
                                      2);
            },
            "active SceneObjHolder",
            "a point-light request without a scene holder must fail loudly");

        auto sentinel = smgpc::render::GXLightState{};
        sentinel.coordinate_space =
            smgpc::render::GXLightCoordinateSpace::World;
        sentinel.color = {7U, 8U, 9U, 10U};
        sentinel.position = {11.0F, 12.0F, 13.0F};
        runtime.scene_lights().set_light(4U, sentinel);
        {
            auto standalone = LightPointCtrl{};
        }
        const auto *preserved = runtime.scene_lights().light(4U);
        require(preserved != nullptr &&
                    preserved->coordinate_space == sentinel.coordinate_space &&
                    preserved->color == sentinel.color &&
                    preserved->position == sentinel.position,
                "a standalone LightPointCtrl destructor must not clear the scene owner's GX_LIGHT4 slot");
        runtime.scene_lights().clear_light(4U);

        auto holder = SceneObjHolder{};
        auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        requireThrows<std::logic_error>(
            [&] {
                MR::requestPointLight(&actor, TVec3f{},
                                      Color8(1U, 2U, 3U, 255U), 0.98F,
                                      2);
            },
            "scene-owned LightDirector",
            "a point-light request must not lazily create its LightDirector owner");
        require(holder.getObj(SceneObj_LightDirector) == nullptr,
                "the missing-director failure must leave the SceneObj slot empty");

        auto *director = dynamic_cast<LightDirector *>(
            MR::createSceneObj(SceneObj_LightDirector));
        require(director != nullptr && director->mPointCtrl != nullptr,
                "the service-boundary fixture must create an initialized director");
        auto *pointController = director->mPointCtrl;
        director->mPointCtrl = nullptr;
        requireThrows<std::logic_error>(
            [&] {
                MR::requestPointLight(&actor, TVec3f{},
                                      Color8(1U, 2U, 3U, 255U), 0.98F,
                                      2);
            },
            "initialized LightPointCtrl",
            "a director without its initialized controller must fail loudly");
        director->mPointCtrl = pointController;
    }

    void testSceneOwnerSchedulingAndTransitions(
        smgpc::runtime::RuntimeContext &runtime, LiveActor &player,
        bool realDiscMounted) {
        auto stageLightBinding =
            std::unique_ptr<smgpc::scene::StageLightSceneBinding>{};
        AreaLightInfo *stageArea = nullptr;
        if (realDiscMounted) {
            runtime.set_current_stage_name("HeavensDoorGalaxy");
            const auto noPlacementTables =
                std::array<smgpc::scene::StagePlacementTable, 0U>{};
            stageLightBinding =
                std::make_unique<smgpc::scene::StageLightSceneBinding>(
                    runtime.dvd(), "HeavensDoorGalaxy", noPlacementTables);
            stageArea = LightFunction::getAreaLightInfo(ZoneLightID{});
            require(stageArea != nullptr,
                    "the real HeavensDoorGalaxy fixture must expose its default AreaLight row");
        }

        const auto nameBaseline =
            smgpc::compat::name_obj_runtime_state_count();
        runtime.scene_lights().clear();

        {
            auto holder = SceneObjHolder{};
            auto binding = smgpc::scene::SceneObjHolderBinding(holder);
            auto *director = dynamic_cast<LightDirector *>(
                MR::createSceneObj(SceneObj_LightDirector));
            require(director != nullptr && director->mPointCtrl != nullptr &&
                        MR::createSceneObj(SceneObj_LightDirector) == director &&
                        holder.getObj(SceneObj_LightDirector) == director,
                    "SceneObj 0x06 must own one reusable real LightDirector");
            require(std::string_view(director->getName()) == "ライト管理" &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            nameBaseline + 1U,
                    "LightDirector must retain its retail identity in the scene owner");
            require(runtime.scene_lights().player_light_ctrl() == nullptr,
                    "the simplified player fixture must exercise the no-registered-controller fallback");

            auto requester = NpcPointLightRequester{};
            auto farActor = LiveActor("point-light transition far");
            player.mPosition.zero();
            requester.mPosition.set(25.0F, 0.0F, 0.0F);
            farActor.mPosition.set(100.0F, 0.0F, 0.0F);
            auto *controller = director->mPointCtrl;

#ifndef NDEBUG
            const auto entries = runtime.scheduler().snapshot();
            const auto directorEntry = std::ranges::find_if(
                entries, [director](const auto &candidate) {
                    return candidate.name == director->getName();
                });
            const auto requesterEntry = std::ranges::find_if(
                entries, [&requester](const auto &candidate) {
                    return candidate.name == requester.getName();
                });
            require(directorEntry != entries.end() &&
                        directorEntry->movement_type ==
                            MR::MovementType_MapObj &&
                        directorEntry->calc_anim_type == -1 &&
                        directorEntry->draw_buffer_type == -1 &&
                        directorEntry->draw_type == -1,
                    "LightDirector must execute through MovementType_MapObj only");
            require(requesterEntry != entries.end() &&
                        requesterEntry->movement_type ==
                            MR::MovementType_NPC,
                    "the point-light requester fixture must submit from the real NPC movement tranche");
#endif

            // MapObj movement observes the previous frame's candidate before
            // the NPC movement tranche submits this frame's request.
            runtime.scheduler().execute_movement();
            require(requester.movementCount == 1 &&
                        controller->_10 == &requester &&
                        controller->_0 == -1 && controller->_4 == 2,
                    "the scheduled NPC request must remain queued for exactly one frame");
            requireAbsent(*controller->_14,
                          "the scheduled request frame must leave the published point light absent");

            runtime.scene_lights().clear_light(4U);
            MR::loadLight(MR::LightType_Strong);
            require(runtime.scene_lights().light(4U) == nullptr,
                    "non-player lighting must not publish the point-light slot");

            for (auto slot = std::size_t{}; slot < 3U; ++slot) {
                runtime.scene_lights().clear_light(slot);
            }
            runtime.scene_lights().clear_light(4U);
            runtime.scene_lights().clear_actor_ambient();
            MR::loadLightPlayer();
            if (stageArea != nullptr) {
                const auto &fallback = stageArea->mPlayerLight;
                requireDiffuseLight(runtime.scene_lights().light(0U),
                                    fallback.mInfo0,
                                    "director player loading must preserve the StageLightData light-0 fallback");
                requireDiffuseLight(runtime.scene_lights().light(1U),
                                    fallback.mInfo1,
                                    "director player loading must preserve the StageLightData light-1 fallback");
                const auto *light2 = runtime.scene_lights().light(2U);
                require(light2 != nullptr &&
                            light2->coordinate_space ==
                                smgpc::render::GXLightCoordinateSpace::View &&
                            light2->color ==
                                std::array<std::uint8_t, 4U>{
                                    0U, 0U, 0U, fallback.mAlpha2} &&
                            light2->position ==
                                std::array<f32, 3U>{0.0F, 0.0F, 0.0F},
                        "director player loading must preserve the StageLightData light-2 fallback");
                const auto &ambient = runtime.scene_lights().actor_ambient();
                require(ambient.has_value() &&
                            *ambient == colorValue(fallback.mColor),
                        "director player loading must preserve the StageLightData player ambient fallback");
            } else {
                require(runtime.scene_lights().light(0U) == nullptr &&
                            runtime.scene_lights().light(1U) == nullptr &&
                            runtime.scene_lights().light(2U) == nullptr &&
                            !runtime.scene_lights().actor_ambient().has_value(),
                        "an absent retail disc must leave the unavailable player fallback explicit");
            }
            const auto *absentLight = runtime.scene_lights().light(4U);
            require(absentLight != nullptr &&
                        absentLight->coordinate_space ==
                            smgpc::render::GXLightCoordinateSpace::World &&
                        absentLight->color ==
                            std::array<std::uint8_t, 4U>{0U, 0U, 0U, 255U} &&
                        absentLight->position ==
                            std::array<f32, 3U>{0.0F, 0.0F, 0.0F},
                    "player loading must append the currently absent GX_LIGHT4 record to its fallback");

            runtime.scheduler().execute_movement();
            require(requester.movementCount == 2 && controller->_0 == 1 &&
                        controller->_8 == &requester,
                    "the following MapObj frame must freeze the scheduled candidate and execute cosine step zero");
            requirePosition(controller->_14->mPosition,
                            TVec3f{25.0F, 5.0F, -2.0F},
                            "fade-in step zero must use the target position");
            requireColor(controller->_14->mColor,
                         _GXColor{0U, 0U, 0U, 255U},
                         "fade-in cosine step zero must begin at black");
            requireNear(controller->_14->mBrightness, 0.95F,
                        "fade-in cosine step zero must use brightness 0.95");
            requireNear(controller->_14->mRadius, 15.0F,
                        "fade-in cosine step zero must use radius 15");

            requester.lightPosition.set(-500.0F, 0.0F, 0.0F);
            requester.color = Color8(1U, 2U, 3U, 255U);
            requester.brightness = 0.95F;
            requester.duration = 20;
            runtime.scheduler().execute_movement();
            require(requester.movementCount == 3 && controller->_0 == 2 &&
                        controller->_4 == 2,
                    "scheduled requests during a blend must not mutate its frozen duration");
            requirePosition(controller->_18->mPosition,
                            TVec3f{25.0F, 5.0F, -2.0F},
                            "scheduled requests during a blend must not mutate its frozen target");
            requirePosition(controller->_14->mPosition,
                            TVec3f{25.0F, 5.0F, -2.0F},
                            "fade-in midpoint must keep the target position");
            requireColor(controller->_14->mColor,
                         _GXColor{100U, 50U, 25U, 255U},
                         "duration-two fade-in step one must be the exact color midpoint");
            requireNear(controller->_14->mBrightness, 0.97F,
                        "duration-two fade-in step one must be the exact brightness midpoint");
            requireNear(controller->_14->mRadius, 15.0F,
                        "duration-two fade-in midpoint must retain radius 15");

            requester.lightPosition.set(30.0F, 6.0F, -1.0F);
            requester.color = Color8(20U, 40U, 60U, 255U);
            requester.brightness = 0.98F;
            requester.duration = 19;
            runtime.scheduler().execute_movement();
            require(requester.movementCount == 4 && controller->_0 == -1 &&
                        controller->_C == &requester &&
                        controller->_10 == &requester &&
                        controller->_4 == 19,
                    "the inclusive fade-in endpoint must accept the NPC's next request after the MapObj footer");
            requirePosition(controller->_14->mPosition,
                            TVec3f{25.0F, 5.0F, -2.0F},
                            "the inclusive fade-in endpoint must retain its frozen target position");
            requireColor(controller->_14->mColor,
                         _GXColor{200U, 100U, 50U, 255U},
                         "the inclusive fade-in endpoint must reach the frozen target color");
            requireNear(controller->_14->mBrightness, 0.99F,
                        "the inclusive fade-in endpoint must reach the frozen target brightness");
            requireNear(controller->_14->mRadius, 15.0F,
                        "the inclusive fade-in endpoint must retain radius 15");

            requester.enabled = false;
            runtime.scheduler().execute_movement();
            require(requester.movementCount == 5 && controller->_0 == -1 &&
                        controller->_C == &requester &&
                        controller->_10 == nullptr,
                    "the same live actor generation must track immediately without a blend");
            requirePosition(controller->_14->mPosition,
                            TVec3f{30.0F, 6.0F, -1.0F},
                            "same-actor tracking must publish its new position immediately");
            requireColor(controller->_14->mColor,
                         _GXColor{20U, 40U, 60U, 255U},
                         "same-actor tracking must publish its new color immediately");
            requireNear(controller->_14->mBrightness, 0.98F,
                        "same-actor tracking must publish its new brightness immediately");
            requireNear(controller->_14->mRadius, 15.0F,
                        "same-actor tracking must publish radius 15 immediately");

            runtime.scene_lights().clear_light(4U);
            MR::loadLightPlayer();
            const auto *activeLight = runtime.scene_lights().light(4U);
            require(activeLight != nullptr && activeLight->loaded &&
                        activeLight->coordinate_space ==
                            smgpc::render::GXLightCoordinateSpace::World &&
                        activeLight->position ==
                            std::array<f32, 3U>{30.0F, 6.0F, -1.0F} &&
                        activeLight->color ==
                            std::array<std::uint8_t, 4U>{20U, 40U, 60U,
                                                         255U},
                    "active player point-light loading must publish GX_LIGHT4 in world space");
            requireNear(activeLight->cosine_attenuation[0U], 1.0F,
                        "active GX_LIGHT4 must use cosine constant one");
            requireNear(activeLight->cosine_attenuation[1U], 0.0F,
                        "active GX_LIGHT4 must use zero cosine linear term");
            requireNear(activeLight->cosine_attenuation[2U], 0.0F,
                        "active GX_LIGHT4 must use zero cosine quadratic term");
            const auto activeSteepQuadratic =
                (1.0F - 0.98F) / (0.98F * 15.0F * 15.0F);
            requireNear(activeLight->distance_attenuation[0U], 1.0F,
                        "active GX_LIGHT4 steep attenuation must use constant one");
            requireNear(activeLight->distance_attenuation[1U], 0.0F,
                        "active GX_LIGHT4 steep attenuation must use zero linear term");
            requireNear(activeLight->distance_attenuation[2U],
                        activeSteepQuadratic,
                        "active GX_LIGHT4 must use independently computed steep attenuation");

            MR::requestPointLight(
                &farActor, TVec3f{100.0F, 20.0F, 10.0F},
                Color8(220U, 120U, 80U, 255U), 0.96F, 2);
            runtime.scheduler().execute_movement();
            require(controller->_0 == 1 && controller->_8 == &farActor,
                    "a different actor must start a frozen interpolation");
            requirePosition(controller->_14->mPosition,
                            TVec3f{30.0F, 6.0F, -1.0F},
                            "actor-switch step zero must retain the old position");
            requireColor(controller->_14->mColor,
                         _GXColor{20U, 40U, 60U, 255U},
                         "actor-switch step zero must retain the old color");
            requireNear(controller->_14->mBrightness, 0.98F,
                        "actor-switch step zero must retain the old brightness");
            requireNear(controller->_14->mRadius, 15.0F,
                        "actor-switch step zero must retain radius 15");

            runtime.scheduler().execute_movement();
            require(controller->_0 == 2,
                    "actor-switch duration-two step one must remain active");
            requirePosition(controller->_14->mPosition,
                            TVec3f{65.0F, 13.0F, 4.5F},
                            "actor-switch cosine midpoint must blend both positions");
            requireColor(controller->_14->mColor,
                         _GXColor{120U, 80U, 70U, 255U},
                         "actor-switch cosine midpoint must blend both colors");
            requireNear(controller->_14->mBrightness, 0.97F,
                        "actor-switch cosine midpoint must blend both brightness values");
            requireNear(controller->_14->mRadius, 15.0F,
                        "actor-switch cosine midpoint must retain radius 15");

            runtime.scheduler().execute_movement();
            require(controller->_0 == -1 && controller->_C == &farActor,
                    "the actor-switch inclusive endpoint must become the tracked actor");
            requirePosition(controller->_14->mPosition,
                            TVec3f{100.0F, 20.0F, 10.0F},
                            "the actor-switch endpoint must reach the target position");
            requireColor(controller->_14->mColor,
                         _GXColor{220U, 120U, 80U, 255U},
                         "the actor-switch endpoint must reach the target color");
            requireNear(controller->_14->mBrightness, 0.96F,
                        "the actor-switch endpoint must reach the target brightness");
            requireNear(controller->_14->mRadius, 15.0F,
                        "the actor-switch endpoint must retain radius 15");

            runtime.scheduler().execute_movement();
            require(controller->_0 == 1 && controller->_8 == nullptr &&
                        controller->_4 == 30,
                    "a missed idle-frame request must begin the retail 30-frame fade-out");
            requirePosition(controller->_14->mPosition,
                            TVec3f{100.0F, 20.0F, 10.0F},
                            "fade-out step zero must freeze the old actor position");
            requireColor(controller->_14->mColor,
                         _GXColor{220U, 120U, 80U, 255U},
                         "fade-out step zero must retain the old color");
            requireNear(controller->_14->mBrightness, 0.96F,
                        "fade-out step zero must retain the old brightness");
            requireNear(controller->_14->mRadius, 15.0F,
                        "fade-out step zero must retain radius 15");

            for (auto step = 1; step <= 15; ++step) {
                runtime.scheduler().execute_movement();
            }
            require(controller->_0 == 16,
                    "fade-out cosine midpoint must leave steps 16 through 30 pending");
            requirePosition(controller->_14->mPosition,
                            TVec3f{100.0F, 20.0F, 10.0F},
                            "fade-out midpoint must keep the frozen old position");
            requireColor(controller->_14->mColor,
                         _GXColor{110U, 60U, 40U, 255U},
                         "fade-out cosine midpoint must blend to half color");
            requireNear(controller->_14->mBrightness, 0.955F,
                        "fade-out cosine midpoint must blend to brightness 0.955");
            requireNear(controller->_14->mRadius, 15.0F,
                        "fade-out cosine midpoint must retain radius 15");

            for (auto step = 16; step <= 30; ++step) {
                runtime.scheduler().execute_movement();
            }
            require(controller->_0 == -1 && controller->_C == nullptr,
                    "fade-out must include cosine step 30 before becoming absent");
            requirePosition(controller->_14->mPosition,
                            TVec3f{100.0F, 20.0F, 10.0F},
                            "the final black fade-out endpoint must retain the old position");
            requireColor(controller->_14->mColor,
                         _GXColor{0U, 0U, 0U, 255U},
                         "the final fade-out endpoint must be black");
            requireNear(controller->_14->mBrightness, 0.95F,
                        "the final present fade-out endpoint must retain brightness 0.95");
            requireNear(controller->_14->mRadius, 15.0F,
                        "the final present fade-out endpoint must retain radius 15");

            runtime.scheduler().execute_movement();
            requireAbsent(*controller->_14,
                          "the frame after fade-out must publish the .001 absent record");

            runtime.scene_lights().clear_light(4U);
            MR::loadLightPlayer();
            const auto *loaded = runtime.scene_lights().light(4U);
            require(loaded != nullptr &&
                        loaded->coordinate_space ==
                            smgpc::render::GXLightCoordinateSpace::World &&
                        loaded->color ==
                            std::array<std::uint8_t, 4U>{0U, 0U, 0U,
                                                         255U} &&
                        loaded->position ==
                            std::array<f32, 3U>{0.0F, 0.0F, 0.0F},
                    "player-light load must route the exact absent record through GX_LIGHT4");
            const auto absentSteepQuadratic =
                (1.0F - 0.001F) / (0.001F * 15.0F * 15.0F);
            requireNear(loaded->distance_attenuation[0U], 1.0F,
                        "absent GX_LIGHT4 steep attenuation must use constant one");
            requireNear(loaded->distance_attenuation[1U], 0.0F,
                        "absent GX_LIGHT4 steep attenuation must use zero linear term");
            requireNear(loaded->distance_attenuation[2U],
                        absentSteepQuadratic,
                        "absent GX_LIGHT4 must use independently computed steep attenuation");
        }

        require(runtime.scene_lights().light(4U) == nullptr &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        nameBaseline,
                "LightDirector destruction must clear GX_LIGHT4 and release its SceneObj identity");

        {
            auto holder = SceneObjHolder{};
            auto binding = smgpc::scene::SceneObjHolderBinding(holder);
            auto *director = dynamic_cast<LightDirector *>(
                MR::createSceneObj(SceneObj_LightDirector));
            require(director != nullptr && director->mPointCtrl != nullptr,
                    "a later scene generation must recreate LightDirector normally");
            runtime.scene_lights().clear_light(4U);
            MR::loadLightPlayer();
            require(runtime.scene_lights().light(4U) != nullptr,
                    "the recreated owner must publish its own absent point-light record");
        }
        require(runtime.scene_lights().light(4U) == nullptr,
                "the recreated owner must clear only its GX_LIGHT4 slot on teardown");
        stageLightBinding.reset();
    }
}  // namespace

int main() {
    try {
        auto discMount = DiscMount{};
        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 320,
            .height = 240,
            .title = "SMG PC point-light runtime proof",
        });
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        auto player = LiveActor("point-light player");
        player.mPosition.zero();
        player.calcAndSetBaseMtx();
        runtime.player_system().attach_actor(player);

        testClampAndCandidateContract(player);
        testStaleAndAbaCandidateSafety();
        auto zeroDurationActor = LiveActor("point-light zero duration");
        testZeroDurationIsNotNormalized(zeroDurationActor);
        testRequestServiceBoundary(runtime, player);
        testSceneOwnerSchedulingAndTransitions(runtime, player,
                                               discMount.mounted());

        runtime.player_system().detach_actor(&player);
        std::cout << "PointLightRuntime tests passed: 6/6 exact request, transition, load, scheduling, fallback, and lifecycle contracts\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "PointLightRuntime tests failed: " << error.what() << '\n';
        return 1;
    }
}
