#include "TestSuites.hpp"
#include "TestSupport.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/Screen/PictureBookLayout.hpp"
#include "Game/Screen/ProloguePictureBook.hpp"
#include "Game/compat/SceneLifecycleService.hpp"
#include "Game/compat/SequenceBootService.hpp"
#include "Game/compat/StagePlacementResolver.hpp"
#include "Application.hpp"
#include "Sqlite.hpp"
#include "TraceAnalysis.hpp"
#include "TraceStore.hpp"

#include <cstdlib>
#include <memory>

namespace smgpc::tests {
    namespace {
        constexpr auto TEST_SUITE = std::string_view{"runtime/scene"};
        constexpr auto cSourceTitleStarPointerMode = smgpc::game::StarPointerMode::ScreenMenu;
        constexpr auto cSourceTargetSelectionStarPointerMode = smgpc::game::StarPointerMode::TargetSelection;
        constexpr auto cSourceSystemModalStarPointerMode = smgpc::game::StarPointerMode::SystemModal;
        constexpr auto cSourceDocumentViewerStarPointerMode = smgpc::game::StarPointerMode::DocumentViewer;
        constexpr auto cSourceFileSelectGuidanceRequest = smgpc::game::StarPointerGuidanceRequest::Primary;
        constexpr auto cSourceFileSelectCopyGuidanceRequest = smgpc::game::StarPointerGuidanceRequest::Secondary;

        template <int Line>
        struct TestCase;

        [[nodiscard]] bool simple_layout_has_texture(SimpleLayout *layout, std::string_view texture_name) {
            if (layout == nullptr) {
                return false;
            }

            const auto materials = layout->debugMaterials();
            return std::ranges::any_of(materials, [texture_name](const auto &material) {
                return std::ranges::any_of(material.textures, [texture_name](const auto &texture) {
                    return texture.texture_name == texture_name;
                });
            });
        }

        class ScopedEnvironmentVariable {
        public:
            ScopedEnvironmentVariable(const char *name, const char *value) : _name(name) {
                if (const auto *previous = std::getenv(name); previous != nullptr) {
                    _previous = previous;
                }
                setenv(name, value, 1);
            }

            ScopedEnvironmentVariable(const ScopedEnvironmentVariable &) = delete;
            ScopedEnvironmentVariable &operator=(const ScopedEnvironmentVariable &) = delete;

            ~ScopedEnvironmentVariable() {
                if (_previous.has_value()) {
                    setenv(_name, _previous->c_str(), 1);
                } else {
                    unsetenv(_name);
                }
            }

        private:
            const char *_name = nullptr;
            std::optional<std::string> _previous{};
        };

        struct SequenceServiceFixture {
            smgpc::game::GameSystemSceneControllerService scene_controller;
            smgpc::game::StorySequenceService story_sequence;
            smgpc::game::StageHostService stage_host;
            smgpc::game::SequenceBootService sequence_boot;

            explicit SequenceServiceFixture(smgpc::game::RuntimeContext &runtime)
                : scene_controller(runtime, runtime.scene_lifecycle()), story_sequence(runtime), stage_host(scene_controller),
                  sequence_boot(runtime, story_sequence, stage_host) {
            }
        };

        [[nodiscard]] bool has_effect_event(const smgpc::game::RuntimeContext &runtime, smgpc::game::EffectEventKind kind,
                                            std::string_view actor_name, std::string_view effect_name) {
            return std::ranges::any_of(runtime.effects().events(), [&](const smgpc::game::EffectEvent &event) {
                return event.kind == kind && event.actor_name == actor_name && event.effect_name == effect_name;
            });
        }

        [[nodiscard]] bool has_active_effect(const smgpc::game::RuntimeContext &runtime, std::string_view actor_name,
                                             std::string_view effect_name) {
            const auto effects = runtime.effects().active_effects(actor_name);
            return std::ranges::any_of(effects, [&](const auto &active_effect) { return active_effect == effect_name; });
        }

        [[nodiscard]] bool has_system_sound_event_prefix(const smgpc::game::RuntimeContext &runtime, std::string_view prefix) {
            return std::ranges::any_of(runtime.audio().events(), [&](const smgpc::game::AudioEvent &event) {
                return (event.kind == smgpc::game::AudioEventKind::SystemSoundStart ||
                        event.kind == smgpc::game::AudioEventKind::SystemLevelSoundStart ||
                        event.kind == smgpc::game::AudioEventKind::AtmosphereSoundStart ||
                        event.kind == smgpc::game::AudioEventKind::SystemMEStart) &&
                       event.name.starts_with(prefix);
            });
        }

        [[nodiscard]] bool has_semantic_trace_event(const smgpc::dump::Json &events, std::string_view category, std::string_view name) {
            const auto category_text = std::string(category);
            const auto name_text = std::string(name);
            return std::ranges::any_of(events, [&](const auto &event) {
                return event.at("category") == category_text && event.at("name") == name_text;
            });
        }

        [[nodiscard]] bool has_semantic_trace_event_detail_containing(const smgpc::dump::Json &events, std::string_view category,
                                                                      std::string_view name, std::string_view required_detail) {
            const auto category_text = std::string(category);
            const auto name_text = std::string(name);
            return std::ranges::any_of(events, [&](const auto &event) {
                if (event.at("category") != category_text || event.at("name") != name_text) {
                    return false;
                }

                return event.at("detail").template get<std::string>().find(required_detail) != std::string::npos;
            });
        }

        [[nodiscard]] bool json_string_array_contains(const smgpc::dump::Json &values, std::string_view expected) {
            const auto expected_text = std::string(expected);
            return values.is_array() && std::ranges::any_of(values, [&](const auto &value) {
                return value.is_string() && value.template get<std::string>() == expected_text;
            });
        }

        [[nodiscard]] std::int64_t sqlite_semantic_event_count(smgpc::sql::Database &db, std::string_view category, std::string_view name) {
            auto select = smgpc::sql::Statement(db, "SELECT count(*) FROM semantic_events WHERE category = ? AND name = ?");
            select.bind(1, category);
            select.bind(2, name);
            require(select.step(), "semantic_events count query should return a row");
            return select.column_int(0).value_or(0);
        }

        [[nodiscard]] const smgpc::trace::SemanticAnchorAlignment *find_semantic_anchor_alignment(
            const std::vector<smgpc::trace::SemanticAnchorAlignment> &anchors, std::string_view category, std::string_view name) {
            for (const auto &anchor : anchors) {
                if (anchor.category == category && anchor.name == name) {
                    return &anchor;
                }
            }
            return nullptr;
        }

        [[nodiscard]] const smgpc::trace::LayoutRuntimeDiff *find_layout_runtime_diff(
            const std::vector<smgpc::trace::LayoutRuntimeDiff> &diffs, std::string_view name, std::string_view layout_name) {
            for (const auto &diff : diffs) {
                if (diff.name == name && diff.layout_name == layout_name) {
                    return &diff;
                }
            }
            return nullptr;
        }

        [[nodiscard]] bool has_file_number_animation(const smgpc::game::RuntimeContext &runtime, std::size_t layer_index,
                                                     std::string_view animation_name) {
            const auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            return std::ranges::any_of(layouts, [&](const auto &layout) {
                return layout.name == "ファイル番号" &&
                       std::ranges::any_of(layout.animations, [&](const auto &animation) {
                           return animation.layer_index == layer_index && animation.name == animation_name;
                       });
            });
        }

        [[nodiscard]] bool has_live_actor_btk_action(const smgpc::game::RuntimeContext &runtime, std::string_view actor_name,
                                                     std::string_view action_name) {
            const auto snapshot = runtime.scheduler().snapshot();
            return std::ranges::any_of(snapshot, [&](const auto &entry) {
                return entry.name == actor_name && !entry.dead && entry.has_live_actor_state && entry.live_actor_btk_name == action_name;
            });
        }

        [[nodiscard]] TVec2f screen_center_for_pane_bounds(const SimpleLayout::PaneBounds &bounds) {
            return TVec2f{
                .x = ((bounds.left + bounds.right) * 0.5F) + (static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F),
                .y = ((bounds.top + bounds.bottom) * 0.5F) + (static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F),
            };
        }

        [[nodiscard]] TVec2f project_world_to_screen(const smgpc::game::CameraPoseCompat &pose, const smgpc::game::CameraParamVec3 &world) {
            constexpr auto PI = 3.14159265358979323846F;
            const auto camera = smgpc::game::transform_world_to_camera(pose, world);
            const auto focal_y = 1.0F / std::tan((pose.fovy_degrees * PI / 180.0F) * 0.5F);
            const auto focal_x = focal_y / pose.aspect_ratio;
            const auto half_width = static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F;
            const auto half_height = static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F;
            return TVec2f{
                .x = ((camera.x / camera.z) * focal_x * half_width) + half_width,
                .y = ((camera.y / camera.z) * focal_y * half_height) + half_height,
            };
        }

        [[nodiscard]] std::vector<std::uint8_t> read_binary_file_for_test(const std::filesystem::path &path) {
            auto stream = std::ifstream(path, std::ios::binary);
            require(stream.good(), "test binary file should open");
            return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
        }

        [[nodiscard]] std::uint32_t read_be_u32(std::span<const std::uint8_t> bytes, std::size_t offset) {
            require(offset + sizeof(std::uint32_t) <= bytes.size(), "test big-endian u32 read should be in bounds");
            return (static_cast<std::uint32_t>(bytes[offset]) << 24U) | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) | bytes[offset + 3U];
        }

        [[nodiscard]] std::uint32_t read_le_u32(std::span<const std::uint8_t> bytes, std::size_t offset) {
            require(offset + sizeof(std::uint32_t) <= bytes.size(), "test little-endian u32 read should be in bounds");
            return (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U) | (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) | bytes[offset];
        }

        [[nodiscard]] TVec2f screen_center_for_layout_pane(SimpleLayout &layout, std::string_view pane_name, std::string_view message) {
            const auto bounds = layout.paneBounds(pane_name);
            require(bounds.has_value(), message);
            return screen_center_for_pane_bounds(*bounds);
        }

        [[nodiscard]] TVec2f screen_center_for_layout_pane(const char *layout_name, const char *pane_name, std::string_view message) {
            auto layout = SimpleLayout("SMGPC test pointer probe", layout_name, 1U, MR::DrawType_Layout);
            layout.initWithoutIter();
            layout.appear();
            return screen_center_for_layout_pane(layout, pane_name, message);
        }

        void set_pointer_to_sys_info_yes(TestWindowService &window) {
            const auto point = screen_center_for_layout_pane("SysInfoWindow", "BoxRight", "SysInfoWindow BoxRight should expose hit-test bounds");
            window.set_pointer(point.x, point.y, true);
        }

        $test("draws FileSelectItem through scheduler-owned parts model") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            auto item = FileSelectItem(1, true);

            item.initWithoutIter();
            const auto init_snapshot = runtime.scheduler().snapshot();
            require(std::ranges::any_of(init_snapshot,
                                        [](const auto &entry) {
                                            return entry.kind == smgpc::game::SceneEntryKind::LayoutActor &&
                                                   entry.name == "ファイル番号" && entry.movement_type == MR::MovementType_Layout &&
                                                   entry.calc_anim_type == MR::CalcAnimType_Layout &&
                                                   entry.draw_type == MR::DrawType_Layout && entry.dead;
                                        }),
                    "FileSelectItem should own an original-style dead FileSelectNumber layout child after init");
            const auto init_layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            require(std::ranges::any_of(init_layouts,
                                        [](const auto &entry) {
                                            return entry.name == "ファイル番号" && entry.layout_name == "FileNumber" &&
                                                   entry.movement_type == MR::MovementType_Layout && entry.draw_type == MR::DrawType_Layout &&
                                                   entry.dead;
                                        }),
                    "FileSelectItem number child should resolve the original FileNumber layout archive through LayoutActor");
            item.setBasePosition({0.0F, 800.0F, 0.0F});
            item.appear();
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            const auto movement_trace = runtime.scheduler().last_execution_trace();
            require(std::ranges::any_of(movement_trace,
                                        [](const auto &entry) {
                                            return entry.kind == smgpc::game::SceneEntryKind::NameObj &&
                                                   entry.name == "ファイルセレクトアイテム" &&
                                                   entry.movement_type == MR::MovementType_MapObj &&
                                                   entry.phase == smgpc::game::SceneSchedulerPhase::Movement;
                                        }),
                    "FileSelectItem parent should move as an original-style movement-only MapObj entry");

            runtime.draw_3d_normal(renderer, far_test_camera_pose());
            const auto draw_trace = runtime.scheduler().last_execution_trace();

