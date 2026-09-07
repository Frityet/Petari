#include "SceneScheduler.hpp"
#include "scene/SceneDrawBufferService.hpp"
#include "Game/System/DrawBufferHolder.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Animation/BtkPlayer.hpp"
#include "Game/Animation/BrkPlayer.hpp"
#include "Game/System/ResourceInfo.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "compat/SceneJ3dScope.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "layout/LayoutHost.hpp"
#include "layout/LayoutRuntime.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/LightUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "compat/ActorMotionCompat.hpp"
#include "compat/ActorPhysicsRuntime.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/BrightVisibilityService.hpp"
#include "runtime/RuntimeContext.hpp"

#include <dolphin/gx.h>

namespace smgpc::runtime {
    namespace {

        SceneScheduler *sActiveSceneScheduler = nullptr;

        // LayoutRuntime is a native layout owner; its bridge is an actual
        // NameObj so the original category delegator handles mixed batches.
        class LayoutDrawAdaptor final : public NameObj {
        public:
            explicit LayoutDrawAdaptor(smgpc::layout::LayoutRuntime& layout)
                : NameObj(layout.getName().c_str()), _layout(layout) {}
            void draw() const override { _layout.draw(); }
        private:
            smgpc::layout::LayoutRuntime& _layout;
        };

        constexpr auto ORIGINAL_MOVEMENT_ORDER = std::array<s32, 43U>{
            MR::MovementType_StopSceneDelayRequest,
            MR::MovementType_Camera,
            MR::MovementType_MirrorCamera,
            MR::MovementType_ClippingDirector,
            MR::MovementType_ScreenEffect,
            MR::MovementType_SensorHitChecker,
            MR::MovementType_MsgSharedGroup,
            MR::MovementType_UNK_0x07,
            MR::MovementType_UNK_0x14,
            MR::MovementType_TalkDirector,
            MR::MovementType_DemoDirector,
            MR::MovementType_UNK_0x0C,
            MR::MovementType_ClippedMapParts,
            MR::MovementType_Planet,
            MR::MovementType_CollisionMapObj,
            MR::MovementType_CollisionEnemy,
            MR::MovementType_CollisionDirector,
            MR::MovementType_Environment,
            MR::MovementType_MapObj,
            MR::MovementType_MapObjDecoration,
            MR::MovementType_UNK_0x15,
            MR::MovementType_NPC,
            MR::MovementType_Ride,
            MR::MovementType_Player,
            MR::MovementType_PlayerDecoration,
            MR::MovementType_Enemy,
            MR::MovementType_EnemyDecoration,
            MR::MovementType_Item,
            MR::MovementType_PlayerMessenger,
            MR::MovementType_AreaObj,
            MR::MovementType_Layout,
            MR::MovementType_LayoutDecoration,
            MR::MovementType_MovieSubtitles,
            MR::MovementType_WipeLayout,
            MR::MovementType_Movie,
            MR::MovementType_Sky,
            MR::MovementType_ImageEffect,
            MR::MovementType_AudEffectDirector,
            MR::MovementType_AudBgmConductor,
            MR::MovementType_AudCameraWatcher,
            MR::MovementType_CameraCover,
            MR::MovementType_SwitchWatcherHolder,
            MR::MovementType_ShadowControllerHolder,
        };

        constexpr auto ORIGINAL_CALC_ANIM_ORDER = std::array<s32, 15U>{
            MR::CalcAnimType_Environment,
            MR::CalcAnimType_MapObj,
            MR::CalcAnimType_NPC,
            MR::CalcAnimType_Ride,
            MR::CalcAnimType_Enemy,
            MR::CalcAnimType_Player,
            MR::CalcAnimType_PlayerDecoration,
            MR::CalcAnimType_MapObjDecoration,
            MR::CalcAnimType_Item,
            MR::CalcAnimType_Layout,
            MR::CalcAnimType_LayoutDecoration,
            MR::CalcAnimType_UNK_0x12,
            MR::CalcAnimType_MirrorMapObj,
            MR::MovementType_ShadowControllerHolder,
            MR::CalcAnimType_AnimParticle,
        };

        constexpr auto ORIGINAL_DRAW_BUFFER_ORDER = std::array<s32, 41U>{
            MR::DrawBufferType_0x26,
            MR::DrawBufferType_Planet,
            MR::DrawBufferType_IndirectPlanet,
            MR::DrawBufferType_Player,
            MR::DrawBufferType_PlayerDecoration,
            MR::DrawBufferType_CrystalBox,
            MR::DrawBufferType_UNK_0x17,
            MR::DrawBufferType_NPC,
            MR::DrawBufferType_Enemy,
            MR::DrawBufferType_EnemyDecoration,
            MR::DrawBufferType_TripodBoss,
            MR::DrawBufferType_ClippedMapParts,
            MR::DrawBufferType_UNK_0x18,
            MR::DrawBufferType_IndirectMapObj,
            MR::DrawBufferType_IndirectMapObjStrongLight,
            MR::DrawBufferType_IndirectNpc,
            MR::DrawBufferType_IndirectEnemy,
            MR::DrawBufferType_0x28,
            MR::DrawBufferType_Ride,
            MR::DrawBufferType_NoShadowedMapObj,
            MR::DrawBufferType_NoShadowedMapObjStrongLight,
            MR::DrawBufferType_NoSilhouettedMapObj,
            MR::DrawBufferType_NoSilhouettedMapObjWeakLight,
            MR::DrawBufferType_NoSilhouettedMapObjStrongLight,
            MR::DrawBufferType_MapObj,
            MR::DrawBufferType_MapObjWeakLight,
            MR::DrawBufferType_MapObjStrongLight,
            MR::DrawBufferType_MirrorMapObj,
            MR::DrawBufferType_Crystal,
            MR::DrawBufferType_CrystalItem,
            MR::DrawBufferType_GlaringLight,
            MR::DrawBufferType_PlanetLow,
            MR::DrawBufferType_Sky,
            MR::DrawBufferType_Air,
            MR::DrawBufferType_Sun,
            MR::DrawBufferType_Environment,
            MR::DrawBufferType_EnvironmentStrongLight,
            MR::DrawBufferType_AstroDomeSky,
            MR::DrawBufferType_BloomModel,
            MR::DrawBufferType_Model3DFor2D,
            MR::DrawBufferType_0x25,
        };

        constexpr auto ORIGINAL_2D_DRAW_ORDER = std::array<s32, 12U>{
            MR::DrawType_CometScreenFilter,
            MR::DrawType_GalaxyNamePlate,
            MR::DrawType_Layout,
            MR::DrawType_LayoutDecoration,
            MR::DrawType_CinemaFrame,
            MR::DrawType_TalkLayout,
            MR::DrawType_0x44,
            MR::DrawType_EffectDraw2D,
            MR::DrawType_EffectDrawFor2DModel,
            MR::DrawType_LayoutOnPause,
            MR::DrawType_Movie,
            MR::DrawType_MovieSubtitles,
        };

        struct DrawBufferPassCommand {
            s32 draw_buffer_type = -1;
            SceneDrawBufferPass pass = SceneDrawBufferPass::None;
        };

        constexpr auto NORMAL_OPA_BEFORE_VOLUME_SHADOW_COMMANDS = std::array<DrawBufferPassCommand, 13U>{
            DrawBufferPassCommand {MR::DrawBufferType_CrystalItem, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_CrystalItem, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_Crystal, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_Crystal, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_AstroDomeSky, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_AstroDomeSky, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_Planet, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_PlanetLow, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_Environment, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_MapObj, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_MapObjWeakLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_MapObjStrongLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_TripodBoss, SceneDrawBufferPass::Opaque},
        };

        constexpr auto PRIOR_AIR_COMMANDS = std::array<DrawBufferPassCommand, 6U>{
            DrawBufferPassCommand {MR::DrawBufferType_Sky, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_Air, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_Sun, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_Sky, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_Air, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_Sun, SceneDrawBufferPass::Translucent},
        };

        constexpr auto NORMAL_OPA_BEFORE_SILHOUETTE_COMMANDS = std::array<DrawBufferPassCommand, 2U>{
            DrawBufferPassCommand {MR::DrawBufferType_NoShadowedMapObj, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_NoShadowedMapObjStrongLight, SceneDrawBufferPass::Opaque},
        };

        constexpr auto NORMAL_OPA_COMMANDS = std::array<DrawBufferPassCommand, 8U>{
            DrawBufferPassCommand {MR::DrawBufferType_NoSilhouettedMapObj, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_NoSilhouettedMapObjWeakLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_NoSilhouettedMapObjStrongLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_NPC, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_Ride, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_Enemy, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_EnemyDecoration, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_PlayerDecoration, SceneDrawBufferPass::Opaque},
        };

