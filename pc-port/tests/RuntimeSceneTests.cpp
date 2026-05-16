#include "TestSuites.hpp"
#include "TestSupport.hpp"

namespace smgpc::tests {
    namespace {
        constexpr auto kTestSuite = std::string_view{"runtime/scene"};

        template <int Line>
        struct TestCase;

        $test("draws FileSelectItem using original planet model") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            auto item = FileSelectItem(1, true);

            item.appear();
            item.update({0.0F, 800.0F, 0.0F});
            item.draw(renderer, smgpc::game::file_select_far_camera_pose());

            require(renderer.texture_count > 0U, "FileSelectItem should load original FileSelectDataPlanet textures when drawn");
            require(renderer.triangle_batch_count > 0U || renderer.gx_material_batch_count > 0U,
                    "FileSelectItem should draw the original FileSelectDataPlanet model");
            require(renderer.submitted_indices > 0U, "FileSelectItem should submit original FileSelectDataPlanet geometry");
        }

        $test("draws FileSelectSky through LiveActor model compatibility") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            auto sky = FileSelectSky("ファイル選択空");

            sky.initWithoutIter();
            sky.appear();
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 0U,
                .frame_time_seconds = 0.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            runtime.draw_3d_normal(renderer, smgpc::game::file_select_title_camera_pose());

            require(sky.getNerveStep() == 1, "FileSelectSky should be updated by the scene sky actor compatibility list");
            require(renderer.texture_count > 0U, "FileSelectSky should load original CometNearOrbitSky textures through LiveActor model compat");
            require(renderer.triangle_batch_count > 0U || renderer.gx_material_batch_count > 0U,
                    "FileSelectSky should draw original CometNearOrbitSky model geometry");
            require(renderer.submitted_indices > 0U, "FileSelectSky should submit original sky J3D geometry");
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
            require(dvd.archive_load_count("ObjectData/CometNearOrbitSky.arc") == 1U, "DVD archive cache should record one physical load");
            require(dvd.cached_archive_count() == 1U, "DVD archive cache should keep one mounted archive entry");

            const auto bytes = dvd.read_file("files/ObjectData/CometNearOrbitSky.arc");
            require(bytes.size() > 0x40U, "DVD service should read disc-relative files with an optional files prefix");

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

            auto save = smgpc::game::SaveDataService();
            const std::array<std::uint8_t, 4U> save_bytes{1U, 2U, 3U, 4U};
            save.write_file("user/slot0.bin", save_bytes);
            require(save.exists("user/slot0.bin"), "save service should report deterministic fixture files");
            const auto loaded_save = save.read_file("user/slot0.bin");
            require(loaded_save.has_value() && *loaded_save == std::vector<std::uint8_t>(save_bytes.begin(), save_bytes.end()),
                    "save service should round-trip file bytes");
            require(save.erase("user/slot0.bin") && !save.exists("user/slot0.bin"), "save service should erase fixture files");

            auto messages = smgpc::game::MessageService();
            messages.set_message("FileSelect_NewFile", "New File");
            require(messages.message_or("FileSelect_NewFile", "fallback") == "New File", "message service should resolve fixture messages");
            require(messages.message_or("Missing", "fallback") == "fallback", "message service should provide deterministic fallback text");

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

            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 10U,
                .frame_time_seconds = 10.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            require(!runtime.is_core_pad_button_a(WPAD_CHAN0), "runtime should route released host input through WPAD state");

            window.set_title_combo(true);
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 11U,
                .frame_time_seconds = 11.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            require(runtime.is_core_pad_button_a(WPAD_CHAN0) && runtime.is_core_pad_button_b(WPAD_CHAN0),
                    "runtime should route host title combo through WPAD core buttons");
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
            runtime.begin_frame(smgpc::render::FrameContext{
                .frame_index = 12U,
                .frame_time_seconds = 12.0 / 60.0,
                .frame_delta_seconds = 1.0 / 60.0,
                .framebuffer = {.width = 640U, .height = 456U},
                .has_focus = true,
                .is_minimized = false,
            });
            require(runtime.is_stage_bgm_prepared(), "runtime BGM preparation should be frame based");
            runtime.unlock_stage_bgm();
            require(runtime.audio().is_stage_bgm_unlocked(), "runtime should route BGM unlock to the audio service");
            runtime.start_system_sound("SE_SY_CURSOR");
            runtime.start_cs_sound("CS_DECIDE");
            runtime.stop_stage_bgm(30);
            require(runtime.audio().events().size() == 5U, "audio service should keep separate deterministic event records");