            require(renderer.texture_count > 0U, "FileSelectItem should load original FileSelectDataPlanet textures when drawn");
            require(renderer.triangle_batch_count > 0U || renderer.gx_material_batch_count > 0U,
                    "FileSelectItem should draw the original FileSelectDataPlanet model");
            require(renderer.submitted_indices > 0U, "FileSelectItem should submit original FileSelectDataPlanet geometry");
            require(std::ranges::any_of(draw_trace,
                                        [](const auto &entry) {
                                            return entry.kind == smgpc::game::SceneEntryKind::LiveActorModel &&
                                                   entry.name == "ニューフェイス" && entry.draw_buffer_type == MR::DrawBufferType_MapObj &&
                                                   entry.phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa;
                                        }),
                    "FileSelectItem child model should be drawn by the central MapObj draw buffer");
        }

        $test("turns FileSelectItem to front with original easing") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto item = FileSelectItem(1, false);

            item.initWithoutIter();
            item.appear();
            item.validateRotate();
            item.mRotation.y = 270.0F;
            item.turnToFront(4);

            item.movement();
            require_near(item.mRotation.y, -84.375F, 0.001F, "FileSelectItem turnToFront should wrap to the shortest front-facing yaw");
            item.movement();
            require_near(item.mRotation.y, -63.28125F, 0.001F, "FileSelectItem turnToFront should use the original squared easing");
            item.movement();
            require_near(item.mRotation.y, -27.685547F, 0.001F, "FileSelectItem turnToFront should continue easing the current yaw");
            item.movement();
            require_near(item.mRotation.y, 0.0F, 0.001F, "FileSelectItem turnToFront should end front-facing");

            item.movement();
            require_near(item.mRotation.y, 0.5F, 0.001F, "FileSelectItem rotation should resume the original idle minimum speed");
        }

        $test("drives original FileSelectItem blink controller for fellow models") {
            auto make_frame = [](std::uint64_t frame) {
                return smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
            };
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto item = FileSelectItem(1, false);

            item.initWithoutIter();
            item.appear();

            for (auto frame = 0U; frame < 5U && !has_live_actor_btk_action(runtime, "キャラフェイス", "normal"); ++frame) {
                runtime.begin_frame(make_frame(frame));
            }
            require(has_live_actor_btk_action(runtime, "キャラフェイス", "normal"),
                    "FileSelectModel should start in the original normal action before blink controller transitions");

            auto saw_blink = false;
            for (auto frame = 5U; frame < 420U && !saw_blink; ++frame) {
                runtime.begin_frame(make_frame(frame));
                saw_blink = has_live_actor_btk_action(runtime, "キャラフェイス", "blink");
            }
            require(saw_blink, "FileSelectItem BlinkController should eventually drive the original fellow blinkOnce action");

            auto returned_normal = false;
            for (auto frame = 420U; frame < 450U && !returned_normal; ++frame) {
                runtime.begin_frame(make_frame(frame));
                returned_normal = has_live_actor_btk_action(runtime, "キャラフェイス", "normal");
            }
            require(returned_normal, "FileSelectItem BlinkController should return the fellow model to the original open/normal action");
        }

        $test("drives original FileSelectItem pointing side effects") {
            auto make_frame = [](std::uint64_t frame) {
                return smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
            };

            {
                auto logger = NullLogger();
                auto window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(logger, window);
                auto item = FileSelectItem(1, false);

                item.initWithoutIter();
                item.appear();
                item.appearIndex();
                item.onPointing();

                require(item.isPointing() && item.wasPointed(), "FileSelectItem::onPointing should set the original pointed state");
                require(has_system_sound_event_prefix(runtime, "ME_ASTRO_DOME_HIT_GALAXY"),
                        "existing FileSelectItem pointing should start one of the original pointed ME cues");

                for (auto i = 0; i < 30 && !has_file_number_animation(runtime, 1U, "SelectIn"); ++i) {
                    runtime.begin_frame(make_frame(static_cast<std::uint64_t>(i)));
                }
                require(has_file_number_animation(runtime, 1U, "SelectIn"),
                        "FileSelectItem::onPointing should drive FileSelectNumber SelectIn on layer 1");

                item.clearPointing();
                require(!item.isPointing() && item.wasPointingCleared(), "FileSelectItem::clearPointing should run the original off-pointing path");

                for (auto i = 30; i < 210 && !has_file_number_animation(runtime, 1U, "SelectOut"); ++i) {
                    runtime.begin_frame(make_frame(static_cast<std::uint64_t>(i)));
                }
                require(has_file_number_animation(runtime, 1U, "SelectOut"),
                        "FileSelectItem::offPointing should drive FileSelectNumber SelectOut on layer 1");
            }

            {
                auto logger = NullLogger();
                auto window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(logger, window);
                auto item = FileSelectItem(2, true);

                item.initWithoutIter();
                item.appear();
                item.onPointing();

                require(has_system_sound_event_prefix(runtime, "ME_ASTRO_DOME_HIT_GALAXY_N"),
                        "new FileSelectItem pointing should start one of the original not-using ME cues");
            }
        }

        $test("emits original FileSelectItem morph effects") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto item = FileSelectItem(1, false);

            item.initWithoutIter();
            item.appear();

            auto icon_id = FileSelectIconID();
            icon_id.setFellowID(FileSelectIconID::Mario);
            item.change(icon_id, true);
            for (auto i = 0; i < 45; ++i) {
                item.movement();
            }

            require(has_effect_event(runtime, smgpc::game::EffectEventKind::Delete, "キャラフェイス", "Complete"),
                    "FileSelectItem::change should delete stale Complete effects before morphing");
            require(has_effect_event(runtime, smgpc::game::EffectEventKind::Emit, "キャラフェイス", "Complete"),
                    "FileSelectItem::exeChangeFellow should emit the original Complete effect for all-complete file icons");
            require(has_effect_event(runtime, smgpc::game::EffectEventKind::Emit, "キャラフェイス", "Open"),
                    "FileSelectItem::exeChangeFellow should emit the original Open effect on the appeared fellow model");

            item.format();
            for (auto i = 0; i < 65; ++i) {
                item.movement();
            }

            require(has_effect_event(runtime, smgpc::game::EffectEventKind::Emit, "キャラフェイス", "Vanish"),
                    "FileSelectItem::exeFormat should emit the original Vanish effect before killing fellow models");
            require(!has_active_effect(runtime, "キャラフェイス", "Complete"),
                    "FileSelectItem::format should remove Complete effects before returning to the new-file planet");
        }

        $test("drives FileSelectNumber through LayoutActor compatibility") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            auto number = FileSelectNumber("ファイル番号");

            number.initWithoutIter();
            number.setNumber(4);
            number.appear();

            auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            const auto layout_it = std::ranges::find_if(layouts, [](const auto &entry) {
                return entry.name == "ファイル番号" && entry.layout_name == "FileNumber" && !entry.dead;
            });
            require(layout_it != layouts.end(), "FileSelectNumber should register as a live LayoutActor-backed FileNumber layout");
            require(layout_it->animations.size() >= 2U && layout_it->animations[0U].name == "Appear" &&
                        layout_it->animations[1U].name == "SelectOut",
                    "FileSelectNumber::appear should start original layer 0 Appear and layer 1 SelectOut animations");
            require(layout_it->pane_count > 0U && layout_it->text_box_count > 0U && layout_it->material_count > 0U,
                    "LayoutActor runtime debug state should expose parsed BRLYT pane, text, and material counts");

            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            const auto movement_trace = runtime.scheduler().last_execution_trace();
            require(std::ranges::any_of(movement_trace,
                                        [](const auto &entry) {
                                            return entry.kind == smgpc::game::SceneEntryKind::LayoutActor &&
                                                   entry.name == "ファイル番号" &&
                                                   entry.phase == smgpc::game::SceneSchedulerPhase::Movement;
                                        }),
                    "FileSelectNumber should run movement/control through the scene scheduler");

            runtime.draw_2d_normal(renderer);
            const auto draw_trace = runtime.scheduler().last_execution_trace();
            require(renderer.texture_count > 0U, "FileSelectNumber should upload original FileNumber layout/font textures");
            require(renderer.quad_count > 0U || renderer.gx_material_batch_count > 0U,
                    "FileSelectNumber should draw through the central 2D draw list");
            require(std::ranges::any_of(draw_trace,
                                        [](const auto &entry) {
                                            return entry.kind == smgpc::game::SceneEntryKind::LayoutActor &&
                                                   entry.name == "ファイル番号" && entry.draw_type == MR::DrawType_Layout &&
                                                   entry.phase == smgpc::game::SceneSchedulerPhase::DrawType;
                                        }),
                    "FileSelectNumber should be drawn by the Layout draw type");
        }

        $test("supports root-style layout screen positions and pane matrix refs") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto number = FileSelectNumber("ファイル番号");

            number.initWithoutIter();
            number.setNumber(4);
            number.appear();

            runtime.set_scene_camera_pose(smgpc::game::CameraPoseCompat{
                .eye = {.x = 0.0F, .y = 0.0F, .z = 0.0F},
                .watch = {.x = 0.0F, .y = 0.0F, .z = 1.0F},
                .up = {.x = 0.0F, .y = 1.0F, .z = 0.0F},
                .fovy_degrees = 90.0F,
                .aspect_ratio = static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferWidth) /
                                static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferHeight),
                .near_clip = 1.0F,
                .far_clip = 1000.0F,
            });

            auto screen_pos = TVec2f{};
            require(MR::calcScreenPosition(&screen_pos, TVec3f(0.0F, 0.0F, 10.0F)),
                    "MR::calcScreenPosition should project visible world points through the active scene camera");
            require_near(screen_pos.x, static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F, 0.001F,
                         "centered world point should project to framebuffer center X");
            require_near(screen_pos.y, static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F, 0.001F,
                         "centered world point should project to framebuffer center Y");

            auto unprojected_center = TVec3f{};
            require(MR::calcWorldPositionFromScreen(&unprojected_center, screen_pos, 10.0F),
                    "MR::calcWorldPositionFromScreen should unproject through the active scene camera");
            require_near(unprojected_center.x, 0.0F, 0.001F, "centered screen point should unproject to camera-forward world X");
            require_near(unprojected_center.y, 0.0F, 0.001F, "centered screen point should unproject to camera-forward world Y");
            require_near(unprojected_center.z, 10.0F, 0.001F, "centered screen point should unproject to requested camera depth");

            auto projected_offset = TVec2f{};
            require(MR::calcScreenPosition(&projected_offset, TVec3f(10.0F, 5.0F, 20.0F)),
                    "MR::calcScreenPosition should project off-center points through the active scene camera");
            auto unprojected_offset = TVec3f{};
            require(MR::calcWorldPositionFromScreen(&unprojected_offset, projected_offset, 20.0F),
                    "MR::calcWorldPositionFromScreen should round-trip off-center projected points");
            require_near(unprojected_offset.x, 10.0F, 0.001F, "off-center projected point should round-trip world X");
            require_near(unprojected_offset.y, 5.0F, 0.001F, "off-center projected point should round-trip world Y");
            require_near(unprojected_offset.z, 20.0F, 0.001F, "off-center projected point should round-trip world Z");

            const auto cam_pos = MR::getCamPos();
            require_near(cam_pos.x, 0.0F, 0.001F, "MR::getCamPos should expose the active camera eye X");
            require_near(cam_pos.y, 0.0F, 0.001F, "MR::getCamPos should expose the active camera eye Y");
            require_near(cam_pos.z, 0.0F, 0.001F, "MR::getCamPos should expose the active camera eye Z");
            require_near(MR::getFovy(), 90.0F, 0.001F, "MR::getFovy should expose the active camera fovy");
            require_near(MR::getNearZ(), 1.0F, 0.001F, "MR::getNearZ should expose the active camera near clip");
            require_near(MR::getFarZ(), 1000.0F, 0.001F, "MR::getFarZ should expose the active camera far clip");

            auto ray = TVec3f{};
            require(MR::calcWorldRayDirectionFromScreen(&ray, screen_pos),
                    "MR::calcWorldRayDirectionFromScreen should produce a camera-forward ray for centered screen positions");
            require_near(ray.x, 0.0F, 0.001F, "centered screen ray should face camera-forward X");
            require_near(ray.y, 0.0F, 0.001F, "centered screen ray should face camera-forward Y");
            require(ray.z > 0.0F, "centered screen ray should face camera-forward Z");

            number.setTrans(TVec2f{screen_pos.x + 17.0F, screen_pos.y + 23.0F});
            const auto roundtrip = number.getTrans();
            require_near(roundtrip.x, screen_pos.x + 17.0F, 0.001F, "LayoutActor::setTrans/getTrans should use root-style screen coordinates");
            require_near(roundtrip.y, screen_pos.y + 23.0F, 0.001F, "LayoutActor::setTrans/getTrans should round-trip screen Y");

            number.createPaneMtxRef("FileNumber");
            number.calcAnim();
            const auto *pane_mtx = number.getPaneMtxRef("FileNumber");
            require(pane_mtx != nullptr, "LayoutActor should delegate pane matrix refs to LayoutManager like the root implementation");

            Mtx expected_mtx{};
            require(number.getSimpleLayout() != nullptr && number.getSimpleLayout()->copyPaneMatrix("FileNumber", expected_mtx),
                    "SimpleLayout should expose the generic pane world matrix used by LayoutManager refs");
            require_near(pane_mtx[0][3], expected_mtx[0][3], 0.001F, "pane matrix ref should copy the current pane X translation");
            require_near(pane_mtx[1][3], expected_mtx[1][3], 0.001F, "pane matrix ref should copy the current pane Y translation");
        }

        $test("exposes original-shaped layout pane controls and button controllers") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto number = FileSelectNumber("ファイル番号");

            number.initWithoutIter();
            number.appear();

            MR::createAndAddPaneCtrl(&number, "FileNumber", 1U);
            require(MR::isExistPaneCtrl(&number, "FileNumber"), "MR::createAndAddPaneCtrl should register a named LayoutPaneCtrl");
            MR::hidePane(&number, "FileNumber");
            auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            auto layout_it = std::ranges::find_if(layouts, [](const auto &entry) { return entry.name == "ファイル番号"; });
            require(layout_it != layouts.end(), "FileSelectNumber should expose layout runtime state");
            require(std::ranges::any_of(layout_it->pane_controls,
                                        [](const auto &pane) {
                                            return pane.pane_name == "FileNumber" && pane.exists_in_layout && !pane.visible;
                                        }),
                    "layout runtime state should expose named pane visibility through generic pane controls");

            MR::showPane(&number, "FileNumber");
            MR::startPaneAnim(&number, "FileNumber", "SelectOut", 0U);
            MR::setPaneAnimFrame(&number, "FileNumber", 2.0F, 0U);
            MR::setPaneAnimRate(&number, "FileNumber", 0.5F, 0U);
            require_near(MR::getPaneAnimFrame(&number, "FileNumber", 0U), 2.0F, 0.001F,
                         "MR pane animation helpers should expose the LayoutPaneCtrl frame");
            require(!MR::isPaneAnimStopped(&number, "FileNumber", 0U), "MR pane animation helpers should expose running pane animation state");

            auto button = ButtonPaneController(&number, "FileNumber", "FileNumber", 0U, true);
            require(button.isHidden(), "ButtonPaneController should initialize hidden and create/hide its pane");
            button.appear();
            button.update();

            layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            layout_it = std::ranges::find_if(layouts, [](const auto &entry) { return entry.name == "ファイル番号"; });
            require(layout_it != layouts.end() &&
                        std::ranges::any_of(layout_it->pane_controls,
                                            [](const auto &pane) {
                                                return pane.pane_name == "FileNumber" && pane.visible &&
                                                       !pane.animations.empty() && pane.animations[0U].name == "ButtonAppear";
                                            }),
                    "LayoutPaneCtrl state should reflect ButtonPaneController pane animation startup");
            require(std::ranges::any_of(layout_it->button_controllers,
                                        [](const auto &controller) {
                                            return controller.pane_name == "FileNumber" &&
                                                   controller.bounding_pane_name == "FileNumber" &&
                                                   controller.nerve == "Appear" && controller.active &&
                                                   controller.appearance_enabled && controller.decide_enabled;
                                        }),
                    "layout runtime state should expose ButtonPaneController nerve and behavior flags");

            const auto frame_context = smgpc::render::FrameContext{
                .frame_index = 1900U,
                .frame_time_seconds = 1900.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            };
            const auto trace_path = std::filesystem::path(".cache/tests/runtime-layout-pane-controls-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(trace_path, frame_context, runtime);
            const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
            const auto &layout_entries = trace_json.at("layout_runtime");
            const auto trace_layout = std::ranges::find_if(layout_entries, [](const auto &entry) { return entry.at("name") == "ファイル番号"; });
            require(trace_layout != layout_entries.end() &&
                        std::ranges::any_of(trace_layout->at("pane_controls"),
                                            [](const auto &pane) {
                                                return pane.at("pane_name") == "FileNumber" && pane.at("visible") == true &&
                                                       !pane.at("animations").empty() &&
                                                       pane.at("animations").front().at("name") == "ButtonAppear";
                                            }) &&
                        std::ranges::any_of(trace_layout->at("button_controllers"),
                                            [](const auto &controller) {
                                                return controller.at("pane_name") == "FileNumber" &&
                                                       controller.at("nerve") == "Appear" &&
                                                       controller.at("active") == true;
                                            }),
                    "runtime parity trace should serialize generic pane controls and button-controller state");
        }

        $test("routes recursive pane visibility and pane scale-position transfer through generic layout helpers") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);

            auto pane_probe = LayoutActor("generic pane recursion probe", true);
            pane_probe.initLayoutManager("PictureBook", 1U);
            pane_probe.appear();
            auto* pane_layout = pane_probe.getSimpleLayout();
            require(pane_layout != nullptr, "PictureBook probe should own a SimpleLayout for generic pane recursion");
            require(pane_layout->paneBounds("").has_value(), "PictureBook probe should load generic layout resource data");

            const auto panes = pane_layout->debugPanes();
            auto child_it = std::ranges::find_if(panes, [](const auto& pane) {
                return pane.parent_index >= 0 && !pane.name.empty();
            });
            require(child_it != panes.end(), "PictureBook layout should expose at least one parented pane");
            require(static_cast<std::size_t>(child_it->parent_index) < panes.size(), "parented pane should refer to a valid parent pane");
            const auto parent_name = panes[static_cast<std::size_t>(child_it->parent_index)].name;
            const auto child_name = child_it->name;
            require(!parent_name.empty(), "parent pane should have a name for recursive visibility calls");

            MR::showPaneRecursive(&pane_probe, parent_name.c_str());
            require(pane_layout->isPaneVisible(parent_name) && pane_layout->isPaneVisible(child_name),
                    "showPaneRecursive should make a parent pane and its descendants visible");
            MR::hidePane(&pane_probe, child_name.c_str());
            require(!pane_layout->isPaneVisible(child_name), "non-recursive hidePane should still be able to hide an individual child pane");
            MR::showPane(&pane_probe, parent_name.c_str());
            require(!pane_layout->isPaneVisible(child_name), "non-recursive showPane should not override a hidden descendant");
            MR::showPaneRecursive(&pane_probe, parent_name.c_str());
            require(pane_layout->isPaneVisible(child_name), "showPaneRecursive should restore hidden descendants");
            MR::hidePaneRecursive(&pane_probe, parent_name.c_str());
            require(!pane_layout->isPaneVisible(parent_name) && !pane_layout->isPaneVisible(child_name),
                    "hidePaneRecursive should hide the parent pane and all descendants");

            auto source = LayoutActor("generic pane scale source", true);
            source.initLayoutManager("TitleLogo", 2U);
            source.appear();
            MR::startAnim(&source, "Appear", 0U);
            MR::setAnimFrame(&source, 0.0F, 0U);
            auto* source_layout = source.getSimpleLayout();
            require(source_layout != nullptr, "TitleLogo source should own a SimpleLayout for pane scale transfer");
            const auto source_scale = source_layout->paneScale("SMGTitleLogo");
            const auto source_bounds = source_layout->paneBounds("SMGTitleLogo");
            require(source_scale.has_value() && source_bounds.has_value(), "source pane should expose native scale and position");
            require(source_scale->x < 0.001F && source_scale->y < 0.001F,
                    "TitleLogo Appear frame 0 should expose the original zero-scale root pane");

            auto dest = LayoutActor("generic pane scale destination", true);
            dest.initLayoutManager("IconAButton", 1U);
            dest.appear();
            MR::setLayoutScalePosAtPaneScaleTrans(&dest, &source, "SMGTitleLogo");

            Mtx dest_root_mtx{};
            require(dest.getSimpleLayout() != nullptr && dest.getSimpleLayout()->copyPaneMatrix("", dest_root_mtx),
                    "destination layout should expose its root matrix after scale transfer");
            require_near(dest_root_mtx[0][0], source_scale->x, 0.001F,
                         "setLayoutScalePosAtPaneScaleTrans should copy source pane X scale to the destination root");
            require_near(dest_root_mtx[1][1], source_scale->y, 0.001F,
                         "setLayoutScalePosAtPaneScaleTrans should copy source pane Y scale to the destination root");

            const auto expected_screen_pos = TVec2f{
                .x = ((source_bounds->left + source_bounds->right) * 0.5F) +
                     (static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F),
                .y = ((source_bounds->top + source_bounds->bottom) * 0.5F) +
                     (static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F),
            };
            const auto dest_screen_pos = dest.getTrans();
            require_near(dest_screen_pos.x, expected_screen_pos.x, 0.001F,
                         "setLayoutScalePosAtPaneScaleTrans should still copy source pane X translation");
            require_near(dest_screen_pos.y, expected_screen_pos.y, 0.001F,
                         "setLayoutScalePosAtPaneScaleTrans should still copy source pane Y translation");
        }

        $test("updates PictureBook page TexMaps with contiguous chapter indices") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);

            auto picture_book = PictureBookLayout(1, 1, true);
            picture_book.init(JMapInfoIter{});
            picture_book.appear();
            picture_book.updateTexMapChapterBase();

            require(picture_book.pageNext(), "PictureBook should advance from chapter title to page spread setup");
            require(picture_book.pageNext(), "PictureBook should advance to the first spread that uses page 1 on the left");
            picture_book.updateTexture();

            auto *layout = picture_book.getSimpleLayout();
            require(simple_layout_has_texture(layout, "chapter1page1.bti"),
                    "PictureBook left page should bind chapter page 1 without a negative TexMap index");
            require(simple_layout_has_texture(layout, "chapter1page2.bti"),
                    "PictureBook right page should bind the native forward spread page");
        }

        $test("routes star-pointer pane checks through generic layout bounds") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto back = BackButton("戻るボタン", false);

            back.initWithoutIter();
            back.appear();
            MR::showPane(&back, "Back");

            auto *layout = back.getSimpleLayout();
            require(layout != nullptr, "BackButton should own a SimpleLayout for pane hit-testing");
            const auto bounds = layout->paneBounds("BoxButton");
            require(bounds.has_value(), "BackButton BoxButton should expose BRLYT pane bounds");

            const auto center = screen_center_for_pane_bounds(*bounds);
            runtime.wpad().set_pointer(WPAD_CHAN0, center.x, center.y, true);
            require(MR::isStarPointerPointingPane(&back, "BoxButton", 0, true, "弱"),
                    "star-pointer pane checks should accept points inside the BRLYT pane bounds");

            runtime.wpad().set_pointer(WPAD_CHAN0,
                                       bounds->right + 400.0F +
                                           (static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferWidth) * 0.5F),
                                       bounds->bottom + 400.0F +
                                           (static_cast<f32>(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F),
                                       true);
            require(!MR::isStarPointerPointingPane(&back, "BoxButton", 0, true, "弱"),
                    "star-pointer pane checks should reject valid pointers outside the BRLYT pane bounds");

            runtime.wpad().set_pointer(WPAD_CHAN0, center.x, center.y, false);
            require(!MR::isStarPointerPointingPane(&back, "BoxButton", 0, true, "弱"),
                    "star-pointer pane checks should still require a valid WPAD pointer");
        }

        $test("routes 3D star-pointer target checks through generic camera projection") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto actor = LiveActor("star pointer target");
            const auto pose = smgpc::game::CameraPoseCompat{
                .eye = {0.0F, 0.0F, 1000.0F},
                .watch = {0.0F, 0.0F, 0.0F},
                .up = {0.0F, 1.0F, 0.0F},
                .fovy_degrees = 45.0F,
                .aspect_ratio = 608.0F / 456.0F,
                .near_clip = 100.0F,
                .far_clip = 800000.0F,
            };

            actor.makeActorAppeared();
            actor.mPosition.set(0.0F, 0.0F, 0.0F);
            MR::initStarPointerTarget(&actor, 120.0F, TVec3f(0.0F, 90.0F, 0.0F));
            require(MR::isExistStarPointerTarget(&actor), "initStarPointerTarget should register a generic LiveActor star-pointer target");
            runtime.set_scene_camera_pose(pose);

            const auto center = project_world_to_screen(pose, {.x = 0.0F, .y = 90.0F, .z = 0.0F});
            const auto actor_name = std::string_view{actor.getName()};
            const auto has_target_event = [&](smgpc::game::StarPointerTargetEventKind kind) {
                return std::ranges::any_of(runtime.star_pointer().target_events(), [&](const auto &event) {
                    return event.kind == kind && event.actor_name == actor_name;
                });
            };
            const auto target_event_count = [&](smgpc::game::StarPointerTargetEventKind kind) {
                return std::ranges::count_if(runtime.star_pointer().target_events(), [&](const auto &event) {
                    return event.kind == kind && event.actor_name == actor_name;
                });
            };
            const auto has_star_pointer_semantic = [&](std::string_view name) {
                return std::ranges::any_of(runtime.semantic_trace_events(), [&](const auto &event) {
                    return event.category == "star_pointer" && event.name == name && event.detail.find("actor=star pointer target") != std::string::npos;
                });
            };

            runtime.wpad().set_pointer(WPAD_CHAN0, center.x, center.y, true);
            require(MR::isStarPointerPointingFileSelect(&actor), "3D star-pointer checks should accept pointers projected onto the actor target");
            require(has_target_event(smgpc::game::StarPointerTargetEventKind::Enter),
                    "generic star-pointer target checks should record target enter events");
            require(has_star_pointer_semantic("target_enter"), "runtime semantic trace should expose generic star-pointer target enter events");

            runtime.wpad().set_button_mask(WPAD_CHAN0, WPAD_BUTTON_A);
            require(MR::isStarPointerPointingFileSelect(&actor), "3D star-pointer target checks should keep accepting the selected target");
            require(target_event_count(smgpc::game::StarPointerTargetEventKind::Select) == 1,
                    "generic star-pointer target checks should record one select event per target and frame");
            require(MR::isStarPointerPointingFileSelect(&actor), "duplicate checks in the same frame should remain accepted");
            require(target_event_count(smgpc::game::StarPointerTargetEventKind::Select) == 1,
                    "generic star-pointer select tracing should avoid duplicate events for repeated checks in one frame");
            require(has_star_pointer_semantic("target_select"), "runtime semantic trace should expose generic star-pointer target select events");

            runtime.wpad().set_pointer(WPAD_CHAN0, center.x + 220.0F, center.y + 220.0F, true);
            require(!MR::isStarPointerPointingFileSelect(&actor), "3D star-pointer checks should reject pointers outside the projected radius");
            require(has_target_event(smgpc::game::StarPointerTargetEventKind::Leave),
                    "generic star-pointer target checks should record target leave events");
            require(has_star_pointer_semantic("target_leave"), "runtime semantic trace should expose generic star-pointer target leave events");

            runtime.wpad().set_pointer(WPAD_CHAN0, center.x, center.y, false);
            require(!MR::isStarPointerPointingFileSelect(&actor), "3D star-pointer checks should require a valid WPAD pointer");
        }

        $test("records generic Wii subsystem requests through compat services") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);

            Mtx matrix = {
                {1.0F, 0.0F, 0.0F, 10.0F},
                {0.0F, 1.0F, 0.0F, 20.0F},
                {0.0F, 0.0F, 1.0F, 30.0F},
            };
            MR::hidePlayer();
            MR::setPlayerBaseMtx(matrix);
            MR::resetCameraMan();
            MR::shakeCameraNormal();
            MR::deactivateDefaultGameLayout();
            MR::startStarPointerModeTitle(nullptr);
            MR::activeStarPointerGuidance();
            require(MR::requestFileSelectGuidance() && MR::requestFileSelectCopyGuidance(),
                    "star-pointer guidance requests should remain accepted by the compatibility service");
            MR::tryRumblePadStrong(nullptr, WPAD_CHAN0);
            MR::tryRumblePadMiddle(nullptr, WPAD_CHAN1);
            MR::tryRumblePadWeak(nullptr, WPAD_CHAN2);

            require(runtime.player_system().is_player_hidden() && runtime.player_system().has_base_matrix(),
                    "player utility wrappers should update generic player compatibility state");
            require(runtime.player_system().base_matrix()[3] == 10.0F && runtime.player_system().base_matrix()[7] == 20.0F &&
                        runtime.player_system().base_matrix()[11] == 30.0F,
                    "player base matrix compatibility state should preserve the incoming matrix translation");
            require(runtime.camera_system().reset_camera_man_count() == 1U && runtime.camera_system().normal_shake_request_count() == 1U,
                    "camera utility wrappers should update generic camera compatibility state");
            require(runtime.camera_system().shake_request_events().size() == 1U &&
                        runtime.camera_system().shake_request_events()[0U].kind == smgpc::game::CameraSystemService::ShakeRequestKind::Normal &&
                        runtime.camera_system().shake_request_events()[0U].frame_index == runtime.frame_index(),
                    "camera utility wrappers should record generic frame-stamped camera shake requests");
            require(!runtime.game_layout().is_default_game_layout_active(), "screen utility wrappers should update default layout state");
            require(runtime.star_pointer().mode() == cSourceTitleStarPointerMode &&
                        runtime.star_pointer().is_guidance_active() &&
                        runtime.star_pointer().is_guidance_requested(cSourceFileSelectGuidanceRequest) &&
                        runtime.star_pointer().is_guidance_requested(cSourceFileSelectCopyGuidanceRequest),
                    "star-pointer wrappers should update generic star-pointer state without caller-specific workarounds");
            MR::startStarPointerModeFileSelect(nullptr);
            require(runtime.star_pointer().mode() == cSourceTargetSelectionStarPointerMode,
                    "source file-select star-pointer mode should map to generic target-selection compat state");
            MR::requestStarPointerModeSaveLoad(nullptr);
            require(runtime.star_pointer().mode() == cSourceSystemModalStarPointerMode,
                    "source save/load star-pointer mode should map to generic system-modal compat state");
            MR::requestStarPointerModePictureBook(nullptr);
            require(runtime.star_pointer().mode() == cSourceDocumentViewerStarPointerMode,
                    "source picturebook star-pointer mode should map to generic document-viewer compat state");
            require(runtime.rumble().events().size() == 3U && runtime.rumble().events()[0U].kind == smgpc::game::RumbleRequestKind::Strong &&
                        runtime.rumble().events()[0U].channel == WPAD_CHAN0 &&
                        runtime.rumble().events()[1U].kind == smgpc::game::RumbleRequestKind::Middle &&
                        runtime.rumble().events()[1U].channel == WPAD_CHAN1 &&
                        runtime.rumble().events()[2U].kind == smgpc::game::RumbleRequestKind::Weak &&
                        runtime.rumble().events()[2U].channel == WPAD_CHAN2,
                    "rumble utility wrappers should record generic rumble requests");

            u8 sample_pattern[12][2]{};
            for (auto i = std::size_t{}; i < 12U; ++i) {
                sample_pattern[i][0U] = static_cast<u8>(i);
                sample_pattern[i][1U] = static_cast<u8>(23U - i);
            }
            const auto vfilter = std::array<u8, 7U>{0U, 0U, 21U, 22U, 21U, 0U, 0U};
            GXSetCopyClear(GXColor{1U, 2U, 3U, 4U}, 0x00abcdefU);
            GXSetCopyFilter(GX_TRUE, sample_pattern, GX_TRUE, vfilter.data());
            GXSetCopyClamp(static_cast<GXFBClamp>(GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
            GXSetDispCopySrc(0U, 0U, 640U, 456U);
            GXSetDispCopyDst(640U, 456U);
            require(GXSetDispCopyYScale(1.0F) == 456U, "GX display copy y-scale should preserve 456 EFB lines at 1:1 scale");
            GXSetDispCopyGamma(GX_GM_1_0);
            GXCopyDisp(nullptr, GX_FALSE);
            GXSetTexCopySrc(0U, 0U, 640U, 456U);
            GXSetTexCopyDst(640U, 456U, GX_TF_RGB565, GX_FALSE);
            GXCopyTex(nullptr, GX_TRUE);
            GXSetTexCopySrc(0U, 0U, 640U, 456U);
            GXSetTexCopyDst(640U, 456U, GX_CTF_A8, GX_FALSE);
            GXCopyTex(nullptr, GX_TRUE);
            require(runtime.copy_events().size() == 3U && runtime.copy_events()[0U].kind == smgpc::render::CopyEventKind::Xfb &&
                        runtime.copy_events()[0U].copy_to_xfb && runtime.copy_events()[0U].dest_stride == 1280U &&
                        runtime.copy_events()[0U].source_rect.height == 456 &&
                        runtime.copy_events()[0U].clear_color == std::array<std::uint8_t, 4U>{1U, 2U, 3U, 4U} &&
                        runtime.copy_events()[0U].clear_depth == 0x00abcdefU && runtime.copy_events()[0U].copy_filter_aa &&
                        runtime.copy_events()[0U].copy_filter_vertical &&
                        runtime.copy_events()[0U].copy_filter_sample_pattern[11U][1U] == 12U &&
                        runtime.copy_events()[0U].copy_filter_vfilter[3U] == 22U &&
                        runtime.copy_events()[1U].kind == smgpc::render::CopyEventKind::Texture &&
                        runtime.copy_events()[1U].clear && runtime.copy_events()[1U].target_pixel_format == 8U &&
                        runtime.copy_events()[1U].real_format == 4U && runtime.copy_events()[1U].dest_stride == 5120U &&
                        runtime.copy_events()[1U].source_rect.left == 0 && runtime.copy_events()[1U].source_rect.top == 0 &&
                        runtime.copy_events()[1U].output_size.height == 456U &&
                        runtime.copy_events()[2U].kind == smgpc::render::CopyEventKind::Texture &&
                        runtime.copy_events()[2U].target_pixel_format == 14U && runtime.copy_events()[2U].real_format == 7U &&
                        runtime.copy_events()[2U].dest_stride == 2560U,
                    "runtime should expose generic GX EFB copy events without caller-specific hooks");
            runtime.begin_frame(smgpc::render::FrameContext{.frame_index = 42U, .framebuffer = smgpc::render::FramebufferInfo{.width = 640U, .height = 456U}});
            require(runtime.copy_events().empty(), "runtime should clear per-frame copy events at the start of each frame");
        }

        $test("drives no-arg 3D drawing from programmable camera compatibility") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            auto camera_actor = LiveActor("ProgrammableCamera");

            runtime.draw_3d_normal(renderer);
            require(runtime.last_camera_pose().has_value(), "runtime should expose the neutral default camera used for missing scene cameras");
            require_near(runtime.last_camera_pose()->watch.z, -1.0F, 0.001F, "missing scene camera fallback should not use a title-specific pose");

            MR::initActorCameraProgrammable(&camera_actor);
            MR::startActorCameraProgrammable(&camera_actor, -1);
            MR::setProgrammableCameraParam(&camera_actor, TVec3f{1.0F, 2.0F, 3.0F}, TVec3f{4.0F, 5.0F, 6.0F}, TVec3f{0.0F, 0.0F, 1.0F});
            MR::setProgrammableCameraParamFovy(&camera_actor, 37.5F);

            runtime.draw_3d_normal(renderer);
            require(runtime.camera_system().active_programmable_camera_name().has_value() &&
                        *runtime.camera_system().active_programmable_camera_name() == "ProgrammableCamera",
                    "programmable actor camera should become the active camera service event");
            require(runtime.camera_system().programmable_camera_param_count() == 1U &&
                        runtime.camera_system().programmable_camera_fovy_count() == 1U,
                    "programmable camera service should record generic param and FOV updates");
            require(runtime.last_camera_pose().has_value(), "active programmable camera should provide the draw camera pose");
            require_near(runtime.last_camera_pose()->watch.x, 1.0F, 0.001F, "programmable camera watch X should flow through MR camera wrappers");
            require_near(runtime.last_camera_pose()->eye.y, 5.0F, 0.001F, "programmable camera eye Y should flow through MR camera wrappers");
            require_near(runtime.last_camera_pose()->up.z, 1.0F, 0.001F, "programmable camera up vector should preserve camera roll input");
            require_near(runtime.last_camera_pose()->fovy_degrees, 37.5F, 0.001F, "programmable camera FOV should update the active draw pose");
        }

        $test("drives original-shaped sys-info save layouts through LayoutActor compatibility") {
            auto logger = NullLogger();
            auto window_service = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window_service);
            auto renderer = RecordingRenderer();
            runtime.messages().set_message("System_Save01", "Saving");

            auto *window = MR::createSysInfoWindowExecuteWithChildren();
            MR::connectToSceneLayout(window);
            window->kill();
            window->appear("System_Save01", SysInfoWindow::Type_Blocking, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
            window->movement();
            window->calcAnim();

            auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            auto layout_it = std::ranges::find_if(layouts, [](const auto &entry) {
                return entry.name == "システム用インフォメーションウィンドウ" && entry.layout_name == "SysInfoWindow";
            });
            require(layout_it != layouts.end() && !layout_it->dead && layout_it->pane_count > 0U && layout_it->text_box_count > 0U,
                    "SysInfoWindow should load the original SysInfoWindow layout as a LayoutActor");
            require(std::ranges::any_of(layout_it->pane_controls,
                                        [](const auto &pane) {
                                            return pane.pane_name == "InfoWindow" && !pane.animations.empty() &&
                                                   pane.animations[0U].name == "ButtonAppear";
                                        }),
                    "SysInfoWindow should drive original pane animation controls");

            auto save_icon = SaveIcon(window);
            MR::connectToSceneLayout(&save_icon);
            save_icon.kill();
            save_icon.appear();
            save_icon.calcAnim();

            layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            require(std::ranges::any_of(layouts,
                                        [](const auto &entry) {
                                            return entry.name == "SaveIcon" && entry.layout_name == "IconSave" && !entry.dead &&
                                                   !entry.animations.empty() && entry.animations[0U].name == "Rotate";
                                        }),
                    "SaveIcon should load the original IconSave layout and start the Rotate animation");

            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 1900U,
                .frame_time_seconds = 1900.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            runtime.draw_2d_normal(renderer);
            require(renderer.texture_count > 0U && (renderer.quad_count > 0U || renderer.gx_material_batch_count > 0U),
                    "SysInfoWindow/SaveIcon should render through the central 2D layout draw path");

            delete window;
        }

        $test("drives original-shaped file-select info and operation button layouts") {
            auto logger = NullLogger();
            auto window_service = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window_service);
            auto renderer = RecordingRenderer();

            auto info = FileSelectInfo(11, "ファイル情報");
            info.initWithoutIter();
            auto name = std::array<u16, 11U>{'M', 'a', 'r', 'i', 'o', 0};
            info.setInfo(name.data(), 2, 7, 123, true, true, false, L"05/17/26", L"04:31", 9);
            info.appear();

            auto button = FileSelectButton("ファイル選択ボタン");
            button.initWithoutIter();
            button.appear();
            button.control();

            auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            const auto info_it = std::ranges::find_if(layouts, [](const auto &entry) {
                return entry.name == "ファイル情報" && entry.layout_name == "FileInfo";
            });
            require(info_it != layouts.end() && !info_it->dead && info_it->animations.size() >= 3U && info_it->animations[0U].name == "Appear" &&
                        info_it->pane_count > 0U && info_it->text_box_count > 0U,
                    "FileSelectInfo should load the original FileInfo layout and start its Appear animation");
            require(info.getFileNumber() == 2 && info.getStarNum() == 7 && info.getStarPieceNum() == 123 && info.isSelectedMario(),
                    "FileSelectInfo should preserve original-shaped setInfo state");

            const auto button_it = std::ranges::find_if(layouts, [](const auto &entry) {
                return entry.name == "ファイル選択ボタン" && entry.layout_name == "FileSelect";
            });
            require(button_it != layouts.end() && !button_it->dead && button_it->button_controllers.size() == 5U,
                    "FileSelectButton should load the original FileSelect layout and own five ButtonPaneControllers");
            require(std::ranges::any_of(button_it->button_controllers,
                                        [](const auto &controller) {
                                            return controller.pane_name == "StartButton" && controller.bounding_pane_name == "BoxStartButton" &&
                                                   controller.active && controller.nerve == "Appear";
                                        }) &&
                        std::ranges::any_of(button_it->button_controllers,
                                            [](const auto &controller) {
                                                return controller.pane_name == "P2ManualButton" && controller.bounding_pane_name == "BoxP2";
                                            }),
                    "FileSelectButton should expose original pane/button bindings through generic layout runtime state");

            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 1900U,
                .frame_time_seconds = 1900.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            runtime.draw_2d_normal(renderer);
            require(renderer.texture_count > 0U && (renderer.quad_count > 0U || renderer.gx_material_batch_count > 0U),
                    "FileSelectInfo/FileSelectButton should render through the central 2D layout draw path");
        }

        $test("fills FileSelector file info through source-shaped save restore state") {
            auto logger = NullLogger();
            auto window_service = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window_service);
            runtime.save_data().set_slot_state(2, smgpc::game::SaveDataService::SlotState{
                                                      .created = true,
                                                      .last_loaded_mario = false,
                                                      .power_star_num = 120,
                                                      .star_piece_num = 777,
                                                      .player_miss_num = 42,
                                                      .icon_id = 2U,
                                                      .view_normal_ending = true,
                                                      .view_complete_ending = true,
                                                      .complete_ending_mario_and_luigi = true,
                                                      .game_event_flags = {},
                                                      .game_event_values = {},
                                                      .last_modified = 1778992260,
                                                  });
            runtime.save_data().set_slot_state(3, smgpc::game::SaveDataService::SlotState{
                                                      .created = true,
                                                      .last_loaded_mario = true,
                                                      .power_star_num = 6,
                                                      .star_piece_num = 11,
                                                      .player_miss_num = 8,
                                                      .icon_id = 1U,
                                                      .view_normal_ending = false,
                                                      .view_complete_ending = false,
                                                      .complete_ending_mario_and_luigi = false,
                                                      .game_event_flags = {},
                                                      .game_event_values = {},
                                                      .last_modified = 1778992260,
                                                  });

            auto selector = FileSelector("ファイルセレクタ");
            selector.initWithoutIter();
            selector.setFileInfo(2);
            require(selector.getCurrentFileInfoFileNo() == 2, "FileSelector should populate FileSelectInfo for the requested slot");
            require(!selector.isCurrentFileInfoSelectedMario(),
                    "FileSelector should restore last-loaded Luigi state through SaveDataHandleSequence before filling FileSelectInfo");
            require(selector.getCurrentFileInfoMissCount() == 42,
                    "FileSelector should show miss count only when checkAllComplete finds Mario and Luigi final stars");
            require(std::wstring_view(selector.getCurrentFileInfoDateMessage()) == L"2026/05/17" &&
                        std::wstring_view(selector.getCurrentFileInfoTimeMessage()) == L"04:31",
                    "FileSelector should format save timestamps through original BMG replacement tags");

            selector.setFileInfo(3);
            require(selector.isCurrentFileInfoSelectedMario(), "FileSelector should keep Mario selected for Mario save data");
            require(selector.getCurrentFileInfoMissCount() == -1,
                    "FileSelector should hide miss count until the source checkAllComplete path marks both sides complete");
        }

        $test("drives remaining original-shaped file-select child layouts") {
            auto logger = NullLogger();
            auto window_service = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window_service);
            auto renderer = RecordingRenderer();
            runtime.messages().set_message("2PGuidance001", "Two-player guidance");

            auto back = BackButton("戻るボタン", false);
            back.initWithoutIter();
            MR::connectToScene(&back, MR::MovementType_Layout, MR::CalcAnimType_Layout, -1, MR::DrawType_LayoutDecoration);
            back.appear();

            auto bros = BrosButton("ルイージ切り替えボタン");
            bros.initWithoutIter();
            bros.appear(true);

            auto info = InformationMessage();
            info.initWithoutIter();
            info.setMessage(L"Confirm");
            info.appearWithButtonLayout();

            auto manual = Manual2P("２Ｐマニュアル");
            manual.initWithoutIter();
            manual.appear();

            auto mii_confirm = MiiConfirmIcon("Mii確認用アイコン");
            mii_confirm.initWithoutIter();
            MR::connectToScene(&mii_confirm, MR::MovementType_Layout, MR::CalcAnimType_Layout, -1, MR::DrawType_LayoutDecoration);
            mii_confirm.appear(nullptr, L"Mii");

            auto mii_select = MiiSelect("MiiSelect");
            mii_select.initWithoutIter();
            mii_select.collectValidMiiIndex();
            mii_select.invalidateSpecialMii(FileSelectIconID::Luigi);
            require(mii_select.getCollectValidMiiIndexCount() == 1 && mii_select.getIconNum() == 5,
                    "MiiSelect should combine valid special icons with the generic RFL service's valid Mii list");
            auto first_icon = FileSelectIconID();
            auto mii_icon = FileSelectIconID();
            mii_select.getIconID(&first_icon, 0);
            mii_select.getIconID(&mii_icon, 4);
            require(first_icon.isFellow() && first_icon.getFellowID() == FileSelectIconID::Mario && mii_icon.isMii() && mii_icon.getMiiIndex() == 0,
                    "MiiSelect should expose original ordered FileSelectIconID values for fellow and RFL Mii entries");
            mii_select.appear();

            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 1900U,
                .frame_time_seconds = 1900.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            require(std::ranges::any_of(layouts,
                                        [](const auto &entry) {
                                            return entry.name == "戻るボタン" && entry.layout_name == "BackButton" && !entry.dead &&
                                                   std::ranges::any_of(entry.button_controllers,
                                                                       [](const auto &controller) {
                                                                           return controller.pane_name == "Back" &&
                                                                                  controller.bounding_pane_name == "BoxButton";
                                                                       });
                                        }),
                    "BackButton should load the original BackButton layout and expose its Back button controller");
            require(std::ranges::any_of(layouts,
                                        [](const auto &entry) {
                                            return entry.name == "ルイージ切り替えボタン" && entry.layout_name == "BrosButton" && !entry.dead &&
                                                   std::ranges::any_of(entry.button_controllers,
                                                                       [](const auto &controller) {
                                                                           return controller.pane_name == "BrosButton" &&
                                                                                  controller.bounding_pane_name == "BoxBButton";
                                                                       });
                                        }),
                    "BrosButton should load the original BrosButton layout and expose its original button binding");
            require(std::ranges::any_of(layouts,
                                        [](const auto &entry) {
                                            return entry.name == "インフォメーションメッセージ" && entry.layout_name == "InformationWindow" &&
                                                   !entry.dead && entry.animations.size() >= 2U && entry.animations[0U].name == "Appear" &&
                                                   entry.animations[1U].name == "Line";
                                        }),
                    "InformationMessage should load InformationWindow and drive its original Appear/Line animations");
            require(std::ranges::any_of(layouts,
                                        [](const auto &entry) {
                                            return entry.name == "２Ｐマニュアル" && entry.layout_name == "P2Manual" && !entry.dead &&
                                                   std::ranges::any_of(entry.button_controllers,
                                                                       [](const auto &controller) {
                                                                           return controller.pane_name == "LeftButton" ||
                                                                                  controller.pane_name == "RightButton";
                                                                       });
                                        }),
                    "Manual2P should load P2Manual and own left/right ButtonPaneControllers");
            require(std::ranges::any_of(layouts,
                                        [](const auto &entry) {
                                            return entry.name == "Mii確認用アイコン" && entry.layout_name == "MiiConfirmIcon" && !entry.dead &&
                                                   !entry.animations.empty() && entry.animations[0U].name == "ButtonAppear";
                                        }),
                    "MiiConfirmIcon should load MiiConfirmIcon and start ButtonAppear through LayoutActor compatibility");
            require(std::ranges::any_of(layouts,
                                        [](const auto &entry) {
                                            return entry.name == "MiiSelect" && entry.layout_name == "MiiSelect" && !entry.dead &&
                                                   !entry.animations.empty() && entry.animations[0U].name == "Appear";
                                        }),
                    "MiiSelect should load the original MiiSelect layout and start Appear through LayoutActor compatibility");

            runtime.draw_2d_normal(renderer);
            require(renderer.texture_count > 0U && (renderer.quad_count > 0U || renderer.gx_material_batch_count > 0U),
                    "remaining file-select child layouts should render through the central 2D layout draw path");
        }

        $test("drives MiiSelect icon selection through layout pane hit testing") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto mii_select = MiiSelect("MiiSelect");
            mii_select.initWithoutIter();
            mii_select.collectValidMiiIndex();
            mii_select.invalidateSpecialMii(FileSelectIconID::Luigi);
            mii_select.appear();

            const auto mario_icon_point =
                screen_center_for_layout_pane("MiiSelect", "Mii01", "MiiSelect Mii01 should expose hit-test bounds for icon selection");
            for (auto frame = 0; frame < 180 && !mii_select.isSelected(); ++frame) {
                window.set_pointer(mario_icon_point.x, mario_icon_point.y, true);
                window.set_core_buttons(frame % 6 == 2, false);
                runtime.begin_frame(smgpc::render::FrameContext{
                    .frame_index = static_cast<std::uint64_t>(frame),
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });
            }
            window.set_core_buttons(false, false);

            auto selected_icon = FileSelectIconID();
            mii_select.getSelectedID(&selected_icon);
            require(mii_select.isSelected() && selected_icon.isFellow() && selected_icon.getFellowID() == FileSelectIconID::Mario,
                    "MiiSelect should select the original first icon when the star pointer clicks Mii01");
        }

        $test("draws existing FileSelectItem through scheduler-owned fellow model") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            auto item = FileSelectItem(1, false);
            runtime.set_j3d_packet_trace_frame(0U);

            item.initWithoutIter();
            item.setBasePosition({0.0F, 0.0F, 0.0F});
            item.appear();
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });

            runtime.draw_3d_normal(renderer, far_test_camera_pose());
            const auto draw_trace = runtime.scheduler().last_execution_trace();
            const auto packets = runtime.j3d_packet_trace();

            require(renderer.texture_count > 0U, "existing FileSelectItem should load original FileSelectDataMario textures when drawn");
            require(renderer.triangle_batch_count > 0U || renderer.gx_material_batch_count > 0U,
                    "existing FileSelectItem should draw the original FileSelectDataMario model");
            require(renderer.submitted_indices > 0U, "existing FileSelectItem should submit original FileSelectDataMario geometry");
            require(std::ranges::any_of(packets, [](const auto &packet) { return packet.model_name == "FileSelectDataMario"; }),
                    "existing FileSelectItem should render through the original FileSelectDataMario archive");
            require(std::ranges::any_of(draw_trace,
                                        [](const auto &entry) {
                                            return entry.kind == smgpc::game::SceneEntryKind::LiveActorModel &&
                                                   entry.name == "キャラフェイス" && entry.movement_type == MR::MovementType_NPC &&
                                                   entry.calc_anim_type == MR::CalcAnimType_NPC &&
                                                   entry.draw_buffer_type == MR::DrawBufferType_NPC &&
                                                   (entry.phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa ||
                                                    entry.phase == smgpc::game::SceneSchedulerPhase::DrawBufferXlu);
                                        }),
                    "FileSelectItem fellow model should be drawn by the central NPC draw buffer");
        }

        $test("draws FileSelectItem fellow model selected by FileSelectIconID") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            auto icon_id = FileSelectIconID();
            icon_id.setFellowID(FileSelectIconID::Peach);
            auto item = FileSelectItem(1, false, icon_id);
            runtime.set_j3d_packet_trace_frame(0U);

            item.initWithoutIter();
            item.setBasePosition({0.0F, 0.0F, 0.0F});
            item.appear();
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });

            runtime.draw_3d_normal(renderer, far_test_camera_pose());
            const auto packets = runtime.j3d_packet_trace();

            require(std::ranges::any_of(packets, [](const auto &packet) { return packet.model_name == "FileSelectDataPeach"; }),
                    "FileSelectIconID Peach should render the original FileSelectDataPeach fellow model");
            require(std::ranges::none_of(packets, [](const auto &packet) { return packet.model_name == "FileSelectDataMario"; }),
                    "FileSelectIconID Peach should not leave the default Mario fellow model appeared");
        }

        $test("threads FileSelector save icon IDs into fellow models") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            runtime.save_data().set_slot_state(3, smgpc::game::SaveDataService::SlotState{
                                                      .created = true,
                                                      .last_loaded_mario = true,
                                                      .power_star_num = 9,
                                                      .star_piece_num = 100,
                                                      .icon_id = 5U,
                                                      .view_normal_ending = false,
                                                      .view_complete_ending = false,
                                                      .complete_ending_mario_and_luigi = false,
                                                      .game_event_flags = {},
                                                      .game_event_values = {},
                                                      .last_modified = 0,
                                                  });
            auto selector = FileSelector("ファイルセレクタ");
            runtime.set_j3d_packet_trace_frame(0U);

            selector.initWithoutIter();
            auto *item = selector.getItemByFileNo(3);
            require(item != nullptr && !item->isNew(), "FileSelector should restore slot 3 as an existing file item");
            item->appear();
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });

            runtime.draw_3d_normal(renderer, far_test_camera_pose());
            const auto packets = runtime.j3d_packet_trace();

            require(std::ranges::any_of(packets, [](const auto &packet) { return packet.model_name == "FileSelectDataPeach"; }),
                    "FileSelector should convert save icon_id 5 into the original Peach fellow model");
        }

        $test("draws FileSelectSky through LiveActor model compatibility") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            runtime.set_current_stage_name("FileSelect");
            runtime.set_j3d_packet_trace_frame(20U);
            auto sky = FileSelectSky("ファイル選択空");

            sky.initWithoutIter();
            sky.appear();
            require(runtime.effects().host_binding("ファイル選択空").has_value() &&
                        runtime.effects().host_binding("ファイル選択空")->source == smgpc::game::EffectHostBindingSource::LiveActorBaseMatrix,
                    "FileSelectSky effect keeper should bind to generic LiveActor base-matrix host state");
            require(std::ranges::any_of(runtime.effects().events(),
                                        [](const auto &event) {
                                            return event.actor_name == "ファイル選択空" &&
                                                   event.effect_name == "CometNearOrbitSky" &&
                                                   std::ranges::any_of(event.resolved_resources, [](const auto &resource) {
                                                       return resource.particle_name == "TitleShootingStar00" &&
                                                              resource.auto_effect_draw_order == "3D" &&
                                                              resource.resource != nullptr &&
                                                              resource.resource->child_shape.has_value() &&
                                                              resource.resource->child_texture_index.value_or(0xffffU) == 0U;
                                                   });
                                        }),
                    "FileSelectSky effect keeper should start generic CometNearOrbitSky auto effects from Effect.arc metadata");
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            runtime.draw_3d_normal(renderer, title_test_camera_pose());

            require(sky.getNerveStep() == 1, "FileSelectSky should be updated by the scene sky actor compatibility list");
            require(renderer.texture_count > 0U, "FileSelectSky should load original CometNearOrbitSky textures through LiveActor model compat");
            require(renderer.triangle_batch_count > 0U || renderer.gx_material_batch_count > 0U,
                    "FileSelectSky should draw original CometNearOrbitSky model geometry");
            require(renderer.submitted_indices > 0U, "FileSelectSky should submit original sky J3D geometry");
            require(std::ranges::any_of(runtime.effects().draw_packets(),
                                        [](const auto &packet) {
                                            return packet.particle_name == "TitleShootingStar00" &&
                                                   packet.draw_order == "3D" &&
                                                   packet.child_particle &&
                                                   packet.primitive_type == "triangle_strip" &&
                                                   packet.vertex_count == 16U &&
                                                   packet.index_count == 19U &&
                                                   packet.color_channel_count == 0U &&
                                                   packet.host_binding_found &&
                                                   packet.host_binding_source == "LiveActorBaseMatrix" &&
                                                   packet.texture.slot == 1U &&
                                                   packet.texture.texture_index == 0U &&
                                                   packet.texture.name == "mr_glow01_i" &&
                                                   packet.texture.format_name == "I8";
                                        }),
                    "3D effect draw should submit generic SSP1 child particles with the source JPA texture slot");
            require(renderer.gx_material_batch_count > 0U, "SSP1 display-list child particles should use the generic GX material renderer path");
            require(renderer.last_gx_material_primitive_topology == smgpc::render::PrimitiveTopology::TriangleStrip,
                    "SSP1 display-list child particles should retain Wii triangle-strip topology in the renderer batch");

            auto frame_1900_renderer = RecordingRenderer();
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 1900U,
                .frame_time_seconds = 1900.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            runtime.draw_3d_normal(frame_1900_renderer, title_test_camera_pose());
            const auto shooting_star_child_count = std::ranges::count_if(runtime.effects().draw_packets(), [](const auto &packet) {
                return packet.particle_name == "TitleShootingStar00" &&
                       packet.draw_order == "3D" &&
                       packet.child_particle &&
                       packet.primitive_type == "triangle_strip" &&
                       packet.texture.slot == 1U &&
                       packet.texture.texture_index == 0U;
            });
            require(shooting_star_child_count >= 1000 && shooting_star_child_count <= 1300,
                    "KFA1 key-rate evaluation should keep long-running SSP1 child particle counts near the Dolphin trace class");
        }

        $test("refreshes modeled LiveActor base matrices before material animation") {
            class MatrixOrderActor final : public LiveActor {
            public:
                MatrixOrderActor() : LiveActor("MatrixOrderActor") {
                }

                void calcAndSetBaseMtx() override {
                    ++base_matrix_updates;
                    setBaseMatrix(smgpc::game::j3d_matrix_from_translation_scale({10.0F, 20.0F, 30.0F}, 2.0F));
                }

                int base_matrix_updates = 0;
            };

            auto actor = MatrixOrderActor();
            actor.initModelManagerWithAnm("FileSelectDataPlanet", nullptr, false);
            actor.makeActorAppeared();
            actor.calcAnim();

            require(actor.base_matrix_updates == 1,
                    "LiveActor::calcAnim should refresh modeled actor base matrices before material controllers read them");
            require_near(actor.getBaseMatrix().m[0U], 2.0F, 0.001F, "calcAnim base matrix update should preserve scale");
            require_near(actor.getBaseMatrix().m[3U], 10.0F, 0.001F, "calcAnim base matrix update should preserve translation X");
            require_near(actor.getBaseMatrix().m[7U], 20.0F, 0.001F, "calcAnim base matrix update should preserve translation Y");
            require_near(actor.getBaseMatrix().m[11U], 30.0F, 0.001F, "calcAnim base matrix update should preserve translation Z");
        }

        $test("draws FileSelectEffect through generic LiveActor BRK/BTK compatibility") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            runtime.set_j3d_packet_trace_frame(0U);
            auto effect = FileSelectEffect("選択時エフェクト");

            effect.initWithoutIter();
            require(effect.isDead(), "FileSelectEffect should initialize dead like the original actor");
            effect.mPosition = TVec3f{100.0F, 20.0F, -400.0F};
            effect.mScale = TVec3f{0.4F, 0.4F, 0.4F};
            effect.appear();
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            runtime.draw_3d_normal(renderer, far_test_camera_pose());

            require(!effect.isDead(), "FileSelectEffect should remain alive while its one-shot BRK is advancing");
            require(effect.getBrkCtrl()->mEnd > 0, "FileSelectEffect should read the original MiniatureGalaxySelect BRK frame range");
            require(renderer.texture_count > 0U, "FileSelectEffect should load original MiniatureGalaxySelect textures through LiveActor model compat");
            require(renderer.triangle_batch_count > 0U || renderer.gx_material_batch_count > 0U,
                    "FileSelectEffect should draw original MiniatureGalaxySelect model geometry");
            require(renderer.submitted_indices > 0U, "FileSelectEffect should submit original selection-effect J3D geometry");
            const auto packets = runtime.j3d_packet_trace();
            require(std::ranges::any_of(packets, [](const auto &packet) { return packet.model_name == "MiniatureGalaxySelect"; }),
                    "FileSelectEffect should render through the original MiniatureGalaxySelect archive");
        }

        $test("routes DVD archive loads through cached runtime service") {
            const auto root = disc_files_root();
            auto dvd = smgpc::game::DvdFileSystemService(root);

            const auto object_path = dvd.find_object_archive("CometNearOrbitSky");
            require(object_path.has_value(), "DVD service should resolve original object archives");
            require(dvd.exists("ObjectData/CometNearOrbitSky.arc"), "DVD service should expose disc-relative file existence");

            auto &archive = dvd.archive("ObjectData/CometNearOrbitSky.arc");
            require(archive.contains("cometnearorbitsky.bdl"), "DVD archive cache should mount original RARC contents");
            auto &cached_archive = dvd.archive("/ObjectData/CometNearOrbitSky.arc");
            require(&archive == &cached_archive, "DVD archive cache should reuse equivalent absolute disc paths");
            auto &host_path_archive = dvd.archive_for_path(*object_path);
            require(&archive == &host_path_archive, "DVD archive cache should reuse resolved host archive paths");
            require(dvd.archive_load_count("ObjectData/CometNearOrbitSky.arc") == 1U, "DVD archive cache should record one physical load");
            require(dvd.archive_load_count_for_path(*object_path) == 1U, "DVD archive cache should count host path loads with the same key");
            require(dvd.cached_archive_count() == 1U, "DVD archive cache should keep one mounted archive entry");
            const auto archive_trace = dvd.archive_load_trace();
            require(archive_trace.size() == 3U, "DVD archive trace should record every archive request");
            require(!archive_trace[0U].cache_hit && archive_trace[0U].load_count == 1U &&
                        archive_trace[0U].cached_archive_count == 1U && archive_trace[0U].resource_count > 0U,
                    "DVD archive trace should identify first physical archive load and decoded resource count");
            require(archive_trace[1U].cache_hit && archive_trace[2U].cache_hit,
                    "DVD archive trace should identify later equivalent path cache hits");
            require(archive_trace[0U].resolved_path == archive_trace[1U].resolved_path &&
                        archive_trace[1U].resolved_path == archive_trace[2U].resolved_path,
                    "DVD archive trace should normalize equivalent archive requests to one resolved path");

            const auto bytes = dvd.read_file("files/ObjectData/CometNearOrbitSky.arc");
            require(bytes.size() > 0x40U, "DVD service should read disc-relative files with an optional files prefix");
            const auto file_trace = dvd.file_read_trace();
            require(file_trace.size() == 1U && file_trace[0U].requested_path == "files/ObjectData/CometNearOrbitSky.arc" &&
                        file_trace[0U].byte_count == bytes.size() && file_trace[0U].resolved_path.ends_with("ObjectData/CometNearOrbitSky.arc"),
                    "DVD file trace should record requested path, resolved path, and byte count");
            dvd.clear_trace();
            require(dvd.archive_load_trace().empty() && dvd.file_read_trace().empty(), "DVD trace buffers should be clearable between probes");

            auto rejected_escape = false;
            try {
                (void)dvd.resolve("../ObjectData/CometNearOrbitSky.arc");
            } catch (const std::runtime_error &) {
                rejected_escape = true;
            }
            require(rejected_escape, "DVD service should reject paths that escape the disc root");
        }

        $test("exposes WPAD save message and RFL runtime services") {
            auto wpad = smgpc::game::WpadService();
            wpad.begin_frame();
            wpad.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_A | WPAD_BUTTON_B);
            require(wpad.is_connected(WPAD_CHAN0), "WPAD service should mark a channel connected when data arrives");
            require(wpad.is_button_held(WPAD_CHAN0, WPAD_BUTTON_A), "WPAD service should expose held core buttons");
            require(wpad.is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A), "WPAD service should expose trigger edges");
            require(!wpad.is_button_released(WPAD_CHAN0, WPAD_BUTTON_A), "WPAD service should not release newly held buttons");

            wpad.begin_frame();
            wpad.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_A);
            require(!wpad.is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A), "WPAD service should suppress repeated trigger edges");
            require(wpad.is_button_released(WPAD_CHAN0, WPAD_BUTTON_B), "WPAD service should expose release edges");
            wpad.set_pointer(WPAD_CHAN0, 320.0F, 228.0F, true);
            wpad.set_pointer(WPAD_CHAN0, 300.0F, 200.0F, true);
            const auto pointer = wpad.pointer(WPAD_CHAN0);
            require(pointer.valid && pointer.x == 300.0F && pointer.y == 200.0F, "WPAD service should preserve pointer position");
            const auto past_pointer = wpad.past_pointer(WPAD_CHAN0, 1U);
            require(wpad.pointer_history_count(WPAD_CHAN0) == 2U && past_pointer.x == 320.0F && past_pointer.y == 228.0F,
                    "WPAD service should preserve pointer history");
            wpad.set_sub_stick(WPAD_CHAN0, -0.75F, 0.5F);
            wpad.set_core_acceleration(WPAD_CHAN0, 1.0F, 2.0F, 3.0F);
            wpad.set_sub_acceleration(WPAD_CHAN0, 4.0F, 5.0F, 6.0F);
            wpad.set_swing(WPAD_CHAN0, true, false);
            wpad.set_distance_to_display(WPAD_CHAN0, 2.25F);
            require(wpad.sub_stick(WPAD_CHAN0).x == -0.75F && wpad.sub_stick(WPAD_CHAN0).y == 0.5F,
                    "WPAD service should preserve nunchuk stick state");
            require(wpad.core_acceleration(WPAD_CHAN0).z == 3.0F && wpad.sub_acceleration(WPAD_CHAN0).z == 6.0F,
                    "WPAD service should preserve acceleration fixtures");
            require(wpad.is_core_swing(WPAD_CHAN0) && wpad.distance_to_display(WPAD_CHAN0) == 2.25F,
                    "WPAD service should preserve swing and display-distance fixtures");

            {
                auto logger = NullLogger();
                auto window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(logger, window);
                window.set_input_pressed(smgpc::render::InputButton::CORE_PAD_A, true);
                window.set_input_pressed(smgpc::render::InputButton::CORE_PAD_RIGHT, true);
                window.set_input_pressed(smgpc::render::InputButton::CORE_PAD_PLUS, true);
                window.set_pointer(240.0F, 180.0F, true);
                runtime.begin_frame({
                    .frame_index = 1U,
                    .frame_time_seconds = 0.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {640U, 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });

                require(runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_A) &&
                            runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A) &&
                            runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_RIGHT) &&
                            runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_PLUS),
                        "RuntimeContext should bridge generic window input into WPAD button state for file-select controls");
                const auto bridged_pointer = runtime.wpad().pointer(WPAD_CHAN0);
                require(bridged_pointer.valid && bridged_pointer.x == 240.0F && bridged_pointer.y == 180.0F &&
                            runtime.wpad().distance_to_display(WPAD_CHAN0) == 1.0F,
                        "RuntimeContext should bridge generic window pointer input into WPAD pointer state for file-select hit testing");

