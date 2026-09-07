#include "scene/GatewaySpinCheckpoint.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/InformationObserver.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/GameDataHolderCompat.hpp"
#include "compat/InformationMessageCompat.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace smgpc::scene {
    namespace {

        constexpr auto cGuideDemoName = std::string_view{"チコガイドデモ"};
        constexpr auto cGuideSheetName = std::string_view{"TicoGuideDemo"};
        constexpr auto cSpinPartFirst = std::string_view{"スピンゲット[デモ1]"};
        constexpr auto cSpinPromptPart = std::string_view{"スピンゲット[デモ5]"};
        constexpr auto cMarioDemoPos4 = std::string_view{"MarioDemoPos4"};
        constexpr auto cMysteriousZone = std::string_view{"HeavensDoorMysteriousZone"};
        constexpr auto cLayerAObjInfo = std::string_view{"jmp/placement/layera/objinfo"};
        constexpr auto cCommonObjInfo = std::string_view{"jmp/placement/common/objinfo"};
        constexpr auto cFadeWipeName = std::string_view{"フェードワイプ"};
        constexpr auto cRosettaTriggerRadius = 500.0F;
        constexpr auto cFadeDurationFrames = std::uint32_t{90U};
        constexpr auto cFadeWipeFrames = std::int32_t{60};
        constexpr auto cExpectedPrePromptTicks = std::uint32_t{1670U};

        [[noreturn]] void reject(std::string_view detail) {
            throw std::runtime_error(
                "Gateway spin checkpoint rejected non-exact route data: " +
                std::string(detail));
        }

        void require(bool condition, std::string_view detail) {
            if (!condition) {
                reject(detail);
            }
        }

        void require_near(float actual, float expected, float tolerance,
                          std::string_view detail) {
            require(std::isfinite(actual) && std::fabs(actual - expected) <= tolerance,
                    detail);
        }

        [[nodiscard]] bool vec_near(const TVec3f &actual,
                                    const std::array<float, 3U> &expected,
                                    float tolerance) {
            return std::isfinite(actual.x) && std::isfinite(actual.y) &&
                   std::isfinite(actual.z) &&
                   std::fabs(actual.x - expected[0U]) <= tolerance &&
                   std::fabs(actual.y - expected[1U]) <= tolerance &&
                   std::fabs(actual.z - expected[2U]) <= tolerance;
        }

        [[nodiscard]] const StagePlacementObject &require_unique_placement(
            std::span<const StagePlacementObject> placements,
            std::string_view object_name, std::string_view table_path,
            std::int32_t source_row) {
            const auto found = std::ranges::find_if(placements, [&](const auto &row) {
                return row.object_name == object_name && row.zone_id == 5 &&
                       row.zone_name == cMysteriousZone &&
                       row.table_path == table_path &&
                       row.jmap_entry_index == source_row;
            });
            require(found != placements.end(),
                    std::string(object_name) + " exact placement row is absent");
            require(std::ranges::count_if(placements, [&](const auto &row) {
                        return row.object_name == object_name && row.zone_id == 5 &&
                               row.zone_name == cMysteriousZone &&
                               row.table_path == table_path &&
                               row.jmap_entry_index == source_row;
                    }) == 1,
                    std::string(object_name) + " exact placement row is ambiguous");
            return *found;
        }

        [[nodiscard]] const StageGeneralPos &require_unique_position(
            std::span<const StageGeneralPos> positions, std::string_view name,
            std::string_view layer_name, std::int32_t source_row,
            const std::array<float, 3U> &expected_local) {
            const auto found = std::ranges::find_if(positions, [&](const auto &row) {
                return row.name == name && row.zone_id == 5 &&
                       row.zone_name == cMysteriousZone &&
                       row.layer_name == layer_name &&
                       row.jmap_entry_index == source_row;
            });
            require(found != positions.end(),
                    std::string(name) + " exact GeneralPos row is absent");
            require(std::ranges::count_if(positions, [&](const auto &row) {
                        return row.name == name && row.zone_id == 5 &&
                               row.zone_name == cMysteriousZone &&
                               row.layer_name == layer_name &&
                               row.jmap_entry_index == source_row;
                    }) == 1,
                    std::string(name) + " exact GeneralPos row is ambiguous");
            for (auto axis = std::size_t{}; axis < expected_local.size(); ++axis) {
                require_near(found->local_position[axis], expected_local[axis], 0.001F,
                             std::string(name) + " local position differs from RMGK01");
            }
            return *found;
        }

        [[nodiscard]] TVec3f world_position(const StagePlacementObject &placement) {
            return TVec3f{placement.translation[0U], placement.translation[1U],
                          placement.translation[2U]};
        }

        class GatewaySpinRouteTico final : public LiveActor {
        public:
            explicit GatewaySpinRouteTico(const StagePlacementObject &source)
                : LiveActor("チコ") {
                mPosition.set(source.translation[0U], source.translation[1U],
                              source.translation[2U]);
                mRotation.set(source.rotation[0U], source.rotation[1U],
                              source.rotation[2U]);
            }
        };

        class GatewaySpinRosettaTrigger final : public LiveActor {
        public:
            explicit GatewaySpinRosettaTrigger(const StagePlacementObject &source)
                : LiveActor("ロゼッタ") {
                mPosition.set(source.translation[0U], source.translation[1U],
                              source.translation[2U]);
                mRotation.set(source.rotation[0U], source.rotation[1U],
                              source.rotation[2U]);
            }
        };

    }  // namespace

    class GatewaySpinCheckpoint::Impl final {
    public:
        Impl(smgpc::runtime::DvdFileSystemService &,
             std::span<const StagePlacementObject> placements,
             std::span<const StageGeneralPos> general_positions,
             GameDataHolder &game_data,
             smgpc::runtime::PlayerSystemService &player,
             smgpc::runtime::WipeService &wipe, LiveActor &mario)
            : _player(player), _wipe(wipe), _mario(mario), _game_data(game_data),
              _rosetta_source(require_unique_placement(
                  placements, "Rosetta", cLayerAObjInfo, 12)),
              _tico_source(require_unique_placement(
                  placements, "TicoBaby", cLayerAObjInfo, 13)),
              _step_during_source(require_unique_placement(
                  placements, "HeavensDoorAppearStepA", cLayerAObjInfo, 14)),
              _step_after_source(require_unique_placement(
                  placements, "HeavensDoorAppearStepAAfter", cCommonObjInfo, 48)),
              _mario_pos4(require_unique_position(
                  general_positions, cMarioDemoPos4, "layera", 0,
                  {-140.0F, 2500.0F, -700.0F})),
              _player_binding(
                  std::make_unique<smgpc::compat::ScopedPlayerSystemServiceOverride>(
                      _player)),
              _information_message_binding(
                  std::make_unique<smgpc::compat::InformationMessageBinding>()),
              // GameScene owns one DemoDirector for the complete scene. The
              // bounded checkpoint joins that owner instead of installing a
              // second active runtime after Gateway actors and Mario exist.
              _demo(&smgpc::compat::require_active_demo_scene_runtime(
                  "Gateway spin checkpoint")),
              _tico(std::make_unique<GatewaySpinRouteTico>(_tico_source)),
              _rosetta(std::make_unique<GatewaySpinRosettaTrigger>(
                  _rosetta_source)) {
            const auto *information_observer = dynamic_cast<InformationObserver *>(
                MR::createSceneObj(SceneObj_InformationObserver));
            require(information_observer != nullptr,
                    "exact InformationObserver SceneObj could not be created eagerly");
            validate_route_data();

            require(GameDataFunction::getCurrentGameDataHolder() == &_game_data &&
                        GameDataFunction::getSceneStartGameDataHolder() == &_game_data &&
                        smgpc::compat::game_data::holder_story_progress(_game_data) == 10U &&
                        GameDataFunction::isPassedStoryEvent("チコガイドデモ終了") &&
                        !GameDataFunction::isPassedStoryEvent("スピン権利"),
                    "checkpoint must borrow the active selected-file holder at retail story progress 10");

            require(_player.attached_actor() == &_mario,
                    "checkpoint requires Mario to be pre-attached with its real entitlement bridge");

            const auto tico_iter = JMapInfoIter(
                &_tico_source.jmap_info, _tico_source.jmap_entry_index);
            require(_demo->try_register_cast(_tico.get(), tico_iter),
                    "typed チコ route cast could not register from TicoBaby row 13");
            const auto guide_index = _demo->find_definition(5, 0);
            require(guide_index.has_value() &&
                        _demo->membership_count(_tico.get()) == 1U &&
                        _demo->cast_name(_tico.get(), *guide_index) == "チコ" &&
                        _demo->cast_id(_tico.get(), *guide_index) == 0,
                    "typed チコ route cast lost its exact zone/link/CastId membership");
        }

        ~Impl() {
            _information_message_binding.reset();
            _player_binding.reset();
        }

        void validate_route_data() const {
            const auto guide_index = _demo->find_definition(5, 0);
            require(guide_index.has_value(),
                    "zone-5/link-0 guide DemoGroup is absent");
            const auto *guide = _demo->definition(*guide_index);
            require(guide != nullptr && guide->demo_name == cGuideDemoName &&
                        guide->time_sheet_name == cGuideSheetName &&
                        guide->source_table_path ==
                            "jmp/placement/layera/demoobjinfo" &&
                        guide->source_row == 0,
                    "zone-5/link-0 DemoGroup differs from TicoGuideDemo");

            require(_rosetta_source.l_id == 28 &&
                        _rosetta_source.demo_group_id == 0 &&
                        _rosetta_source.cast_id == -1 &&
                        _rosetta_source.switch_dead_id == 1016 &&
                        _rosetta_source.switch_a_id == 1015 &&
                        _rosetta_source.switch_b_id == 90,
                    "Rosetta row 12 route switches differ from RMGK01");
            require(_tico_source.l_id == 45 &&
                        _tico_source.demo_group_id == 0 &&
                        _tico_source.cast_id == 0,
                    "TicoBaby row 13 route identity differs from RMGK01");
            require(_step_during_source.l_id == 82 &&
                        _step_during_source.switch_appear_id == 1015 &&
                        _step_after_source.l_id == 83,
                    "post-high-tower StepA-to-After placement pair differs from RMGK01");

            constexpr auto spin_parts = std::array{
                std::pair{std::string_view{"スピンゲット[デモ1]"}, 420},
                std::pair{std::string_view{"スピンゲット[会話1]"}, 120},
                std::pair{std::string_view{"スピンゲット[デモ2]"}, 150},
                std::pair{std::string_view{"スピンゲット[会話2]"}, 120},
                std::pair{std::string_view{"スピンゲット[デモ3]"}, 240},
                std::pair{std::string_view{"スピンゲット[会話3]"}, 120},
                std::pair{std::string_view{"スピンゲット[デモ4]"}, 500},
                std::pair{std::string_view{"スピンゲット[デモ5]"}, 60},
            };
            require(guide->sheet.time_rows().size() == 27U,
                    "TicoGuideDemo must retain all 27 retail Time rows");
            auto pre_prompt_ticks = std::uint32_t{};
            for (auto offset = std::size_t{}; offset < spin_parts.size(); ++offset) {
                const auto &row = guide->sheet.time_rows()[15U + offset];
                require(row.part_name == spin_parts[offset].first &&
                            row.total_step == spin_parts[offset].second &&
                            !row.suspend,
                        "spin checkpoint Time rows differ from RMGK01");
                if (offset + 1U < spin_parts.size()) {
                    pre_prompt_ticks += static_cast<std::uint32_t>(row.total_step);
                }
            }
            require(pre_prompt_ticks == cExpectedPrePromptTicks,
                    "spin prompt prelude is not exactly 1670 demo ticks");

            require(std::ranges::any_of(guide->sheet.action_rows(), [](const auto &row) {
                        return row.part_name == cSpinPartFirst &&
                               row.cast_name == "ロゼッタ" && row.cast_id == -1 &&
                               row.action_type == 2 &&
                               row.position_name == cMarioDemoPos4;
                    }),
                    "part 15 Rosetta action row differs from RMGK01");
            require(std::ranges::any_of(guide->sheet.action_rows(), [](const auto &row) {
                        return row.part_name == cSpinPartFirst &&
                               row.cast_name == "ベビチコ" && row.cast_id == 0 &&
                               row.action_type == 2 &&
                               row.position_name == cMarioDemoPos4;
                    }),
                    "part 15 TicoBaby action row differs from RMGK01");
            require(std::ranges::any_of(guide->sheet.player_rows(), [](const auto &row) {
                        return row.part_name == cSpinPartFirst &&
                               row.position_name == cMarioDemoPos4 &&
                               row.bck_name.empty();
                    }),
                    "part 15 MarioDemoPos4 Player row differs from RMGK01");
            require(std::ranges::count_if(
                        guide->sheet.wipe_rows(), [](const auto &row) {
                            return row.part_name == cSpinPartFirst &&
                                   row.wipe_name == cFadeWipeName &&
                                   row.wipe_type == 0 && row.wipe_frame == -1;
                        }) == 1,
                    "part 15 fade-open Wipe row differs from RMGK01");
            require(guide->sheet.sound_rows().empty() &&
                        std::ranges::none_of(
                            guide->sheet.camera_rows(), [](const auto &row) {
                                return row.part_name == cSpinPartFirst ||
                                       row.part_name == cSpinPromptPart ||
                                       row.part_name.starts_with("スピンゲット[");
                            }),
                    "spin checkpoint must not invent Camera or Sound rows");
        }

        void movement() {
            switch (_state) {
            case GatewaySpinCheckpointState::AwaitingRosetta:
                try_begin_fade();
                break;
            case GatewaySpinCheckpointState::FadeHandoff:
                update_fade();
                break;
            case GatewaySpinCheckpointState::SpinDemo:
                observe_spin_demo();
                break;
            case GatewaySpinCheckpointState::PromptDelegated:
                break;
            }
        }

        void try_begin_fade() {
            if (_demo->is_time_keep_active()) {
                return;
            }
            const auto displacement = _mario.mPosition - world_position(_rosetta_source);
            if (displacement.squared() >
                cRosettaTriggerRadius * cRosettaTriggerRadius) {
                return;
            }
            _player.disable_control();
            _wipe.close(cFadeWipeName, cFadeWipeFrames);
            _state = GatewaySpinCheckpointState::FadeHandoff;
        }

        void update_fade() {
            ++_evidence.fade_handoff_frames;
            if (_evidence.fade_handoff_frames < cFadeDurationFrames) {
                return;
            }
            require(_evidence.fade_handoff_frames == cFadeDurationFrames,
                    "fade handoff exceeded the retail 90-frame wait");
            _player.enable_control(true);
            const auto result = _demo->start_demo(
                _rosetta.get(), cGuideDemoName, cSpinPartFirst,
                smgpc::compat::DemoPlayerMode::MarioPuppetable);
            require(result.has_value() &&
                        *result == smgpc::compat::DemoSheetStartResult::Started &&
                        _demo->is_active(cGuideDemoName) &&
                        _demo->part_step(cSpinPartFirst) == -1,
                    "90-frame handoff could not start exact part 15 MarioPuppetable");
            _state = GatewaySpinCheckpointState::SpinDemo;
        }

        void observe_spin_demo() {
            require(_demo->is_active(cGuideDemoName),
                    "TicoGuideDemo ended before the spin prompt handoff");
            if (_demo->is_part_active(cSpinPromptPart) &&
                _demo->part_step(cSpinPromptPart) == 0) {
                require(_evidence.pre_prompt_demo_ticks == cExpectedPrePromptTicks,
                        "spin prompt did not begin after exactly 1670 prior demo ticks");
                ++_evidence.prompt_delegate_calls;
                require(_evidence.prompt_delegate_calls == 1U,
                        "spin explanation provider was invoked more than once");
                MR::explainEnableToSpin(_tico.get());
                _state = GatewaySpinCheckpointState::PromptDelegated;
                return;
            }

            if (_demo->is_part_active(cSpinPartFirst) &&
                _demo->part_step(cSpinPartFirst) == 0) {
                require(vec_near(_mario.mPosition, _mario_pos4.world_position,
                                 0.001F),
                        "real Player row did not dispatch Mario to MarioDemoPos4");
                _evidence.player_row_dispatched_to_mario_demo_pos4 = true;
            }
            ++_evidence.pre_prompt_demo_ticks;
            require(_evidence.pre_prompt_demo_ticks <= cExpectedPrePromptTicks,
                    "spin checkpoint passed the authored prompt boundary");
        }

        smgpc::runtime::PlayerSystemService &_player;
        smgpc::runtime::WipeService &_wipe;
        LiveActor &_mario;
        GameDataHolder &_game_data;
        const StagePlacementObject &_rosetta_source;
        const StagePlacementObject &_tico_source;
        const StagePlacementObject &_step_during_source;
        const StagePlacementObject &_step_after_source;
        const StageGeneralPos &_mario_pos4;
        std::unique_ptr<smgpc::compat::ScopedPlayerSystemServiceOverride>
            _player_binding;
        std::unique_ptr<smgpc::compat::InformationMessageBinding>
            _information_message_binding;
        smgpc::compat::DemoSceneRuntime *_demo;
        std::unique_ptr<GatewaySpinRouteTico> _tico;
        std::unique_ptr<GatewaySpinRosettaTrigger> _rosetta;
        GatewaySpinCheckpointState _state =
            GatewaySpinCheckpointState::AwaitingRosetta;
        GatewaySpinCheckpointEvidence _evidence{};
    };

    GatewaySpinCheckpoint::GatewaySpinCheckpoint(
        smgpc::runtime::DvdFileSystemService &dvd,
        std::span<const StagePlacementObject> placements,
        std::span<const StageGeneralPos> general_positions,
        GameDataHolder &game_data,
        smgpc::runtime::PlayerSystemService &player,
        smgpc::runtime::WipeService &wipe, LiveActor &mario)
        : NameObj("GatewaySpinCheckpoint"),
          _impl(std::make_unique<Impl>(
              dvd, placements, general_positions, game_data, player, wipe, mario)) {
        MR::connectToScene(this, MR::MovementType_NPC, -1, -1, -1);
    }

    GatewaySpinCheckpoint::~GatewaySpinCheckpoint() {
        MR::disconnectToScene(this);
    }

    void GatewaySpinCheckpoint::movement() {
        _impl->movement();
    }

    GatewaySpinCheckpointState GatewaySpinCheckpoint::state() const {
        return _impl->_state;
    }

    const GatewaySpinCheckpointEvidence &GatewaySpinCheckpoint::evidence() const {
        return _impl->_evidence;
    }

    const LiveActor &GatewaySpinCheckpoint::tico_cast() const {
        return *_impl->_tico;
    }

    LiveActor &GatewaySpinCheckpoint::tico_cast() {
        return *_impl->_tico;
    }

    const smgpc::compat::DemoSceneRuntime &
    GatewaySpinCheckpoint::demo_runtime() const {
        return *_impl->_demo;
    }

    smgpc::compat::DemoSceneRuntime &GatewaySpinCheckpoint::demo_runtime() {
        return *_impl->_demo;
    }

    const GameDataHolder &GatewaySpinCheckpoint::checkpoint_game_data() const {
        return _impl->_game_data;
    }

}  // namespace smgpc::scene