            runtime.emit_effect("TitleLogo", "Decision");
            require(runtime.effects().active_effects("TitleLogo").size() == 1U, "runtime should route effect emission to the effect service");
            runtime.delete_effect_all("TitleLogo");
            require(runtime.effects().active_effects("TitleLogo").empty(), "runtime should route effect cleanup to the effect service");
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

            scheduler.execute_draw_buffer_list_normal(renderer, smgpc::game::file_select_title_camera_pose());
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

        $test("routes layout and sky actors through central scene scheduler") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();

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
                require(sky.getBaseMatrix().m == smgpc::game::file_select_sky_actor_matrix(0U).m,
                        "runtime scene scheduler should update sky base matrix during calcViewAndEntry");
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

                runtime.draw_3d_normal(renderer, smgpc::game::file_select_title_camera_pose());
                const auto draw_trace = runtime.scheduler().last_execution_trace();
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
                const auto trace_path = std::filesystem::path(".cache/tests/runtime-parity-trace.json");
                smgpc::game::write_runtime_parity_trace(trace_path, frame_context, runtime);
                const auto trace_json = smgpc::game::load_runtime_parity_trace(trace_path);
                require(trace_json.at("schema") == "smgpc-runtime-parity-trace-v1",
                        "runtime parity trace should write and load a stable schema identifier");
                require(trace_json.contains("camera_pose") && trace_json.contains("scene_trace"),
                        "runtime parity trace should include camera pose and scene execution evidence");
                const auto &layout_entries = trace_json.at("layout_runtime");
                require(std::ranges::any_of(layout_entries, [](const auto &entry) {
                            return entry.at("layout_name") == "TitleLogo" && !entry.at("animations").empty() &&
                                   entry.at("animations").front().at("name") == "Appear";
                        }),
                        "runtime parity trace should include layout animation frame evidence");
                const auto &scene_entries = trace_json.at("scene_trace");
                const auto has_phase = [&scene_entries](std::string_view phase) {
                    return std::ranges::any_of(scene_entries, [phase](const auto &entry) { return entry.at("phase") == phase; });
                };
                require(has_phase("DrawBufferOpa") && has_phase("DrawBufferXlu") && has_phase("DrawType"),
                        "runtime parity trace should include original-style draw-buffer and draw-type phases");
                const auto &packets = trace_json.at("render_packets");
                const auto has_packet = [&packets](std::string_view material, std::string_view mode) {
                    return std::ranges::any_of(packets, [material, mode](const auto &packet) {
                        return packet.at("material_name") == material && packet.at("packet_mode") == mode;
                    });
                };
                const auto space_packet = std::ranges::find_if(packets, [](const auto &packet) {
                    return packet.at("material_name") == "Space_Mat_v";
                });
                require(has_packet("Space_Mat_v", "ShaderGxTev") && has_packet("CometHalo_v", "ComposedMaterial") &&
                            space_packet != packets.end() && space_packet->at("model_name") == "CometNearOrbitSky" &&
                            space_packet->at("indirect_stage_count") == 0 && space_packet->at("matrix_group_index") == 0 &&
                            space_packet->at("display_list_size") == 3232 && space_packet->at("draw_packet_triangle_count") == 480 &&
                            space_packet->at("color_channels").front().contains("material_color") &&
                            space_packet->at("color_channels").front().contains("color_control") && space_packet->contains("fog_type") &&
                            space_packet->at("bck_active") == true && space_packet->at("bck_frame_max") == 3000 &&
                            space_packet->at("btk_active") == true && space_packet->at("btk_frame_max") == 10000 &&
                            space_packet->at("draw_pass") == "Opaque",
                        "runtime parity trace should include submitted J3D material packet sequence, animation frames, and draw state");
            }

            require(runtime.scheduler().snapshot().empty(), "runtime scene scheduler should unregister destroyed layout and sky actor entries");
        }

        $test("matches FileSelector title autorush gate behavior") {
            auto selector = FileSelector();
            require(selector.getSkyStep() == 0U, "FileSelector sky actor step should start at zero");
            require(selector.getItemCount() == 6, "FileSelector should create the six original file-select item slots");
            require(selector.getItemFileNo(0) == 1 && selector.getItemFileNo(1) == 2 && selector.getItemFileNo(2) == 4 &&
                        selector.getItemFileNo(3) == 6 && selector.getItemFileNo(4) == 5 && selector.getItemFileNo(5) == 3,
                    "FileSelector should preserve the original file-number order table");
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

            selector.update();
            require(!selector.isTitleStarted(), "FileSelector WaitBind should not start the title by itself");
            const auto &item0_position = selector.getItemPosition(0);
            require_near(item0_position.x, item0.x * 0.05F, 0.01F, "FileSelector item position should ease toward original base X");
            require_near(item0_position.y, item0.y * 0.05F, 0.01F, "FileSelector item position should ease toward original base Y");
            require_near(item0_position.z, item0.z * 0.05F, 0.01F, "FileSelector item position should ease toward original base Z");
            require(selector.receiveOtherMsg(ACTMES_UPDATE_BASEMTX), "FileSelector should accept UpdateBaseMtx messages");
            require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should accept AutoRushBegin while waiting for bind");
            require(!selector.isTitleStarted(), "FileSelector should defer title startup until the Title nerve first step");

            selector.update();
            require(selector.isTitleStarted(), "FileSelector should start TitleSequenceProduct on the first Title nerve step");
            require(selector.isTitleActive(), "FileSelector title should be active after AutoRushBegin");
            require(!selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should not restart title outside WaitBind");
        }

        $test("matches FileSelector title-end to file-select transition") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto selector = FileSelector();

            require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should accept AutoRushBegin for title-end integration test");

            auto saw_title_end = false;
            auto saw_camera_transition_before_far_point = false;

            for (std::uint64_t frame = 0; frame < 720U && !selector.isFileSelectStarted(); ++frame) {
                runtime.begin_frame(smgpc::render::FrameContext{
                    .frame_index = frame,
                    .frame_time_seconds = static_cast<double>(frame) / 60.0,
                    .frame_delta_seconds = 1.0 / 60.0,
                    .framebuffer = {.width = 640U, .height = 456U},
                    .has_focus = true,
                    .is_minimized = false,
                });

                window.set_title_combo(frame >= 160U);
                selector.update();

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
            require(selector.getSelectInvalidItemCount() == 6, "FileSelector TitleEnd should fan out select invalidation to all six items");
            require(selector.getMiiValidIndexCollectionCount() == 1, "FileSelector TitleEnd should collect valid Mii indices once");
            require(selector.getBasePosRatio() == 0.0F, "FileSelector TitleEnd should calculate base positions with ratio 0");
            require(selector.isCameraAtFarPoint(), "FileSelector camera should reach far point before FileSelect starts");
            require(selector.didValidateRotateAllItems(), "FileSelector should validate item rotation after the camera reaches far point");
            require(selector.getRotateInvalidItemCount() == 0, "FileSelector should fan out rotate validation to all six items");
            require(selector.isFileSelectStarted(), "FileSelector should enter the FileSelect nerve after TitleEnd camera completion");
            require(runtime.current_stage_bgm_name() == "MBGM_FILE_SELECT", "FileSelector TitleEnd should start the original file-select BGM");
        }

    }  // namespace

    void run_runtime_scene_tests() {
        run_registered_tests(kTestSuite);
    }

}  // namespace smgpc::tests