#ifndef NDEBUG
                const auto trace_path = std::filesystem::path(".cache/tests/runtime-input-trace-host.ndjson");
                smgpc::game::write_runtime_parity_trace(trace_path,
                                                        smgpc::render::FrameContext{
                                                            .frame_index = 1U,
                                                            .frame_time_seconds = 0.0,
                                                            .frame_delta_seconds = 1.0 / 60.0,
                                                            .framebuffer = {640U, 456U},
                                                            .has_focus = true,
                                                            .is_minimized = false,
                                                        },
                                                        runtime);
                const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
                const auto &host_input = trace_json.at("host_input");
                const auto &wpad0 = trace_json.at("wpad0");
                require(host_input.at("raw_hold_mask") == (WPAD_BUTTON_A | WPAD_BUTTON_RIGHT | WPAD_BUTTON_PLUS) &&
                            host_input.at("effective_hold_mask") == host_input.at("raw_hold_mask") &&
                            json_string_array_contains(host_input.at("raw_buttons"), "A") &&
                            json_string_array_contains(host_input.at("raw_buttons"), "RIGHT") &&
                            json_string_array_contains(host_input.at("raw_buttons"), "PLUS") &&
                            host_input.at("raw_pointer").at("x") == 240.0F &&
                            host_input.at("raw_pointer").at("y") == 180.0F &&
                            host_input.at("raw_pointer").at("valid") == true &&
                            host_input.at("raw_pointer").at("on_screen") == true &&
                            wpad0.at("hold_mask") == host_input.at("effective_hold_mask") &&
                            wpad0.at("trigger_mask") == host_input.at("effective_hold_mask") &&
                            json_string_array_contains(wpad0.at("held_buttons"), "PLUS") &&
                            json_string_array_contains(wpad0.at("triggered_buttons"), "RIGHT") &&
                            wpad0.at("pointer").at("x") == 240.0F &&
                            wpad0.at("pointer").at("on_screen") == true,
                        "runtime parity trace should expose raw host input and derived WPAD state for generic controls");
#endif
            }

            {
                auto button_script = ScopedEnvironmentVariable("SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "10-12:A+B;14:DOWN+PLUS");
                auto pointer_script = ScopedEnvironmentVariable("SMGPC_DEBUG_WPAD_POINTER_SCRIPT", "11-13:320,228,true;15:0,0,false");
                auto logger = NullLogger();
                auto window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(logger, window);

                runtime.begin_frame({
                    .frame_index = 9U,
                    .frame_time_seconds = 9.0 / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {640U, 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });
                require(!runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_A),
                        "debug WPAD script should not affect frames before its generic button span");

                runtime.begin_frame({
                    .frame_index = 10U,
                    .frame_time_seconds = 10.0 / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {640U, 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });
                require(runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_A) &&
                            runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_B) &&
                            runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A),
                        "debug WPAD button script should feed generic button holds and trigger edges through RuntimeContext");

                runtime.begin_frame({
                    .frame_index = 11U,
                    .frame_time_seconds = 11.0 / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {640U, 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });
                const auto scripted_pointer = runtime.wpad().pointer(WPAD_CHAN0);
                require(scripted_pointer.valid && scripted_pointer.x == 320.0F && scripted_pointer.y == 228.0F &&
                            runtime.wpad().distance_to_display(WPAD_CHAN0) == 1.0F,
                        "debug WPAD pointer script should feed generic pointer coordinates through RuntimeContext");
                require(runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_A) &&
                            !runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A),
                        "debug WPAD button script should preserve repeated hold semantics across frames");

#ifndef NDEBUG
                const auto trace_path = std::filesystem::path(".cache/tests/runtime-input-trace-script.ndjson");
                smgpc::game::write_runtime_parity_trace(trace_path,
                                                        smgpc::render::FrameContext{
                                                            .frame_index = 11U,
                                                            .frame_time_seconds = 11.0 / 60.0,
                                                            .frame_delta_seconds = 1.0 / 60.0,
                                                            .framebuffer = {640U, 456U},
                                                            .has_focus = true,
                                                            .is_minimized = false,
                                                        },
                                                        runtime);
                const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
                const auto &host_input = trace_json.at("host_input");
                const auto &wpad0 = trace_json.at("wpad0");
                require(host_input.at("raw_hold_mask") == 0U &&
                            host_input.at("effective_hold_mask") == (WPAD_BUTTON_A | WPAD_BUTTON_B) &&
                            host_input.at("debug_button_script_applied") == true &&
                            host_input.at("debug_pointer_script_applied") == true &&
                            host_input.at("raw_pointer").at("valid") == false &&
                            host_input.at("effective_pointer").at("x") == 320.0F &&
                            host_input.at("effective_pointer").at("y") == 228.0F &&
                            host_input.at("effective_pointer").at("on_screen") == true &&
                            wpad0.at("hold_mask") == host_input.at("effective_hold_mask") &&
                            wpad0.at("trigger_mask") == 0U &&
                            wpad0.at("repeat_mask") == 0U &&
                            wpad0.at("pointer").at("x") == 320.0F &&
                            wpad0.at("previous_pointer").at("valid") == false &&
                            wpad0.at("pointer_history_count") >= 2U,
                        "runtime parity trace should distinguish raw host input from debug-scripted derived WPAD state");