        constexpr auto NORMAL_XLU_COMMANDS = std::array<DrawBufferPassCommand, 18U>{
            DrawBufferPassCommand {MR::DrawBufferType_Planet, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_PlanetLow, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_Environment, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_EnvironmentStrongLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_MapObj, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_MapObjWeakLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_MapObjStrongLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_TripodBoss, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_NoShadowedMapObj, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_NoShadowedMapObjStrongLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_NoSilhouettedMapObj, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_NoSilhouettedMapObjWeakLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_NoSilhouettedMapObjStrongLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_NPC, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_Ride, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_Enemy, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_EnemyDecoration, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_PlayerDecoration, SceneDrawBufferPass::Translucent},
        };

        // SceneFunction::executeDrawAfterIndirect executes these draw buffers
        // after CaptureScreenIndirect has populated the shared capture texture.
        constexpr auto AFTER_INDIRECT_COMMANDS = std::array<DrawBufferPassCommand, 16U>{
            DrawBufferPassCommand {MR::DrawBufferType_IndirectPlanet, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_IndirectMapObj, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_IndirectMapObjStrongLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_IndirectNpc, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_IndirectEnemy, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_GlaringLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_UNK_0x17, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_CrystalBox, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand {MR::DrawBufferType_IndirectPlanet, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_IndirectMapObj, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_IndirectMapObjStrongLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_IndirectNpc, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_IndirectEnemy, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_GlaringLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_UNK_0x17, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand {MR::DrawBufferType_CrystalBox, SceneDrawBufferPass::Translucent},
        };

        constexpr auto AFTER_INDIRECT_DRAW_TYPES = std::array<s32, 12U>{
            MR::DrawType_0x11,
            MR::DrawType_GCapture,
            MR::DrawType_WaterRoad,
            MR::DrawType_BigBubble,
            MR::DrawType_ElectricRailHolder,
            MR::DrawType_OceanRing,
            MR::DrawType_OceanBowl,
            MR::DrawType_0x20,
            MR::DrawType_EffectDrawIndirect,
            MR::DrawType_EffectDrawAfterIndirect,
            MR::DrawType_OceanRingPipeInside,
            MR::DrawType_WaterCameraFilter,
        };

        constexpr auto NORMAL_2D_DRAW_TYPES_BEFORE_0X25 = std::array<s32, 7U>{
            MR::DrawType_CometScreenFilter,
            MR::DrawType_GalaxyNamePlate,
            MR::DrawType_Layout,
            MR::DrawType_LayoutDecoration,
            MR::DrawType_CinemaFrame,
            MR::DrawType_TalkLayout,
            MR::DrawType_0x44,
        };

        constexpr auto NORMAL_2D_DRAW_TYPES_AFTER_0X25 = std::array<s32, 2U>{
            MR::DrawType_EffectDraw2D,
            MR::DrawType_EffectDrawFor2DModel,
        };

        [[nodiscard]] bool draw_buffer_uses_model_3d_for_2d(s32 draw_buffer_type) {
            return draw_buffer_type == MR::DrawBufferType_Model3DFor2D ||
                   draw_buffer_type == MR::DrawBufferType_0x25;
        }

        [[nodiscard]] smgpc::render::Model3DFor2DProjection model_3d_for_2d_projection() {
            return {
                .screen_width = static_cast<float>(MR::getScreenWidth()),
                .screen_height = static_cast<float>(MR::getScreenHeight()),
            };
        }

        [[nodiscard]] std::size_t category_rank(s32 category, std::span<const s32> order) {
            if (category < 0) {
                return std::numeric_limits<std::size_t>::max();
            }

            for (auto i = std::size_t {}; i < order.size(); ++i) {
                if (order[i] == category) {
                    return i;
                }
            }

            return order.size() + static_cast<std::size_t>(category);
        }

        [[nodiscard]] bool movement_category_less(const SceneScheduler::Entry *lhs, const SceneScheduler::Entry *rhs) {
            const auto lhs_rank = category_rank(lhs->movement_type, ORIGINAL_MOVEMENT_ORDER);
            const auto rhs_rank = category_rank(rhs->movement_type, ORIGINAL_MOVEMENT_ORDER);
            if (lhs_rank != rhs_rank) {
                return lhs_rank < rhs_rank;
            }

            return lhs->order < rhs->order;
        }

        [[nodiscard]] bool calc_category_less(const SceneScheduler::Entry *lhs, const SceneScheduler::Entry *rhs) {
            const auto lhs_rank = category_rank(lhs->calc_anim_type, ORIGINAL_CALC_ANIM_ORDER);
            const auto rhs_rank = category_rank(rhs->calc_anim_type, ORIGINAL_CALC_ANIM_ORDER);
            if (lhs_rank != rhs_rank) {
                return lhs_rank < rhs_rank;
            }

            return lhs->order < rhs->order;
        }

        [[nodiscard]] bool draw_category_less(const SceneScheduler::Entry *lhs, const SceneScheduler::Entry *rhs) {
            const auto lhs_buffer_rank = category_rank(lhs->draw_buffer_type, ORIGINAL_DRAW_BUFFER_ORDER);
            const auto rhs_buffer_rank = category_rank(rhs->draw_buffer_type, ORIGINAL_DRAW_BUFFER_ORDER);
            if (lhs_buffer_rank != rhs_buffer_rank) {
                return lhs_buffer_rank < rhs_buffer_rank;
            }

            const auto lhs_draw_rank = category_rank(lhs->draw_type, ORIGINAL_2D_DRAW_ORDER);
            const auto rhs_draw_rank = category_rank(rhs->draw_type, ORIGINAL_2D_DRAW_ORDER);
            if (lhs_draw_rank != rhs_draw_rank) {
                return lhs_draw_rank < rhs_draw_rank;
            }

            return lhs->order < rhs->order;
        }

        [[nodiscard]] std::size_t calc_view_entry_rank(const SceneScheduler::Entry *entry) {
            if (entry->kind == SceneEntryKind::Layout || entry->kind == SceneEntryKind::LayoutActor ||
                (entry->draw_buffer_type < 0 && entry->draw_type >= 0)) {
                return category_rank(entry->draw_type, ORIGINAL_2D_DRAW_ORDER);
            }
            if (entry->draw_buffer_type >= 0) {
                return ORIGINAL_2D_DRAW_ORDER.size() + category_rank(entry->draw_buffer_type, ORIGINAL_DRAW_BUFFER_ORDER);
            }

            return ORIGINAL_2D_DRAW_ORDER.size() + ORIGINAL_DRAW_BUFFER_ORDER.size() + entry->order;
        }

        [[nodiscard]] bool calc_view_entry_less(const SceneScheduler::Entry *lhs, const SceneScheduler::Entry *rhs) {
            const auto lhs_rank = calc_view_entry_rank(lhs);
            const auto rhs_rank = calc_view_entry_rank(rhs);
            if (lhs_rank != rhs_rank) {
                return lhs_rank < rhs_rank;
            }

            return lhs->order < rhs->order;
        }

        [[nodiscard]] bool participates_in_calc_view_and_entry(const SceneScheduler::Entry &entry) {
            return entry.kind == SceneEntryKind::Layout || entry.kind == SceneEntryKind::LayoutActor || entry.draw_buffer_type >= 0 ||
                   entry.draw_type >= 0;
        }

        [[nodiscard]] s32 light_type_for_draw_buffer(s32 draw_buffer_type) {
            if (draw_buffer_type < 0) {
                return MR::LightType_None;
            }

            switch (draw_buffer_type) {
            case MR::DrawBufferType_Player:
            case MR::DrawBufferType_PlayerDecoration:
            case MR::DrawBufferType_CrystalBox:
                return MR::LightType_Player;
            case MR::DrawBufferType_UNK_0x17:
            case MR::DrawBufferType_NoSilhouettedMapObjWeakLight:
            case MR::DrawBufferType_MapObjWeakLight:
                return MR::LightType_Weak;
            case MR::DrawBufferType_NPC:
            case MR::DrawBufferType_Enemy:
            case MR::DrawBufferType_EnemyDecoration:
            case MR::DrawBufferType_TripodBoss:
            case MR::DrawBufferType_IndirectNpc:
            case MR::DrawBufferType_IndirectEnemy:
            case MR::DrawBufferType_Ride:
            case MR::DrawBufferType_NoShadowedMapObjStrongLight:
            case MR::DrawBufferType_NoSilhouettedMapObjStrongLight:
            case MR::DrawBufferType_MapObjStrongLight:
            case MR::DrawBufferType_CrystalItem:
                return MR::LightType_Strong;
            case MR::DrawBufferType_0x26:
            case MR::DrawBufferType_Model3DFor2D:
            case MR::DrawBufferType_0x25:
                return MR::LightType_None;
            default:
                return MR::LightType_Planet;
            }
        }

        [[nodiscard]] const LiveActor *entry_live_actor(const SceneScheduler::Entry &entry) {
            if (entry.kind == SceneEntryKind::LiveActorModel) {
                return entry.live_actor;
            }
            if (entry.name_obj != nullptr) {
                return dynamic_cast<const LiveActor *>(entry.name_obj);
            }

            return nullptr;
        }

        [[nodiscard]] LiveActor *entry_live_actor(SceneScheduler::Entry &entry) {
            return const_cast<LiveActor *>(entry_live_actor(static_cast<const SceneScheduler::Entry &>(entry)));
        }

#ifndef NDEBUG
        [[nodiscard]] std::string_view scene_entry_kind_name(SceneEntryKind kind) {
            switch (kind) {
            case SceneEntryKind::NameObj:
                return "name_obj";
            case SceneEntryKind::Layout:
                return "layout";
            case SceneEntryKind::LayoutActor:
                return "layout_actor";
            case SceneEntryKind::LiveActorModel:
                return "live_actor_model";
            }
            return "unknown";
        }

        void emit_connect_to_scene_trace(SceneEntryKind kind, std::string_view name, s32 movement_type, s32 calc_anim_type,
                                         s32 draw_buffer_type, s32 draw_type) {
            if (auto *runtime = RuntimeContext::try_instance()) {
                runtime->emit_semantic_trace_event(
                    "name_obj_lifecycle", "connect_to_scene",
                    "object=" + std::string(name) + ";kind=" + std::string(scene_entry_kind_name(kind)) +
                        ";movement=" + std::to_string(movement_type) + ";calc_anim=" + std::to_string(calc_anim_type) +
                        ";draw_buffer=" + std::to_string(draw_buffer_type) + ";draw_type=" + std::to_string(draw_type));
            }
        }

        [[nodiscard]] std::string sensor_host_name(const HitSensor *sensor) {
            const auto *host = MR::getSensorHost(sensor);
            return host != nullptr ? host->getName() : "";
        }

        [[nodiscard]] std::array<float, 3U> vec3_state(const TVec3f &value) {
            return {value.x, value.y, value.z};
        }
#endif

    }  // namespace

    SceneScheduler *try_active_scene_scheduler() {
        return sActiveSceneScheduler;
    }

    SceneSchedulerBinding::SceneSchedulerBinding(SceneScheduler &scheduler)
        : _bound(&scheduler), _previous(sActiveSceneScheduler) {
        sActiveSceneScheduler = &scheduler;
    }

    SceneSchedulerBinding::~SceneSchedulerBinding() {
        if (sActiveSceneScheduler != _bound) {
            std::terminate();
        }
        sActiveSceneScheduler = _previous;
    }

    SceneScheduler::SceneScheduler() {
        smgpc::compat::JkrHostAllocationScope host;
        _draw_buffers = std::make_unique<smgpc::scene::SceneDrawBufferService>();
    }
    SceneScheduler::~SceneScheduler() { clear(); }

    void SceneScheduler::begin_draw_buffer_registration(std::shared_ptr<smgpc::compat::JkrAllocationDomain> domain) {
        smgpc::compat::JkrHostAllocationScope host;
        retire_draw_buffers();
        _draw_buffers->begin_draw_buffer_registration(std::move(domain));
    }

    void SceneScheduler::allocate_draw_buffers() {
        if (!_draw_buffers->has_draw_buffers()) throw std::logic_error("Original draw buffers need an explicit scene construction owner");
        _draw_buffers->allocate_actor_lists();
        refresh_draw_buffer_activation();
    }

    void SceneScheduler::retire_draw_buffers() {
        if (_draw_buffers && _draw_buffers->registration_count() != 0)
            throw std::logic_error("Remove every actor registration before retiring original scene draw buffers");
        _draw_buffers->retire_draw_buffers();
    }

    void SceneScheduler::find_actor_light_info(LiveActor &actor) {
        const auto* entry = find_entry(SceneEntryKind::LiveActorModel, &actor);
        if (!entry || !entry->has_draw_buffer_registration) return;
        _draw_buffers->find_light_info(actor);
    }

    void SceneScheduler::refresh_draw_buffer_activation() {
        if (!_draw_buffers || !_draw_buffers->is_allocated()) return;
        for (auto& entry : _entries)
            if (entry.has_draw_buffer_registration)
                _draw_buffers->set_active(*entry.live_actor,
                    entry.draw_connected && !entry_is_dead(entry) && !entry.live_actor->mFlag.mIsClipped);
    }

    void SceneScheduler::connect_name_obj(NameObj &obj, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type) {
        smgpc::compat::JkrHostAllocationScope host;
        if (auto *entry = find_entry(SceneEntryKind::NameObj, &obj)) {
            entry->movement_type = movement_type;
            entry->calc_anim_type = calc_anim_type;
            entry->draw_buffer_type = draw_buffer_type;
            entry->draw_type = draw_type;
            entry->draw_connected = true;
#ifndef NDEBUG
            emit_connect_to_scene_trace(SceneEntryKind::NameObj, obj.getName(), movement_type, calc_anim_type, draw_buffer_type, draw_type);
#endif
            return;
        }

        _entries.push_back(Entry {
            .kind = SceneEntryKind::NameObj,
            .name_obj = &obj,
            .movement_type = movement_type,
            .calc_anim_type = calc_anim_type,
            .draw_buffer_type = draw_buffer_type,
            .draw_type = draw_type,
            .order = _next_order++,
        });
#ifndef NDEBUG
        emit_connect_to_scene_trace(SceneEntryKind::NameObj, obj.getName(), movement_type, calc_anim_type, draw_buffer_type, draw_type);
#endif
    }

    void SceneScheduler::disconnect_name_obj(NameObj &obj) {
        _draw_buffers->remove_draw_object(obj);
        for (auto &entry : _entries)
            if (entry.name_obj == &obj && entry.has_draw_buffer_registration)
                _draw_buffers->remove_actor(*entry.live_actor);
        smgpc::layout::release_layout_actor_if_registered(&obj);
        std::erase_if(_entries, [&obj](const auto &entry) {
            return entry.name_obj == &obj;
        });
    }

    void SceneScheduler::connect_draw(NameObj &obj) {
        auto found = false;
        for (auto &entry : _entries) {
            if (entry.name_obj == &obj) {
                entry.draw_connected = true;
                found = true;
            }
        }
        if (!found) {
            throw std::logic_error("Cannot connect an unregistered NameObj to draw.");
        }
    }

    void SceneScheduler::disconnect_draw(NameObj &obj) {
        _draw_buffers->remove_draw_object(obj);
        auto found = false;
        for (auto &entry : _entries) {
            if (entry.name_obj == &obj) {
                entry.draw_connected = false;
                found = true;
            }
        }
        if (!found) {
            throw std::logic_error("Cannot disconnect an unregistered NameObj from draw.");
        }
    }

    bool SceneScheduler::is_draw_connected(const NameObj &obj) const {
        const auto entry = std::ranges::find_if(_entries, [&obj](const auto &candidate) {
            return candidate.name_obj == &obj;
        });
        return entry != _entries.end() && entry->draw_connected;
    }

    std::optional<s32> SceneScheduler::light_type_for_actor(const LiveActor &actor) const {
        const auto *entry = find_entry(SceneEntryKind::LiveActorModel, &actor);
        if (entry == nullptr) {
            return std::nullopt;
        }
        if (!entry->has_draw_buffer_registration) return std::nullopt;
        return _draw_buffers->holder().getDrawBufferGroup(entry->draw_buffer_type)->mLightType;
    }

    void SceneScheduler::register_layout(smgpc::layout::LayoutRuntime &layout, s32 movement_type, s32 calc_anim_type, s32 draw_type) {
        smgpc::compat::JkrHostAllocationScope host;
        if (auto *entry = find_entry(SceneEntryKind::Layout, &layout)) {
            entry->movement_type = movement_type;
            entry->calc_anim_type = calc_anim_type;
            entry->draw_type = draw_type;
            entry->draw_connected = true;
#ifndef NDEBUG
            emit_connect_to_scene_trace(SceneEntryKind::Layout, layout.getName(), movement_type, calc_anim_type, -1, draw_type);
#endif
            return;
        }

        _layout_draw_adaptors.try_emplace(&layout, std::make_unique<LayoutDrawAdaptor>(layout));
        _entries.push_back(Entry {
            .kind = SceneEntryKind::Layout,
            .layout = &layout,
            .movement_type = movement_type,
            .calc_anim_type = calc_anim_type,
            .draw_type = draw_type,
            .order = _next_order++,
        });
#ifndef NDEBUG
        emit_connect_to_scene_trace(SceneEntryKind::Layout, layout.getName(), movement_type, calc_anim_type, -1, draw_type);
#endif
    }

    void SceneScheduler::unregister_layout(smgpc::layout::LayoutRuntime &layout) {
        if (const auto found = _layout_draw_adaptors.find(&layout); found != _layout_draw_adaptors.end())
            _draw_buffers->remove_draw_object(*found->second);
        std::erase_if(_entries, [&layout](const auto &entry) {
            return entry.kind == SceneEntryKind::Layout && entry.layout == &layout;
        });
        _layout_draw_adaptors.erase(&layout);
    }

    void SceneScheduler::register_layout_actor(LayoutActor &layout, s32 movement_type, s32 calc_anim_type, s32 draw_type) {
        smgpc::compat::JkrHostAllocationScope host;
        if (auto *entry = find_entry(SceneEntryKind::LayoutActor, &layout)) {
            entry->movement_type = movement_type;
            entry->calc_anim_type = calc_anim_type;
            entry->draw_type = draw_type;
            entry->draw_connected = true;
#ifndef NDEBUG
            emit_connect_to_scene_trace(SceneEntryKind::LayoutActor, layout.getName(), movement_type, calc_anim_type, -1, draw_type);
#endif
            return;
        }

        _entries.push_back(Entry {
            .kind = SceneEntryKind::LayoutActor,
            .name_obj = &layout,
            .layout_actor = &layout,
            .movement_type = movement_type,
            .calc_anim_type = calc_anim_type,
            .draw_type = draw_type,
            .order = _next_order++,
        });
#ifndef NDEBUG
        emit_connect_to_scene_trace(SceneEntryKind::LayoutActor, layout.getName(), movement_type, calc_anim_type, -1, draw_type);
#endif
    }

    void SceneScheduler::unregister_layout_actor(LayoutActor &layout) {
        _draw_buffers->remove_draw_object(layout);
        std::erase_if(_entries, [&layout](const auto &entry) {
            return entry.kind == SceneEntryKind::LayoutActor && entry.layout_actor == &layout;
        });
    }

    void SceneScheduler::register_live_actor_model(LiveActor &actor, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type) {
        smgpc::compat::JkrHostAllocationScope host;
        if (auto *entry = find_entry(SceneEntryKind::LiveActorModel, &actor)) {
            if (entry->draw_buffer_type != draw_buffer_type)
                throw std::logic_error("An original draw registration cannot change categories before retirement");
            entry->movement_type = movement_type;
            entry->calc_anim_type = calc_anim_type;
            entry->draw_type = draw_type;
            entry->draw_connected = true;
            return;
        }
        if (draw_buffer_type >= 0) {
            if (!_draw_buffers) throw std::logic_error("Construct a scene draw holder before registering a model");
            _draw_buffers->register_actor(actor, draw_buffer_type, smgpc::compat::retain_actor_model_owner(&actor));
        }
        try {
            _entries.push_back(Entry {
                .kind = SceneEntryKind::LiveActorModel,
                .name_obj = &actor,
                .live_actor = &actor,
                .movement_type = movement_type,
                .calc_anim_type = calc_anim_type,
                .draw_buffer_type = draw_buffer_type,
                .draw_type = draw_type,
                .has_draw_buffer_registration = draw_buffer_type >= 0,
                .order = _next_order++,
            });
        } catch (...) {
            if (draw_buffer_type >= 0) _draw_buffers->remove_actor(actor);
            throw;
        }
#ifndef NDEBUG
        emit_connect_to_scene_trace(SceneEntryKind::LiveActorModel, actor.getName(), movement_type, calc_anim_type, draw_buffer_type, draw_type);
#endif
    }

    void SceneScheduler::unregister_live_actor_model(LiveActor &actor) {
        _draw_buffers->remove_draw_object(actor);
        if (_draw_buffers) _draw_buffers->remove_actor(actor);
        std::erase_if(_entries, [&actor](const auto &entry) {
            return entry.kind == SceneEntryKind::LiveActorModel && entry.live_actor == &actor;
        });
    }

    void SceneScheduler::request_movement_on(s32 movement_type) {
        for (auto &entry : _entries) {
            // Retail NameObjExecuteInfo stores its category in an s8 and
            // narrows the requested category before comparing it.
            if (static_cast<s8>(entry.movement_type) == static_cast<s8>(movement_type)) {
                NameObjFunction::requestMovementOn(entry.name_obj);
            }
        }
    }

    void SceneScheduler::request_movement_off(s32 movement_type) {
        for (auto &entry : _entries) {
            if (static_cast<s8>(entry.movement_type) == static_cast<s8>(movement_type)) {
                NameObjFunction::requestMovementOff(entry.name_obj);
            }
        }
    }

    void SceneScheduler::execute_movement() {
        smgpc::compat::SceneJ3dScope j3d_scope;
#ifndef NDEBUG
        _last_execution_trace.clear();
#endif
        auto clipping_camera = std::optional<smgpc::camera::CameraPose>{};
        if (auto* runtime = RuntimeContext::try_instance(); runtime != nullptr) {
            clipping_camera = runtime->camera_system().effective_camera_pose();
            if (!clipping_camera.has_value()) {
                clipping_camera = runtime->scene_camera_pose();
            }
            if (!clipping_camera.has_value()) {
                clipping_camera = runtime->last_camera_pose();
            }
        }
        if (clipping_camera.has_value()) {
            auto updated_actors = std::vector<LiveActor*>{};
            for (auto& entry : _entries) {
                auto* actor = entry_live_actor(entry);
                if (actor == nullptr || actor->mFlag.mIsDead ||
                    draw_buffer_uses_model_3d_for_2d(entry.draw_buffer_type) ||
                    std::ranges::find(updated_actors, actor) != updated_actors.end()) {
                    continue;
                }
                smgpc::compat::update_actor_clipping(*actor, *clipping_camera);
                {
                    smgpc::compat::JkrHostAllocationScope host;
                    updated_actors.push_back(actor);
                }
            }
        }
        for (auto *entry : sorted_entries_for_movement()) {
            if (entry->movement_type < 0 || entry_is_dead(*entry) || entry_is_suspended(*entry)) {
                continue;
            }

            if (auto *actor = entry_live_actor(*entry); actor != nullptr && actor->mFlag.mIsClipped) {
                continue;
            }

            switch (entry->kind) {
            case SceneEntryKind::NameObj:
                entry->name_obj->executeMovement();
                break;
            case SceneEntryKind::Layout:
                entry->layout->update();
                break;
            case SceneEntryKind::LayoutActor:
                entry->layout_actor->executeMovement();
                break;
            case SceneEntryKind::LiveActorModel:
                entry->live_actor->movement();
                break;
            }

            if (auto *actor = entry_live_actor(*entry); actor != nullptr && !actor->mFlag.mIsDead) {
                if (auto *runtime = RuntimeContext::try_instance();
                    runtime != nullptr && runtime->player_system().attached_actor() == actor) {
                    runtime->player_system().synchronize_attached_actor();
                }
            }
#ifndef NDEBUG
            push_trace(*entry, SceneSchedulerPhase::Movement);
#endif
        }

        execute_sensor_hit_check();
    }

    void SceneScheduler::execute_sensor_hit_check() {
        auto actors = std::vector<LiveActor *>{};
        auto sensors = std::vector<HitSensor *>{};

        for (auto &entry : _entries) {
            auto *actor = entry_live_actor(entry);
            if (actor == nullptr || entry_is_dead(entry) || entry_is_suspended(entry) ||
                actor->mFlag.mIsClipped ||
                std::ranges::find(actors, actor) != actors.end()) {
                continue;
            }

            smgpc::compat::update_actor_hit_sensors(actor);
            {
                smgpc::compat::JkrHostAllocationScope host;
                smgpc::compat::collect_actor_hit_sensors(actor, sensors);
                actors.push_back(actor);
            }
        }

        for (auto *sensor : sensors) {
            if (sensor != nullptr) {
                sensor->mSensorCount = 0U;
            }
        }

        for (auto lhs_index = std::size_t{}; lhs_index < sensors.size(); ++lhs_index) {
            auto *lhs = sensors[lhs_index];
            if (lhs == nullptr || lhs->mHost == nullptr || !lhs->mValidByHost || !lhs->mValidBySystem || lhs->mHost->mFlag.mIsDead) {
                continue;
            }

            for (auto rhs_index = lhs_index + 1U; rhs_index < sensors.size(); ++rhs_index) {
                auto *rhs = sensors[rhs_index];
                if (rhs == nullptr || rhs->mHost == nullptr || rhs->mHost == lhs->mHost || !rhs->mValidByHost ||
                    !rhs->mValidBySystem || rhs->mHost->mFlag.mIsDead) {
                    continue;
                }

                const auto dx = lhs->mPosition.x - rhs->mPosition.x;
                const auto dy = lhs->mPosition.y - rhs->mPosition.y;
                const auto dz = lhs->mPosition.z - rhs->mPosition.z;
                const auto radius = lhs->mRadius + rhs->mRadius;
                const auto distance_squared = (dx * dx) + (dy * dy) + (dz * dz);
                if (distance_squared > (radius * radius)) {
                    continue;
                }

                lhs->addHitSensor(rhs);
                rhs->addHitSensor(lhs);

                if (!lhs->mHost->mFlag.mIsDead) {
                    lhs->mHost->attackSensor(lhs, rhs);
                }
                if (!rhs->mHost->mFlag.mIsDead) {
                    rhs->mHost->attackSensor(rhs, lhs);
                }
            }
        }
    }

    void SceneScheduler::execute_calc_anim() {
        smgpc::compat::SceneJ3dScope j3d_scope;
        // SceneNameObjListExecutor calls NameObj::calcAnim directly. Only
        // its movement list uses executeMovement's movement-off flag.
        for (auto *entry : sorted_entries_for_calc_anim()) {
            if (entry->calc_anim_type < 0 || entry_is_dead(*entry)) {
                continue;
            }
            if (const auto* actor = entry_live_actor(*entry); actor != nullptr && actor->mFlag.mIsClipped) {
                continue;
            }

            switch (entry->kind) {
            case SceneEntryKind::NameObj:
                entry->name_obj->calcAnim();
                break;
            case SceneEntryKind::Layout:
                break;
            case SceneEntryKind::LayoutActor:
                entry->layout_actor->calcAnim();
                break;
            case SceneEntryKind::LiveActorModel:
                entry->live_actor->calcAnim();
                break;
            }
#ifndef NDEBUG
            push_trace(*entry, SceneSchedulerPhase::CalcAnim);
#endif
        }
    }

    void SceneScheduler::execute_calc_view_and_entry() {
        if (!_draw_buffers->has_draw_buffers()) return;
        if (!_draw_buffers->is_allocated())
            throw std::logic_error("Scene construction must allocate draw lists before view entry");
        if (auto* runtime = RuntimeContext::try_instance(); runtime && runtime->scene_camera_pose()) {
            for (auto& entry : _entries)
                if (entry.has_draw_buffer_registration && !entry_is_dead(entry) &&
                    !draw_buffer_uses_model_3d_for_2d(entry.draw_buffer_type))
                    smgpc::compat::update_actor_clipping(*entry.live_actor, *runtime->scene_camera_pose());
        }
        refresh_draw_buffer_activation();
        smgpc::compat::SceneJ3dScope commands;
        // SceneFunction::executeCalcViewAndEntryList: the actual holder invokes
        // each active actor once, in its original camera-category list.
        TMtx34f mtx;
        mtx.identity();
        PSMTXCopy(mtx, j3dSys.mViewMtx);
        _draw_buffers->entry(1);
        MR::loadViewMtx();
        _draw_buffers->entry(0);
#ifndef NDEBUG
        for (const auto& entry : _entries)
            if (entry.has_draw_buffer_registration && entry.draw_connected && !entry_is_dead(entry) &&
                !entry.live_actor->mFlag.mIsClipped)
                push_trace(entry, SceneSchedulerPhase::CalcViewAndEntry);
#endif
    }

    void SceneScheduler::execute_draw_buffer_opa(const smgpc::camera::CameraPose &camera_pose, s32 draw_buffer_type) {
        execute_draw_buffer(camera_pose, draw_buffer_type, SceneDrawBufferPass::Opaque);
    }

    void SceneScheduler::execute_draw_buffer_xlu(const smgpc::camera::CameraPose &camera_pose, s32 draw_buffer_type) {
        execute_draw_buffer(camera_pose, draw_buffer_type, SceneDrawBufferPass::Translucent);
    }

    void SceneScheduler::execute_draw_buffer_list_normal_opa_before_volume_shadow(const smgpc::camera::CameraPose &camera_pose, bool prior_draw_air) {
        for (const auto &command : NORMAL_OPA_BEFORE_VOLUME_SHADOW_COMMANDS) {
            execute_draw_buffer(camera_pose, command.draw_buffer_type, command.pass);
            if (command.draw_buffer_type == MR::DrawBufferType_AstroDomeSky && command.pass == SceneDrawBufferPass::Translucent &&
                prior_draw_air) {
                for (const auto &air_command : PRIOR_AIR_COMMANDS) {
                    execute_draw_buffer(camera_pose, air_command.draw_buffer_type, air_command.pass);
                }
            }
            if (command.draw_buffer_type == MR::DrawBufferType_PlanetLow) {
                execute_draw_type(MR::DrawType_FlexibleSphere);
            }
        }
    }

    void SceneScheduler::execute_draw_buffer_list_normal_opa_before_silhouette(const smgpc::camera::CameraPose &camera_pose) {
        for (const auto &command : NORMAL_OPA_BEFORE_SILHOUETTE_COMMANDS) {
            execute_draw_buffer(camera_pose, command.draw_buffer_type, command.pass);
        }
    }

    void SceneScheduler::execute_draw_buffer_list_normal_opa(const smgpc::camera::CameraPose &camera_pose, bool prior_draw_air) {
        for (const auto &command : NORMAL_OPA_COMMANDS) {
            execute_draw_buffer(camera_pose, command.draw_buffer_type, command.pass);
        }
        if (!prior_draw_air) {
            for (const auto &air_command : PRIOR_AIR_COMMANDS) {
                execute_draw_buffer(camera_pose, air_command.draw_buffer_type, air_command.pass);
            }
        }
    }

    void SceneScheduler::execute_draw_buffer_list_normal_xlu(const smgpc::camera::CameraPose &camera_pose) {
        for (const auto &command : NORMAL_XLU_COMMANDS) {
            execute_draw_buffer(camera_pose, command.draw_buffer_type, command.pass);
        }
    }

    void SceneScheduler::execute_draw_buffer_list_normal(const smgpc::camera::CameraPose &camera_pose, bool prior_draw_air,
                                                          s32 interleaved_draw_type, s32 interleaved_light_type) {
        auto *runtime = RuntimeContext::try_instance();
        const auto previous_pixel_update_state = runtime != nullptr ? runtime->j3d_pixel_update_state() :
                                                                      std::optional<RuntimeContext::GxPixelUpdateState>{};
        if (runtime != nullptr) {
            runtime->set_j3d_pixel_update_state(RuntimeContext::GxPixelUpdateState {.color_update = true, .alpha_update = true});
        }
        GXSetColorUpdate(GX_TRUE);
        GXSetAlphaUpdate(GX_TRUE);
        GXSetDstAlpha(GX_TRUE, 0U);
        execute_draw_buffer_list_normal_opa_before_volume_shadow(camera_pose, prior_draw_air);
        if (runtime != nullptr) {
            runtime->set_j3d_pixel_update_state(RuntimeContext::GxPixelUpdateState {.color_update = true, .alpha_update = true});
        }
        GXSetAlphaUpdate(GX_TRUE);
        execute_draw_type(MR::DrawType_ShadowVolume);
        GXSetColorUpdate(GX_TRUE);
        GXSetDstAlpha(GX_TRUE, 0U);
        if (runtime != nullptr) {
            runtime->set_j3d_pixel_update_state(RuntimeContext::GxPixelUpdateState {.color_update = true, .alpha_update = true});
        }
        execute_draw_buffer_list_normal_opa_before_silhouette(camera_pose);
        execute_draw_type(MR::DrawType_0x28);
        MR::fillSilhouetteColor();
        MR::loadViewMtx();
        MR::loadProjectionMtx();
        execute_draw_type(MR::DrawType_AlphaShadow);
        MR::loadViewMtx();
        smgpc::render::begin_bright_visibility_draw_pass(
            MR::getLensFlareDrawSyncTokenIndex());
        execute_draw_type(MR::DrawType_BrightSun);
        MR::setLensFlareDrawSyncToken();
        GXSetAlphaUpdate(GX_FALSE);
        GXSetDstAlpha(GX_FALSE, 0U);
        if (runtime != nullptr) {
            runtime->set_j3d_pixel_update_state(RuntimeContext::GxPixelUpdateState {.color_update = true, .alpha_update = false});
        }
        execute_draw_buffer_list_normal_opa(camera_pose, prior_draw_air);
        if (interleaved_draw_type >= 0) {
            if (interleaved_light_type >= 0) {
                MR::loadLight(interleaved_light_type);
            }
            execute_draw_type(interleaved_draw_type);
        }
        execute_draw_buffer_list_normal_xlu(camera_pose);
        if (runtime != nullptr) {
            runtime->set_j3d_pixel_update_state(previous_pixel_update_state);
        }
    }

    void SceneScheduler::execute_draw_after_indirect(const smgpc::camera::CameraPose &camera_pose) {
        for (const auto &command : AFTER_INDIRECT_COMMANDS) {
            execute_draw_buffer(camera_pose, command.draw_buffer_type, command.pass);
        }
        for (const auto draw_type : AFTER_INDIRECT_DRAW_TYPES) {
            execute_draw_type(draw_type);
        }
    }

    void SceneScheduler::execute_draw_list_2d_normal() {
        const auto projection = model_3d_for_2d_projection();
        MR::drawInitFor2DModel();
        execute_draw_buffer_model_3d_for_2d(
            projection, MR::DrawBufferType_Model3DFor2D,
            SceneDrawBufferPass::Opaque);
        execute_draw_buffer_model_3d_for_2d(
            projection, MR::DrawBufferType_Model3DFor2D,
            SceneDrawBufferPass::Translucent);
        for (const auto draw_type : NORMAL_2D_DRAW_TYPES_BEFORE_0X25) {
            execute_draw_type(draw_type);
        }
        MR::drawInitFor2DModel();
        execute_draw_buffer_model_3d_for_2d(
            projection, MR::DrawBufferType_0x25,
            SceneDrawBufferPass::Opaque);
        execute_draw_buffer_model_3d_for_2d(
            projection, MR::DrawBufferType_0x25,
            SceneDrawBufferPass::Translucent);
        for (const auto draw_type : NORMAL_2D_DRAW_TYPES_AFTER_0X25) {
            execute_draw_type(draw_type);
        }
    }

    std::size_t SceneScheduler::send_message_to_live_actors(u32 msg, LiveActor *exclude_actor) {
        auto seen_actors = std::vector<LiveActor *>{};
        auto accepted_count = std::size_t {};
        auto *message_sensor = MR::getMessageSensor();

        for (auto &entry : _entries) {
            auto *actor = entry_live_actor(entry);
            if (actor == nullptr || std::ranges::find(seen_actors, actor) != seen_actors.end()) {
                continue;
            }
            {
                smgpc::compat::JkrHostAllocationScope host;
                seen_actors.push_back(actor);
            }

            const auto dead = entry_is_dead(entry);
            const auto suspended = entry_is_suspended(entry);
            const auto excluded = actor == exclude_actor;
            const auto delivered = !dead && !suspended && !excluded;
            auto accepted = false;
            if (delivered) {
                accepted = actor->receiveMessage(msg, message_sensor, message_sensor);
                if (accepted) {
                    ++accepted_count;
                }
            }

#ifndef NDEBUG
            smgpc::compat::JkrHostAllocationScope host;
            push_message_trace(SceneSchedulerMessageTraceEntry {
                .sequence = _next_message_sequence++,
                .message = msg,
                .target_name = entry_name(entry),
                .target_kind = entry.kind,
                .target_movement_type = entry.movement_type,
                .target_calc_anim_type = entry.calc_anim_type,
                .target_draw_buffer_type = entry.draw_buffer_type,
                .target_draw_type = entry.draw_type,
                .target_order = entry.order,
                .target_dead = dead,
                .target_suspended = suspended,
                .excluded = excluded,
                .delivered = delivered,
                .accepted = accepted,
                .sender_sensor_present = message_sensor != nullptr,
                .receiver_sensor_present = message_sensor != nullptr,
                .sender_sensor_type = message_sensor != nullptr ? message_sensor->mType : 0U,
                .receiver_sensor_type = message_sensor != nullptr ? message_sensor->mType : 0U,
                .sender_sensor_host_name = sensor_host_name(message_sensor),
                .receiver_sensor_host_name = sensor_host_name(message_sensor),
            });
#endif
        }

        return accepted_count;
    }

    std::size_t SceneScheduler::registration_marker() const {
        return _next_order;
    }

    std::vector<SceneSchedulerRegistration> SceneScheduler::remove_registrations_since(std::size_t marker) {
        auto registrations = std::vector<SceneSchedulerRegistration>{};
        {
            smgpc::compat::JkrHostAllocationScope host;
            for (auto &entry : _entries) {
                if (entry.order < marker) {
                    continue;
                }

                registrations.push_back(SceneSchedulerRegistration{
                    .kind = entry.kind,
                    .name_obj = entry.name_obj,
                    .layout = entry.layout,
                    .layout_actor = entry.layout_actor,
                    .live_actor = entry_live_actor(entry),
                    .name = entry_name(entry),
                    .order = entry.order,
                });
            }
        }

        for (const auto &entry : _entries)
            if (entry.order >= marker && entry.has_draw_buffer_registration)
                _draw_buffers->remove_actor(*entry.live_actor);
        _draw_buffers->rollback_pre_draw_functions(marker);
        for (const auto& registration : registrations) {
            if (registration.name_obj) _draw_buffers->remove_draw_object(*registration.name_obj);
            if (registration.layout) {
                const auto found = _layout_draw_adaptors.find(registration.layout);
                if (found != _layout_draw_adaptors.end()) _draw_buffers->remove_draw_object(*found->second);
            }
        }
        std::erase_if(_entries, [marker](const auto &entry) { return entry.order >= marker; });
        for (const auto& registration : registrations)
            if (registration.layout) _layout_draw_adaptors.erase(registration.layout);
        return registrations;
    }

    void SceneScheduler::execute_draw_buffer(const smgpc::camera::CameraPose &camera_pose, s32 draw_buffer_type, SceneDrawBufferPass pass) {
        if (!_draw_buffers->has_draw_buffers()) return;
        if (!_draw_buffers->is_allocated()) throw std::logic_error("Draw lists have not completed scene construction");
        refresh_draw_buffer_activation();
        smgpc::compat::SceneJ3dScope commands;
        if (pass == SceneDrawBufferPass::Translucent) _draw_buffers->draw_translucent(draw_buffer_type);
        else _draw_buffers->draw_opaque(draw_buffer_type);
#ifndef NDEBUG
        const auto phase = pass == SceneDrawBufferPass::Translucent ? SceneSchedulerPhase::DrawBufferXlu : SceneSchedulerPhase::DrawBufferOpa;
        for (const auto& entry : _entries)
            if (entry.has_draw_buffer_registration && entry.draw_buffer_type == draw_buffer_type &&
                entry.draw_connected && !entry_is_dead(entry) && !entry.live_actor->mFlag.mIsClipped)
                push_trace(entry, phase, pass);
#endif
    }

    void SceneScheduler::execute_draw_buffer_model_3d_for_2d(
        const smgpc::render::Model3DFor2DProjection &projection,
        s32 draw_buffer_type, SceneDrawBufferPass pass) {
        if (!draw_buffer_uses_model_3d_for_2d(draw_buffer_type))
            throw std::logic_error("Only original 2D camera categories use the model 3D-for-2D pass");
        execute_draw_buffer({}, draw_buffer_type, pass);
    }

    void SceneScheduler::register_pre_draw_function(const MR::FunctorBase& functor, s32 draw_type) {
        _draw_buffers->register_pre_draw_function(functor, draw_type, _next_order);
        ++_next_order;
    }

    void SceneScheduler::execute_draw_type(s32 draw_type) {
        smgpc::compat::SceneJ3dScope j3d_scope;
        std::vector<Entry> draw_entries;
        std::vector<NameObj*> objects;
        {
            smgpc::compat::JkrHostAllocationScope host;
            for (const auto& entry : _entries)
                if (entry.draw_type == draw_type && entry.draw_connected && !entry_is_dead(entry))
                    draw_entries.push_back(entry);
            std::ranges::stable_sort(draw_entries, [](const Entry& a, const Entry& b) { return draw_category_less(&a, &b); });
            objects.reserve(draw_entries.size());
            for (const auto& entry : draw_entries) {
                if (entry.kind == SceneEntryKind::Layout) objects.push_back(_layout_draw_adaptors.at(entry.layout).get());
                else objects.push_back(entry.name_obj);
            }
        }
        const auto drawn = _draw_buffers->execute_draw_category(draw_type, objects);
#ifndef NDEBUG
        {
            smgpc::compat::JkrHostAllocationScope host;
            // Resolve current registrations after callbacks: rollback may have
            // removed and destroyed members of the batch during its pre-draw call.
            for (auto* object : drawn) {
                for (const auto& entry : _entries) {
                    const auto* registered = entry.name_obj;
                    if (entry.kind == SceneEntryKind::Layout) registered = _layout_draw_adaptors.at(entry.layout).get();
                    if (registered == object) { push_trace(entry, SceneSchedulerPhase::DrawType); break; }
                }
            }
        }
#endif
        if (auto *runtime = RuntimeContext::try_instance()) {
            const auto &camera_pose = runtime->last_camera_pose();
            runtime->effects().draw(draw_type, camera_pose.has_value() ? &*camera_pose : nullptr);
        }
    }

#ifndef NDEBUG
    void fill_actor_model_debug_state(SceneSchedulerEntryState&, const LiveActor*);

    std::vector<SceneSchedulerEntryState> SceneScheduler::snapshot() const {
        smgpc::compat::JkrHostAllocationScope host;
        auto states = std::vector<SceneSchedulerEntryState>{};
        states.reserve(_entries.size());
        for (const auto &entry : _entries) {
            auto state = SceneSchedulerEntryState {
                .kind = entry.kind,
                .phase = SceneSchedulerPhase::None,
                .name = entry_name(entry),
                .movement_type = entry.movement_type,
                .calc_anim_type = entry.calc_anim_type,
                .draw_buffer_type = entry.draw_buffer_type,
                .draw_type = entry.draw_type,
                .draw_buffer_pass = SceneDrawBufferPass::None,
                .order = entry.order,
                .draw_connected = entry.draw_connected,
                .suspended = entry_is_suspended(entry),
                .dead = entry_is_dead(entry),
                .has_live_actor_state = false,
                .live_actor_nerve_step = 0,
                .live_actor_position = {},
                .live_actor_rotation = {},
                .live_actor_scale = {},
                .live_actor_base_matrix = {},
                .live_actor_bck_name = {},
                .live_actor_brk_name = {},
                .live_actor_btk_name = {},
            };
            if (const auto *actor = entry_live_actor(entry)) {
                state.has_live_actor_state = true;
                state.live_actor_nerve_step = actor->getNerveStep();
                state.live_actor_position = vec3_state(actor->mPosition);
                state.live_actor_rotation = vec3_state(actor->mRotation);
                state.live_actor_scale = vec3_state(actor->mScale);
                fill_actor_model_debug_state(state, actor);
            }
            states.push_back(std::move(state));
        }

        return states;
    }

    std::span<const SceneSchedulerEntryState> SceneScheduler::last_execution_trace() const {
        return _last_execution_trace;
    }

    std::span<const SceneSchedulerMessageTraceEntry> SceneScheduler::message_trace() const {
        return _message_trace;
    }

    std::vector<SceneLayoutRuntimeDebugState> SceneScheduler::debug_layout_runtime_snapshot() const {
        smgpc::compat::JkrHostAllocationScope host;
        auto states = std::vector<SceneLayoutRuntimeDebugState>{};
        for (const auto &entry : _entries) {
            if ((entry.kind != SceneEntryKind::Layout || entry.layout == nullptr) &&
                (entry.kind != SceneEntryKind::LayoutActor || entry.layout_actor == nullptr ||
                 smgpc::layout::layout_runtime(entry.layout_actor) == nullptr)) {
                continue;
            }
            const auto *layout = entry.kind == SceneEntryKind::Layout ? entry.layout : smgpc::layout::layout_runtime(entry.layout_actor);

            auto state = SceneLayoutRuntimeDebugState {
                .name = entry_name(entry),
                .layout_name = layout->getLayoutName(),
                .has_archive_path = false,
                .archive_path = {},
                .movement_type = entry.movement_type,
                .calc_anim_type = entry.calc_anim_type,
                .draw_type = entry.draw_type,
                .order = entry.order,
                .suspended = entry_is_suspended(entry),
                .dead = entry_is_dead(entry),
                .pane_count = layout->debugPaneCount(),
                .picture_count = layout->debugPictureCount(),
                .text_box_count = layout->debugTextBoxCount(),
                .material_count = layout->debugMaterialCount(),
                .texture_count = layout->debugTextureCount(),
                .font_count = layout->debugFontCount(),
                .committed_pane_frame_count = layout->debugCommittedPaneFrameCount(),
                .animations = {},
                .pane_controls = {},
                .button_controllers = {},
                .panes = {},
                .materials = {},
                .textures = {},
            };
            if (layout->getArchivePath().has_value()) {
                state.has_archive_path = true;
                state.archive_path = layout->getArchivePath()->string();
            }

            const auto layer_count = layout->debugAnimLayerCount();
            state.animations.reserve(layer_count);
            for (auto layer_index = std::size_t {}; layer_index < layer_count; ++layer_index) {
                const auto layer = static_cast<u32>(layer_index);
                auto animation = SceneLayoutAnimationDebugState {
                    .layer_index = layer_index,
                    .active = layout->hasActiveAnimation(layer),
                    .name = std::string(layout->debugAnimName(layer)),
                };
                if (animation.active) {
                    animation.frame = layout->getAnimFrame(layer);
                    animation.end_frame = layout->debugAnimEndFrame(layer);
                    animation.rate = layout->debugAnimRate(layer);
                    animation.stopped = layout->debugAnimStopped(layer);
                    animation.looping = layout->debugAnimLooping(layer);
                }
                state.animations.push_back(std::move(animation));
            }

            const auto panes = layout->debugPanes();
            state.panes.reserve(panes.size());
            for (const auto &pane : panes) {
                auto pane_state = SceneLayoutPaneRuntimeDebugState {
                    .index = pane.index,
                    .name = pane.name,
                    .parent_index = pane.parent_index,
                    .base_visible = pane.base_visible,
                    .effective_visible = pane.effective_visible,
                    .translate_x = pane.translate_x,
                    .translate_y = pane.translate_y,
                    .scale_x = pane.scale_x,
                    .scale_y = pane.scale_y,
                    .alpha = pane.alpha,
                    .width = pane.width,
                    .height = pane.height,
                    .contents = {},
                };
                pane_state.contents.reserve(pane.contents.size());
                for (const auto &content : pane.contents) {
                    pane_state.contents.push_back(SceneLayoutPaneContentDebugState {
                        .kind = content.kind,
                        .name = content.name,
                        .material_index = content.material_index,
                        .material_name = content.material_name,
                        .texture_name = content.texture_name,
                        .font_name = content.font_name,
                        .visible = content.visible,
                    });
                }
                state.panes.push_back(std::move(pane_state));
            }

            const auto materials = layout->debugMaterials();
            state.materials.reserve(materials.size());
            for (const auto &material : materials) {
                auto material_state = SceneLayoutMaterialDebugState {
                    .index = material.index,
                    .name = material.name,
                    .texture_count = material.texture_count,
                    .tex_coord_gen_count = material.tex_coord_gen_count,
                    .tev_stage_count = material.tev_stage_count,
                    .alpha_compare_enabled = material.alpha_compare_enabled,
                    .blend_enabled = material.blend_enabled,
                    .textures = {},
                };
                material_state.textures.reserve(material.textures.size());
                for (const auto &texture : material.textures) {
                    material_state.textures.push_back(SceneLayoutMaterialTextureDebugState {
                        .slot = texture.slot,
                        .texture_index = texture.texture_index,
                        .texture_name = texture.texture_name,
                        .wrap_s = texture.wrap_s,
                        .wrap_t = texture.wrap_t,
                        .min_filter = texture.min_filter,
                        .mag_filter = texture.mag_filter,
                    });
                }
                state.materials.push_back(std::move(material_state));
            }

            const auto textures = layout->debugTextures();
            state.textures.reserve(textures.size());
            for (const auto &texture : textures) {
                state.textures.push_back(SceneLayoutTextureDebugState {
                    .index = texture.index,
                    .name = texture.name,
                    .width = texture.width,
                    .height = texture.height,
                    .format_raw = texture.format_raw,
                    .format_name = texture.format_name,
                    .uploaded = texture.uploaded,
                    .rgba_byte_count = texture.rgba_byte_count,
                });
            }

            if (entry.kind == SceneEntryKind::LayoutActor && entry.layout_actor->getLayoutManager() != nullptr) {
                const auto pane_controls = smgpc::layout::debug_pane_controls(entry.layout_actor->getLayoutManager());
                state.pane_controls.reserve(pane_controls.size());
                for (const auto &pane_control : pane_controls) {
                    auto pane_state = SceneLayoutPaneControlDebugState {
                        .pane_name = pane_control.pane_name,
                        .exists_in_layout = pane_control.exists_in_layout,
                        .visible = pane_control.visible,
                        .animations = {},
                    };
                    pane_state.animations.reserve(pane_control.animations.size());
                    for (const auto &animation : pane_control.animations) {
                        pane_state.animations.push_back(SceneLayoutPaneControlAnimationDebugState {
                            .layer_index = animation.layer_index,
                            .name = animation.name,
                            .frame = animation.frame,
                            .end_frame = animation.end_frame,
                            .rate = animation.rate,
                            .stopped = animation.stopped,
                            .looping = animation.looping,
                        });
                    }
                    state.pane_controls.push_back(std::move(pane_state));
                }

                const auto button_controllers = smgpc::layout::debug_button_controllers(entry.layout_actor->getLayoutManager());
                state.button_controllers.reserve(button_controllers.size());
                for (const auto &button : button_controllers) {
                    state.button_controllers.push_back(SceneLayoutButtonControllerDebugState {
                        .pane_name = button.pane_name,
                        .bounding_pane_name = button.bounding_pane_name,
                        .nerve = button.nerve,
                        .anim_layer = button.anim_layer,
                        .active = button.active,
                        .selected = button.selected,
                        .pointing = button.pointing,
                        .appearance_enabled = button.appearance_enabled,
                        .decide_enabled = button.decide_enabled,
                        .pointing_anim_start_frame = button.pointing_anim_start_frame,
                    });
                }
            }

            states.push_back(std::move(state));
        }

        return states;
    }
#endif

    void SceneScheduler::clear() {
        for (const auto& entry : _entries)
            if (entry.has_draw_buffer_registration) _draw_buffers->remove_actor(*entry.live_actor);
        _entries.clear();
        _draw_buffers->clear_draw_categories();
        _layout_draw_adaptors.clear();
        retire_draw_buffers();
#ifndef NDEBUG
        _last_execution_trace.clear();
        _message_trace.clear();
        _next_message_sequence = 0U;
#endif
        _next_order = 0U;
    }

    SceneScheduler::Entry *SceneScheduler::find_entry(SceneEntryKind kind, const void *ptr) {
        const auto it = std::ranges::find_if(_entries, [kind, ptr](const auto &entry) {
            if (entry.kind != kind) {
                return false;
            }
            switch (kind) {
            case SceneEntryKind::NameObj:
                return entry.name_obj == ptr;
            case SceneEntryKind::Layout:
                return entry.layout == ptr;
            case SceneEntryKind::LayoutActor:
                return entry.layout_actor == ptr;
            case SceneEntryKind::LiveActorModel:
                return entry.live_actor == ptr;
            }

            return false;
        });

        return it == _entries.end() ? nullptr : &*it;
    }

    const SceneScheduler::Entry *SceneScheduler::find_entry(SceneEntryKind kind, const void *ptr) const {
        return const_cast<SceneScheduler *>(this)->find_entry(kind, ptr);
    }

    std::vector<SceneScheduler::Entry *> SceneScheduler::sorted_entries_for_movement() {
        smgpc::compat::JkrHostAllocationScope host;
        auto entries = std::vector<Entry *>{};
        for (auto &entry : _entries) {
            entries.push_back(&entry);
        }
        std::ranges::stable_sort(entries, movement_category_less);
        return entries;
    }

    std::vector<SceneScheduler::Entry *> SceneScheduler::sorted_entries_for_calc_anim() {
        smgpc::compat::JkrHostAllocationScope host;
        auto entries = std::vector<Entry *>{};
        for (auto &entry : _entries) {
            entries.push_back(&entry);
        }
        std::ranges::stable_sort(entries, calc_category_less);
        return entries;
    }

    std::vector<SceneScheduler::Entry *> SceneScheduler::sorted_entries_for_calc_view_and_entry() {
        smgpc::compat::JkrHostAllocationScope host;
        auto entries = std::vector<Entry *>{};
        for (auto &entry : _entries) {
            entries.push_back(&entry);
        }
        std::ranges::stable_sort(entries, calc_view_entry_less);
        return entries;
    }

    bool SceneScheduler::entry_is_dead(const Entry &entry) {
        switch (entry.kind) {
        case SceneEntryKind::NameObj:
            return false;
        case SceneEntryKind::Layout:
            return entry.layout == nullptr || entry.layout->isDead();
        case SceneEntryKind::LayoutActor:
            return smgpc::layout::is_layout_actor_dead(entry.layout_actor);
        case SceneEntryKind::LiveActorModel:
            return entry.live_actor == nullptr || entry.live_actor->mFlag.mIsDead;
        }

        return true;
    }

    bool SceneScheduler::entry_is_suspended(const Entry &entry) {
        return smgpc::compat::name_obj_is_suspended(entry.name_obj);
    }

    std::string SceneScheduler::entry_name(const Entry &entry) {
        switch (entry.kind) {
        case SceneEntryKind::NameObj:
            return entry.name_obj == nullptr ? std::string {} : std::string(entry.name_obj->getName());
        case SceneEntryKind::Layout:
            return entry.layout == nullptr ? std::string {} : entry.layout->getName();
        case SceneEntryKind::LayoutActor:
            return entry.layout_actor == nullptr ? std::string {} : std::string(entry.layout_actor->getName());
        case SceneEntryKind::LiveActorModel:
            return entry.live_actor == nullptr ? std::string {} : std::string(entry.live_actor->getName());
        }

        return {};
    }

#ifndef NDEBUG
        [[nodiscard]] std::string material_animation_name(const AnmPlayerBase* player) {
            if (!player || !player->mAnmRes || !player->mResTable) return {};
            const auto* name = player->mResTable->findResName(player->mAnmRes);
            return name ? name : "";
        }

        void fill_actor_model_debug_state(SceneSchedulerEntryState& state, const LiveActor* actor) {
            if (const auto matrix = actor->getBaseMtx())
                std::copy_n(&matrix[0][0], 12, state.live_actor_base_matrix.begin());
            if (const auto* manager = actor->mModelManager) {
                const auto* name = manager->getPlayingBckName();
                state.live_actor_bck_name = name ? name : "";
                state.live_actor_brk_name = material_animation_name(manager->mBrkPlayer);
                state.live_actor_btk_name = material_animation_name(manager->mBtkPlayer);
            }
        }

    void SceneScheduler::push_trace(const Entry &entry, SceneSchedulerPhase phase, SceneDrawBufferPass pass) {
        // Debug history outlives scene arenas. Scope only metadata construction;
        // movement, animation and draw callbacks keep their caller's Game heap.
        smgpc::compat::JkrHostAllocationScope host;
        auto state = SceneSchedulerEntryState {
            .kind = entry.kind,
            .phase = phase,
            .name = entry_name(entry),
            .movement_type = entry.movement_type,
            .calc_anim_type = entry.calc_anim_type,
            .draw_buffer_type = entry.draw_buffer_type,
            .draw_type = entry.draw_type,
            .draw_buffer_pass = pass,
            .order = entry.order,
            .draw_connected = entry.draw_connected,
            .suspended = entry_is_suspended(entry),
            .dead = entry_is_dead(entry),
            .has_live_actor_state = false,
            .live_actor_nerve_step = 0,
            .live_actor_position = {},
            .live_actor_rotation = {},
            .live_actor_scale = {},
            .live_actor_base_matrix = {},
            .live_actor_bck_name = {},
            .live_actor_brk_name = {},
            .live_actor_btk_name = {},
        };
        if (const auto *actor = entry_live_actor(entry)) {
            state.has_live_actor_state = true;
            state.live_actor_nerve_step = actor->getNerveStep();
            state.live_actor_position = vec3_state(actor->mPosition);
            state.live_actor_rotation = vec3_state(actor->mRotation);
            state.live_actor_scale = vec3_state(actor->mScale);
            fill_actor_model_debug_state(state, actor);
        }
        _last_execution_trace.push_back(std::move(state));
    }

    void SceneScheduler::push_message_trace(SceneSchedulerMessageTraceEntry trace) {
        smgpc::compat::JkrHostAllocationScope host;
        constexpr auto max_message_trace_entries = std::size_t {512U};
        if (_message_trace.size() >= max_message_trace_entries) {
            _message_trace.erase(_message_trace.begin());
        }
        _message_trace.push_back(std::move(trace));
    }
#endif

}  // namespace smgpc::runtime
