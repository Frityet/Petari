#include "SceneScheduler.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <utility>

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/LightUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace smgpc::game {
    namespace {

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
            DrawBufferPassCommand{MR::DrawBufferType_CrystalItem, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_CrystalItem, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_Crystal, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_Crystal, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_AstroDomeSky, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_AstroDomeSky, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_Planet, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_PlanetLow, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_Environment, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_MapObj, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_MapObjWeakLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_MapObjStrongLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_TripodBoss, SceneDrawBufferPass::Opaque},
        };

        constexpr auto PRIOR_AIR_COMMANDS = std::array<DrawBufferPassCommand, 6U>{
            DrawBufferPassCommand{MR::DrawBufferType_Sky, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_Air, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_Sun, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_Sky, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_Air, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_Sun, SceneDrawBufferPass::Translucent},
        };

        constexpr auto NORMAL_OPA_BEFORE_SILHOUETTE_COMMANDS = std::array<DrawBufferPassCommand, 2U>{
            DrawBufferPassCommand{MR::DrawBufferType_NoShadowedMapObj, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_NoShadowedMapObjStrongLight, SceneDrawBufferPass::Opaque},
        };

        constexpr auto NORMAL_OPA_COMMANDS = std::array<DrawBufferPassCommand, 8U>{
            DrawBufferPassCommand{MR::DrawBufferType_NoSilhouettedMapObj, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_NoSilhouettedMapObjWeakLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_NoSilhouettedMapObjStrongLight, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_NPC, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_Ride, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_Enemy, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_EnemyDecoration, SceneDrawBufferPass::Opaque},
            DrawBufferPassCommand{MR::DrawBufferType_PlayerDecoration, SceneDrawBufferPass::Opaque},
        };

        constexpr auto NORMAL_XLU_COMMANDS = std::array<DrawBufferPassCommand, 18U>{
            DrawBufferPassCommand{MR::DrawBufferType_Planet, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_PlanetLow, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_Environment, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_EnvironmentStrongLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_MapObj, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_MapObjWeakLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_MapObjStrongLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_TripodBoss, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_NoShadowedMapObj, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_NoShadowedMapObjStrongLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_NoSilhouettedMapObj, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_NoSilhouettedMapObjWeakLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_NoSilhouettedMapObjStrongLight, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_NPC, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_Ride, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_Enemy, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_EnemyDecoration, SceneDrawBufferPass::Translucent},
            DrawBufferPassCommand{MR::DrawBufferType_PlayerDecoration, SceneDrawBufferPass::Translucent},
        };

        constexpr auto NORMAL_2D_DRAW_TYPES = std::array<s32, 9U>{
            MR::DrawType_CometScreenFilter,
            MR::DrawType_GalaxyNamePlate,
            MR::DrawType_Layout,
            MR::DrawType_LayoutDecoration,
            MR::DrawType_CinemaFrame,
            MR::DrawType_TalkLayout,
            MR::DrawType_0x44,
            MR::DrawType_EffectDraw2D,
            MR::DrawType_EffectDrawFor2DModel,
        };

        [[nodiscard]] std::size_t category_rank(s32 category, std::span<const s32> order) {
            if (category < 0) {
                return std::numeric_limits<std::size_t>::max();
            }

            for (auto i = std::size_t{}; i < order.size(); ++i) {
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
        [[nodiscard]] std::string sensor_host_name(const HitSensor *sensor) {
            const auto *host = MR::getSensorHost(sensor);
            return host != nullptr ? host->getName() : "";
        }

        [[nodiscard]] std::array<float, 3U> vec3_state(const TVec3f &value) {
            return {value.x, value.y, value.z};
        }
#endif

    }  // namespace

    void SceneScheduler::connect_name_obj(NameObj &obj, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type) {
        if (auto *entry = find_entry(SceneEntryKind::NameObj, &obj)) {
            entry->movement_type = movement_type;
            entry->calc_anim_type = calc_anim_type;
            entry->draw_buffer_type = draw_buffer_type;
            entry->draw_type = draw_type;
            return;
        }

        _entries.push_back(Entry{
            .kind = SceneEntryKind::NameObj,
            .name_obj = &obj,
            .movement_type = movement_type,
            .calc_anim_type = calc_anim_type,
            .draw_buffer_type = draw_buffer_type,
            .draw_type = draw_type,
            .order = _next_order++,
        });
    }

    void SceneScheduler::disconnect_name_obj(NameObj &obj) {
        std::erase_if(_entries, [&obj](const auto &entry) {
            return entry.kind == SceneEntryKind::NameObj && entry.name_obj == &obj;
        });
    }

    void SceneScheduler::register_layout(SimpleLayout &layout, s32 movement_type, s32 calc_anim_type, s32 draw_type) {
        if (auto *entry = find_entry(SceneEntryKind::Layout, &layout)) {
            entry->movement_type = movement_type;
            entry->calc_anim_type = calc_anim_type;
            entry->draw_type = draw_type;
            return;
        }

        _entries.push_back(Entry{
            .kind = SceneEntryKind::Layout,
            .layout = &layout,
            .movement_type = movement_type,
            .calc_anim_type = calc_anim_type,
            .draw_type = draw_type,
            .order = _next_order++,
        });
    }

    void SceneScheduler::unregister_layout(SimpleLayout &layout) {
        std::erase_if(_entries, [&layout](const auto &entry) {
            return entry.kind == SceneEntryKind::Layout && entry.layout == &layout;
        });
    }

    void SceneScheduler::register_layout_actor(LayoutActor &layout, s32 movement_type, s32 calc_anim_type, s32 draw_type) {
        if (auto *entry = find_entry(SceneEntryKind::LayoutActor, &layout)) {
            entry->movement_type = movement_type;
            entry->calc_anim_type = calc_anim_type;
            entry->draw_type = draw_type;
            return;
        }

        _entries.push_back(Entry{
            .kind = SceneEntryKind::LayoutActor,
            .name_obj = &layout,
            .layout_actor = &layout,
            .movement_type = movement_type,
            .calc_anim_type = calc_anim_type,
            .draw_type = draw_type,
            .order = _next_order++,
        });
    }

    void SceneScheduler::unregister_layout_actor(LayoutActor &layout) {
        std::erase_if(_entries, [&layout](const auto &entry) {
            return entry.kind == SceneEntryKind::LayoutActor && entry.layout_actor == &layout;
        });
    }

    void SceneScheduler::register_live_actor_model(LiveActor &actor, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type) {
        if (auto *entry = find_entry(SceneEntryKind::LiveActorModel, &actor)) {
            entry->movement_type = movement_type;
            entry->calc_anim_type = calc_anim_type;
            entry->draw_buffer_type = draw_buffer_type;
            entry->draw_type = draw_type;
            MR::initActorLightInfoLightType(&actor, light_type_for_draw_buffer(draw_buffer_type));
            return;
        }

        _entries.push_back(Entry{
            .kind = SceneEntryKind::LiveActorModel,
            .name_obj = &actor,
            .live_actor = &actor,
            .movement_type = movement_type,
            .calc_anim_type = calc_anim_type,
            .draw_buffer_type = draw_buffer_type,
            .draw_type = draw_type,
            .order = _next_order++,
        });
        MR::initActorLightInfoLightType(&actor, light_type_for_draw_buffer(draw_buffer_type));
    }

    void SceneScheduler::unregister_live_actor_model(LiveActor &actor) {
        std::erase_if(_entries, [&actor](const auto &entry) {
            return entry.kind == SceneEntryKind::LiveActorModel && entry.live_actor == &actor;
        });
    }

    void SceneScheduler::execute_movement() {
#ifndef NDEBUG
        _last_execution_trace.clear();
#endif
        for (auto *entry : sorted_entries_for_movement()) {
            if (entry->movement_type < 0 || entry_is_dead(*entry) || entry_is_suspended(*entry)) {
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
#ifndef NDEBUG
            push_trace(*entry, SceneSchedulerPhase::Movement);
#endif
        }
    }

    void SceneScheduler::execute_calc_anim() {
        for (auto *entry : sorted_entries_for_calc_anim()) {
            if (entry->calc_anim_type < 0 || entry_is_dead(*entry) || entry_is_suspended(*entry)) {
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
        for (auto *entry : sorted_entries_for_calc_view_and_entry()) {
            if (!participates_in_calc_view_and_entry(*entry) || entry_is_dead(*entry) || entry_is_suspended(*entry)) {
                continue;
            }

            switch (entry->kind) {
            case SceneEntryKind::NameObj:
                entry->name_obj->calcViewAndEntry();
                break;
            case SceneEntryKind::Layout:
                break;
            case SceneEntryKind::LayoutActor:
                entry->layout_actor->calcViewAndEntry();
                break;
            case SceneEntryKind::LiveActorModel:
                entry->live_actor->calcViewAndEntry();
                break;
            }
#ifndef NDEBUG
            push_trace(*entry, SceneSchedulerPhase::CalcViewAndEntry);
#endif
        }
    }

    void SceneScheduler::execute_draw_buffer_opa(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose, s32 draw_buffer_type) {
        execute_draw_buffer(renderer, camera_pose, draw_buffer_type, SceneDrawBufferPass::Opaque);
    }

    void SceneScheduler::execute_draw_buffer_xlu(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose, s32 draw_buffer_type) {
        execute_draw_buffer(renderer, camera_pose, draw_buffer_type, SceneDrawBufferPass::Translucent);
    }

    void SceneScheduler::execute_draw_buffer_list_normal_opa_before_volume_shadow(render::IRendererEngine &renderer,
                                                                                  const CameraPoseCompat &camera_pose, bool prior_draw_air) {
        for (const auto &command : NORMAL_OPA_BEFORE_VOLUME_SHADOW_COMMANDS) {
            execute_draw_buffer(renderer, camera_pose, command.draw_buffer_type, command.pass);
            if (command.draw_buffer_type == MR::DrawBufferType_AstroDomeSky && command.pass == SceneDrawBufferPass::Translucent &&
                prior_draw_air) {
                for (const auto &air_command : PRIOR_AIR_COMMANDS) {
                    execute_draw_buffer(renderer, camera_pose, air_command.draw_buffer_type, air_command.pass);
                }
            }
            if (command.draw_buffer_type == MR::DrawBufferType_PlanetLow) {
                execute_draw_type(renderer, MR::DrawType_FlexibleSphere);
            }
        }
    }

    void SceneScheduler::execute_draw_buffer_list_normal_opa_before_silhouette(render::IRendererEngine &renderer,
                                                                               const CameraPoseCompat &camera_pose) {
        for (const auto &command : NORMAL_OPA_BEFORE_SILHOUETTE_COMMANDS) {
            execute_draw_buffer(renderer, camera_pose, command.draw_buffer_type, command.pass);
        }
    }

    void SceneScheduler::execute_draw_buffer_list_normal_opa(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose,
                                                             bool prior_draw_air) {
        for (const auto &command : NORMAL_OPA_COMMANDS) {
            execute_draw_buffer(renderer, camera_pose, command.draw_buffer_type, command.pass);
        }
        if (!prior_draw_air) {
            for (const auto &air_command : PRIOR_AIR_COMMANDS) {
                execute_draw_buffer(renderer, camera_pose, air_command.draw_buffer_type, air_command.pass);
            }
        }
    }

    void SceneScheduler::execute_draw_buffer_list_normal_xlu(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose) {
        for (const auto &command : NORMAL_XLU_COMMANDS) {
            execute_draw_buffer(renderer, camera_pose, command.draw_buffer_type, command.pass);
        }
    }

    void SceneScheduler::execute_draw_buffer_list_normal(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose,
                                                         bool prior_draw_air) {
        auto *runtime = RuntimeContext::try_instance();
        const auto previous_pixel_update_state = runtime != nullptr ? runtime->j3d_pixel_update_state() :
                                                                      std::optional<RuntimeContext::GxPixelUpdateState>{};
        if (runtime != nullptr) {
            runtime->set_j3d_pixel_update_state(RuntimeContext::GxPixelUpdateState{.color_update = true, .alpha_update = true});
        }
        execute_draw_buffer_list_normal_opa_before_volume_shadow(renderer, camera_pose, prior_draw_air);
        if (runtime != nullptr) {
            runtime->set_j3d_pixel_update_state(RuntimeContext::GxPixelUpdateState{.color_update = true, .alpha_update = true});
        }
        execute_draw_buffer_list_normal_opa_before_silhouette(renderer, camera_pose);
        MR::fillSilhouetteColor();
        if (runtime != nullptr) {
            runtime->set_j3d_pixel_update_state(RuntimeContext::GxPixelUpdateState{.color_update = true, .alpha_update = false});
        }
        execute_draw_buffer_list_normal_opa(renderer, camera_pose, prior_draw_air);
        execute_draw_buffer_list_normal_xlu(renderer, camera_pose);
        if (runtime != nullptr) {
            runtime->set_j3d_pixel_update_state(previous_pixel_update_state);
        }
    }

    void SceneScheduler::execute_draw_list_2d_normal(render::IRendererEngine &renderer) {
        for (const auto draw_type : NORMAL_2D_DRAW_TYPES) {
            execute_draw_type(renderer, draw_type);
        }
    }

    std::size_t SceneScheduler::send_message_to_live_actors(u32 msg, LiveActor *exclude_actor) {
        auto seen_actors = std::vector<LiveActor *>{};
        auto accepted_count = std::size_t{};
        auto *message_sensor = MR::getMessageSensor();

        for (auto &entry : _entries) {
            auto *actor = entry_live_actor(entry);
            if (actor == nullptr || std::ranges::find(seen_actors, actor) != seen_actors.end()) {
                continue;
            }
            seen_actors.push_back(actor);

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
            push_message_trace(SceneSchedulerMessageTraceEntry{
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

    void SceneScheduler::execute_draw_buffer(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose, s32 draw_buffer_type,
                                             SceneDrawBufferPass pass) {
        auto actor_entries = std::vector<Entry *>{};
        for (auto &entry : _entries) {
            if (entry.kind == SceneEntryKind::LiveActorModel && entry.draw_buffer_type == draw_buffer_type && !entry_is_dead(entry) &&
                !entry_is_suspended(entry)) {
                actor_entries.push_back(&entry);
            }
        }
        if (actor_entries.empty()) {
            return;
        }
        std::ranges::stable_sort(actor_entries, draw_category_less);
        MR::loadLight(light_type_for_draw_buffer(draw_buffer_type));

        const auto model_pass = pass == SceneDrawBufferPass::Translucent ? LiveActorModelCompat::DrawPass::Translucent :
                                                                           LiveActorModelCompat::DrawPass::Opaque;
#ifndef NDEBUG
        const auto phase = pass == SceneDrawBufferPass::Translucent ? SceneSchedulerPhase::DrawBufferXlu : SceneSchedulerPhase::DrawBufferOpa;
#endif
        for (auto *entry : actor_entries) {
            entry->live_actor->loadActorLight();
            entry->live_actor->drawModel(renderer, camera_pose, static_cast<std::uint64_t>(entry->live_actor->getNerveStep()), model_pass);
#ifndef NDEBUG
            push_trace(*entry, phase, pass);
#endif
        }
    }

    void SceneScheduler::execute_draw_type(render::IRendererEngine &renderer, s32 draw_type) {
        auto draw_entries = std::vector<Entry *>{};
        for (auto &entry : _entries) {
            if (entry.draw_type == draw_type && !entry_is_dead(entry) && !entry_is_suspended(entry)) {
                draw_entries.push_back(&entry);
            }
        }
        std::ranges::stable_sort(draw_entries, draw_category_less);

        for (auto *entry : draw_entries) {
            switch (entry->kind) {
            case SceneEntryKind::NameObj:
                entry->name_obj->draw();
                break;
            case SceneEntryKind::Layout:
                entry->layout->draw(renderer);
                break;
            case SceneEntryKind::LayoutActor:
                entry->layout_actor->drawLayout(renderer);
                break;
            case SceneEntryKind::LiveActorModel:
                entry->live_actor->draw();
                break;
            }
#ifndef NDEBUG
            push_trace(*entry, SceneSchedulerPhase::DrawType);
#endif
        }
    }

#ifndef NDEBUG
    std::vector<SceneSchedulerEntryState> SceneScheduler::snapshot() const {
        auto states = std::vector<SceneSchedulerEntryState>{};
        states.reserve(_entries.size());
        for (const auto &entry : _entries) {
            auto state = SceneSchedulerEntryState{
                .kind = entry.kind,
                .phase = SceneSchedulerPhase::None,
                .name = entry_name(entry),
                .movement_type = entry.movement_type,
                .calc_anim_type = entry.calc_anim_type,
                .draw_buffer_type = entry.draw_buffer_type,
                .draw_type = entry.draw_type,
                .draw_buffer_pass = SceneDrawBufferPass::None,
                .order = entry.order,
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
                state.live_actor_base_matrix = actor->getBaseMatrix().m;
                state.live_actor_bck_name = std::string(actor->currentBckName());
                state.live_actor_brk_name = std::string(actor->currentBrkName());
                state.live_actor_btk_name = std::string(actor->currentBtkName());
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
        auto states = std::vector<SceneLayoutRuntimeDebugState>{};
        for (const auto &entry : _entries) {
            if ((entry.kind != SceneEntryKind::Layout || entry.layout == nullptr) &&
                (entry.kind != SceneEntryKind::LayoutActor || entry.layout_actor == nullptr || entry.layout_actor->getSimpleLayout() == nullptr)) {
                continue;
            }
            const auto *layout = entry.kind == SceneEntryKind::Layout ? entry.layout : entry.layout_actor->getSimpleLayout();

            auto state = SceneLayoutRuntimeDebugState{
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
            for (auto layer_index = std::size_t{}; layer_index < layer_count; ++layer_index) {
                const auto layer = static_cast<u32>(layer_index);
                state.animations.push_back(SceneLayoutAnimationDebugState{
                    .layer_index = layer_index,
                    .name = std::string(layout->debugAnimName(layer)),
                    .frame = layout->getAnimFrame(layer),
                    .end_frame = layout->debugAnimEndFrame(layer),
                    .rate = layout->debugAnimRate(layer),
                    .stopped = layout->debugAnimStopped(layer),
                    .looping = layout->debugAnimLooping(layer),
                });
            }

            const auto panes = layout->debugPanes();
            state.panes.reserve(panes.size());
            for (const auto &pane : panes) {
                auto pane_state = SceneLayoutPaneRuntimeDebugState{
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
                    pane_state.contents.push_back(SceneLayoutPaneContentDebugState{
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
                auto material_state = SceneLayoutMaterialDebugState{
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
                    material_state.textures.push_back(SceneLayoutMaterialTextureDebugState{
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
                state.textures.push_back(SceneLayoutTextureDebugState{
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
                const auto pane_controls = entry.layout_actor->getLayoutManager()->debugPaneControls();
                state.pane_controls.reserve(pane_controls.size());
                for (const auto &pane_control : pane_controls) {
                    auto pane_state = SceneLayoutPaneControlDebugState{
                        .pane_name = pane_control.pane_name,
                        .exists_in_layout = pane_control.exists_in_layout,
                        .visible = pane_control.visible,
                        .animations = {},
                    };
                    pane_state.animations.reserve(pane_control.animations.size());
                    for (const auto &animation : pane_control.animations) {
                        pane_state.animations.push_back(SceneLayoutPaneControlAnimationDebugState{
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

                const auto button_controllers = entry.layout_actor->getLayoutManager()->debugButtonControllers();
                state.button_controllers.reserve(button_controllers.size());
                for (const auto &button : button_controllers) {
                    state.button_controllers.push_back(SceneLayoutButtonControllerDebugState{
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
        _entries.clear();
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
        auto entries = std::vector<Entry *>{};
        for (auto &entry : _entries) {
            entries.push_back(&entry);
        }
        std::ranges::stable_sort(entries, movement_category_less);
        return entries;
    }

    std::vector<SceneScheduler::Entry *> SceneScheduler::sorted_entries_for_calc_anim() {
        auto entries = std::vector<Entry *>{};
        for (auto &entry : _entries) {
            entries.push_back(&entry);
        }
        std::ranges::stable_sort(entries, calc_category_less);
        return entries;
    }

    std::vector<SceneScheduler::Entry *> SceneScheduler::sorted_entries_for_calc_view_and_entry() {
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
            return entry.layout_actor == nullptr || entry.layout_actor->isDead();
        case SceneEntryKind::LiveActorModel:
            return entry.live_actor == nullptr || entry.live_actor->isDead();
        }

        return true;
    }

    bool SceneScheduler::entry_is_suspended(const Entry &entry) {
        return entry.name_obj != nullptr && entry.name_obj->isSuspended();
    }

    std::string SceneScheduler::entry_name(const Entry &entry) {
        switch (entry.kind) {
        case SceneEntryKind::NameObj:
            return entry.name_obj == nullptr ? std::string{} : std::string(entry.name_obj->getName());
        case SceneEntryKind::Layout:
            return entry.layout == nullptr ? std::string{} : entry.layout->getName();
        case SceneEntryKind::LayoutActor:
            return entry.layout_actor == nullptr ? std::string{} : std::string(entry.layout_actor->getName());
        case SceneEntryKind::LiveActorModel:
            return entry.live_actor == nullptr ? std::string{} : std::string(entry.live_actor->getName());
        }

        return {};
    }

#ifndef NDEBUG
    void SceneScheduler::push_trace(const Entry &entry, SceneSchedulerPhase phase, SceneDrawBufferPass pass) {
        auto state = SceneSchedulerEntryState{
            .kind = entry.kind,
            .phase = phase,
            .name = entry_name(entry),
            .movement_type = entry.movement_type,
            .calc_anim_type = entry.calc_anim_type,
            .draw_buffer_type = entry.draw_buffer_type,
            .draw_type = entry.draw_type,
            .draw_buffer_pass = pass,
            .order = entry.order,
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
            state.live_actor_base_matrix = actor->getBaseMatrix().m;
            state.live_actor_bck_name = std::string(actor->currentBckName());
            state.live_actor_brk_name = std::string(actor->currentBrkName());
            state.live_actor_btk_name = std::string(actor->currentBtkName());
        }
        _last_execution_trace.push_back(std::move(state));
    }

    void SceneScheduler::push_message_trace(SceneSchedulerMessageTraceEntry trace) {
        constexpr auto max_message_trace_entries = std::size_t{512U};
        if (_message_trace.size() >= max_message_trace_entries) {
            _message_trace.erase(_message_trace.begin());
        }
        _message_trace.push_back(std::move(trace));
    }
#endif

}  // namespace smgpc::game