#endif

                runtime.begin_frame({
                    .frame_index = 14U,
                    .frame_time_seconds = 14.0 / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {640U, 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });
                require(runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_DOWN) &&
                            runtime.wpad().is_button_held(WPAD_CHAN0, WPAD_BUTTON_PLUS) &&
                            runtime.wpad().is_button_released(WPAD_CHAN0, WPAD_BUTTON_A),
                        "debug WPAD button script should support generic named Wii buttons and release edges");

                runtime.begin_frame({
                    .frame_index = 15U,
                    .frame_time_seconds = 15.0 / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {640U, 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });
                require(!runtime.wpad().pointer(WPAD_CHAN0).valid && runtime.wpad().distance_to_display(WPAD_CHAN0) == 0.0F,
                        "debug WPAD pointer script should support explicit invalid pointer spans");
            }

            auto save = smgpc::game::SaveDataService();
            const std::array<std::uint8_t, 4U> save_bytes{1U, 2U, 3U, 4U};
            save.write_file("user/slot0.bin", save_bytes);
            require(save.exists("user/slot0.bin"), "save service should report deterministic fixture files");
            const auto loaded_save = save.read_file("user/slot0.bin");
            require(loaded_save.has_value() && *loaded_save == std::vector<std::uint8_t>(save_bytes.begin(), save_bytes.end()),
                    "save service should round-trip file bytes");
            require(save.erase("user/slot0.bin") && !save.exists("user/slot0.bin"), "save service should erase fixture files");
            save.set_slot_state(2, smgpc::game::SaveDataService::SlotState{
                                       .created = true,
                                       .game_data_corrupted = false,
                                       .config_data_corrupted = true,
                                       .last_loaded_mario = false,
                                       .power_star_num = 42,
                                       .star_piece_num = 1200,
                                       .player_miss_num = 7,
                                       .has_mii_id = true,
                                       .rfl_mii_index = 3,
                                       .icon_id = std::nullopt,
                                       .view_normal_ending = true,
                                       .view_complete_ending = false,
                                       .complete_ending_mario_and_luigi = true,
                                       .game_event_flags = {},
                                       .game_event_values = {},
                                       .last_modified = 123456,
                                   });
            const auto slot_it = std::ranges::find_if(save.slot_states(), [](const auto &slot) { return slot.slot_index == 2; });
            require(save.slot_states().size() == 6U && slot_it != save.slot_states().end() && slot_it->created &&
                        slot_it->config_data_corrupted && !slot_it->last_loaded_mario && slot_it->power_star_num == 42 &&
                        slot_it->rfl_mii_index == 3 && !slot_it->icon_id.has_value(),
                    "save service should expose original-shaped per-slot UserFile state for file-select parity traces");
            auto restored_file = UserFile();
            save.restore_user_file(restored_file, 2, false);
            require(restored_file.isCreated() && !restored_file.isLastLoadedMario() && !restored_file.mIsPlayerMario &&
                        restored_file.mIsConfigDataCorrupted && restored_file.getPowerStarNum() == 42 &&
                        restored_file.getStarPieceNum() == 1200 && restored_file.getPlayerMissNum() == 7 &&
                        restored_file.isOnCompleteEndingMarioAndLuigi(),
                    "save service should restore original-shaped UserFile state from a generic slot snapshot");
            const auto restored_slot = restored_file.makeSaveDataServiceSlot(2);
            require(restored_slot.created && restored_slot.has_mii_id && restored_slot.rfl_mii_index == 3 &&
                        !restored_slot.icon_id.has_value() && restored_slot.power_star_num == 42,
                    "UserFile should round-trip back into generic save slot state");
            const auto persistent_save_dir = std::filesystem::path(".cache/tests/save-service-host");
            std::filesystem::remove_all(persistent_save_dir);
            auto persistent_save = smgpc::game::SaveDataService();
            persistent_save.set_host_directory(persistent_save_dir);
            auto persistent_file = UserFile();
            persistent_file.restoreFromSaveDataServiceSlot(smgpc::game::SaveDataService::SlotState{
                                                               .slot_index = 5,
                                                               .created = true,
                                                               .last_loaded_mario = true,
                                                               .power_star_num = 61,
                                                               .star_piece_num = 900,
                                                               .player_miss_num = 4,
                                                               .icon_id = 2U,
                                                               .view_normal_ending = true,
                                                               .view_complete_ending = true,
                                                               .complete_ending_mario_and_luigi = true,
                                                               .game_event_flags = {},
                                                               .game_event_values = {},
                                                               .last_modified = 987654,
                                                           },
                                                           5, true);
            persistent_save.store_user_file(5, persistent_file);
            persistent_save.set_sys_config_time_announced(44);
            persistent_save.set_sys_config_time_sent(55);
            persistent_save.set_sys_config_sent_bytes(66U);
            persistent_save.flush_host_files();
            require(std::filesystem::exists(persistent_save_dir / "GameData.bin") && std::filesystem::exists(persistent_save_dir / "config5") &&
                        std::filesystem::exists(persistent_save_dir / "mario5") && std::filesystem::exists(persistent_save_dir / "sysconf"),
                    "save service should persist an original-style GameData.bin plus original-named loose files when a host directory is configured");
            const auto container_bytes = read_binary_file_for_test(persistent_save_dir / "GameData.bin");
            require(read_be_u32(container_bytes, 4U) == 2U && read_be_u32(container_bytes, 8U) == 19U &&
                        read_be_u32(container_bytes, 12U) == 0xBE00U && read_be_u32(container_bytes, 28U) == 0x140U,
                    "save service should write Wii big-endian GameData.bin header, file count, data size, and first data offset");
            require(std::string(reinterpret_cast<const char *>(container_bytes.data() + 16U), 6U) == "mario1",
                    "save service should preserve original GameData.bin file order starting with mario1");
            const auto nand_host_container = persistent_save.read_nand_file("GameData.bin");
            require(nand_host_container.has_value() && read_le_u32(*nand_host_container, 4U) == 2U &&
                        read_le_u32(*nand_host_container, 8U) == 19U && read_le_u32(*nand_host_container, 12U) == 0xBE00U,
                    "save service NAND reads should expose GameData.bin in host byte order for source-shaped Game code");
            auto nand_write_save = smgpc::game::SaveDataService();
            nand_write_save.write_nand_file("GameData.bin", *nand_host_container);
            const auto stored_wii_container = nand_write_save.read_file("GameData.bin");
            require(stored_wii_container.has_value() && read_be_u32(*stored_wii_container, 4U) == 2U &&
                        read_be_u32(*stored_wii_container, 8U) == 19U && nand_write_save.has_valid_game_data_container(),
                    "save service NAND writes should persist Wii byte-order GameData.bin while decoding slot files generically");
            auto reloaded_save = smgpc::game::SaveDataService();
            reloaded_save.set_host_directory(persistent_save_dir);
            const auto *reloaded_slot = reloaded_save.slot_state(5);
            require(reloaded_save.has_valid_game_data_container() && reloaded_slot != nullptr && reloaded_slot->created && reloaded_slot->power_star_num == 61 &&
                        reloaded_slot->star_piece_num == 900 && reloaded_slot->icon_id == 2U &&
                        reloaded_save.sys_config_time_announced() == 44 && reloaded_save.sys_config_time_sent() == 55 &&
                        reloaded_save.sys_config_sent_bytes() == 66U && reloaded_save.file_count() >= 3U,
                    "save service should reload original-style GameData.bin into generic slot and sys-config state");
            std::filesystem::remove(persistent_save_dir / "config5");
            std::filesystem::remove(persistent_save_dir / "mario5");
            std::filesystem::remove(persistent_save_dir / "sysconf");
            auto container_only_save = smgpc::game::SaveDataService();
            container_only_save.set_host_directory(persistent_save_dir);
            const auto *container_only_slot = container_only_save.slot_state(5);
            require(container_only_save.has_valid_game_data_container() && container_only_slot != nullptr && container_only_slot->created &&
                        container_only_slot->power_star_num == 61 && container_only_slot->star_piece_num == 900 &&
                        container_only_save.sys_config_time_sent() == 55,
                    "save service should restore FileSelect save state from GameData.bin even when loose compatibility files are absent");

            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            runtime.save_data().set_slot_state(3, smgpc::game::SaveDataService::SlotState{
                                                      .created = true,
                                                      .last_loaded_mario = true,
                                                      .power_star_num = 7,
                                                      .star_piece_num = 88,
                                                      .player_miss_num = 9,
                                                      .icon_id = 5U,
                                                      .view_normal_ending = true,
                                                      .view_complete_ending = false,
                                                      .complete_ending_mario_and_luigi = false,
                                                      .game_event_flags = {},
                                                      .game_event_values = {},
                                                      .last_modified = 1234,
                                                  });
            auto sequence_file = UserFile();
            GameSequenceFunction::restoreUserFile(&sequence_file, 3, true);
            require(sequence_file.isCreated() && sequence_file.mIsPlayerMario && sequence_file.getPowerStarNum() == 7 &&
                        sequence_file.getStarPieceNum() == 88 && sequence_file.getPlayerMissNum() == 9 &&
                        GameDataFunction::getUserFileIndex() == 1,
                    "GameSequenceFunction should restore UserFile data through the runtime save service");
            GameSequenceFunction::restoreUserFile(&sequence_file, 3, false);
            require(sequence_file.isCreated() && !sequence_file.mIsPlayerMario && !sequence_file.isLastLoadedMario() &&
                        std::string_view(sequence_file.getGameDataName()) == "luigi3",
                    "GameSequenceFunction bool restore should match original last-loaded player semantics");
            GameSequenceFunction::startCreateUserFileSequence(4);
            require(GameSequenceFunction::isActiveSaveDataHandleSequence(), "create user file sequence should expose original active polling state");
            for (auto i = 0; i < 180 && GameSequenceFunction::isActiveSaveDataHandleSequence(); ++i) {
                smgpc::game::save_data_handle_sequence().update();
            }
            const auto *created_slot = runtime.save_data().slot_state(4);
            require(!GameSequenceFunction::isActiveSaveDataHandleSequence() && GameSequenceFunction::isSuccessSaveDataHandleSequence() &&
                        created_slot != nullptr && created_slot->created && created_slot->last_loaded_mario,
                    "create user file sequence should drive the original-shaped save window flow and then report success");
            const auto peach_icon = u32{5U};
            GameSequenceFunction::startSetMiiOrIconIdUserFileSequence(4, nullptr, &peach_icon);
            smgpc::game::save_data_handle_sequence().update();
            created_slot = runtime.save_data().slot_state(4);
            require(created_slot != nullptr && created_slot->created && created_slot->icon_id == 5U,
                    "set icon user file sequence should store icon IDs through original-shaped save wrappers");
            GameDataFunction::setSysConfigFileTimeSent(222);
            GameDataFunction::setSysConfigFileSentBytes(333U);
            GameDataFunction::updateSysConfigFileTimeAnnounced();
            require(GameDataFunction::getSysConfigFileTimeSent() == 222 && GameDataFunction::getSysConfigFileSentBytes() == 333U &&
                        GameDataFunction::getSysConfigFileTimeAnnounced() != 0,
                    "GameDataFunction sys-config accessors should use the runtime save service backing store");
            auto handler_file = UserFile();
            handler_file.restoreFromSaveDataServiceSlot(smgpc::game::SaveDataService::SlotState{
                                                            .slot_index = 6,
                                                            .created = true,
                                                            .last_loaded_mario = true,
                                                            .power_star_num = 22,
                                                            .star_piece_num = 222,
                                                            .player_miss_num = 2,
                                                            .icon_id = 3U,
                                                            .view_normal_ending = true,
                                                            .view_complete_ending = false,
                                                            .complete_ending_mario_and_luigi = false,
                                                            .game_event_flags = {},
                                                            .game_event_values = {},
                                                            .last_modified = 0,
                                                        },
                                                        6, true);
            auto sys_config = SysConfigFile();
            sys_config.setTimeAnnounced(444);
            sys_config.setTimeSent(555);
            sys_config.setSentBytes(666U);
            auto save_handler = SaveDataHandler(&sys_config, &handler_file);
            save_handler.storeSysConfigFile(&sys_config);
            save_handler.initializeUserFileMemory(6, &handler_file);
            require(save_handler.isDone() && save_handler.getLastResultCode().isSuccess() && runtime.save_data().exists("config6") &&
                        runtime.save_data().exists("mario6"),
                    "SaveDataHandler should expose original-shaped file store APIs over the runtime save service");
            auto config_buffer = std::vector<u8>(SaveDataHandler::getEnoughtTempBufferSize());
            auto game_buffer = std::vector<u8>(SaveDataHandler::getEnoughtTempBufferSize());
            save_handler.restoreGameDataFile("config6", config_buffer.data(), static_cast<u32>(config_buffer.size()));
            save_handler.restoreGameDataFile("mario6", game_buffer.data(), static_cast<u32>(game_buffer.size()));
            auto handler_restore_file = UserFile();
            handler_restore_file.loadFromConfigDataBinary("config6", config_buffer.data(), static_cast<u32>(config_buffer.size()));
            handler_restore_file.loadFromGameDataBinary("mario6", game_buffer.data(), static_cast<u32>(game_buffer.size()));
            require(handler_restore_file.isCreated() && handler_restore_file.getPowerStarNum() == 22 &&
                        handler_restore_file.getStarPieceNum() == 222 && !handler_restore_file.mIsGameDataCorrupted &&
                        !handler_restore_file.mIsConfigDataCorrupted,
                    "SaveDataHandler should restore original-named config/game files into UserFile binaries");
            save_handler.copyUserFileMemory(1, 6);
            save_handler.requestSaveSaveData();
            for (auto i = 0; i < 16 && !save_handler.isDone(); ++i) {
                save_handler.update();
            }
            require(runtime.save_data().slot_state(1) != nullptr && runtime.save_data().slot_state(1)->power_star_num == 22 &&
                        runtime.save_data().exists("config1") && runtime.save_data().exists("mario1"),
                    "SaveDataHandler should copy original-named slot files through the generic save backing");
            auto remove_done = false;
            require(save_handler.tryRemoveFile("mario1", &remove_done) && remove_done && !runtime.save_data().exists("mario1"),
                    "SaveDataHandler should expose original-shaped remove-file completion semantics");

            auto messages = smgpc::game::MessageService();
            messages.set_message("FileSelect_NewFile", "New File");
            require(messages.message_or("FileSelect_NewFile", "fallback") == "New File", "message service should resolve fixture messages");
            require(messages.message_or("Missing", "fallback") == "fallback", "message service should provide deterministic fallback text");

            auto scene_lights = smgpc::game::SceneLightService();
            auto light = smgpc::game::GXLightState{};
            light.color = {0x10U, 0x20U, 0x30U, 0x40U};
            light.position = {1.0F, 2.0F, 3.0F};
            light.direction = {0.0F, 0.0F, -1.0F};
            scene_lights.set_light(2U, light);
            require(scene_lights.loaded_mask() == 4U && scene_lights.light(2U) != nullptr && scene_lights.light(2U)->loaded &&
                        scene_lights.light(2U)->color == smgpc::game::GXColorValue{0x10U, 0x20U, 0x30U, 0x40U},
                    "scene light service should expose typed GX light slots without material-specific compat hooks");
            scene_lights.clear_light(2U);
            require(scene_lights.loaded_mask() == 0U && scene_lights.light(2U) == nullptr,
                    "scene light service should clear individual GX light slots");

            auto rfl = smgpc::game::RflService();
            require(rfl.is_initialized() && !rfl.has_error(), "RFL service should default to a ready deterministic Mii fixture");
            require(!rfl.valid_miis().empty() && rfl.valid_miis()[0].name == "Mario", "RFL service should expose a fixture Mii entry");
            rfl.set_error(true);
            require(rfl.has_error(), "RFL service should allow tests to force Mii errors");
        }

        $test("routes MR DVD input audio and effects through RuntimeContext services") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto begin_frame = [&](std::uint64_t frame) {
                runtime.begin_frame(smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });
            };

            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 10U,
                .frame_time_seconds = 10.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            require(!runtime.is_core_pad_button_a(WPAD_CHAN0), "runtime should route released host input through WPAD state");

            window.set_core_buttons(true, true);
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 11U,
                .frame_time_seconds = 11.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            require(runtime.is_core_pad_button_a(WPAD_CHAN0) && runtime.is_core_pad_button_b(WPAD_CHAN0),
                    "runtime should route host A+B input through WPAD core buttons");
            require(runtime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A), "runtime WPAD service should retain trigger edges");
            runtime.wpad().set_pointer(WPAD_CHAN0, 160.0F, 120.0F, true);
            runtime.wpad().set_sub_stick(WPAD_CHAN0, 0.75F, -0.6F);
            runtime.wpad().set_core_acceleration(WPAD_CHAN0, 0.0F, 1.0F, 2.0F);
            runtime.wpad().set_sub_acceleration(WPAD_CHAN0, 3.0F, 4.0F, 5.0F);
            runtime.wpad().set_distance_to_display(WPAD_CHAN0, 1.5F);
            auto kpad_status = std::array<KPADStatus, 1U>{};
            require(KPADRead(WPAD_CHAN0, kpad_status.data(), static_cast<u32>(kpad_status.size())) == 1,
                    "KPADRead should expose runtime WPAD channel state");
            require((kpad_status[0].hold & WPAD_BUTTON_A) != 0U && kpad_status[0].trig == (WPAD_BUTTON_A | WPAD_BUTTON_B) &&
                        kpad_status[0].pos.x == 160.0F && kpad_status[0].wpad_err == WPAD_ERR_NONE && kpad_status[0].dpd_valid_fg == 1,
                    "KPADRead should preserve hold bits, pointer coordinates, and success status");
            auto pointing = TVec2f{};
            MR::getCorePadPointingPosBasedOnScreen(&pointing, WPAD_CHAN0);
            require(pointing.x == 160.0F && pointing.y == 120.0F && MR::isCorePadPointInScreen(WPAD_CHAN0),
                    "MR GamePadUtil should expose core pointer state");
            auto accel = TVec3f{};
            MR::getCorePadAcceleration(&accel, WPAD_CHAN0);
            require(accel.y == 1.0F && MR::getCorePadDistanceToDisplay(WPAD_CHAN0) == 1.5F,
                    "MR GamePadUtil should expose core acceleration and pointer distance fixtures");
            require(MR::testCorePadButtonA(WPAD_CHAN0) && MR::testCorePadTriggerA(WPAD_CHAN0) && MR::testSystemPadTriggerDecide(),
                    "MR GamePadUtil should route button held/trigger/system aliases through WPAD state");
            require(MR::getSubPadStickX(WPAD_CHAN0) == 0.75F && MR::getSubPadStickY(WPAD_CHAN0) == -0.6F &&
                        MR::testSubPadStickTriggerRight(WPAD_CHAN0) && MR::testSubPadStickTriggerDown(WPAD_CHAN0),
                    "MR GamePadUtil should expose KB&M-backed nunchuk stick fixtures");
            MR::getSubPadAcceleration(&accel, WPAD_CHAN0);
            require(accel.z == 5.0F && MR::getWPadMaxCount() == static_cast<u32>(WPAD_MAX_CONTROLLERS) && MR::isConnectedWPad(WPAD_CHAN0),
                    "MR GamePadUtil should expose sub acceleration, controller count, and connected state");

            auto dvd_file = DVDFileInfo{};
            require(DVDConvertPathToEntrynum("/ObjectData/CometNearOrbitSky.arc") >= 0, "DVD path conversion should use the runtime DVD service");
            require(DVDOpen("/ObjectData/CometNearOrbitSky.arc", &dvd_file) != 0, "DVDOpen should resolve disc-relative files");
            require(DVDGetLength(&dvd_file) > 0x40U, "DVDGetLength should report mounted disc file size");
            auto dvd_magic = std::array<std::uint8_t, 4U>{};
            require(DVDReadPrio(&dvd_file, dvd_magic.data(), static_cast<s32>(dvd_magic.size()), 0, 2) == 4,
                    "DVDReadPrio should synchronously read from the runtime DVD service");
            require((dvd_magic[0] == 'Y' && dvd_magic[1] == 'a' && dvd_magic[2] == 'z' && dvd_magic[3] == '0') ||
                        (dvd_magic[0] == 'R' && dvd_magic[1] == 'A' && dvd_magic[2] == 'R' && dvd_magic[3] == 'C'),
                    "DVDReadPrio should return original archive bytes");
            require(DVDClose(&dvd_file) != 0 && dvd_file.internal == nullptr, "DVDClose should release runtime DVD file handles");

            char object_archive_path[128]{};
            require(MR::makeObjectArchiveFileNameFromPrefix(object_archive_path, sizeof(object_archive_path), "CometNearOrbitSky", false),
                    "MR FileUtil should resolve original object archive paths");
            require(std::string_view(object_archive_path) == "/ObjectData/CometNearOrbitSky.arc",
                    "MR FileUtil object archive path should stay disc-relative");
            require(MR::isFileExist(object_archive_path, false), "MR FileUtil should test disc-relative file existence");
            require(MR::getFileSize(object_archive_path, false) > 0x40U, "MR FileUtil should report original file sizes");
            require(MR::convertPathToEntrynumConsideringLanguage(object_archive_path) >= 0, "MR FileUtil should convert paths through DVD service");
            require(MR::loadToMainRAM(object_archive_path, nullptr, nullptr, JKRDvdRipper::ALLOC_DIR_TOP) != nullptr,
                    "MR FileUtil should synchronously load original files");
            require(MR::isLoadedFile(object_archive_path), "MR FileUtil should retain loaded-file state");
            auto *mounted_archive = MR::mountArchive(object_archive_path, nullptr);
            require(mounted_archive != nullptr && mounted_archive->contains("cometnearorbitsky.bdl"), "MR FileUtil should mount RARC archives");
            require(MR::isMountedArchive(object_archive_path), "MR FileUtil should retain mounted-archive state");
            require(mounted_archive->getResource("cometnearorbitsky.bdl") != nullptr, "JKR archive shim should expose resources by path");
            const auto dvd_trace_path = std::filesystem::path(".cache/tests/runtime-dvd-fileutil-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(dvd_trace_path,
                                                    smgpc::render::FrameContext{
                                                        .frame_index = 11U,
                                                        .frame_time_seconds = 11.0 / 60.0,
                                                        .frame_delta_seconds = 1.0 / 60.0,
                                                        .framebuffer = {.width = 640U, .height = 456U},
                                                        .has_focus = true,
                                                        .is_minimized = false,
                                                    },
                                                    runtime);
            const auto dvd_trace_json = smgpc::game::load_runtime_parity_trace(dvd_trace_path);
            const auto &dvd_trace = dvd_trace_json.at("runtime_services").at("dvd");
            require(dvd_trace.at("cached_archive_count") >= 1U, "runtime parity trace should serialize DVD archive cache size");
            require(std::ranges::any_of(dvd_trace.at("file_reads"),
                                        [&](const auto &entry) {
                                            return entry.at("requested_path").template get<std::string>() == object_archive_path &&
                                                   entry.at("resolved_path").template get<std::string>().ends_with("ObjectData/CometNearOrbitSky.arc") &&
                                                   entry.at("byte_count").template get<std::size_t>() > 0U;
                                        }),
                    "runtime parity trace should serialize MR FileUtil-backed DVD file reads");
            require(std::ranges::any_of(dvd_trace.at("archive_loads"),
                                        [&](const auto &entry) {
                                            return entry.at("requested_path").template get<std::string>() == object_archive_path &&
                                                   entry.at("resolved_path").template get<std::string>().ends_with("ObjectData/CometNearOrbitSky.arc") &&
                                                   !entry.at("cache_hit").template get<bool>() &&
                                                   entry.at("load_count").template get<std::size_t>() == 1U &&
                                                   entry.at("resource_count").template get<std::size_t>() > 0U;
                                        }),
                    "runtime parity trace should serialize MR FileUtil-backed archive loads with decoded resource counts");
            JKRArchive *archive_out = nullptr;
            JKRHeap *heap_out = reinterpret_cast<JKRHeap *>(std::uintptr_t{1U});
            MR::getMountedArchiveAndHeap(object_archive_path, &archive_out, &heap_out);
            require(archive_out == mounted_archive && heap_out == nullptr, "MR FileUtil should expose mounted archive handles");

            char layout_archive_path[128]{};
            require(MR::makeLayoutArchiveFileNameFromPrefix(layout_archive_path, sizeof(layout_archive_path), "TitleLogo", false),
                    "MR FileUtil should resolve localized layout archives");
            require(std::string_view(layout_archive_path) == "/KrKorean/LayoutData/TitleLogo.arc",
                    "MR FileUtil should prefer localized layout archives");
            char scenario_path[128]{};
            MR::makeScenarioArchiveFileName(scenario_path, sizeof(scenario_path), "FileSelect");
            require(std::string_view(scenario_path) == "/StageData/FileSelect/FileSelectScenario.arc",
                    "MR FileUtil should build original-style scenario archive paths");

            runtime.start_stage_bgm("MBGM_FILE_SELECT");
            require(runtime.current_stage_bgm_name() == "MBGM_FILE_SELECT", "runtime BGM facade should use the audio event service");
            require(!runtime.is_stage_bgm_prepared(), "runtime BGM should not be prepared until the next frame");
            begin_frame(12U);
            require(runtime.is_stage_bgm_prepared(), "runtime BGM preparation should be frame based");
            runtime.unlock_stage_bgm();
            require(runtime.audio().is_stage_bgm_unlocked(), "runtime should route BGM unlock to the audio service");
            MR::setStageBGMState(5, 60U);
            require(runtime.audio().stage_bgm_state() == 5 && runtime.audio().stage_bgm_state_change_frames() == 60U,
                    "MR::setStageBGMState should route original BGM state changes through the audio service");
            runtime.start_system_sound("SE_SY_CURSOR");
            MR::startSystemLevelSE("SE_SY_LEVEL_LOOP", -1, -1);
            MR::startAtmosphereSE("SE_DM_ARRIVE_CASTLE_STAR", -1, -1);
            MR::startSystemME("ME_ASTRO_DOME_SELECT1");
            MR::startCSSound("CS_DECIDE", static_cast<s32>(0), static_cast<s32>(0));
            MR::stopSystemSE("SE_SY_CURSOR", 7U);
            MR::submitLevelSE();
            MR::permitLevelSE();
            runtime.stop_stage_bgm(30);
            require(runtime.audio().events().size() == 12U &&
                        runtime.audio().events()[4U].kind == smgpc::game::AudioEventKind::SystemLevelSoundStart &&
                        runtime.audio().events()[4U].name == "SE_SY_LEVEL_LOOP" &&
                        runtime.audio().events()[5U].kind == smgpc::game::AudioEventKind::AtmosphereSoundStart &&
                        runtime.audio().events()[5U].name == "SE_DM_ARRIVE_CASTLE_STAR" &&
                        runtime.audio().events()[6U].kind == smgpc::game::AudioEventKind::SystemMEStart &&
                        runtime.audio().events()[6U].name == "ME_ASTRO_DOME_SELECT1" &&
                        runtime.audio().events()[7U].kind == smgpc::game::AudioEventKind::ControllerSpeakerSoundStart &&
                        runtime.audio().events()[7U].name == "CS_DECIDE" &&
                        runtime.audio().events()[8U].kind == smgpc::game::AudioEventKind::SystemSoundStop &&
                        runtime.audio().events()[8U].name == "SE_SY_CURSOR" &&
                        runtime.audio().events()[8U].delay_frames == 7U &&
                        runtime.audio().events()[9U].kind == smgpc::game::AudioEventKind::LevelSoundSubmit &&
                        runtime.audio().events()[10U].kind == smgpc::game::AudioEventKind::LevelSoundPermit,
                    "audio service should keep separate deterministic event records for system, level, atmosphere, ME, controller speaker, stop, and level gate calls");

            auto logo_layout = SimpleLayout("ロゴ", "TitleLogo", 2U, MR::DrawType_Layout);
            logo_layout.initEffectKeeper(1, "TitleLogo", nullptr);
            MR::emitEffect(&logo_layout, "TitleLogoLight");
            require(runtime.effects().active_effects("ロゴ").size() == 1U, "runtime should route layout effect emission to the effect service");
            require(runtime.effects().host_binding("ロゴ").has_value() &&
                        runtime.effects().host_binding("ロゴ")->source == smgpc::game::EffectHostBindingSource::SimpleLayoutOrigin,
                    "layout effect keepers should bind to generic SimpleLayout host state");
            require(runtime.effects().resource_library() != nullptr, "runtime should load original Effect.arc through the effect service");
            require(!runtime.effects().events().empty() && !runtime.effects().events().back().resolved_resources.empty(),
                    "runtime effect events should resolve emitted names to original JPA resource metadata");
            require(runtime.effects().events().back().keeper.has_value() &&
                        runtime.effects().events().back().keeper->host_kind == smgpc::game::EffectKeeperHostKind::SimpleLayout &&
                        runtime.effects().events().back().keeper->resource_group_name == "TitleLogo",
                    "runtime effect events should retain generic keeper registration context");
            require(std::ranges::any_of(runtime.effects().events().back().resolved_resources,
                                        [](const auto &resource) {
                                            return resource.particle_name == "TitleLogoLightA00" && resource.user_index == 3030U &&
                                                   resource.resource != nullptr &&
                                                   resource.resource->dynamics.has_value() &&
                                                   resource.resource->dynamics->start_frame == 250 &&
                                                   std::ranges::any_of(resource.textures, [](const auto &texture) {
                                                       return texture.width == 64U && texture.height == 64U &&
                                                              texture.format == smgpc::game::TplTextureFormat::I8 &&
                                                              texture.image.rgba.size() == static_cast<std::size_t>(texture.width) *
                                                                                               texture.height * 4U;
                                                   });
                                        }),
                    "runtime effect events should resolve TitleLogoLight through autoeffectlist.bcsv and particles.jpc");
            auto effect_renderer = RecordingRenderer();
            runtime.scheduler().execute_draw_type(effect_renderer, MR::DrawType_EffectDraw2D);
            require(effect_renderer.quad_count == 0U && runtime.effects().draw_packets().empty(),
                    "generic JPA emitters should respect BEM1 start-frame delay instead of drawing a permanent synthetic quad");
            for (auto frame = std::uint64_t{13U}; frame <= 262U; ++frame) {
                begin_frame(frame);
            }
            runtime.scheduler().execute_draw_type(effect_renderer, MR::DrawType_EffectDraw2D);
            require(effect_renderer.texture_count > 0U && effect_renderer.quad_count > 0U,
                    "generic effect draw slots should submit decoded JPA 2D particle textures");
            require(!runtime.effects().draw_packets().empty() &&
                        std::ranges::any_of(runtime.effects().draw_packets(),
                                            [](const auto &packet) {
                                                return packet.particle_name == "TitleLogoLightA00" &&
                                                       packet.draw_order == "2D" &&
                                                       packet.texture.texture_index == 102U &&
                                                       packet.texture.name == "mr_kirakira03_i" &&
                                                       packet.texture.width == 64U &&
                                                       packet.texture.height == 64U &&
                                                       packet.texture.format_name == "I8" &&
                                                       packet.particle_lifetime > 0U &&
                                                       packet.particle_scale_x > 0.0F &&
                                                       packet.particle_scale_y > 0.0F &&
                                                       packet.particle_alpha > 0.0F &&
                                                       packet.live_particle_count > 0U;
                                            }),
                    "effect draw packets should retain generic JPA emitter particle, transform, and texture metadata");
            const auto effect_trace_path = std::filesystem::path(".cache/tests/runtime-effect-active-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(effect_trace_path,
                                                    smgpc::render::FrameContext{
                                                        .frame_index = runtime.frame_index(),
                                                        .frame_time_seconds = static_cast<double>(runtime.frame_index()) / 60.0,
                                                        .frame_delta_seconds = 1.0 / 60.0,
                                                        .framebuffer = {.width = 640U, .height = 456U},
                                                        .has_focus = true,
                                                        .is_minimized = false,
                                                    },
                                                    runtime);
            const auto effect_trace_json = smgpc::game::load_runtime_parity_trace(effect_trace_path);
            require(std::ranges::any_of(effect_trace_json.at("effects").at("active_effects"),
                                        [](const auto &active) {
                                            return active.at("actor_name").template get<std::string>() == "ロゴ" &&
                                                   active.at("effect_name").template get<std::string>() == "TitleLogoLight" &&
                                                   active.at("host_binding").at("source") == "SimpleLayoutOrigin" &&
                                                   active.at("emitter_count") > 0U &&
                                                   std::ranges::any_of(active.at("emitters"), [](const auto &emitter) {
                                                       return emitter.at("particle_name").template get<std::string>() == "TitleLogoLightA00" &&
                                                              emitter.at("live_particle_count") > 0U &&
                                                              !emitter.at("particles").empty() &&
                                                              emitter.at("particles").front().at("position").size() == 3U &&
                                                              emitter.at("particles").front().at("scale").size() == 2U;
                                                   });
                                        }),
                    "runtime parity trace should expose active effect emitters and particle transform state");
            require(std::ranges::any_of(effect_trace_json.at("render_packets"),
                                        [](const auto &packet) {
                                            return packet.at("draw_pass") == "effect" &&
                                                   packet.at("particle_name").template get<std::string>() == "TitleLogoLightA00" &&
                                                   packet.at("host_binding_found") == true &&
                                                   packet.at("host_binding_source") == "SimpleLayoutOrigin" &&
                                                   packet.at("host_translation").size() == 3U &&
                                                   packet.at("particle_position").size() == 3U &&
                                                   packet.at("particle_scale").size() == 2U &&
                                                   packet.at("particle_alpha") > 0.0F;
                                        }),
                    "runtime parity trace should expose effect draw-packet particle transform metadata");
            MR::deleteEffectAll(&logo_layout);
            require(runtime.effects().active_effects("ロゴ").empty(), "runtime should route effect cleanup to the effect service");

            require(MR::isWipeOpen() && !MR::isWipeActive() && !MR::isWipeBlank(),
                    "scene wipe service should default to the original open screen state");
            MR::closeWipeFade(3);
            require(MR::isWipeActive() && !MR::isWipeBlank(), "MR::closeWipeFade should start a generic scene fade close transition");
            begin_frame(13U);
            begin_frame(14U);
            require(MR::isWipeActive(), "scene wipe should remain active until its requested frame count elapses");
            begin_frame(15U);
            require(MR::isWipeBlank() && !MR::isWipeActive(), "scene wipe should report blank after the close transition completes");
            MR::openWipeFade(2);
            begin_frame(16U);
            require(MR::isWipeActive() && !MR::isWipeOpen(), "MR::openWipeFade should start a generic scene fade open transition");
            begin_frame(17U);
            require(MR::isWipeOpen() && !MR::isWipeActive(), "scene wipe should report open after the open transition completes");
            MR::closeSystemWipeFade(1);
            require(MR::isSystemWipeActive(), "system wipe facade should route through the generic system wipe service");
            begin_frame(18U);
            require(!MR::isSystemWipeActive() && runtime.system_wipe().is_blank(), "system wipe should complete independently of scene wipe");
            MR::forceOffImageEffect();
            require(runtime.image_effects().is_forced_off() && !runtime.image_effects().is_control_auto() &&
                        runtime.image_effects().events().size() == 1U &&
                        runtime.image_effects().events().back().kind == smgpc::game::ImageEffectControlKind::ForceOff,
                    "MR::forceOffImageEffect should record generic image-effect forced-off state");
            MR::setImageEffectControlAuto();
            require(!runtime.image_effects().is_forced_off() && runtime.image_effects().is_control_auto() &&
                        runtime.image_effects().events().size() == 2U &&
                        runtime.image_effects().events().back().kind == smgpc::game::ImageEffectControlKind::ControlAuto,
                    "MR::setImageEffectControlAuto should restore generic image-effect auto control state");
            MR::requestChangeStageInGameAfterLoadingGameData();
            require(runtime.sequence_requests().is_change_stage_in_game_after_loading_game_data_requested() &&
                        runtime.sequence_requests().events().size() == 1U,
                    "MR sequence util should record stage-change requests through a generic runtime service");
            window.set_core_buttons(true, true);
            begin_frame(19U);
            MR::shakeCameraNormal();
            MR::tryRumblePadStrong(nullptr, WPAD_CHAN0);
            MR::tryRumblePadMiddle(nullptr, WPAD_CHAN1);
            MR::tryRumblePadWeak(nullptr, WPAD_CHAN2);
            runtime.emit_semantic_trace_event("test", "manual_runtime_anchor", "runtime scene test");

            const auto trace_path = std::filesystem::path(".cache/tests/runtime-wipe-service-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(trace_path,
                                                    smgpc::render::FrameContext{
                                                        .frame_index = runtime.frame_index(),
                                                        .frame_time_seconds = static_cast<double>(runtime.frame_index()) / 60.0,
                                                        .frame_delta_seconds = 1.0 / 60.0,
                                                        .framebuffer = {.width = 640U, .height = 456U},
                                                        .has_focus = true,
                                                        .is_minimized = false,
                                                    },
                                                    runtime);
            const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
            const auto &audio_trace = trace_json.at("audio");
            require(audio_trace.at("stage_bgm_active") == false && audio_trace.at("stage_bgm_prepared") == false &&
                        audio_trace.at("stage_bgm_unlocked") == true && audio_trace.at("stage_bgm_state") == 5 &&
                        audio_trace.at("stage_bgm_state_change_frames") == 60U,
                    "runtime parity trace should expose generic stage BGM active, prepared, unlocked, and state-change fields");
            require(std::ranges::any_of(audio_trace.at("events"),
                                        [](const auto &event) {
                                            return event.at("kind") == "SystemLevelSoundStart" &&
                                                   event.at("name").template get<std::string>() == "SE_SY_LEVEL_LOOP";
                                        }) &&
                        std::ranges::any_of(audio_trace.at("events"),
                                            [](const auto &event) {
                                                return event.at("kind") == "AtmosphereSoundStart" &&
                                                       event.at("name").template get<std::string>() == "SE_DM_ARRIVE_CASTLE_STAR";
                                            }) &&
                        std::ranges::any_of(audio_trace.at("events"),
                                            [](const auto &event) {
                                                return event.at("kind") == "SystemMEStart" &&
                                                       event.at("name").template get<std::string>() == "ME_ASTRO_DOME_SELECT1";
                                            }) &&
                        std::ranges::any_of(audio_trace.at("events"),
                                            [](const auto &event) {
                                                return event.at("kind") == "ControllerSpeakerSoundStart" &&
                                                       event.at("name").template get<std::string>() == "CS_DECIDE";
                                            }) &&
                        std::ranges::any_of(audio_trace.at("events"),
                                            [](const auto &event) {
                                                return event.at("kind") == "SystemSoundStop" &&
                                                       event.at("name").template get<std::string>() == "SE_SY_CURSOR" &&
                                                       event.at("delay_frames") == 7U;
                                            }) &&
                        std::ranges::any_of(audio_trace.at("events"),
                                            [](const auto &event) {
                                                return event.at("kind") == "LevelSoundSubmit";
                                            }) &&
                        std::ranges::any_of(audio_trace.at("events"),
                                            [](const auto &event) {
                                                return event.at("kind") == "LevelSoundPermit";
                                            }),
                    "runtime parity trace should preserve distinct level, atmosphere, ME, controller speaker, stop, and level gate audio event kinds");
            require(trace_json.at("runtime_services").at("wipe").at("scene").at("state") == "Open" &&
                        trace_json.at("runtime_services").at("wipe").at("scene").at("events").size() == 2U &&
                        trace_json.at("runtime_services").at("wipe").at("system").at("state") == "Closed" &&
                        trace_json.at("runtime_services").at("sequence_requests").at("change_stage_in_game_after_loading_game_data") == true,
                    "runtime parity trace should expose generic scene wipe, system wipe, and sequence request state");
            require(trace_json.at("runtime_services").at("image_effects").at("forced_off") == false &&
                        trace_json.at("runtime_services").at("image_effects").at("control_auto") == true &&
                        trace_json.at("runtime_services").at("image_effects").at("events").size() == 2U &&
                        trace_json.at("runtime_services").at("image_effects").at("events")[0U].at("kind") == "ForceOff" &&
                        trace_json.at("runtime_services").at("image_effects").at("events")[1U].at("kind") == "ControlAuto",
                    "runtime parity trace should expose generic image-effect control state and events");
            require(trace_json.at("runtime_services").at("camera").at("shake_events").size() == 1U &&
                        trace_json.at("runtime_services").at("camera").at("shake_events").front().at("kind") == "Normal" &&
                        trace_json.at("runtime_services").at("camera").at("shake_events").front().at("frame_index") == 19U,
                    "runtime parity trace should expose generic frame-stamped camera shake events");
            require(trace_json.at("runtime_services").at("rumble").at("events").size() == 3U &&
                        trace_json.at("runtime_services").at("rumble").at("events")[0U].at("kind") == "Strong" &&
                        trace_json.at("runtime_services").at("rumble").at("events")[0U].at("channel") == WPAD_CHAN0 &&
                        trace_json.at("runtime_services").at("rumble").at("events")[1U].at("kind") == "Middle" &&
                        trace_json.at("runtime_services").at("rumble").at("events")[1U].at("channel") == WPAD_CHAN1 &&
                        trace_json.at("runtime_services").at("rumble").at("events")[2U].at("kind") == "Weak" &&
                        trace_json.at("runtime_services").at("rumble").at("events")[2U].at("channel") == WPAD_CHAN2,
                    "runtime parity trace should expose generic frame-stamped rumble request events");
            require(std::ranges::any_of(trace_json.at("effects").at("registered_keepers"),
                                        [](const auto &keeper) {
                                            return keeper.at("host_kind") == "SimpleLayout" &&
                                                   keeper.at("host_name").template get<std::string>() == "ロゴ" &&
                                                   keeper.at("resource_group_name") == "TitleLogo";
                                        }) &&
                        trace_json.at("effects").at("events").back().at("keeper").at("resource_group_name") == "TitleLogo",
                    "runtime parity trace should expose registered effect keepers and event keeper context");
            const auto &semantic_events = trace_json.at("semantic_events");
            require(has_semantic_trace_event(semantic_events, "test", "manual_runtime_anchor") &&
                        has_semantic_trace_event(semantic_events, "input", "wpad_buttons_held") &&
                        has_semantic_trace_event(semantic_events, "sequence", "change_stage_in_game_after_loading_game_data"),
                    "runtime parity trace should expose durable semantic anchors for runtime, input, and sequence events");

            const auto db_path = std::filesystem::path(".cache/tests/runtime-semantic-events.sqlite");
            std::filesystem::remove(db_path);
            auto db = smgpc::sql::Database(db_path);
            db.exec("PRAGMA foreign_keys = ON");
            smgpc::trace::create_trace_sqlite_schema(db);
            const auto import_result = smgpc::trace::import_trace_ndjson_file(db, trace_path);
            require(import_result.semantic_event_count == semantic_events.size(),
                    "trace SQLite import should report imported semantic event count");
            require(sqlite_semantic_event_count(db, "sequence", "change_stage_in_game_after_loading_game_data") == 1 &&
                        sqlite_semantic_event_count(db, "input", "wpad_buttons_held") == 1,
                    "trace SQLite import should index semantic anchors by category and name");
        }

        $test("aligns semantic trace anchors from SQLite by first event frame") {
            using smgpc::dump::Json;

            const auto reference_path = std::filesystem::path(".cache/tests/semantic-anchor-reference.ndjson");
            const auto candidate_path = std::filesystem::path(".cache/tests/semantic-anchor-candidate.ndjson");
            const auto db_path = std::filesystem::path(".cache/tests/semantic-anchor-alignment.sqlite");
            std::filesystem::remove(db_path);

            const auto reference_trace = Json{
                {"schema", "smgpc-runtime-parity-trace-v1"},
                {"requested_frame", 64},
                {"frame", Json{{"index", 64}, {"framebuffer", Json{{"width", 640}, {"height", 456}}}}},
                {"render_packets", Json::array()},
                {"copy_events", Json::array()},
                {"semantic_events", Json::array({
                                        Json{{"index", 0}, {"frame_index", 30}, {"category", "title"}, {"name", "title_product_visible"}, {"stage", "Title"}, {"detail", "reference visible"}},
                                        Json{{"index", 1}, {"frame_index", 55}, {"category", "file_select"}, {"name", "file_select_selectable"}, {"stage", "FileSelect"}, {"detail", "reference selectable"}},
                                        Json{{"index", 2}, {"frame_index", 59}, {"category", "reference_only"}, {"name", "anchor"}, {"stage", "Reference"}, {"detail", "missing from candidate"}},
                                    })},
            };
            const auto candidate_trace = Json{
                {"schema", "smgpc-runtime-parity-trace-v1"},
                {"requested_frame", 70},
                {"frame", Json{{"index", 70}, {"framebuffer", Json{{"width", 640}, {"height", 456}}}}},
                {"render_packets", Json::array()},
                {"copy_events", Json::array()},
                {"semantic_events", Json::array({
                                        Json{{"index", 0}, {"frame_index", 32}, {"category", "title"}, {"name", "title_product_visible"}, {"stage", "Title"}, {"detail", "candidate visible"}},
                                        Json{{"index", 1}, {"frame_index", 57}, {"category", "file_select"}, {"name", "file_select_selectable"}, {"stage", "FileSelect"}, {"detail", "candidate selectable"}},
                                        Json{{"index", 2}, {"frame_index", 58}, {"category", "file_select"}, {"name", "file_select_selectable"}, {"stage", "FileSelect"}, {"detail", "repeat should count"}},
                                        Json{{"index", 3}, {"frame_index", 61}, {"category", "candidate_only"}, {"name", "anchor"}, {"stage", "Candidate"}, {"detail", "missing from reference"}},
                                    })},
            };

            smgpc::trace::write_trace_ndjson_file(reference_path, reference_trace, "dolphin");
            smgpc::trace::write_trace_ndjson_file(candidate_path, candidate_trace, "pc-port");

            auto db = smgpc::sql::Database(db_path);
            db.exec("PRAGMA foreign_keys = ON");
            smgpc::trace::create_trace_sqlite_schema(db);
            const auto reference_import = smgpc::trace::import_trace_ndjson_file(db, reference_path);
            const auto candidate_import = smgpc::trace::import_trace_ndjson_file(db, candidate_path);

            const auto summaries = smgpc::trace::load_trace_summaries(db);
            require(summaries.size() == 2U && summaries[0].semantic_event_count == 3 && summaries[1].semantic_event_count == 4,
                    "trace summaries should expose semantic-event counts for alignment diagnostics");

            const auto anchors = smgpc::trace::load_semantic_anchor_alignments(db, reference_import.trace_id, candidate_import.trace_id, 16);
            const auto *title = find_semantic_anchor_alignment(anchors, "title", "title_product_visible");
            require(title != nullptr && title->reference_frame_index.value_or(-1) == 30 &&
                        title->candidate_frame_index.value_or(-1) == 32 && title->frame_delta.value_or(0) == 2 &&
                        title->reference_count == 1 && title->candidate_count == 1 &&
                        title->reference_stage.value_or("") == "Title" && title->candidate_stage.value_or("") == "Title",
                    "semantic anchor alignment should report shared anchor frames, delta, counts, and stages");

            const auto *file_select = find_semantic_anchor_alignment(anchors, "file_select", "file_select_selectable");
            require(file_select != nullptr && file_select->frame_delta.value_or(0) == 2 &&
                        file_select->reference_count == 1 && file_select->candidate_count == 2,
                    "semantic anchor alignment should count repeated anchors while using the first event frame");

            const auto *reference_only = find_semantic_anchor_alignment(anchors, "reference_only", "anchor");
            const auto *candidate_only = find_semantic_anchor_alignment(anchors, "candidate_only", "anchor");
            require(reference_only != nullptr && reference_only->reference_count == 1 && reference_only->candidate_count == 0 &&
                        reference_only->reference_frame_index.has_value() && !reference_only->candidate_frame_index.has_value() &&
                        candidate_only != nullptr && candidate_only->reference_count == 0 && candidate_only->candidate_count == 1 &&
                        !candidate_only->reference_frame_index.has_value() && candidate_only->candidate_frame_index.has_value(),
                    "semantic anchor alignment should surface anchors missing from either trace");
        }

        $test("imports layout runtime trace state into SQLite diagnostics") {
            using smgpc::dump::Json;

            const auto reference_path = std::filesystem::path(".cache/tests/layout-runtime-reference.ndjson");
            const auto candidate_path = std::filesystem::path(".cache/tests/layout-runtime-candidate.ndjson");
            const auto db_path = std::filesystem::path(".cache/tests/layout-runtime-diagnostics.sqlite");
            std::filesystem::remove(db_path);

            const auto make_trace = [](std::int64_t requested_frame) {
                auto trace = Json::object();
                trace["schema"] = "smgpc-runtime-parity-trace-v1";
                trace["requested_frame"] = requested_frame;
                trace["frame"] = Json{{"index", requested_frame}, {"framebuffer", Json{{"width", 640}, {"height", 456}}}};
                trace["render_packets"] = Json::array();
                trace["copy_events"] = Json::array();
                trace["semantic_events"] = Json::array();
                trace["layout_runtime"] = Json::array();
                return trace;
            };
            const auto make_pane = [](std::int64_t index, const char *name, bool effective_visible) {
                auto pane = Json::object();
                pane["index"] = index;
                pane["name"] = name;
                pane["parent_index"] = index == 0 ? -1 : 0;
                pane["base_visible"] = true;
                pane["effective_visible"] = effective_visible;
                pane["alpha"] = effective_visible ? 255 : 0;
                pane["width"] = index == 0 ? 608 : 512;
                pane["height"] = index == 0 ? 456 : 96;
                pane["contents"] = Json::array();
                return pane;
            };
            const auto make_layout = [](std::int64_t index, const char *name, const char *layout_name, std::int64_t pane_count,
                                        std::int64_t material_count, std::int64_t texture_count, bool suspended) {
                auto layout = Json::object();
                layout["index"] = index;
                layout["name"] = name;
                layout["layout_name"] = layout_name;
                layout["archive_path"] = "LayoutData/TitleLogo.arc";
                layout["movement_type"] = 1;
                layout["calc_anim_type"] = 2;
                layout["draw_type"] = 3;
                layout["order"] = 4;
                layout["dead"] = false;
                layout["suspended"] = suspended;
                layout["pane_count"] = pane_count;
                layout["picture_count"] = pane_count > 1 ? 1 : 0;
                layout["text_box_count"] = 0;
                layout["material_count"] = material_count;
                layout["texture_count"] = texture_count;
                layout["font_count"] = 0;
                layout["committed_pane_frame_count"] = pane_count;
                layout["animations"] = Json::array();
                layout["panes"] = Json::array();
                layout["materials"] = Json::array();
                layout["textures"] = Json::array();
                return layout;
            };
            const auto make_material = [](std::int64_t texture_count) {
                auto material = Json::object();
                material["index"] = 0;
                material["name"] = "PicLogoGalaxy";
                material["texture_count"] = texture_count;
                material["tex_coord_gen_count"] = texture_count;
                material["tev_stage_count"] = texture_count;
                material["alpha_compare_enabled"] = true;
                material["blend_enabled"] = true;
                material["textures"] = Json::array();
                return material;
            };
            const auto make_texture = [](std::int64_t index, const char *name, const char *format, std::int64_t format_raw) {
                auto texture = Json::object();
                texture["index"] = index;
                texture["name"] = name;
                texture["width"] = 512;
                texture["height"] = 96;
                texture["format_raw"] = format_raw;
                texture["format"] = format;
                texture["uploaded"] = true;
                texture["rgba_byte_count"] = 196608;
                return texture;
            };

            auto reference_trace = make_trace(80);
            auto reference_title = make_layout(0, "TitleLogoProbe", "TitleLogo", 2, 1, 1, false);
            reference_title["animations"].push_back(Json{{"name", "Appear"}, {"frame", 1}});
            reference_title["panes"].push_back(make_pane(0, "RootPane", true));
            auto reference_logo_pane = make_pane(1, "LogoPane", true);
            reference_logo_pane["alpha"] = 192;
            reference_logo_pane["contents"].push_back(Json{{"kind", "picture"},
                                                           {"material_index", 0},
                                                           {"material_name", "PicLogoGalaxy"},
                                                           {"texture_name", "MyTitleSpaceKOR.tpl"}});
            reference_title["panes"].push_back(reference_logo_pane);
            auto reference_material = make_material(1);
            reference_material["textures"].push_back(Json{{"slot", 0},
                                                          {"texture_index", 0},
                                                          {"texture_name", "MyTitleSpaceKOR.tpl"},
                                                          {"wrap_s", 1},
                                                          {"wrap_t", 0},
                                                          {"min_filter", 0},
                                                          {"mag_filter", 0}});
            reference_title["materials"].push_back(reference_material);
            reference_title["textures"].push_back(make_texture(0, "MyTitleSpaceKOR.tpl", "RGB565", 4));
            reference_trace["layout_runtime"].push_back(reference_title);
            auto reference_only_layout = make_layout(1, "ReferenceOnlyLayout", "SaveBanner", 1, 0, 0, false);
            reference_only_layout["panes"].push_back(make_pane(0, "OnlyPane", true));
            reference_trace["layout_runtime"].push_back(reference_only_layout);

            auto candidate_trace = make_trace(84);
            auto candidate_title = make_layout(0, "TitleLogoProbe", "TitleLogo", 3, 1, 2, true);
            candidate_title["panes"].push_back(make_pane(0, "RootPane", true));
            candidate_title["panes"].push_back(make_pane(1, "LogoPane", true));
            candidate_title["panes"].push_back(make_pane(2, "ExtraPane", false));
            auto candidate_material = make_material(2);
            candidate_material["textures"].push_back(Json{{"slot", 0},
                                                          {"texture_index", 0},
                                                          {"texture_name", "MyTitleSpaceKOR.tpl"},
                                                          {"wrap_s", 1},
                                                          {"wrap_t", 0},
                                                          {"min_filter", 0},
                                                          {"mag_filter", 0}});
            candidate_material["textures"].push_back(Json{{"slot", 1},
                                                          {"texture_index", 1},
                                                          {"texture_name", "MyTitleMaskKOR.tpl"},
                                                          {"wrap_s", 0},
                                                          {"wrap_t", 0},
                                                          {"min_filter", 0},
                                                          {"mag_filter", 0}});
            candidate_title["materials"].push_back(candidate_material);
            candidate_title["textures"].push_back(make_texture(0, "MyTitleSpaceKOR.tpl", "RGB565", 4));
            candidate_title["textures"].push_back(make_texture(1, "MyTitleMaskKOR.tpl", "IA8", 14));
            candidate_trace["layout_runtime"].push_back(candidate_title);
            auto candidate_only_layout = make_layout(1, "CandidateOnlyLayout", "FileSelectInfo", 1, 0, 0, false);
            candidate_only_layout["panes"].push_back(make_pane(0, "OnlyPane", true));
            candidate_trace["layout_runtime"].push_back(candidate_only_layout);

            smgpc::trace::write_trace_ndjson_file(reference_path, reference_trace, "dolphin");
            smgpc::trace::write_trace_ndjson_file(candidate_path, candidate_trace, "pc-port");

            auto db = smgpc::sql::Database(db_path);
            db.exec("PRAGMA foreign_keys = ON");
            smgpc::trace::create_trace_sqlite_schema(db);
            const auto reference_import = smgpc::trace::import_trace_ndjson_file(db, reference_path);
            const auto candidate_import = smgpc::trace::import_trace_ndjson_file(db, candidate_path);

            require(reference_import.layout_runtime_count == 2 && reference_import.layout_pane_count == 3 &&
                        reference_import.layout_material_count == 1 && reference_import.layout_texture_count == 1 &&
                        candidate_import.layout_runtime_count == 2 && candidate_import.layout_pane_count == 4 &&
                        candidate_import.layout_material_count == 1 && candidate_import.layout_texture_count == 2,
                    "trace SQLite import should report normalized layout runtime row counts");

            auto select_logo_panes = smgpc::sql::Statement(db, "SELECT count(*) FROM layout_runtime_panes WHERE name = ?");
            select_logo_panes.bind(1, "LogoPane");
            require(select_logo_panes.step() && select_logo_panes.column_int(0).value_or(0) == 2,
                    "layout_runtime_panes should index pane names from both traces");

            auto select_textures = smgpc::sql::Statement(db, "SELECT count(*) FROM layout_runtime_textures WHERE format = ?");
            select_textures.bind(1, "RGB565");
            require(select_textures.step() && select_textures.column_int(0).value_or(0) == 2,
                    "layout_runtime_textures should index decoded texture metadata from both traces");

            const auto summaries = smgpc::trace::load_trace_summaries(db);
            require(summaries.size() == 2U && summaries[0].layout_runtime_count == 2 && summaries[1].layout_runtime_count == 2,
                    "trace summaries should expose layout runtime counts for compare diagnostics");

            const auto diffs = smgpc::trace::load_layout_runtime_diffs(db, reference_import.trace_id, candidate_import.trace_id, 16);
            const auto *title_logo = find_layout_runtime_diff(diffs, "TitleLogoProbe", "TitleLogo");
            require(title_logo != nullptr && title_logo->reference_layout_count == 1 && title_logo->candidate_layout_count == 1 &&
                        title_logo->reference_suspended_count == 0 && title_logo->candidate_suspended_count == 1 &&
                        title_logo->reference_pane_count == 2 && title_logo->candidate_pane_count == 3 &&
                        title_logo->reference_material_count == 1 && title_logo->candidate_material_count == 1 &&
                        title_logo->reference_texture_count == 1 && title_logo->candidate_texture_count == 2,
                    "layout runtime diffs should report shared layout pane/material/texture and state deltas");

            const auto *reference_only = find_layout_runtime_diff(diffs, "ReferenceOnlyLayout", "SaveBanner");
            const auto *candidate_only = find_layout_runtime_diff(diffs, "CandidateOnlyLayout", "FileSelectInfo");
            require(reference_only != nullptr && reference_only->reference_layout_count == 1 &&
                        reference_only->candidate_layout_count == 0 && candidate_only != nullptr &&
                        candidate_only->reference_layout_count == 0 && candidate_only->candidate_layout_count == 1,
                    "layout runtime diffs should surface layouts missing from either trace");
        }

        $test("boots current FileSelector host through sequence stage host service") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto sequence_services = SequenceServiceFixture(runtime);
            auto &sequence_boot = sequence_services.sequence_boot;

            require(smgpc::game::can_create_name_obj("FileSelector") && smgpc::game::can_create_name_obj("PrologueDirector") &&
                        !smgpc::game::can_create_name_obj("ProloguePictureBook") &&
                        !smgpc::game::can_create_name_obj("MissingStageObject"),
                    "compat NameObj factory should expose source stage roots while leaving ProloguePictureBook owned by PrologueDirector");
            const auto file_select_placement = smgpc::game::resolve_stage_root_placement(runtime.dvd(), "FileSelect", 1);
            require(file_select_placement.has_value() && file_select_placement->object_name == "FileSelector" &&
                        file_select_placement->l_id == 2,
                    "stage placement resolver should discover FileSelector from original FileSelect objinfo");
            require(file_select_placement->jmap_info.dataExists() && file_select_placement->jmap_info.getNumEntries() == 3 &&
                        file_select_placement->jmap_entry_index == 1,
                    "stage placement resolver should keep original BCSV JMapInfo row backing for source init paths");
            const auto placement_iter = JMapInfoIter(&file_select_placement->jmap_info, file_select_placement->jmap_entry_index);
            const char *placement_name = nullptr;
            auto placement_l_id = s32{};
            auto placement_arg0 = s32{};
            require(placement_iter.getValue("name", &placement_name) && std::string_view(placement_name) == "FileSelector" &&
                        placement_iter.getValue("l_id", &placement_l_id) && placement_l_id == 2 &&
                        placement_iter.getValue("Obj_arg0", &placement_arg0) && placement_arg0 == file_select_placement->object_args[0U],
                    "placement-backed JMapInfoIter should expose original objinfo fields to source init paths");
            auto jmap_arg0 = s32{};
            const auto has_arg0 = MR::getJMapInfoArg0WithInit(placement_iter, &jmap_arg0);
            require(MR::isValidInfo(placement_iter) && MR::isObjectName(placement_iter, "FileSelector") &&
                        MR::getJMapInfoLinkID(placement_iter, &placement_l_id) && placement_l_id == 2 &&
                        has_arg0 == (file_select_placement->object_args[0U] != -1) && jmap_arg0 == file_select_placement->object_args[0U],
                    "MR JMapUtil compatibility should expose object names, link IDs, and source Obj_arg sentinel behavior from placement iterators");
            if (file_select_placement->has_translation && file_select_placement->has_rotation && file_select_placement->has_scale) {
                auto trans = TVec3f{};
                auto rotate = TVec3f{};
                auto scale = TVec3f{};
                require(MR::getJMapInfoTrans(placement_iter, &trans) && MR::getJMapInfoRotate(placement_iter, &rotate) &&
                            MR::getJMapInfoScale(placement_iter, &scale),
                        "MR JMapUtil compatibility should expose standard placement transforms when present");
                require_near(trans.x, file_select_placement->translation[0U], 0.001F, "placement iterator should expose pos_x");
                require_near(rotate.y, file_select_placement->rotation[1U], 0.001F, "placement iterator should expose dir_y");
                require_near(scale.z, file_select_placement->scale[2U], 0.001F, "placement iterator should expose scale_z");
            }
            const auto file_select_placements = smgpc::game::resolve_stage_root_placements(runtime.dvd(), "FileSelect", 1);
            require(file_select_placements.size() == 1U && file_select_placements.front().object_name == "FileSelector",
                    "stage placement resolver should return every currently supported FileSelect placement row");
            sequence_boot.request_boot_to_initial_stage();
            require(sequence_boot.is_boot_requested() && sequence_services.scene_controller.has_pending_request(),
                    "SequenceBootService should queue the FileSelect stage request through the scene controller");
            sequence_boot.update_after_runtime_frame();
            require(sequence_boot.is_boot_requested() && sequence_boot.is_initial_stage_host_active(),
                    "SequenceBootService should apply the queued FileSelect stage host through the scene controller update");
            const auto snapshot = runtime.scheduler().snapshot();
            require(std::ranges::any_of(snapshot,
                                        [](const auto &entry) {
                                            return entry.kind == smgpc::game::SceneEntryKind::NameObj &&
                                                   entry.name == "FileSelector";
                                        }),
                    "sequence boot host should register FileSelector from original stage placement through the runtime scheduler");

            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 1U,
                .frame_time_seconds = 1.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            sequence_boot.update_after_runtime_frame();
            require(sequence_boot.has_sent_autorush_begin(), "sequence boot host should send the same autorush message as the old app harness");

