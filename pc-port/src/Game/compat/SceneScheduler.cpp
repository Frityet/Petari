#include "SceneScheduler.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/SimpleLayout.hpp"

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
            if (entry->kind == SceneEntryKind::Layout || (entry->draw_buffer_type < 0 && entry->draw_type >= 0)) {
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
            return entry.kind == SceneEntryKind::Layout || entry.draw_buffer_type >= 0 || entry.draw_type >= 0;
        }

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

    void SceneScheduler::register_live_actor_model(LiveActor &actor, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type) {
        if (auto *entry = find_entry(SceneEntryKind::LiveActorModel, &actor)) {
            entry->movement_type = movement_type;
            entry->calc_anim_type = calc_anim_type;
            entry->draw_buffer_type = draw_buffer_type;
            entry->draw_type = draw_type;
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
    }

    void SceneScheduler::unregister_live_actor_model(LiveActor &actor) {
        std::erase_if(_entries, [&actor](const auto &entry) {
            return entry.kind == SceneEntryKind::LiveActorModel && entry.live_actor == &actor;
        });
    }

    void SceneScheduler::execute_movement() {
        _last_execution_trace.clear();
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
            case SceneEntryKind::LiveActorModel:
                entry->live_actor->movement();
                break;
            }
            push_trace(*entry, SceneSchedulerPhase::Movement);
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
            case SceneEntryKind::LiveActorModel:
                entry->live_actor->calcAnim();
                break;
            }
            push_trace(*entry, SceneSchedulerPhase::CalcAnim);
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
            case SceneEntryKind::LiveActorModel:
                entry->live_actor->calcViewAndEntry();
                break;
            }
            push_trace(*entry, SceneSchedulerPhase::CalcViewAndEntry);
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
        execute_draw_buffer_list_normal_opa_before_volume_shadow(renderer, camera_pose, prior_draw_air);
        execute_draw_buffer_list_normal_opa_before_silhouette(renderer, camera_pose);
        execute_draw_buffer_list_normal_opa(renderer, camera_pose, prior_draw_air);
        execute_draw_buffer_list_normal_xlu(renderer, camera_pose);
    }

    void SceneScheduler::execute_draw_list_2d_normal(render::IRendererEngine &renderer) {
        for (const auto draw_type : NORMAL_2D_DRAW_TYPES) {
            execute_draw_type(renderer, draw_type);
        }
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
        std::ranges::stable_sort(actor_entries, draw_category_less);

        const auto model_pass = pass == SceneDrawBufferPass::Translucent ? LiveActorModelCompat::DrawPass::Translucent :
                                                                           LiveActorModelCompat::DrawPass::Opaque;
        const auto phase = pass == SceneDrawBufferPass::Translucent ? SceneSchedulerPhase::DrawBufferXlu : SceneSchedulerPhase::DrawBufferOpa;
        for (auto *entry : actor_entries) {
            entry->live_actor->drawModel(renderer, camera_pose, static_cast<std::uint64_t>(entry->live_actor->getNerveStep()), model_pass);
            push_trace(*entry, phase, pass);
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
            case SceneEntryKind::LiveActorModel:
                entry->live_actor->draw();
                break;
            }
            push_trace(*entry, SceneSchedulerPhase::DrawType);
        }
    }

    std::vector<SceneSchedulerEntryState> SceneScheduler::snapshot() const {
        auto states = std::vector<SceneSchedulerEntryState>{};
        states.reserve(_entries.size());
        for (const auto &entry : _entries) {
            states.push_back(SceneSchedulerEntryState{
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
            });
        }

        return states;
    }

    std::span<const SceneSchedulerEntryState> SceneScheduler::last_execution_trace() const {
        return _last_execution_trace;
    }

    std::vector<SceneLayoutRuntimeDebugState> SceneScheduler::debug_layout_runtime_snapshot() const {
        auto states = std::vector<SceneLayoutRuntimeDebugState>{};
        for (const auto &entry : _entries) {
            if (entry.kind != SceneEntryKind::Layout || entry.layout == nullptr) {
                continue;
            }

            auto state = SceneLayoutRuntimeDebugState{
                .name = entry.layout->getName(),
                .layout_name = entry.layout->getLayoutName(),
                .has_archive_path = false,
                .archive_path = {},
                .movement_type = entry.movement_type,
                .calc_anim_type = entry.calc_anim_type,
                .draw_type = entry.draw_type,
                .order = entry.order,
                .suspended = entry_is_suspended(entry),
                .dead = entry_is_dead(entry),
                .animations = {},
            };
            if (entry.layout->getArchivePath().has_value()) {
                state.has_archive_path = true;
                state.archive_path = entry.layout->getArchivePath()->string();
            }

            const auto layer_count = entry.layout->debugAnimLayerCount();
            state.animations.reserve(layer_count);
            for (auto layer_index = std::size_t{}; layer_index < layer_count; ++layer_index) {
                const auto layer = static_cast<u32>(layer_index);
                state.animations.push_back(SceneLayoutAnimationDebugState{
                    .layer_index = layer_index,
                    .name = std::string(entry.layout->debugAnimName(layer)),
                    .frame = entry.layout->getAnimFrame(layer),
                    .end_frame = entry.layout->debugAnimEndFrame(layer),
                    .rate = entry.layout->debugAnimRate(layer),
                    .stopped = entry.layout->debugAnimStopped(layer),
                    .looping = entry.layout->debugAnimLooping(layer),
                });
            }

            states.push_back(std::move(state));
        }

        return states;
    }

    void SceneScheduler::clear() {
        _entries.clear();
        _last_execution_trace.clear();
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
        case SceneEntryKind::LiveActorModel:
            return entry.live_actor == nullptr ? std::string{} : std::string(entry.live_actor->getName());
        }

        return {};
    }

    void SceneScheduler::push_trace(const Entry &entry, SceneSchedulerPhase phase, SceneDrawBufferPass pass) {
        _last_execution_trace.push_back(SceneSchedulerEntryState{
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
        });
    }

}  // namespace smgpc::game