#ifndef NDEBUG
            const auto trace_path = std::filesystem::path(".cache/tests/sequence-boot-service-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(trace_path,
                                                    smgpc::render::FrameContext{
                                                        .frame_index = runtime.frame_index(),
                                                        .frame_time_seconds = static_cast<double>(runtime.frame_index()) / 60.0,
                                                        .frame_delta_seconds = 1.0 / 60.0,
                                                        .framebuffer = {.width = 640U, .height = 456U},
                                                        .has_focus = true,
                                                        .is_minimized = false,
                                                    },
                                                    runtime);
            const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
            const auto &semantic_events = trace_json.at("semantic_events");
            require(has_semantic_trace_event(semantic_events, "sequence", "boot_stage_requested") &&
                        has_semantic_trace_event(semantic_events, "sequence", "stage_host_constructed") &&
                        has_semantic_trace_event(semantic_events, "sequence", "stage_host_initialized") &&
                        has_semantic_trace_event(semantic_events, "sequence", "autorush_begin_sent") &&
                        has_semantic_trace_event_detail_containing(semantic_events, "sequence_state", "stage_requested",
                                                                   "current_stage=FileSelect") &&
                        has_semantic_trace_event_detail_containing(semantic_events, "sequence_state", "autorush_begin_sent",
                                                                   "message=ACTMES_AUTORUSH_BEGIN"),
                    "runtime trace should identify the generic stage-host boot path and autorush message");
#endif
        }

        $test("constructs runtime and game system through application service graph") {
#ifndef NDEBUG
            const auto graph_path = std::filesystem::path(".cache/tests/app-service-graph.dot");
            std::filesystem::create_directories(graph_path.parent_path());
            std::filesystem::remove(graph_path);
            const auto graph_path_text = graph_path.string();
            auto graph_dump = ScopedEnvironmentVariable("SMGPC_DI_GRAPHVIZ_PATH", graph_path_text.c_str());
#endif
            auto overrides = smgpc::app::ServiceGraphOverrides{
                .logger = std::make_unique<NullLogger>(),
                .window_service = std::make_unique<TestWindowService>(),
            };
            auto services = smgpc::app::build_service_graph(smgpc::app::BootstrapConfiguration{}, std::move(overrides));

            require(services.has<smgpc::game::RuntimeContext>() && services.has<smgpc::game::SequenceBootService>() &&
                        services.has<smgpc::game::GameSystemService>() &&
                        services.has<smgpc::game::GameSystemSceneControllerService>() &&
                        services.has<smgpc::game::StorySequenceService>() && services.has<smgpc::game::StageHostService>() &&
                        services.has<smgpc::game::DvdFileSystemService>() && services.has<smgpc::game::SaveDataService>() &&
                        services.has<smgpc::game::SceneScheduler>() && services.has<smgpc::game::SceneLifecycleService>() &&
                        services.has<smgpc::app::IApplication>(),
                    "application service graph should register runtime, runtime subservices, sequence subservices, sequence boot, game system, and app services");

#ifndef NDEBUG
            require(std::filesystem::exists(graph_path), "application service graph should emit a Graphviz dependency artifact when requested");
            const auto graph_bytes = read_binary_file_for_test(graph_path);
            const auto graph_text = std::string(graph_bytes.begin(), graph_bytes.end());
            auto graph_contains = [&](std::string_view text) {
                return graph_text.find(std::string(text)) != std::string::npos;
            };
            require(graph_contains("SMG PC Dependency Graph") && graph_contains("RuntimeContext") &&
                        graph_contains("StorySequenceService") && graph_contains("StageHostService") &&
                        graph_contains("SequenceBootService") && graph_contains("GameSystemSceneControllerService") &&
                        graph_contains("GameSystemService"),
                    "Graphviz dependency artifact should render the runtime sequence, scene controller, and game system services as named graph nodes");
            require(graph_contains("\"smgpc::game::RuntimeContext\" -> \"smgpc::game::StorySequenceService\"") &&
                        graph_contains("\"smgpc::game::RuntimeContext\" -> \"smgpc::game::GameSystemSceneControllerService\"") &&
                        graph_contains("\"smgpc::game::RuntimeContext\" -> \"smgpc::game::SaveDataService\"") &&
                        graph_contains("\"smgpc::game::RuntimeContext\" -> \"smgpc::game::SceneScheduler\"") &&
                        graph_contains("\"smgpc::game::RuntimeContext\" -> \"smgpc::game::SceneLifecycleService\"") &&
                        graph_contains("\"smgpc::game::SceneLifecycleService\" -> \"smgpc::game::GameSystemSceneControllerService\"") &&
                        graph_contains("\"smgpc::game::GameSystemSceneControllerService\" -> \"smgpc::game::StageHostService\"") &&
                        graph_contains("\"smgpc::game::StorySequenceService\" -> \"smgpc::game::SequenceBootService\"") &&
                        graph_contains("\"smgpc::game::StageHostService\" -> \"smgpc::game::SequenceBootService\"") &&
                        graph_contains("\"smgpc::game::GameSystemSceneControllerService\" -> \"smgpc::game::GameSystemService\"") &&
                        graph_contains("\"smgpc::game::SequenceBootService\" -> \"smgpc::game::GameSystemService\"") &&
                        graph_contains("\"smgpc::game::GameSystemService\" -> \"smgpc::app::IApplication\""),
                    "Graphviz dependency artifact should contain real DI dependency edges from providers to consumers");
#endif

            auto &runtime = services.get<smgpc::game::RuntimeContext>();
            require(&services.get<smgpc::game::DvdFileSystemService>() == &runtime.dvd() &&
                        &services.get<smgpc::game::SaveDataService>() == &runtime.save_data() &&
                        &services.get<smgpc::game::SceneScheduler>() == &runtime.scheduler() &&
                        &services.get<smgpc::game::SceneLifecycleService>() == &runtime.scene_lifecycle(),
                    "DI runtime subservice registrations should borrow the active RuntimeContext services instead of constructing duplicate services");
            auto &game_system = services.get<smgpc::game::GameSystemService>();
            game_system.begin_frame(smgpc::render::FrameContext{
                .frame_index = 1U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 0.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            game_system.update();

            require(game_system.has_boot_requested_initial_stage() && game_system.is_initial_stage_host_active() &&
                        game_system.has_sent_autorush_begin(),
                    "DI-owned GameSystemService should request the initial FileSelect stage and update sequence messages through its DI-owned RuntimeContext");
            require(runtime.scene_lifecycle().active_stage_name() == "FileSelect" && runtime.scene_lifecycle().active_scenario_no() == 1,
                    "DI-owned RuntimeContext should own the active FileSelect scene lifecycle state");

            const auto snapshot = runtime.scheduler().snapshot();
            require(std::ranges::any_of(snapshot,
                                        [](const auto &entry) {
                                            return entry.kind == smgpc::game::SceneEntryKind::NameObj &&
                                                   entry.name == "FileSelector";
                                        }),
                    "DI-owned boot path should instantiate FileSelector through placement/factory into the shared runtime scheduler");
        }

        $test("routes source FileSelector demo request to source-backed PrologueDirector host") {
            auto make_frame = [](std::uint64_t frame) {
                return smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
            };

            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto sequence_services = SequenceServiceFixture(runtime);
            auto &sequence_boot = sequence_services.sequence_boot;
            auto advance_one = [&]() {
                runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
                sequence_boot.update_after_runtime_frame();
            };
            auto has_active_prologue_layout = [&]() {
                const auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
                return std::ranges::any_of(layouts, [](const auto &layout) {
                    return layout.layout_name == "PrologueDemo" && !layout.dead && !layout.suspended;
                });
            };

            runtime.save_data().set_slot_state(2, smgpc::game::SaveDataService::SlotState{
                                                      .created = true,
                                                      .last_loaded_mario = true,
                                                      .power_star_num = 3,
                                                      .star_piece_num = 40,
                                                      .icon_id = 1U,
                                                      .view_normal_ending = false,
                                                      .view_complete_ending = false,
                                                      .complete_ending_mario_and_luigi = false,
                                                      .game_event_flags = {},
                                                      .game_event_values = {},
                                                      .last_modified = 0,
                                                  });
            sequence_boot.request_boot_to_initial_stage();
            sequence_boot.update_after_runtime_frame();
            auto selector = FileSelector("ファイルセレクタ");
            selector.initWithoutIter();
            require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector picturebook route test should begin from the title autorush gate");

            for (std::uint64_t frame = 0; frame < 720U && !selector.isFileSelectStarted(); ++frame) {
                window.set_core_buttons(frame >= 160U, frame >= 160U);
                runtime.begin_frame(make_frame(frame));
                sequence_boot.update_after_runtime_frame();
            }
            window.set_core_buttons(false, false);
            require(selector.isFileSelect() && selector.didAppearAllIndex(), "FileSelector picturebook route test should reach original FileSelect");

            auto *item = selector.getItemByFileNo(2);
            require(item != nullptr && !item->isNew() && runtime.scene_camera_pose().has_value(),
                    "FileSelector picturebook route test should use an existing save slot and active FileSelect camera");
            const auto &item_position = item->getPosition();
            const auto item_pointer = project_world_to_screen(*runtime.scene_camera_pose(),
                                                              {.x = item_position.x, .y = item_position.y + 900.0F, .z = item_position.z});
            window.set_pointer(item_pointer.x, item_pointer.y, true);
            advance_one();
            advance_one();
            require(selector.getPreviousPointingFileNo() == 2 && selector.wasItemPointed(2),
                    "FileSelector picturebook route test should point at the deterministic save slot through star-pointer projection");

            window.set_input_pressed(smgpc::render::InputButton::CORE_PAD_A, true);
            advance_one();
            window.set_input_pressed(smgpc::render::InputButton::CORE_PAD_A, false);
            for (auto i = 0; i < 80 && !selector.isFileConfirm(); ++i) {
                advance_one();
            }
            require(selector.isFileConfirm() && selector.getSelectedFileNo() == 2,
                    "FileSelector picturebook route test should confirm the deterministic save slot before starting");

            selector.callbackStart();
            for (auto i = 0; i < 180 && !has_active_prologue_layout(); ++i) {
                advance_one();
            }

            require(selector.didStartDemoStartWait() && selector.didStartDemo() &&
                        runtime.sequence_requests().is_change_stage_in_game_after_loading_game_data_requested(),
                    "source FileSelector Demo should request the original stage change after loading save data");
            require(runtime.current_stage_name() == "PeachCastleGardenGalaxy" &&
                        runtime.next_sequence_scene_name() == "PeachCastleGardenGalaxy",
                    "story sequence compatibility should resolve after-loading prologue to PeachCastleGardenGalaxy like the root flow");
            require(has_active_prologue_layout(),
                    "story prologue host should appear the source-backed PrologueDemo layout");

#ifndef NDEBUG
            const auto trace_path = std::filesystem::path(".cache/tests/picturebook-reached-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(trace_path,
                                                    smgpc::render::FrameContext{
                                                        .frame_index = runtime.frame_index(),
                                                        .frame_time_seconds = static_cast<double>(runtime.frame_index()) / 60.0,
                                                        .frame_delta_seconds = 1.0 / 60.0,
                                                        .framebuffer = {.width = 640U, .height = 456U},
                                                        .has_focus = true,
                                                        .is_minimized = false,
                                                    },
                                                    runtime);
            const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
            const auto &semantic_events = trace_json.at("semantic_events");
            const auto &layout_entries = trace_json.at("layout_runtime");
            require(has_semantic_trace_event_detail_containing(semantic_events, "story", "story_stage_prepared",
                                                               "event=cDemoPrologue") &&
                        std::ranges::any_of(layout_entries,
                                            [](const auto &layout) {
                                                return layout.at("layout_name") == "PrologueDemo" &&
                                                       layout.at("dead") == false && layout.at("suspended") == false;
                                            }),
                    "runtime trace should expose story prologue preparation and active PrologueDemo layout through generic trace state");
#endif
        }

        $test("pauses source ProloguePictureBook page waits through J3DFrameCtrl compatibility") {
            auto make_frame = [](std::uint64_t frame) {
                return smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
            };

            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto picturebook = ProloguePictureBook();
            picturebook.initWithoutIter();
            picturebook.appear();

            auto frame = std::uint64_t{0};
            auto advance_one = [&]() {
                runtime.begin_frame(make_frame(frame));
                ++frame;
            };
            auto prologue_anim = [&]() -> std::optional<smgpc::game::SceneLayoutAnimationDebugState> {
                const auto snapshot = runtime.scheduler().debug_layout_runtime_snapshot();
                for (const auto& layout : snapshot) {
                    if (layout.name != "プロローグの絵本" || layout.layout_name != "PrologueDemo" || layout.dead || layout.animations.empty()) {
                        continue;
                    }
                    return layout.animations.front();
                }
                return std::nullopt;
            };
            auto is_page_wait = [&]() {
                const auto anim = prologue_anim();
                return anim.has_value() && anim->name == "Prologue" && anim->stopped && anim->rate == 0.0F && anim->frame >= 349.0F &&
                       anim->frame <= 351.0F;
            };
            auto is_a_button_open = [&]() {
                const auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
                return std::ranges::any_of(layouts, [](const auto& layout) {
                    return layout.name == "Aボタンアイコン" && layout.layout_name == "IconAButton" && !layout.dead;
                });
            };

            for (auto i = 0; i < 420 && !is_page_wait(); ++i) {
                advance_one();
            }

            require(is_page_wait(), "ProloguePictureBook should pause the Prologue BRLAN through direct J3DFrameCtrl rate writes");
            require(is_a_button_open(), "ProloguePictureBook key wait should open the source A-button icon");

            window.set_input_pressed(smgpc::render::InputButton::CORE_PAD_A, true);
            advance_one();
            window.set_input_pressed(smgpc::render::InputButton::CORE_PAD_A, false);
            for (auto i = 0; i < 20 && is_page_wait(); ++i) {
                advance_one();
            }

            require(!is_page_wait(), "A trigger should resume the Prologue BRLAN after the page wait");
            require(has_system_sound_event_prefix(runtime, "SE_SY_TALK_FOCUS_ITEM"),
                    "ProloguePictureBook page advance should play the original focus-item system sound");
        }

#ifndef NDEBUG
        $test("emits generic sequence host anchors through sequence boot service") {
            auto make_frame = [](std::uint64_t frame) {
                return smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
            };
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto sequence_services = SequenceServiceFixture(runtime);
            auto &sequence_boot = sequence_services.sequence_boot;

            sequence_boot.request_boot_to_initial_stage();
            for (std::uint64_t frame = 0; frame < 720U; ++frame) {
                window.set_core_buttons(frame >= 160U, frame >= 160U);
                runtime.begin_frame(make_frame(frame));
                sequence_boot.update_after_runtime_frame();
            }

            const auto trace_path = std::filesystem::path(".cache/tests/title-semantic-anchors-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(trace_path,
                                                    smgpc::render::FrameContext{
                                                        .frame_index = runtime.frame_index(),
                                                        .frame_time_seconds = static_cast<double>(runtime.frame_index()) / 60.0,
                                                        .frame_delta_seconds = 1.0 / 60.0,
                                                        .framebuffer = {.width = 640U, .height = 456U},
                                                        .has_focus = true,
                                                        .is_minimized = false,
                                                    },
                                                    runtime);
            const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
            const auto &semantic_events = trace_json.at("semantic_events");
            require(has_semantic_trace_event(semantic_events, "sequence", "boot_stage_requested") &&
                        has_semantic_trace_event(semantic_events, "sequence", "stage_host_constructed") &&
                        has_semantic_trace_event(semantic_events, "sequence", "stage_host_initialized") &&
                        has_semantic_trace_event(semantic_events, "sequence", "autorush_begin_sent") &&
                        has_semantic_trace_event(semantic_events, "input", "wpad_buttons_held") &&
                        has_semantic_trace_event_detail_containing(semantic_events, "sequence_state", "stage_requested",
                                                                   "requested_stage=FileSelect"),
                    "sequence boot host should expose only generic stage-host and input anchors from compat");
        }
#endif

        $test("emits generic sequence state anchors for trace frame draw phases") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto renderer = RecordingRenderer();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            runtime.set_current_sequence_scene_name("Game");
            runtime.set_next_sequence_scene_name("FileSelect");
            runtime.set_current_stage_name("FileSelect");
            runtime.set_j3d_packet_trace_frame(7U);

            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 7U,
                .frame_time_seconds = 7.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            runtime.emit_sequence_state_trace_event("manual_sequence_state_anchor", "requested_stage=FileSelect");
            runtime.draw_3d_normal(renderer, title_test_camera_pose());
            runtime.draw_2d_normal(renderer);

            const auto trace_path = std::filesystem::path(".cache/tests/sequence-state-anchors-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(trace_path,
                                                    smgpc::render::FrameContext{
                                                        .frame_index = runtime.frame_index(),
                                                        .frame_time_seconds = static_cast<double>(runtime.frame_index()) / 60.0,
                                                        .frame_delta_seconds = 1.0 / 60.0,
                                                        .framebuffer = {.width = 640U, .height = 456U},
                                                        .has_focus = true,
                                                        .is_minimized = false,
                                                    },
                                                    runtime);
            const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
            const auto &semantic_events = trace_json.at("semantic_events");
            require(has_semantic_trace_event_detail_containing(semantic_events, "sequence_state", "manual_sequence_state_anchor",
                                                               "current_scene=Game;next_scene=FileSelect;current_stage=FileSelect") &&
                        has_semantic_trace_event_detail_containing(semantic_events, "sequence_state", "manual_sequence_state_anchor",
                                                                   "requested_stage=FileSelect") &&
                        has_semantic_trace_event_detail_containing(semantic_events, "sequence_state", "draw_3d_normal",
                                                                   "draw_phase=3d_normal;frame=7") &&
                        has_semantic_trace_event_detail_containing(semantic_events, "sequence_state", "draw_2d_normal",
                                                                   "draw_phase=2d_normal;frame=7"),
                    "runtime trace should carry generic sequence state anchors with scene, stage, wipe, draw phase, and frame context");
        }

        $test("bridges original LightFunction loads into generic scene light service") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);

            auto actor_light = ActorLightInfo{};
            actor_light.mInfo0.mColor = {0x10U, 0x20U, 0x30U, 0x40U};
            actor_light.mInfo0.mPos = {1.0F, 2.0F, 3.0F};
            actor_light.mInfo1.mColor = {0x50U, 0x60U, 0x70U, 0x80U};
            actor_light.mInfo1.mPos = {4.0F, 5.0F, 6.0F};
            actor_light.mAlpha2 = 0x90U;

            LightFunction::loadActorLightInfo(&actor_light);

            require(runtime.scene_lights().loaded_mask() == ((1U << 0U) | (1U << 1U) | (1U << 2U)),
                    "LightFunction::loadActorLightInfo should populate the same GX light slots used by original draw-buffer light loading");
            require(runtime.scene_lights().light(0U) != nullptr &&
                        runtime.scene_lights().light(0U)->color == smgpc::game::GXColorValue{0x10U, 0x20U, 0x30U, 0x40U} &&
                        runtime.scene_lights().light(0U)->position == std::array<float, 3U>{1.0F, 2.0F, 3.0F},
                    "LightFunction should convert ActorLightInfo::mInfo0 into GX light slot 0");
            require(runtime.scene_lights().light(1U) != nullptr &&
                        runtime.scene_lights().light(1U)->color == smgpc::game::GXColorValue{0x50U, 0x60U, 0x70U, 0x80U} &&
                        runtime.scene_lights().light(1U)->position == std::array<float, 3U>{4.0F, 5.0F, 6.0F},
                    "LightFunction should convert ActorLightInfo::mInfo1 into GX light slot 1");
            require(runtime.scene_lights().light(2U) != nullptr &&
                        runtime.scene_lights().light(2U)->color == smgpc::game::GXColorValue{0U, 0U, 0U, 0x90U},
                    "LightFunction should expose the original black alpha light in GX light slot 2");

            LightFunction::loadAllLightWhite();
            require(runtime.scene_lights().loaded_mask() == 0xffU, "LightFunction::loadAllLightWhite should populate every GX light slot");
            for (auto light_index = std::size_t{}; light_index < 8U; ++light_index) {
                require(runtime.scene_lights().light(light_index) != nullptr &&
                            runtime.scene_lights().light(light_index)->color == smgpc::game::GXColorValue{255U, 255U, 255U, 255U},
                        "LightFunction::loadAllLightWhite should use white light payloads for every slot");
            }
        }

        $test("loads FileSelect stage area light data from original LightData archive") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            runtime.set_current_stage_name("FileSelect");

            auto zone_id = ZoneLightID{};
            const auto *area_light = LightFunction::getAreaLightInfo(zone_id);
            require(area_light != nullptr, "LightFunction should resolve the current stage area light from LightData.arc");
            require(LightFunction::getDefaultAreaLightName() != nullptr &&
                        std::string_view(LightFunction::getDefaultAreaLightName()) == std::string_view(area_light->mAreaLightName),
                    "LightFunction should use the current stage zone BCSV default area-light name");
            require(area_light->mInterpolate == -1 && area_light->mFix,
                    "FileSelect area light should preserve original Interpolate and Fix BCSV values");
            require(area_light->mPlanetLight.mInfo0.mColor.r == 255U && area_light->mPlanetLight.mInfo0.mColor.g == 255U &&
                        area_light->mPlanetLight.mInfo0.mColor.b == 255U && area_light->mPlanetLight.mInfo0.mColor.a == 0U &&
                        area_light->mPlanetLight.mInfo1.mColor.r == 100U && area_light->mPlanetLight.mInfo1.mColor.g == 150U &&
                        area_light->mPlanetLight.mInfo1.mColor.b == 255U && area_light->mPlanetLight.mInfo1.mColor.a == 0U &&
                        area_light->mPlanetLight.mAlpha2 == 128U && area_light->mPlanetLight.mColor.r == 64U &&
                        area_light->mPlanetLight.mColor.g == 64U && area_light->mPlanetLight.mColor.b == 64U &&
                        area_light->mPlanetLight.mColor.a == 128U,
                    "FileSelect planet light colors should decode signed BCSV color bytes as original GX channel values");
            require_near(area_light->mPlanetLight.mInfo0.mPos.x, 700000.0F, 0.01F, "FileSelect planet light 0 X should come from LightData.bcsv");
            require_near(area_light->mPlanetLight.mInfo0.mPos.y, 700000.0F, 0.01F, "FileSelect planet light 0 Y should come from LightData.bcsv");
            require_near(area_light->mPlanetLight.mInfo0.mPos.z, 700000.0F, 0.01F, "FileSelect planet light 0 Z should come from LightData.bcsv");
            require_near(area_light->mPlanetLight.mInfo1.mPos.x, -100000.0F, 0.01F, "FileSelect planet light 1 X should come from LightData.bcsv");
            require_near(area_light->mPlanetLight.mInfo1.mPos.y, -1300.0F, 0.01F, "FileSelect planet light 1 Y should come from LightData.bcsv");
            require_near(area_light->mPlanetLight.mInfo1.mPos.z, 21500.0F, 0.01F, "FileSelect planet light 1 Z should come from LightData.bcsv");
            require(!area_light->mPlanetLight.mInfo0.mIsFollowCamera && !area_light->mPlanetLight.mInfo1.mIsFollowCamera,
                    "FileSelect planet light follow-camera flags should decode from the original FollowCamera fields");
        }

        $test("loads draw-buffer light type from stage LightData through MR LightUtil") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            runtime.set_current_stage_name("FileSelect");

            MR::loadLight(MR::LightType_Planet);

            require(runtime.scene_lights().loaded_mask() == ((1U << 0U) | (1U << 1U) | (1U << 2U)),
                    "MR::loadLight should populate original actor-light GX slots from the current stage area light");
            require(runtime.scene_lights().light(0U) != nullptr &&
                        runtime.scene_lights().light(0U)->color == smgpc::game::GXColorValue{255U, 255U, 255U, 0U} &&
                        runtime.scene_lights().light(0U)->position == std::array<float, 3U>{700000.0F, 700000.0F, 700000.0F},
                    "MR::loadLight should load FileSelect planet light 0 into GX light slot 0");
            require(runtime.scene_lights().light(1U) != nullptr &&
                        runtime.scene_lights().light(1U)->color == smgpc::game::GXColorValue{100U, 150U, 255U, 0U} &&
                        runtime.scene_lights().light(1U)->position == std::array<float, 3U>{-100000.0F, -1300.0F, 21500.0F},
                    "MR::loadLight should load FileSelect planet light 1 into GX light slot 1");
            require(runtime.scene_lights().light(2U) != nullptr &&
                        runtime.scene_lights().light(2U)->color == smgpc::game::GXColorValue{0U, 0U, 0U, 128U},
                    "MR::loadLight should load the original black alpha light from the parsed area light");
        }

        $test("loads original draw-buffer light before scheduler model drawing") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            runtime.set_current_stage_name("FileSelect");
            auto renderer = RecordingRenderer();
            auto actor = SchedulerProbeActor("draw-buffer-light-probe");

            actor.appear();
            runtime.scheduler().register_live_actor_model(actor, -1, -1, MR::DrawBufferType_Sky, -1);
            runtime.scheduler().execute_draw_buffer_opa(renderer, title_test_camera_pose(), MR::DrawBufferType_Sky);

            require(runtime.scene_lights().loaded_mask() == ((1U << 0U) | (1U << 1U) | (1U << 2U)),
                    "scene scheduler should load the draw-buffer light type before drawing active model buffers");
        }

        $test("loads ActorLightCtrl through central draw-buffer actor execution") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            auto actor = SchedulerProbeActor("light-loader-probe");

            actor.initActorLightCtrl();
            actor.mActorLightCtrl->_1C = 1U;
            actor.mActorLightCtrl->mLightInfo.mInfo0.mColor = {0x11U, 0x22U, 0x33U, 0x44U};
            actor.mActorLightCtrl->mLightInfo.mInfo0.mPos = {10.0F, 20.0F, 30.0F};
            actor.mActorLightCtrl->mLightInfo.mInfo1.mColor = {0x55U, 0x66U, 0x77U, 0x88U};
            actor.mActorLightCtrl->mLightInfo.mInfo1.mPos = {40.0F, 50.0F, 60.0F};
            actor.mActorLightCtrl->mLightInfo.mAlpha2 = 0x99U;
            actor.appear();

            runtime.scheduler().register_live_actor_model(actor, -1, -1, MR::DrawBufferType_MapObj, -1);
            runtime.scheduler().execute_draw_buffer_opa(renderer, title_test_camera_pose(), MR::DrawBufferType_MapObj);

            require(runtime.scene_lights().loaded_mask() == ((1U << 0U) | (1U << 1U) | (1U << 2U)),
                    "central draw-buffer execution should load actor lights before drawing an actor model");
            require(runtime.scene_lights().light(0U) != nullptr &&
                        runtime.scene_lights().light(0U)->color == smgpc::game::GXColorValue{0x11U, 0x22U, 0x33U, 0x44U} &&
                        runtime.scene_lights().light(0U)->position == std::array<float, 3U>{10.0F, 20.0F, 30.0F},
                    "draw-buffer actor light loading should bridge ActorLightCtrl::mInfo0 through LightFunction");
            require(runtime.scene_lights().light(1U) != nullptr &&
                        runtime.scene_lights().light(1U)->color == smgpc::game::GXColorValue{0x55U, 0x66U, 0x77U, 0x88U} &&
                        runtime.scene_lights().light(1U)->position == std::array<float, 3U>{40.0F, 50.0F, 60.0F},
                    "draw-buffer actor light loading should bridge ActorLightCtrl::mInfo1 through LightFunction");
            require(runtime.scene_lights().light(2U) != nullptr &&
                        runtime.scene_lights().light(2U)->color == smgpc::game::GXColorValue{0U, 0U, 0U, 0x99U},
                    "draw-buffer actor light loading should preserve the original alpha light slot");
            const auto trace = runtime.scheduler().last_execution_trace();
            require(!trace.empty() && trace.back().name == "light-loader-probe" &&
                        trace.back().phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa,
                    "scene scheduler should still trace the draw-buffer actor after loading lights");
        }

        $test("orders scene scheduler movement calcAnim and calcView categories") {
            auto scheduler = smgpc::game::SceneScheduler();
            auto sky = SchedulerProbeObj("sky-probe");
            auto layout = SchedulerProbeObj("layout-probe");
            auto map_obj = SchedulerProbeObj("map-probe");
            auto area_obj = SchedulerProbeObj("area-probe");
            auto movie = SchedulerProbeObj("movie-probe");
            auto npc_actor = SchedulerProbeActor("npc-probe");
            npc_actor.appear();

            scheduler.connect_name_obj(sky, MR::MovementType_Sky, MR::CalcAnimType_MapObj, MR::DrawBufferType_Sky, -1);
            scheduler.connect_name_obj(layout, MR::MovementType_Layout, MR::CalcAnimType_Layout, -1, MR::DrawType_Layout);
            scheduler.connect_name_obj(map_obj, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_MapObj, -1);
            scheduler.connect_name_obj(area_obj, MR::MovementType_AreaObj, -1, -1, -1);
            scheduler.connect_name_obj(movie, MR::MovementType_Movie, -1, -1, MR::DrawType_Movie);
            scheduler.register_live_actor_model(npc_actor, MR::MovementType_NPC, MR::CalcAnimType_NPC, MR::DrawBufferType_NPC, -1);

            const auto snapshot = scheduler.snapshot();
            require(snapshot.size() == 6U, "scene scheduler should retain all connected NameObj and LiveActor entries");
            require(snapshot[0].name == "sky-probe" && snapshot[0].movement_type == MR::MovementType_Sky,
                    "scene scheduler snapshot should preserve registration state");

            scheduler.execute_movement();
            const auto movement_trace = scheduler.last_execution_trace();
            require(movement_trace.size() == 6U, "scene scheduler should trace movement execution");
            require(movement_trace[0].name == "map-probe" && movement_trace[0].phase == smgpc::game::SceneSchedulerPhase::Movement &&
                        movement_trace[1].name == "npc-probe" && movement_trace[2].name == "area-probe" &&
                        movement_trace[3].name == "layout-probe" && movement_trace[4].name == "movie-probe" && movement_trace[5].name == "sky-probe",
                    "scene scheduler movement should execute by original SceneExecutor movement category order");
            require(sky.movement_count == 1 && layout.movement_count == 1 && map_obj.movement_count == 1 && area_obj.movement_count == 1 &&
                        movie.movement_count == 1 && npc_actor.control_count == 1,
                    "scene scheduler should call NameObj movement and LiveActor control once for active entries");

            NameObjFunction::requestMovementOff(&map_obj);
            scheduler.execute_movement();
            const auto suspended_trace = scheduler.last_execution_trace();
            require(suspended_trace.size() == 5U && suspended_trace[0].name == "npc-probe" && suspended_trace[1].name == "area-probe" &&
                        suspended_trace[2].name == "layout-probe" && suspended_trace[3].name == "movie-probe" &&
                        suspended_trace[4].name == "sky-probe" && map_obj.movement_count == 1 && npc_actor.control_count == 2,
                    "scene scheduler should honor NameObj movement suspension");

            scheduler.execute_calc_anim();
            const auto calc_trace = scheduler.last_execution_trace();
            require(calc_trace.size() == 8U && calc_trace[5].name == "sky-probe" &&
                        calc_trace[5].phase == smgpc::game::SceneSchedulerPhase::CalcAnim && calc_trace[6].name == "npc-probe" &&
                        calc_trace[7].name == "layout-probe" && layout.calc_anim_count == 1 && sky.calc_anim_count == 1 &&
                        npc_actor.calc_anim_count == 1,
                    "scene scheduler should append phase-tagged calcAnim evidence in original SceneExecutor category order");

            scheduler.execute_calc_view_and_entry();
            const auto calc_view_trace = scheduler.last_execution_trace();
            require(calc_view_trace.size() == 12U && calc_view_trace[8].name == "layout-probe" &&
                        calc_view_trace[8].phase == smgpc::game::SceneSchedulerPhase::CalcViewAndEntry &&
                        calc_view_trace[9].name == "movie-probe" && calc_view_trace[10].name == "npc-probe" &&
                        calc_view_trace[11].name == "sky-probe" && sky.calc_view_count == 1 && layout.calc_view_count == 1 &&
                        area_obj.calc_view_count == 0 && movie.calc_view_count == 1 && npc_actor.calc_view_count == 1,
                    "scene scheduler should execute calcViewAndEntry as original 2D entry before 3D draw-buffer entry");
        }

        $test("executes original normal draw-buffer opaque and translucent passes") {
            auto scheduler = smgpc::game::SceneScheduler();
            auto renderer = RecordingRenderer();
            auto map_actor = SchedulerProbeActor("map-draw-probe");
            auto npc_actor = SchedulerProbeActor("npc-draw-probe");
            auto sky_actor = SchedulerProbeActor("sky-draw-probe");

            map_actor.appear();
            npc_actor.appear();
            sky_actor.appear();
            scheduler.register_live_actor_model(map_actor, -1, -1, MR::DrawBufferType_MapObj, -1);
            scheduler.register_live_actor_model(npc_actor, -1, -1, MR::DrawBufferType_NPC, -1);
            scheduler.register_live_actor_model(sky_actor, -1, -1, MR::DrawBufferType_Sky, -1);

            scheduler.execute_draw_buffer_list_normal(renderer, title_test_camera_pose());
            const auto trace = scheduler.last_execution_trace();
            require(trace.size() == 6U, "normal draw-buffer list should trace opa and xlu buffer passes with active actors");
            require(trace[0].name == "map-draw-probe" && trace[0].phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa &&
                        trace[0].draw_buffer_type == MR::DrawBufferType_MapObj &&
                        trace[0].draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Opaque,
                    "normal draw-buffer opa-before-volume-shadow should draw MapObj opaque before later actor buffers");
            require(trace[1].name == "npc-draw-probe" && trace[1].phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa &&
                        trace[1].draw_buffer_type == MR::DrawBufferType_NPC,
                    "normal draw-buffer opa list should draw NPC opaque before deferred sky when prior air is off");
            require(trace[2].name == "sky-draw-probe" && trace[2].phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa &&
                        trace[2].draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Opaque && trace[3].name == "sky-draw-probe" &&
                        trace[3].phase == smgpc::game::SceneSchedulerPhase::DrawBufferXlu &&
                        trace[3].draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Translucent,
                    "normal draw-buffer opa list should draw deferred Sky opaque and translucent passes together");
            require(trace[4].name == "map-draw-probe" && trace[4].phase == smgpc::game::SceneSchedulerPhase::DrawBufferXlu &&
                        trace[5].name == "npc-draw-probe" && trace[5].phase == smgpc::game::SceneSchedulerPhase::DrawBufferXlu,
                    "normal xlu draw-buffer list should revisit map objects before NPC");
        }

        $test("routes all-live-actor messages through scheduler broadcast") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto selector = FileSelector("ファイルセレクタ");
            auto probe = SchedulerProbeActor("message-probe");

            selector.initWithoutIter();
            probe.appear();
            runtime.scheduler().register_live_actor_model(probe, MR::MovementType_NPC, MR::CalcAnimType_NPC, MR::DrawBufferType_NPC, -1);

            MR::sendMsgToAllLiveActor(ACTMES_AUTORUSH_BEGIN, nullptr);
            const auto messages = runtime.scheduler().message_trace();
            auto *message_sensor = MR::getMessageSensor();
            auto *message_sensor_host = MR::getSensorHost(message_sensor);
            require(message_sensor != nullptr && message_sensor_host != nullptr,
                    "MR::getMessageSensor should expose a concrete neutral message sensor and host");
            require(probe.message_count == 1 && probe.last_message == ACTMES_AUTORUSH_BEGIN,
                    "MR::sendMsgToAllLiveActor should broadcast original actor messages to generic live actors");
            require(probe.last_sender == message_sensor && probe.last_receiver == message_sensor,
                    "scheduler broadcasts should pass the original-style global message sensor as sender and receiver");
            require(probe.last_sender_host == message_sensor_host && probe.last_receiver_host == message_sensor_host,
                    "broadcast message sensors should resolve to a concrete sensor host");
            require(std::ranges::any_of(messages,
                                        [](const auto &message) {
                                            return message.target_name == "ファイルセレクタ" && message.message == ACTMES_AUTORUSH_BEGIN &&
                                                   message.delivered && message.accepted && !message.target_dead && !message.target_suspended &&
                                                   message.sender_sensor_present && message.receiver_sensor_present &&
                                                   message.sender_sensor_type == ATYPE_MESSAGE_SENSOR &&
                                                   message.receiver_sensor_type == ATYPE_MESSAGE_SENSOR;
                                        }),
                    "scene scheduler message trace should record FileSelector accepting AutoRushBegin through the generic broadcast path");

            const auto frame_context = smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            };
            runtime.begin_frame(frame_context);
            require(selector.isTitleStarted(), "scheduler-broadcast AutoRushBegin should put FileSelector into the original title nerve flow");

            const auto trace_path = std::filesystem::path(".cache/tests/runtime-message-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(trace_path, frame_context, runtime);
            const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
            require(trace_json.contains("scene_messages"), "runtime parity trace should include generic actor-message routing evidence");
            const auto &scene_messages = trace_json.at("scene_messages");
            require(std::ranges::any_of(scene_messages,
                                        [](const auto &message) {
                                            return message.at("target_name") == "ファイルセレクタ" &&
                                                   message.at("message_name") == "ACTMES_AUTORUSH_BEGIN" &&
                                                   message.at("delivered") == true && message.at("accepted") == true &&
                                                   message.at("sender_sensor_present") == true &&
                                                   message.at("receiver_sensor_present") == true &&
                                                   message.at("sender_sensor_type") == ATYPE_MESSAGE_SENSOR &&
                                                   message.at("receiver_sensor_type") == ATYPE_MESSAGE_SENSOR;
                                        }),
                    "runtime parity trace should serialize accepted all-live-actor message broadcasts");
        }

        $test("routes layout and sky actors through central scene scheduler") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            runtime.set_current_stage_name("FileSelect");
            runtime.set_j3d_packet_trace_frame(20U);

            {
                auto layout = SimpleLayout("TitleLogoProbe", "TitleLogo", 1U, MR::DrawType_Layout);
                auto sky = FileSelectSky("ファイル選択空");

                sky.initWithoutIter();
                layout.appear();
                layout.startAnim("Appear", 0U);
                sky.appear();

                const auto snapshot = runtime.scheduler().snapshot();
                const auto has_layout = std::ranges::any_of(snapshot, [](const auto &entry) {
                    return entry.kind == smgpc::game::SceneEntryKind::Layout && entry.name == "TitleLogoProbe" &&
                           entry.movement_type == MR::MovementType_Layout;
                });
                const auto has_sky = std::ranges::any_of(snapshot, [](const auto &entry) {
                    return entry.kind == smgpc::game::SceneEntryKind::LiveActorModel && entry.name == "ファイル選択空" &&
                           entry.draw_buffer_type == MR::DrawBufferType_Sky;
                });
                require(has_layout && has_sky, "runtime should register layouts and sky actors in the central scene scheduler");

                const auto frame_context = smgpc::render::FrameContext{
                    .frame_index = 20U,
                    .frame_time_seconds = 20.0 / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
                runtime.begin_frame(frame_context);
                const auto trace = runtime.scheduler().last_execution_trace();
                require(!trace.empty() && trace.front().kind == smgpc::game::SceneEntryKind::Layout &&
                            trace.front().phase == smgpc::game::SceneSchedulerPhase::Movement,
                        "runtime scene scheduler should execute layout category before sky movement");
                require(std::ranges::any_of(trace, [](const auto &entry) {
                            return entry.name == "ファイル選択空" && entry.phase == smgpc::game::SceneSchedulerPhase::CalcViewAndEntry;
                        }),
                        "runtime scene scheduler should run sky calcViewAndEntry during the frame phase");
                require(sky.getNerveStep() == 1, "runtime scene scheduler should still move the sky actor nerve");
                require_near(sky.getBaseMatrix().m[0U], 0.8F, 0.000001F,
                             "runtime scene scheduler should update scaled sky base matrix during calcViewAndEntry");
                require_near(sky.getBaseMatrix().m[5U], 0.8F, 0.000001F,
                             "runtime scene scheduler should update scaled sky base matrix during calcViewAndEntry");
                require_near(sky.getBaseMatrix().m[10U], 0.8F, 0.000001F,
                             "runtime scene scheduler should update scaled sky base matrix during calcViewAndEntry");
                const auto layout_runtime = runtime.scheduler().debug_layout_runtime_snapshot();
                const auto title_logo_runtime = std::ranges::find_if(layout_runtime, [](const auto &entry) {
                    return entry.name == "TitleLogoProbe" && entry.layout_name == "TitleLogo";
                });
                require(title_logo_runtime != layout_runtime.end() && !title_logo_runtime->dead && !title_logo_runtime->animations.empty() &&
                            title_logo_runtime->animations[0U].name == "Appear" && title_logo_runtime->animations[0U].rate == 1.0F &&
                            !title_logo_runtime->animations[0U].stopped,
                        "runtime scene scheduler should expose debug-only layout animation state for parity traces");
                require_near(title_logo_runtime->animations[0U].frame, 1.0F, 0.001F,
                             "runtime layout animation evidence should reflect scheduler movement updates");
                require(title_logo_runtime->panes.size() == title_logo_runtime->pane_count &&
                            title_logo_runtime->materials.size() == title_logo_runtime->material_count &&
                            title_logo_runtime->textures.size() == title_logo_runtime->texture_count &&
                            std::ranges::any_of(title_logo_runtime->panes,
                                                [](const auto &pane) {
                                                    return pane.effective_visible && !pane.contents.empty() &&
                                                           std::ranges::any_of(pane.contents, [](const auto &content) {
                                                               return content.kind == "picture" && content.material_index >= 0 &&
                                                                      !content.material_name.empty() && !content.texture_name.empty();
                                                           });
                                                }) &&
                            std::ranges::any_of(title_logo_runtime->textures,
                                                [](const auto &texture) {
                                                    return !texture.name.empty() && texture.width > 0U && texture.height > 0U &&
                                                           !texture.format_name.empty();
                                                }),
                        "runtime layout debug snapshot should expose pane/material/texture details for 2D parity diagnosis");

                GXSetCopyClear(GXColor{9U, 8U, 7U, 6U}, 0x00123456U);
                runtime.draw_3d_normal(renderer, title_test_camera_pose());
                const auto draw_trace = runtime.scheduler().last_execution_trace();
                require(runtime.scene_lights().loaded_mask() == ((1U << 0U) | (1U << 1U) | (1U << 2U)),
                        "runtime 3D draw should leave the scheduler-loaded FileSelect planet lights in the scene light service");
                require(std::ranges::any_of(draw_trace,
                                            [](const auto &entry) {
                                                return entry.name == "ファイル選択空" &&
                                                       entry.phase == smgpc::game::SceneSchedulerPhase::DrawBufferOpa &&
                                                       entry.draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Opaque;
                                            }) &&
                            std::ranges::any_of(draw_trace,
                                                [](const auto &entry) {
                                                    return entry.name == "ファイル選択空" &&
                                                           entry.phase == smgpc::game::SceneSchedulerPhase::DrawBufferXlu &&
                                                           entry.draw_buffer_pass == smgpc::game::SceneDrawBufferPass::Translucent;
                                                }),
                        "runtime scene scheduler should tag original sky draw-buffer opa/xlu passes in the execution trace");
                require(renderer.triangle_batch_count > 0U || renderer.gx_material_batch_count > 0U,
                        "runtime scene scheduler should draw sky actors through the sky draw buffer");

                runtime.draw_2d_normal(renderer);
                require(runtime.scene_lights().loaded_mask() == ((1U << 0U) | (1U << 1U) | (1U << 2U)),
                        "runtime 2D draw should not replace scheduler-loaded 3D scene lights before parity trace capture");
                runtime.save_data().set_slot_state(1, smgpc::game::SaveDataService::SlotState{
                                                          .created = true,
                                                          .last_loaded_mario = true,
                                                          .power_star_num = 12,
                                                          .star_piece_num = 345,
                                                          .player_miss_num = 6,
                                                          .has_mii_id = false,
                                                          .icon_id = 4U,
                                                          .view_normal_ending = true,
                                                          .view_complete_ending = false,
                                                          .complete_ending_mario_and_luigi = false,
                                                          .game_event_flags = {},
                                                          .game_event_values = {},
                                                          .last_modified = 1900,
                                                      });
                runtime.save_data().set_sys_config_time_announced(101);
                runtime.save_data().set_sys_config_time_sent(202);
                runtime.save_data().set_sys_config_sent_bytes(303U);
                const auto trace_path = std::filesystem::path(".cache/tests/runtime-parity-trace.ndjson");
                smgpc::game::write_runtime_parity_trace(trace_path, frame_context, runtime);
                const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
                require(trace_json.at("schema") == "smgpc-runtime-parity-trace-v1",
                        "runtime parity trace should write and load a stable schema identifier");
                require(trace_json.contains("camera_pose") && trace_json.contains("scene_trace"),
                        "runtime parity trace should include camera pose and scene execution evidence");
                const auto &copy_events = trace_json.at("copy_events");
                require(trace_json.at("frame").at("viewport").at("right") == 640U && trace_json.at("frame").at("viewport").at("bottom") == 456U &&
                            trace_json.at("frame").at("scissor").at("width") == 640U && copy_events.size() == 3U &&
                            copy_events[0U].at("kind") == "xfb" && copy_events[0U].at("copy_to_xfb") == true &&
                            copy_events[0U].at("dest_stride") == 1280U && copy_events[0U].at("source_rect").at("height") == 456U &&
                            copy_events[0U].at("render_pass") == "EfbToXfb" && copy_events[0U].at("view_id") == 0 &&
                            copy_events[1U].at("kind") == "texture" && copy_events[1U].at("target_pixel_format") == 14U &&
                            copy_events[1U].at("real_format") == 7U && copy_events[1U].at("dest_stride") == 2560U &&
                            copy_events[1U].at("clear_color").at(0U) == 9U && copy_events[1U].at("clear_color").at(3U) == 6U &&
                            copy_events[1U].at("clear_depth") == 0x00123456U && copy_events[1U].at("copy_filter_vertical") == true &&
                            copy_events[1U].at("copy_filter_vfilter").at(3U) == 22U &&
                            copy_events[2U].at("kind") == "texture" && copy_events[2U].at("target_pixel_format") == 8U &&
                            copy_events[2U].at("real_format") == 4U && copy_events[2U].at("dest_stride") == 5120U &&
                            copy_events[2U].at("clear_depth") == 0x00123456U && copy_events[2U].at("copy_filter_vertical") == false &&
                            copy_events[2U].at("copy_filter_sample_pattern").at(11U).at(1U) == 0U,
                        "runtime parity trace should include Dolphin-comparable viewport, scissor, XFB, and screen-capture texture copy state");
                const auto &layout_entries = trace_json.at("layout_runtime");
                require(std::ranges::any_of(layout_entries, [](const auto &entry) {
                            return entry.at("layout_name") == "TitleLogo" && !entry.at("animations").empty() &&
                                   entry.at("animations").front().at("name") == "Appear" && entry.at("pane_count") > 0U &&
                                   entry.at("picture_count") > 0U && entry.at("material_count") > 0U &&
                                   entry.at("panes").size() == entry.at("pane_count") &&
                                   entry.at("materials").size() == entry.at("material_count") &&
                                   entry.at("textures").size() == entry.at("texture_count") &&
                                   std::ranges::any_of(entry.at("panes"), [](const auto &pane) {
                                       return pane.at("effective_visible") == true && !pane.at("contents").empty() &&
                                              std::ranges::any_of(pane.at("contents"), [](const auto &content) {
                                                  return content.at("kind") == "picture" &&
                                                         content.at("material_index").template get<int>() >= 0 &&
                                                         !content.at("material_name").template get<std::string>().empty() &&
                                                         !content.at("texture_name").template get<std::string>().empty();
                                              });
                                   }) &&
                                   std::ranges::any_of(entry.at("textures"), [](const auto &texture) {
                                       return !texture.at("name").template get<std::string>().empty() &&
                                              texture.at("width").template get<unsigned int>() > 0U &&
                                              texture.at("height").template get<unsigned int>() > 0U &&
                                              !texture.at("format").template get<std::string>().empty();
                                   });
                        }),
                        "runtime parity trace should include layout animation plus pane/material/texture evidence");
                const auto &scene_entries = trace_json.at("scene_trace");
                const auto has_phase = [&scene_entries](std::string_view phase) {
                    return std::ranges::any_of(scene_entries, [phase](const auto &entry) { return entry.at("phase") == phase; });
                };
                require(has_phase("DrawBufferOpa") && has_phase("DrawBufferXlu") && has_phase("DrawType"),
                        "runtime parity trace should include original-style draw-buffer and draw-type phases");
                const auto &scene_snapshot = trace_json.at("scene_snapshot");
                const auto sky_snapshot = std::ranges::find_if(scene_snapshot, [](const auto &entry) { return entry.at("name") == "ファイル選択空"; });
                require(sky_snapshot != scene_snapshot.end() && !sky_snapshot->at("live_actor").is_null() &&
                            sky_snapshot->at("live_actor").at("nerve_step") == 1 &&
                            sky_snapshot->at("live_actor").at("base_matrix").size() == 12U &&
                            trace_json.at("runtime_services").at("rfl").at("initialized") == true &&
                            trace_json.at("runtime_services").at("save").at("file_count").template get<unsigned int>() >= 3U &&
                            trace_json.at("runtime_services").at("save").at("slot_count") == 6U &&
                            trace_json.at("runtime_services").at("save").at("slots")[0U].at("slot_index") == 1 &&
                            trace_json.at("runtime_services").at("save").at("slots")[0U].at("created") == true &&
                            trace_json.at("runtime_services").at("save").at("slots")[0U].at("icon_id") == 4U &&
                            trace_json.at("runtime_services").at("save").at("sys_config").at("time_announced") == 101 &&
                            trace_json.at("runtime_services").at("save").at("sys_config").at("time_sent") == 202 &&
                            trace_json.at("runtime_services").at("save").at("sys_config").at("sent_bytes") == 303U &&
                            trace_json.at("runtime_services").at("scene_lights").at("loaded_mask") ==
                                ((1U << 0U) | (1U << 1U) | (1U << 2U)) &&
                            trace_json.at("runtime_services").at("scene_lights").at("lights").size() == 3U,
                        "runtime parity trace should expose generic live-actor and runtime-service state without actor-specific compat hooks");
                const auto &packets = trace_json.at("render_packets");
                const auto has_packet = [&packets](std::string_view material, std::string_view mode) {
                    return std::ranges::any_of(packets, [material, mode](const auto &packet) {
                        return packet.at("material_name") == material && packet.at("packet_mode") == mode;
                    });
                };
                const auto title_logo_packet = std::ranges::find_if(packets, [](const auto &packet) {
                    return packet.at("model_name") == "TitleLogo" && packet.at("packet_mode") == "BrlytGxMaterial2D";
                });
                require(title_logo_packet != packets.end() && title_logo_packet->at("render_pass") == "2d_layout" &&
                            title_logo_packet->at("texture_bindings").size() >= 1U &&
                            title_logo_packet->at("texture_bindings")[0U].at("width").template get<unsigned int>() > 0U &&
                            title_logo_packet->at("used_textures_mask").template get<unsigned int>() != 0U &&
                            title_logo_packet->at("source_vertex_count") == 4U && title_logo_packet->at("num_indices") == 6U,
                        "runtime parity trace should include actual submitted BRLYT layout render packets");
                const auto space_packet = std::ranges::find_if(packets, [](const auto &packet) {
                    return packet.at("material_name") == "Space_Mat_v";
                });
                const auto comet_halo_packet = std::ranges::find_if(packets, [](const auto &packet) {
                    return packet.at("material_name") == "CometHalo_v";
                });
                require(has_packet("Space_Mat_v", "ShaderGxTev") && has_packet("CometHalo_v", "ComposedMaterial") &&
                            space_packet != packets.end() && space_packet->at("model_name") == "CometNearOrbitSky" &&
                            space_packet->at("indirect_stage_count") == 0 && space_packet->at("matrix_group_index") == 0 &&
                            space_packet->at("display_list_size") == 3232 && space_packet->at("draw_packet_triangle_count") == 480 &&
                            space_packet->at("color_channels").front().contains("material_color") &&
                            space_packet->at("color_channels").front().contains("color_control") && space_packet->contains("fog_type") &&
                            space_packet->at("bck_active") == true && space_packet->at("bck_frame_max") == 3000 &&
                            space_packet->at("btk_active") == true && space_packet->at("btk_frame_max") == 10000 &&
                            space_packet->at("draw_pass") == "Opaque" && space_packet->at("render_pass") == "Opaque" &&
                            space_packet->at("view_id") == 0 && space_packet->at("material_mode") == 1 &&
                            space_packet->at("packet_mode_fallback") == false &&
                            space_packet->at("packet_mode_reason") == "shader_gx_tev_supported" &&
                            space_packet->at("draw_buffer_opaque") == true && comet_halo_packet != packets.end() &&
                            comet_halo_packet->at("material_mode") == 1 && comet_halo_packet->at("draw_pass") == "Opaque" &&
                            comet_halo_packet->at("draw_buffer_opaque") == true && comet_halo_packet->at("blend") == true &&
                            comet_halo_packet->at("packet_mode_fallback") == true &&
                            comet_halo_packet->at("packet_mode_reason") == "cpu_composed_multi_pass_or_indirect" &&
                            space_packet->at("gx_blend").at("alpha_update") == false &&
                            comet_halo_packet->at("gx_blend").at("alpha_update") == false,
                        "runtime parity trace should include submitted J3D material packet sequence, animation frames, and draw state");
                require(space_packet->at("texture_bindings").size() == 3U &&
                            space_packet->at("texture_bindings")[0U].at("name") == "OrbitUniverseL" &&
                            space_packet->at("texture_bindings")[1U].at("format") == "CMPR" &&
                            space_packet->at("texture_bindings")[0U].at("has_sampler_metadata") == true &&
                            space_packet->at("texture_bindings")[0U].at("wrap_s") == 1 &&
                            space_packet->at("texture_bindings")[0U].at("min_filter") == 1 &&
                            !space_packet->at("tex_coord_scales").empty() &&
                            (space_packet->at("tex_coord_scales")[0U].at("derived_from_texture") == true ||
                             space_packet->at("tex_coord_scales")[0U].at("s_loaded") == true) &&
                            space_packet->at("used_textures_mask") == 7U &&
                            space_packet->at("used_texture_slots").size() == 3U &&
                            space_packet->at("used_texture_slots")[0U] == 0U &&
                            space_packet->at("used_texture_slots")[1U] == 1U &&
                            space_packet->at("used_texture_slots")[2U] == 2U &&
                            space_packet->at("tex_coord_gens").size() == 3U && space_packet->at("tex_matrices").size() == 3U &&
                            space_packet->at("tev_orders").size() >= 3U && space_packet->at("tev_stages").size() >= 3U &&
                            space_packet->at("tev_orders")[0U].at("tex_map") == 1 &&
                            !space_packet->at("mdl3_register_loads").empty() && space_packet->at("gx_z_mode").at("enabled") == true &&
                            space_packet->at("gx_fog").contains("raw"),
                        "runtime parity trace should include comparable low-level GX texture, TEV, texgen, z, fog, and MDL3 register arrays");
                const auto earth_far_packet = std::ranges::find_if(packets, [](const auto &packet) {
                    return packet.at("material_name") == "EarthFar_v";
                });
                require(earth_far_packet != packets.end() && earth_far_packet->at("tex_matrices").size() >= 1U,
                        "runtime parity trace should include FileSelectSky projected Earth material tex matrices");
                const auto &earth_effect = earth_far_packet->at("tex_matrices")[0U].at("effect_matrix");
                const auto row0_len_sq = (earth_effect[0U].template get<double>() * earth_effect[0U].template get<double>()) +
                                         (earth_effect[1U].template get<double>() * earth_effect[1U].template get<double>()) +
                                         (earth_effect[2U].template get<double>() * earth_effect[2U].template get<double>());
                require(row0_len_sq > 0.95 && row0_len_sq < 1.05,
                        "ProjmapEffectMtxSetter should remove actor base scale before inverting projected texture matrices");
                const auto core_rock_packet = std::ranges::find_if(packets, [](const auto &packet) {
                    return packet.at("material_name") == "CoreRock";
                });
                require(core_rock_packet != packets.end() && core_rock_packet->at("material_loaded_light_mask") == 0U &&
                            core_rock_packet->at("scene_loaded_light_mask") == ((1U << 0U) | (1U << 1U) | (1U << 2U)) &&
                            core_rock_packet->at("loaded_light_mask") == ((1U << 0U) | (1U << 1U) | (1U << 2U)) &&
                            core_rock_packet->at("requested_light_mask") == 4U &&
                            core_rock_packet->at("unsatisfied_light_mask") == 0U,
                        "runtime parity trace should show GX material lights satisfied from generic runtime scene light state");
            }

            const auto remaining_entries = runtime.scheduler().snapshot();
            require(std::ranges::all_of(remaining_entries,
                                        [](const auto &entry) {
                                            return entry.kind == smgpc::game::SceneEntryKind::NameObj &&
                                                   entry.name == "画面キャプチャ" &&
                                                   (entry.draw_type == MR::DrawType_CaptureScreenIndirect ||
                                                    entry.draw_type == MR::DrawType_CaptureScreenCamera);
                                        }),
                    "runtime scene scheduler should unregister destroyed layout and sky actor entries while keeping persistent capture actors");
        }

        $test("matches FileSelector title autorush gate behavior") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            runtime.save_data().set_slot_state(2, smgpc::game::SaveDataService::SlotState{
                                                      .created = true,
                                                      .last_loaded_mario = false,
                                                      .power_star_num = 3,
                                                      .star_piece_num = 40,
                                                      .icon_id = 1U,
                                                      .view_normal_ending = false,
                                                      .view_complete_ending = false,
                                                      .complete_ending_mario_and_luigi = false,
                                                      .game_event_flags = {},
                                                      .game_event_values = {},
                                                      .last_modified = 0,
                                                  });
            auto selector = FileSelector("ファイルセレクタ");
            selector.initWithoutIter();
            selector.setUserFileMario(2, false);
            require(selector.isUserFileLuigi(2) && selector.isUserFileAppearLuigi(2),
                    "FileSelector should expose the original Luigi/Bros-button eligibility helpers for Luigi saves");
            require(selector.getSkyStep() == 0U, "FileSelector sky actor step should start at zero");
            require(selector.getItemCount() == 6, "FileSelector should create the six original file-select item slots");
            require(selector.isOperationButtonCreated() && selector.isFileInfoCreated() && selector.isBackButtonCreated() &&
                        selector.isBrosButtonCreated() && selector.isInfoMessageCreated() && selector.isSysInfoWindowCreated() &&
                        selector.isMiiConfirmIconCreated() && selector.isManualCreated() && selector.isSelectEffectCreated() &&
                        selector.getSelectEffectCount() == 6,
                    "FileSelector should create original-shaped file-select children during init");
            require(selector.getItemFileNo(0) == 1 && selector.getItemFileNo(1) == 2 && selector.getItemFileNo(2) == 4 &&
                        selector.getItemFileNo(3) == 6 && selector.getItemFileNo(4) == 5 && selector.getItemFileNo(5) == 3,
                    "FileSelector should preserve the original file-number order table");
            require(selector.isItemNew(0) && !selector.isItemNew(1) && selector.isItemNew(2),
                    "FileSelector should derive new/existing item state from restored UserFile slots");
            require(!selector.isTitleStarted(), "FileSelector title should not start before AutoRushBegin");
            require(!selector.isTitleActive(), "FileSelector::createTitle should keep TitleSequenceProduct killed before AutoRushBegin");
            require(!selector.receiveOtherMsg(0U), "FileSelector should reject unrelated messages");

            const auto &item0 = selector.getItemBasePosition(0);
            require_near(item0.x, -1710.1007F, 0.01F, "FileSelector item 0 base X should match original ring math");
            require_near(item0.y, -1606.9690F, 0.01F, "FileSelector item 0 base Y should match original 20-degree ring pitch");
            require_near(item0.z, 4415.1113F, 0.01F, "FileSelector item 0 base Z should match original ring math");
            const auto &item2 = selector.getItemBasePosition(2);
            require_near(item2.x, 5000.0F, 0.01F, "FileSelector item 2 base X should match original ring radius");
            require_near(item2.y, 0.0F, 0.01F, "FileSelector item 2 base Y should match original ring pitch");
            require_near(item2.z, 0.0F, 0.01F, "FileSelector item 2 base Z should match original ring pitch");

            const auto snapshot = runtime.scheduler().snapshot();
            const auto has_selector_parent = std::ranges::any_of(snapshot, [](const auto &entry) {
                return entry.kind == smgpc::game::SceneEntryKind::NameObj && entry.name == "ファイルセレクタ" &&
                       entry.movement_type == MR::MovementType_Environment && entry.calc_anim_type < 0 && entry.draw_buffer_type < 0;
            });
            const auto item_parent_count =
                std::ranges::count_if(snapshot, [](const auto &entry) {
                    return entry.kind == smgpc::game::SceneEntryKind::NameObj && entry.name == "ファイルセレクトアイテム" &&
                           entry.movement_type == MR::MovementType_MapObj && entry.calc_anim_type < 0 && entry.draw_buffer_type < 0;
                });
            const auto item_new_model_count =
                std::ranges::count_if(snapshot, [](const auto &entry) {
                    return entry.kind == smgpc::game::SceneEntryKind::LiveActorModel && entry.name == "ニューフェイス" &&
                           entry.draw_buffer_type == MR::DrawBufferType_MapObj;
                });
            const auto item_fellow_model_count =
                std::ranges::count_if(snapshot, [](const auto &entry) {
                    return entry.kind == smgpc::game::SceneEntryKind::LiveActorModel && entry.name == "キャラフェイス" &&
                           entry.movement_type == MR::MovementType_NPC && entry.calc_anim_type == MR::CalcAnimType_NPC &&
                           entry.draw_buffer_type == MR::DrawBufferType_NPC;
                });
            const auto select_effect_model_count =
                std::ranges::count_if(snapshot, [](const auto &entry) {
                    return entry.kind == smgpc::game::SceneEntryKind::LiveActorModel && entry.name == "選択時エフェクト" &&
                           entry.movement_type == MR::MovementType_MapObj && entry.calc_anim_type == MR::CalcAnimType_MapObj &&
                           entry.draw_buffer_type == MR::DrawBufferType_MapObj;
                });
            require(has_selector_parent, "FileSelector should register itself as the original environment movement actor");
            require(item_parent_count == 6 && item_new_model_count == 6 && item_fellow_model_count == 30 && select_effect_model_count == 6,
                    "FileSelector should register file-select item parents, original model children, and six selection-effect MapObj actors");

            auto renderer = RecordingRenderer();
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            runtime.draw_3d_normal(renderer);
            require(runtime.camera_system().active_programmable_camera_name().has_value() &&
                        *runtime.camera_system().active_programmable_camera_name() == "ファイル選択カメラ",
                    "FileSelector should activate the original programmable title camera event");
            require(runtime.camera_system().programmable_camera_param_count() == 1U &&
                        runtime.camera_system().programmable_camera_fovy_count() == 1U,
                    "FileSelector title camera should be emitted through MR programmable camera params");
            require(runtime.last_camera_pose().has_value(), "FileSelector should provide a Game-driven draw camera pose");
            require_near(runtime.last_camera_pose()->eye.y, 15800.0F, 0.001F, "FileSelector title camera eye Y should come from Game control");
            require_near(runtime.last_camera_pose()->watch.y, 0.0F, 0.001F,
                         "FileSelector first title camera frame should preserve the original one-frame latched watch point");
            require(!std::ranges::any_of(runtime.semantic_trace_events(),
                                         [](const auto &event) {
                                             return event.category == "camera" && event.name == "missing_scene_camera_pose";
                                         }),
                    "FileSelector draw should not use the compat default camera fallback");
            require(!selector.isTitleStarted(), "FileSelector WaitBind should not start the title by itself");
            const auto &item0_position = selector.getItemPosition(0);
            require_near(item0_position.x, item0.x * 0.05F, 0.01F, "FileSelector item position should ease toward original base X");
            require_near(item0_position.y, item0.y * 0.05F, 0.01F, "FileSelector item position should ease toward original base Y");
            require_near(item0_position.z, item0.z * 0.05F, 0.01F, "FileSelector item position should ease toward original base Z");
            require(selector.receiveOtherMsg(ACTMES_UPDATE_BASEMTX), "FileSelector should accept UpdateBaseMtx messages");
            require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should accept AutoRushBegin while waiting for bind");
            require(!selector.isTitleStarted(), "FileSelector should defer title startup until the Title nerve first step");

            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 1U,
                .frame_time_seconds = 1.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            runtime.draw_3d_normal(renderer);
            require_near(runtime.last_camera_pose()->watch.y, 15800.0F, 0.001F,
                         "FileSelector title camera should latch the title watch point from Game control on the next frame");
            require(selector.isTitleStarted(), "FileSelector should start TitleSequenceProduct on the first Title nerve step");
            require(selector.isTitleActive(), "FileSelector title should be active after AutoRushBegin");
            require(!selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should not restart title outside WaitBind");
        }

        $test("matches FileSelector title-end to file-select transition") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto selector = FileSelector("ファイルセレクタ");
            selector.initWithoutIter();

            require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should accept AutoRushBegin for title-end integration test");

            auto saw_title_end = false;
            auto saw_camera_transition_before_far_point = false;

            for (std::uint64_t frame = 0; frame < 720U && !selector.isFileSelectStarted(); ++frame) {
                window.set_core_buttons(frame >= 160U, frame >= 160U);
                runtime.begin_frame(smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });

                if (selector.isTitleEnded()) {
                    saw_title_end = true;
                    saw_camera_transition_before_far_point = saw_camera_transition_before_far_point || !selector.isCameraAtFarPoint();
                }
            }

            require(saw_title_end, "FileSelector should enter TitleEnd after the original title sequence is decided");
            require(saw_camera_transition_before_far_point,
                    "FileSelector TitleEnd should wait for camera far-point transition instead of finishing immediately");
            require(selector.didAppearAllItems(), "FileSelector TitleEnd should appear file-select items on first step");
            require(selector.getAppearedItemCount() == 6, "FileSelector TitleEnd should fan out appear to all six items");
            require(selector.didInitAllItems(), "FileSelector TitleEnd should initialize file-select items on first step");
            require(selector.didInvalidateSelectAll(), "FileSelector TitleEnd should invalidate file-select item selection on first step");
            require(selector.getSelectInvalidItemCount() == 0, "FileSelector FileSelect first step should revalidate all six file-select items");
            require(selector.getMiiValidIndexCollectionCount() == 1, "FileSelector TitleEnd should collect valid Mii indices once");
            require(selector.getBasePosRatio() == 0.0F, "FileSelector TitleEnd should calculate base positions with ratio 0");
            require(selector.isCameraAtFarPoint(), "FileSelector camera should reach far point before FileSelect starts");
            require(selector.didValidateRotateAllItems(), "FileSelector should validate item rotation after the camera reaches far point");
            require(selector.getRotateInvalidItemCount() == 0, "FileSelector should fan out rotate validation to all six items");
            require(selector.isFileSelectStarted(), "FileSelector should enter the FileSelect nerve after TitleEnd camera completion");
            require(selector.didAppearAllIndex(), "FileSelector FileSelect first step should appear all file-number indices");
            require(!selector.isOperationButtonAlive() && !selector.isFileInfoAlive(),
                    "FileSelector FileSelect should wait for item pointing/confirmation before showing file-info and operation buttons");
            require(runtime.current_stage_bgm_name() == "MBGM_FILE_SELECT", "FileSelector TitleEnd should start the original file-select BGM");

            const auto trace_path = std::filesystem::path(".cache/tests/runtime-file-select-generic-trace.ndjson");
            smgpc::game::write_runtime_parity_trace(trace_path,
                                                    smgpc::render::FrameContext{
                                                        .frame_index = runtime.frame_index(),
                                                        .frame_time_seconds = static_cast<double>(runtime.frame_index()) / 60.0,
                                                        .frame_delta_seconds = 1.0 / 60.0,
                                                        .framebuffer = {.width = 640U, .height = 456U},
                                                        .has_focus = true,
                                                        .is_minimized = false,
                                                    },
                                                    runtime);
            const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
            const auto &snapshot = trace_json.at("scene_snapshot");
            const auto selector_entry = std::ranges::find_if(snapshot, [](const auto &entry) { return entry.at("name") == "ファイルセレクタ"; });
            const auto item_actor_count = std::ranges::count_if(snapshot, [](const auto &entry) {
                return entry.at("name") == "ファイルセレクトアイテム" && !entry.at("live_actor").is_null();
            });
            const auto select_effect_actor_count = std::ranges::count_if(snapshot, [](const auto &entry) {
                return entry.at("name") == "選択時エフェクト" && !entry.at("live_actor").is_null();
            });
            require(selector_entry != snapshot.end() && !selector_entry->at("live_actor").is_null() &&
                        selector_entry->at("live_actor").at("nerve_step") >= 0 && item_actor_count == 6 && select_effect_actor_count == 6,
                    "runtime parity trace should expose FileSelector, item, and selection-effect state through the generic scene actor snapshot");
            require(trace_json.at("runtime_services").at("rfl").at("initialized") == true &&
                        trace_json.at("runtime_services").at("rfl").at("valid_mii_count") == 1 &&
                        trace_json.at("runtime_services").at("save").at("file_count") == 0 &&
                        trace_json.at("runtime_services").at("save").at("slot_count") == 6 &&
                        trace_json.at("runtime_services").at("save").at("slots").front().at("created") == false,
                    "runtime parity trace should expose RFL and save state through generic runtime service diagnostics");
            const auto &layouts = trace_json.at("layout_runtime");
            require(std::ranges::any_of(layouts,
                                        [](const auto &entry) {
                                            return entry.at("name") == "ファイル情報" && entry.at("layout_name") == "FileInfo" &&
                                                   entry.at("dead") == true;
                                        }) &&
                        std::ranges::any_of(layouts,
                                            [](const auto &entry) {
                                                return entry.at("name") == "ファイル選択ボタン" && entry.at("layout_name") == "FileSelect" &&
                                                       entry.at("dead") == true && entry.at("button_controllers").size() == 5;
                                            }) &&
                        std::ranges::any_of(layouts,
                                            [](const auto &entry) {
                                                return entry.at("name") == "戻るボタン" && entry.at("layout_name") == "BackButton";
                                            }) &&
                        std::ranges::any_of(layouts,
                                            [](const auto &entry) {
                                                return entry.at("name") == "ルイージ切り替えボタン" &&
                                                       entry.at("layout_name") == "BrosButton";
                                            }) &&
                        std::ranges::any_of(layouts,
                                            [](const auto &entry) {
                                                return entry.at("name") == "インフォメーションメッセージ" &&
                                                       entry.at("layout_name") == "InformationWindow";
                                            }) &&
                        std::ranges::any_of(layouts,
                                            [](const auto &entry) {
                                                return entry.at("name") == "Mii確認用アイコン" &&
                                                       entry.at("layout_name") == "MiiConfirmIcon";
                                            }) &&
                        std::ranges::any_of(layouts,
                                            [](const auto &entry) {
                                                return entry.at("name") == "２Ｐマニュアル" && entry.at("layout_name") == "P2Manual";
                                            }),
                    "runtime parity trace should expose FileSelector's original-shaped file-select child layouts");
        }

        $test("matches FileSelector item pointing and file-confirm entry flow") {
            auto make_frame = [](std::uint64_t frame) {
                return smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
            };
            auto has_audio_event = [](const smgpc::game::RuntimeContext &runtime, smgpc::game::AudioEventKind kind, std::string_view name) {
                return std::ranges::any_of(runtime.audio().events(), [&](const smgpc::game::AudioEvent &event) {
                    return event.kind == kind && event.name == name;
                });
            };

            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            runtime.save_data().set_slot_state(2, smgpc::game::SaveDataService::SlotState{
                                                      .created = true,
                                                      .last_loaded_mario = true,
                                                      .power_star_num = 3,
                                                      .star_piece_num = 40,
                                                      .icon_id = 1U,
                                                      .view_normal_ending = false,
                                                      .view_complete_ending = false,
                                                      .complete_ending_mario_and_luigi = false,
                                                      .game_event_flags = {},
                                                      .game_event_values = {},
                                                      .last_modified = 0,
                                                  });
            auto selector = FileSelector("ファイルセレクタ");
            selector.initWithoutIter();

            require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector confirm test should start through AutoRushBegin");
            for (std::uint64_t frame = 0; frame < 720U && !selector.isFileSelectStarted(); ++frame) {
                window.set_core_buttons(frame >= 160U, frame >= 160U);
                runtime.begin_frame(make_frame(frame));
            }
            window.set_core_buttons(false, false);
            runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            require(selector.isFileSelect() && selector.didAppearAllIndex(), "FileSelector confirm test should reach original FileSelect");
            require(runtime.audio().stage_bgm_state() == 5 && runtime.audio().stage_bgm_state_change_frames() == 0x3cU,
                    "FileSelector far camera state should request the original far-point file-select BGM state");

            auto *item = selector.getItemByFileNo(2);
            require(item != nullptr && !item->isNew(), "FileSelector confirm test should use an existing save-slot item");
            require(runtime.scene_camera_pose().has_value(), "FileSelector confirm test should have an active FileSelect camera pose");
            const auto &item_position = item->getPosition();
            const auto item_pointer = project_world_to_screen(*runtime.scene_camera_pose(), {.x = item_position.x, .y = item_position.y + 900.0F, .z = item_position.z});
            window.set_pointer(item_pointer.x, item_pointer.y, true);
            runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            require(selector.getPreviousPointingFileNo() == 2 && selector.getCurrentFileInfoFileNo() == 2 && selector.wasItemPointed(2),
                    "FileSelector FileSelect should promote star-pointer-targeted items into FileSelectInfo through updateFileInfo");
            require(selector.isFileInfoAlive(), "FileSelector should show file-info after pointing at an existing item");

            window.set_input_pressed(smgpc::render::InputButton::CORE_PAD_A, true);
            runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            window.set_input_pressed(smgpc::render::InputButton::CORE_PAD_A, false);
            require(selector.isFileConfirmStart() && selector.getSelectedFileNo() == 2,
                    "FileSelector item star-pointer decide should enter FileConfirmStart and remember the selected file number");
            require(has_audio_event(runtime, smgpc::game::AudioEventKind::SystemSoundStart, "SE_SY_GALAXY_SELECTED"),
                    "FileSelector item select should start the original galaxy-selected sound");
            require(has_system_sound_event_prefix(runtime, "ME_ASTRO_DOME_SELECT"),
                    "FileSelector item select should start one of the original selected ME cues");

            for (auto i = 0; i < 80 && !selector.isFileConfirm(); ++i) {
                runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            }
            require(selector.didStartFileConfirmStart(), "FileSelector FileConfirmStart should run first-step side effects");
            require(selector.didDisappearAllIndex(), "FileSelector FileConfirmStart should disappear all file-number indices");
            require(selector.didItemTurnToFront(2) && selector.getItemTurnToFrontFrameCount(2) == 0x28,
                    "FileSelector FileConfirmStart should turn the selected item to the front over the original frame count");
            require(selector.isCameraAtNearPoint() && selector.isFileConfirm(),
                    "FileSelector FileConfirmStart should wait for the near-point camera before FileConfirm");
            require(runtime.audio().stage_bgm_state() == 6 && runtime.audio().stage_bgm_state_change_frames() == 0x3cU,
                    "FileSelector near camera state should request the original near-point file-confirm BGM state");

            runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            require(selector.didStartFileConfirm(), "FileSelector FileConfirm should run first-step layout side effects");
            require(selector.isOperationButtonAlive() && selector.isFileInfoAlive(),
                    "FileSelector FileConfirm should show the operation button and file-info layouts");
            require(selector.getCurrentFileInfoFileNo() == 2, "FileSelector FileConfirm should keep FileSelectInfo on the selected file");

            window.set_core_buttons(false, true);
            runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            window.set_core_buttons(false, false);
            require(has_audio_event(runtime, smgpc::game::AudioEventKind::SystemSoundStart, "SE_SY_GALAXY_DECIDE_CANCEL"),
                    "FileSelector FileConfirm should route B-trigger back selection through checkSelectedBackButton");
            require(selector.isFileSelectStart() && selector.didClearPointing(),
                    "FileSelector FileConfirm B-trigger back selection should clear pointing and return toward FileSelectStart");
        }

        $test("matches FileSelector create and delete save continuations") {
            auto make_frame = [](std::uint64_t frame) {
                return smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
            };
            auto advance_one = [&](smgpc::game::RuntimeContext &runtime) {
                runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            };
            auto advance_to_file_select = [&](smgpc::game::RuntimeContext &runtime, TestWindowService &window, FileSelector &selector) {
                require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN),
                        "FileSelector create/delete test should begin from the original title autorush gate");

                for (std::uint64_t frame = 0; frame < 720U && !selector.isFileSelectStarted(); ++frame) {
                    window.set_core_buttons(frame >= 160U, frame >= 160U);
                    runtime.begin_frame(make_frame(frame));
                }

                window.set_core_buttons(false, false);
                require(selector.isFileSelectStarted() && selector.isFileSelect(),
                        "FileSelector create/delete test should reach FileSelect before selecting an item");
            };
            auto pulse_yes_until = [&](smgpc::game::RuntimeContext &runtime, TestWindowService &window, auto &&condition, const char *message) {
                for (auto i = 0; i < 360 && !condition(); ++i) {
                    set_pointer_to_sys_info_yes(window);
                    window.set_core_buttons(i % 6 == 1, i % 6 == 1);
                    advance_one(runtime);
                }
                window.set_core_buttons(false, false);
                set_pointer_to_sys_info_yes(window);
                require(condition(), message);
            };

            {
                auto logger = NullLogger();
                auto window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(logger, window);
                auto selector = FileSelector("ファイルセレクタ");
                selector.initWithoutIter();
                advance_to_file_select(runtime, window, selector);

                auto *new_item = selector.getItemByFileNo(1);
                require(new_item != nullptr && new_item->isNew(), "FileSelector create test should start from a new save slot item");
                selector.notifyItem(new_item, 1);
                require(selector.isCreateConfirmStart() && selector.getSelectedFileNo() == 1,
                        "selecting a new file item should enter CreateConfirmStart and remember the file number");

                for (auto i = 0; i < 120 && !selector.didStartCreateConfirm(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartCreateConfirm() && selector.isCreateConfirm(),
                        "FileSelector CreateConfirm should show the original yes/no system window after the near camera arrives");

                pulse_yes_until(runtime, window, [&]() { return selector.didStartCreate(); }, "FileSelector CreateConfirm yes should advance to the original Create save sequence");
                for (auto i = 0; i < 240 && selector.isCreate(); ++i) {
                    advance_one(runtime);
                }

                const auto *created_slot = runtime.save_data().slot_state(1);
                require(created_slot != nullptr && created_slot->created && created_slot->last_loaded_mario &&
                            (selector.isMiiSelectStartFirst() || selector.isMiiSelect()),
                        "FileSelector Create should persist the new user file and continue into the native first icon-selection state");
                for (auto i = 0; i < 180 && !selector.didStartMiiSelectStart(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartMiiSelectStart() && (selector.isMiiSelectStartFirst() || selector.isMiiSelect()),
                        "FileSelector Create should open MiiSelect instead of taking the old PC-only FileSelectStart shortcut");
            }

            {
                auto logger = NullLogger();
                auto window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(logger, window);
                runtime.save_data().set_slot_state(2, smgpc::game::SaveDataService::SlotState{
                                                          .created = true,
                                                          .last_loaded_mario = true,
                                                          .power_star_num = 3,
                                                          .star_piece_num = 40,
                                                          .icon_id = 1U,
                                                          .view_normal_ending = false,
                                                          .view_complete_ending = false,
                                                          .complete_ending_mario_and_luigi = false,
                                                          .game_event_flags = {},
                                                          .game_event_values = {},
                                                          .last_modified = 0,
                                                      });
                auto selector = FileSelector("ファイルセレクタ");
                selector.initWithoutIter();
                advance_to_file_select(runtime, window, selector);

                auto *existing_item = selector.getItemByFileNo(2);
                require(existing_item != nullptr && !existing_item->isNew(), "FileSelector delete test should start from an existing save slot item");
                selector.notifyItem(existing_item, 1);
                for (auto i = 0; i < 120 && !selector.didStartFileConfirm(); ++i) {
                    advance_one(runtime);
                }
                require(selector.isFileConfirm() && selector.didStartFileConfirm(),
                        "FileSelector delete test should reach FileConfirm before invoking the delete operation");

                selector.callbackDelete();
                require(selector.isDeleteConfirmStart(), "FileSelector delete callback should enter DeleteConfirmStart from FileConfirm");
                for (auto i = 0; i < 120 && !selector.didStartDeleteConfirm(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartDeleteConfirm() && selector.isDeleteConfirm(),
                        "FileSelector DeleteConfirm should show the original yes/no system window after hiding file-confirm layouts");

                pulse_yes_until(runtime, window, [&]() { return selector.didStartDelete(); }, "FileSelector DeleteConfirm yes should advance to the original Delete save sequence");
                for (auto i = 0; i < 240 && !selector.didStartDeleteDemo(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartDeleteDemo(), "FileSelector Delete should start DeleteDemo formatting");
                for (auto i = 0; i < 120 && !(existing_item->isNew() && (selector.isFileSelectStart() || selector.isFileSelect())); ++i) {
                    advance_one(runtime);
                }

                const auto *deleted_slot = runtime.save_data().slot_state(2);
                require(existing_item->isNew() && deleted_slot != nullptr && !deleted_slot->created &&
                            (selector.isFileSelectStart() || selector.isFileSelect()),
                        "FileSelector Delete should persist an empty user file, run DeleteDemo formatting, and return toward FileSelectStart");
            }
        }

        $test("matches FileSelector copy save continuations") {
            auto make_frame = [](std::uint64_t frame) {
                return smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
            };
            auto advance_one = [&](smgpc::game::RuntimeContext &runtime) {
                runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            };
            auto advance_to_file_select = [&](smgpc::game::RuntimeContext &runtime, TestWindowService &window, FileSelector &selector) {
                require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN),
                        "FileSelector copy test should begin from the original title autorush gate");

                for (std::uint64_t frame = 0; frame < 720U && !selector.isFileSelectStarted(); ++frame) {
                    window.set_core_buttons(frame >= 160U, frame >= 160U);
                    runtime.begin_frame(make_frame(frame));
                }

                window.set_core_buttons(false, false);
                require(selector.isFileSelectStarted() && selector.isFileSelect(),
                        "FileSelector copy test should reach FileSelect before selecting the source item");
            };
            auto advance_existing_item_to_confirm = [&](smgpc::game::RuntimeContext &runtime, FileSelector &selector, s32 file_no) {
                auto *item = selector.getItemByFileNo(file_no);
                require(item != nullptr && !item->isNew(), "FileSelector copy test should select an existing source item");
                selector.notifyItem(item, 1);
                for (auto i = 0; i < 140 && !selector.didStartFileConfirm(); ++i) {
                    advance_one(runtime);
                }
                require(selector.isFileConfirm() && selector.getSelectedFileNo() == file_no,
                        "FileSelector copy test should enter FileConfirm on the source slot");
            };
            auto advance_to_copy_select = [&](smgpc::game::RuntimeContext &runtime, FileSelector &selector) {
                selector.callbackCopy();
                require(selector.isCopyWait(), "FileSelector copy callback should enter CopyWait");
                for (auto i = 0; i < 180 && !selector.didStartCopySelect(); ++i) {
                    advance_one(runtime);
                }
                require(selector.isCopySelect() && selector.didStartCopySelect() && selector.getSelectInvalidItemCount() == 1,
                        "FileSelector CopySelect should validate all slots except the copied source slot");
            };
            auto pulse_yes_until = [&](smgpc::game::RuntimeContext &runtime, TestWindowService &window, auto &&condition, const char *message) {
                for (auto i = 0; i < 360 && !condition(); ++i) {
                    set_pointer_to_sys_info_yes(window);
                    window.set_core_buttons(i % 6 == 1, i % 6 == 1);
                    advance_one(runtime);
                }
                window.set_core_buttons(false, false);
                set_pointer_to_sys_info_yes(window);
                require(condition(), message);
            };

            {
                auto logger = NullLogger();
                auto window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(logger, window);
                runtime.save_data().set_slot_state(2, smgpc::game::SaveDataService::SlotState{
                                                          .created = true,
                                                          .last_loaded_mario = false,
                                                          .power_star_num = 17,
                                                          .star_piece_num = 120,
                                                          .icon_id = 4U,
                                                          .view_normal_ending = false,
                                                          .view_complete_ending = false,
                                                          .complete_ending_mario_and_luigi = false,
                                                          .game_event_flags = {},
                                                          .game_event_values = {},
                                                          .last_modified = 0,
                                                      });
                auto selector = FileSelector("ファイルセレクタ");
                selector.initWithoutIter();
                advance_to_file_select(runtime, window, selector);
                advance_existing_item_to_confirm(runtime, selector, 2);
                advance_to_copy_select(runtime, selector);

                auto *new_destination = selector.getItemByFileNo(1);
                require(new_destination != nullptr && new_destination->isNew(), "FileSelector copy test should use a new destination slot");
                selector.notifyItem(new_destination, 1);
                require(selector.isCopyConfirmStart() && selector.getSelectedFileNo() == 1,
                        "selecting a copy destination should enter CopyConfirmStart and remember the destination file number");

                for (auto i = 0; i < 120 && !selector.didStartCopyConfirm(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartCopyConfirm() && selector.isCopyConfirm(),
                        "FileSelector CopyConfirm should show the original yes/no copy system window");

                pulse_yes_until(runtime, window, [&]() { return selector.didStartCopySave(); }, "FileSelector CopyConfirm yes should advance to CopySave for a new destination");
                for (auto i = 0; i < 260 && !selector.didStartCopyDemo(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartCopyDemo(), "FileSelector CopySave should start CopyDemo for a new destination");
                for (auto i = 0; i < 180 && !selector.isFileSelect(); ++i) {
                    advance_one(runtime);
                }

                const auto *copied_slot = runtime.save_data().slot_state(1);
                require(selector.isFileSelect() && copied_slot != nullptr && copied_slot->created &&
                            !copied_slot->last_loaded_mario && copied_slot->power_star_num == 17 && copied_slot->star_piece_num == 120 &&
                            copied_slot->icon_id == 4U && new_destination->isExist(),
                        "FileSelector CopySave should persist source data into a new destination and run CopyDemo");
            }

            {
                auto logger = NullLogger();
                auto window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(logger, window);
                runtime.save_data().set_slot_state(2, smgpc::game::SaveDataService::SlotState{
                                                          .created = true,
                                                          .last_loaded_mario = true,
                                                          .power_star_num = 33,
                                                          .star_piece_num = 440,
                                                          .icon_id = 5U,
                                                          .view_normal_ending = false,
                                                          .view_complete_ending = false,
                                                          .complete_ending_mario_and_luigi = false,
                                                          .game_event_flags = {},
                                                          .game_event_values = {},
                                                          .last_modified = 0,
                                                      });
                runtime.save_data().set_slot_state(3, smgpc::game::SaveDataService::SlotState{
                                                          .created = true,
                                                          .last_loaded_mario = false,
                                                          .power_star_num = 1,
                                                          .star_piece_num = 2,
                                                          .icon_id = 1U,
                                                          .view_normal_ending = false,
                                                          .view_complete_ending = false,
                                                          .complete_ending_mario_and_luigi = false,
                                                          .game_event_flags = {},
                                                          .game_event_values = {},
                                                          .last_modified = 0,
                                                      });
                auto selector = FileSelector("ファイルセレクタ");
                selector.initWithoutIter();
                advance_to_file_select(runtime, window, selector);
                advance_existing_item_to_confirm(runtime, selector, 2);
                advance_to_copy_select(runtime, selector);

                auto *existing_destination = selector.getItemByFileNo(3);
                require(existing_destination != nullptr && !existing_destination->isNew(),
                        "FileSelector overwrite copy test should use an existing destination slot");
                selector.notifyItem(existing_destination, 1);
                for (auto i = 0; i < 120 && !selector.didStartCopyConfirm(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartCopyConfirm() && selector.isCopyConfirm(),
                        "FileSelector overwrite copy should reach the original copy confirm window");

                pulse_yes_until(runtime, window, [&]() { return selector.didStartCopySave(); }, "FileSelector overwrite copy confirm yes should advance to CopySave");
                for (auto i = 0; i < 30 && !selector.didStartCopySaveMii(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartCopySaveMii(), "FileSelector overwrite copy should route through CopySaveMii like the original");
                for (auto i = 0; i < 260 && !selector.didStartCopyDemo(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartCopyDemo(), "FileSelector CopySaveMii should start CopyDemo for an overwrite destination");
                for (auto i = 0; i < 180 && !selector.isFileSelect(); ++i) {
                    advance_one(runtime);
                }

                const auto *copied_slot = runtime.save_data().slot_state(3);
                require(selector.isFileSelect() && copied_slot != nullptr && copied_slot->created &&
                            copied_slot->last_loaded_mario && copied_slot->power_star_num == 33 && copied_slot->star_piece_num == 440 &&
                            copied_slot->icon_id == 5U && existing_destination->isExist(),
                        "FileSelector CopySaveMii should persist source data over an existing destination and run CopyDemo");
            }
        }

        $test("matches FileSelector operation callback wait-state entry behavior") {
            auto make_frame = [](std::uint64_t frame) {
                return smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                };
            };
            auto advance_one = [&](smgpc::game::RuntimeContext &runtime) {
                runtime.begin_frame(make_frame(runtime.frame_index() + 1U));
            };
            auto advance_to_file_select = [&](smgpc::game::RuntimeContext &runtime, TestWindowService &window, FileSelector &selector) {
                require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN),
                        "FileSelector callback test should begin from the original title autorush gate");

                for (std::uint64_t frame = 0; frame < 720U && !selector.isFileSelectStarted(); ++frame) {
                    window.set_core_buttons(frame >= 160U, frame >= 160U);
                    runtime.begin_frame(make_frame(frame));
                }

                require(selector.isFileSelectStarted(), "FileSelector callback test should reach FileSelect before invoking operation callbacks");
                require(selector.didAppearAllIndex(), "FileSelector callback test should reach original FileSelect first-step index appearance");
            };
            auto run_case = [&](auto &&body) {
                auto logger = NullLogger();
                auto window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(logger, window);
                auto selector = FileSelector("ファイルセレクタ");
                selector.initWithoutIter();
                advance_to_file_select(runtime, window, selector);
                body(runtime, window, selector);
            };
            auto has_audio_event = [](const smgpc::game::RuntimeContext &runtime, smgpc::game::AudioEventKind kind,
                                      std::string_view name, s32 fade_frames = 0) {
                return std::ranges::any_of(runtime.audio().events(), [&](const smgpc::game::AudioEvent &event) {
                    return event.kind == kind && event.name == name && event.fade_frames == fade_frames;
                });
            };

            run_case([&](smgpc::game::RuntimeContext &runtime, TestWindowService &, FileSelector &selector) {
                selector.callbackStart();
                require(selector.wasStartCallbackCalled() && selector.isDemoStartWait(),
                        "FileSelector start callback should enter DemoStartWait like the original");
                advance_one(runtime);
                require(selector.didStartDemoStartWait(), "FileSelector DemoStartWait should run first-step side effects");
                require(has_audio_event(runtime, smgpc::game::AudioEventKind::SystemSoundStart, "SE_SY_FILE_SELECTED"),
                        "FileSelector DemoStartWait should start the original file-selected system sound");
                require(has_audio_event(runtime, smgpc::game::AudioEventKind::StageBgmStop, "MBGM_FILE_SELECT", 0x5a),
                        "FileSelector DemoStartWait should stop file-select BGM with the original fade");
                require(runtime.scene_wipe().is_active() && runtime.scene_wipe().remaining_frames() == 0x3c &&
                            runtime.scene_wipe().events().back().kind == smgpc::game::WipeEventKind::Close,
                        "FileSelector DemoStartWait should close the scene fade wipe over the original 60 frames");
                for (auto i = 0; i < 0x3c; ++i) {
                    advance_one(runtime);
                }
                require(selector.isDemo() && runtime.scene_wipe().is_blank(),
                        "FileSelector DemoStartWait should advance to Demo once the scene wipe is blank");
                advance_one(runtime);
                require(selector.didStartDemo(), "FileSelector Demo should run after the blank-wipe transition");
                require(runtime.sequence_requests().is_change_stage_in_game_after_loading_game_data_requested(),
                        "FileSelector Demo should request the original stage change after loading game data");
            });

            run_case([&](smgpc::game::RuntimeContext &runtime, TestWindowService &, FileSelector &selector) {
                selector.callbackCopy();
                require(selector.wasCopyCallbackCalled() && selector.isCopyWait(),
                        "FileSelector copy callback should enter CopyWait like the original");
                advance_one(runtime);
                require(selector.didStartCopyWait(), "FileSelector CopyWait should run first-step side effects");
                require(selector.didClearPointing(), "FileSelector CopyWait should clear file-select pointing state");
                require(selector.getBasePosRatio() == 0.0F, "FileSelector CopyWait should recalculate base positions with ratio 0");
                require(selector.isCopyWait() || selector.isCopySelect(),
                        "FileSelector CopyWait should either wait for the far camera or advance to CopySelect");
                require(has_audio_event(runtime, smgpc::game::AudioEventKind::SystemSoundStart, "SE_SY_FILE_SEL_UPPER_DECIDE"),
                        "FileSelector CopyWait should start the original upper-menu decide sound");
            });

            run_case([&](smgpc::game::RuntimeContext &runtime, TestWindowService &, FileSelector &selector) {
                selector.validateSelectAll();
                require(selector.getSelectInvalidItemCount() == 0, "FileSelector Mii callback precondition should restore selectable items");
                selector.callbackMii();
                require(selector.wasMiiCallbackCalled() && selector.didInvalidateSelectAll() && selector.isMiiWait(),
                        "FileSelector Mii callback should hide layouts, invalidate file items, and enter MiiWait");
                advance_one(runtime);
                require(selector.didStartMiiWait(), "FileSelector MiiWait should run first-step side effects");
                require(selector.isMiiWait() || selector.isMiiSelectStart(),
                        "FileSelector MiiWait should either wait for hidden layouts or advance to MiiSelectStart");
                require(has_audio_event(runtime, smgpc::game::AudioEventKind::SystemSoundStart, "SE_SY_FILE_SEL_UPPER_DECIDE"),
                        "FileSelector MiiWait should start the original upper-menu decide sound");
                for (auto i = 0; i < 120 && !selector.didStartMiiSelectStart(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartMiiSelectStart(), "FileSelector MiiWait should continue into original-shaped MiiSelectStart");
                require(has_audio_event(runtime, smgpc::game::AudioEventKind::SystemSoundStart, "SE_SY_FILE_SEL_MIISEL_OPEN"),
                        "FileSelector MiiSelectStart should appear MiiSelect and start the original Mii-select open sound");
            });

            run_case([&](smgpc::game::RuntimeContext &runtime, TestWindowService &, FileSelector &selector) {
                selector.callbackDelete();
                require(selector.wasDeleteCallbackCalled() && selector.isDeleteConfirmStart(),
                        "FileSelector delete callback should enter DeleteConfirmStart like the original");
                advance_one(runtime);
                require(selector.didStartDeleteConfirmStart(), "FileSelector DeleteConfirmStart should run first-step side effects");
                require(selector.isDeleteConfirmStart() || selector.isDeleteConfirm(),
                        "FileSelector DeleteConfirmStart should wait for hidden layouts before DeleteConfirm");
                require(has_audio_event(runtime, smgpc::game::AudioEventKind::SystemSoundStart, "SE_SY_FILE_SEL_UPPER_DECIDE"),
                        "FileSelector DeleteConfirmStart should start the original upper-menu decide sound");
            });

            run_case([&](smgpc::game::RuntimeContext &runtime, TestWindowService &window, FileSelector &selector) {
                selector.callbackManual();
                require(selector.wasManualCallbackCalled() && selector.isManualStart(),
                        "FileSelector manual callback should start ManualStart like the original");
                require(has_audio_event(runtime, smgpc::game::AudioEventKind::SystemSoundStart, "SE_SY_FILE_SEL_TIPS_OPEN"),
                        "FileSelector manual callback should start the original tips-open system sound");
                advance_one(runtime);
                require(selector.didStartManualStart(), "FileSelector ManualStart should run first-step side effects");
                require(selector.isManualStart() || selector.isManual(),
                        "FileSelector ManualStart should wait for hidden layouts before Manual");
                for (auto i = 0; i < 120 && !selector.didStartManual(); ++i) {
                    advance_one(runtime);
                }
                require(selector.didStartManual() && selector.isManual(), "FileSelector Manual should appear the Manual2P layout");

                for (auto i = 0; i < 360 && !selector.isFileConfirm(); ++i) {
                    window.set_core_buttons(i % 8 == 1, i % 8 == 1);
                    advance_one(runtime);
                }
                window.set_core_buttons(false, false);
                require(selector.isFileConfirm(), "FileSelector Manual should return to FileConfirm once Manual2P closes");
            });
        }

    }  // namespace

    void run_runtime_scene_tests() {
        run_registered_tests(TEST_SUITE);
    }

}  // namespace smgpc::tests
