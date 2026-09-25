target("smg-pc-debug-path-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/tools")
    add_includedirs("../src")
    add_files("DebugPathsTests.cpp")
    add_deps("smg-pc-debug-common")
    add_tests("debug_paths", {group = "tools", rundir = os.projectdir()})

target("smg-pc-game-source-mirror-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/source-boundary")
    set_rundir(os.projectdir())
    add_files("GameSourceMirrorTests.cpp")
    add_tests("game_source_mirrors", {
        group = "source-boundary",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-player-source-mirror-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/source-boundary")
    set_rundir(os.projectdir())
    add_files("PlayerSourceMirrorTests.cpp")
    add_tests("player_source_mirrors", {
        group = "source-boundary",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-mario-model-demo-surface-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "MarioModelDemoSurfaceTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("mario_model_demo_surface", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-original-camera-runtime-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), {force = true})
    add_cxxflags("-fno-builtin-sprintf", "-fno-builtin-snprintf", "-fno-builtin-vsprintf", "-fno-builtin-vsnprintf", {force = true})
    add_files {
        "CameraLocalUtilRuntimeTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("original_camera_runtime", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-only-camera-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), {force = true})
    add_cxxflags("-fno-builtin-sprintf", "-fno-builtin-snprintf", "-fno-builtin-vsprintf", "-fno-builtin-vsnprintf", {force = true})
    add_files {
        "OnlyCameraTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("only_camera", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-camera-view-interpolator-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), {force = true})
    add_cxxflags("-fno-builtin-sprintf", "-fno-builtin-snprintf", "-fno-builtin-vsprintf", "-fno-builtin-vsnprintf", {force = true})
    add_files {
        "CameraViewInterpolatorTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("camera_view_interpolator", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-camera-view-service-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), {force = true})
    add_cxxflags("-fno-builtin-sprintf", "-fno-builtin-snprintf", "-fno-builtin-vsprintf", "-fno-builtin-vsnprintf", {force = true})
    add_files {
        "CameraViewServiceTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("camera_view_service", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-file-select-exact-source-compile")
    set_kind("static")
    set_default(false)
    set_group("tests/source-boundary")
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), { force = true })
    add_cxxflags("-fno-builtin-sprintf", "-fno-builtin-snprintf", "-fno-builtin-vsprintf", "-fno-builtin-vsnprintf", {force = true})
    add_cxxflags("-include " .. path.join(os.projectdir(), "tests/FileSelectExactSourceCompileCompat.hpp"), { force = true })
    add_files {
        "../src/Game/Map/FileSelectEffect.cpp",
        "../src/Game/Map/FileSelectFunc.cpp",
        "../src/Game/Map/FileSelectItem.cpp",
        "../src/Game/Map/FileSelector.cpp",
        "../src/Game/Map/FileSelectSky.cpp",
        "../src/Game/Screen/FileSelectInfo.cpp",
        "../src/Game/Screen/FullScreenBlur.cpp"
    }
    add_deps("smg-pc-game")

target("smg-pc-file-select-exact-source-compile-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/source-boundary")
    set_rundir(os.projectdir())
    add_files("FileSelectExactSourceCompileTests.cpp")
    add_deps("smg-pc-file-select-exact-source-compile")
    add_tests("file_select_exact_source_compile", {
        group = "source-boundary",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-aurora-native-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "AuroraNativeTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("aurora_native", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-gx-copy-fifo-order-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("GxCopyFifoOrderTests.cpp")
    add_deps("smg-pc-game")
    add_tests("gx_copy_fifo_order", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-stage-start-camera-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "StageStartCameraTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("stage_start_camera", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-stationed-archive-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "StationedArchiveRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-dvd"
    }
    add_tests("stationed_archive_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-lod-ctrl-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "LodCtrlCompatTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("lod_ctrl_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-object-name-table-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "ObjectNameTableTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("object_name_table", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-nameobj-factory-placement-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "NameObjFactoryPlacementTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("nameobj_factory_placement", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-stage-collision-registration-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "StageCollisionRegistrationTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("stage_collision_registration", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-message-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "MessageRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("message_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-rfl-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "RflRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("rfl_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-fixed-position-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "FixedPositionRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("fixed_position_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-sceneobj-holder-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "SceneObjHolderRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("sceneobj_holder_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-camera-util-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "CameraUtilRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("camera_util_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-player-util-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "PlayerUtilRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("player_util_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-story-sequence-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "StorySequenceRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("story_sequence_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-live-actor-util-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "LiveActorUtilRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("live_actor_util_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-btp-real-resource-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "BtpRealResourceTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("btp_real_resource", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-save-data-core-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "SaveDataCoreRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("save_data_core_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-save-config-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "SaveConfigRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("save_config_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-layout-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "LayoutRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("layout_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-talk-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "TalkRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("talk_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-game-actor-physics-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "GameActorPhysicsRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("game_actor_physics_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-actor-sensor-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "ActorSensorRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("actor_sensor_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-actor-runtime-registry-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "ActorRuntimeRegistryTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("actor_runtime_registry", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-file-select-name-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "FileSelectNameRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("file_select_name_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-j3d-frame-ctrl-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "J3DFrameCtrlTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("j3d_frame_ctrl", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-original-j3d-joint-traversal-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "OriginalJ3DJointTraversalTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("original_j3d_joint_traversal", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-original-xanime-core-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "OriginalXanimeCoreTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("original_xanime_core", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-original-xanime-player-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalXanimePlayerTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd", "aurora-gd",
        "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("original_xanime_player", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-original-j3d-vertex-buffer-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "OriginalJ3DVertexBufferTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("original_j3d_vertex_buffer", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

for _, fixture in ipairs {
    {"packet", "OriginalJ3DPacketTests.cpp"},
    {"mtx-buffer", "OriginalJ3DMtxBufferTests.cpp"},
    {"texture-mtx", "OriginalJ3DTextureMtxTests.cpp"},
    {"material-animation", "OriginalMaterialAnimationTests.cpp"},
    {"material-block", "OriginalJ3DMaterialBlockTests.cpp"},
    {"joint-resource", "OriginalJ3DJointResourceTests.cpp"},
    {"material-resource", "OriginalJ3DMaterialResourceTests.cpp"},
    {"geometry-resource", "OriginalJ3DGeometryResourceTests.cpp"},
    {"texture-resource", "OriginalJ3DTextureResourceTests.cpp"},
    {"animation-resource", "OriginalJ3DAnimationResourceTests.cpp"},
    {"material-table", "OriginalJ3DMaterialTableTests.cpp"},
    {"model-resource", "OriginalJ3DModelResourceTests.cpp"}
} do
    target("smg-pc-original-j3d-" .. fixture[1] .. "-tests")
        set_kind("binary")
        set_default(false)
        set_group("tests/aurora")
        add_files(fixture[2])
        add_deps {
            "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
            "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
        }
        add_tests("original_j3d_" .. fixture[1]:gsub("-", "_"), {
            group = "aurora",
            rundir = os.projectdir(),
            realtime_output = true
        })
end

for _, fixture in ipairs {
    {"jkr-heap", "OriginalJkrHeapTests.cpp"},
    {"joint-controller", "OriginalJointControllerTests.cpp"},
    {"actor-broadcast", "OriginalActorBroadcastTests.cpp"},
    {"message-holder", "OriginalMessageHolderTests.cpp"},
    {"jkr-heap-finalizer", "JkrHeapFinalizerTests.cpp"},
    {"jkr-allocation-domain", "JkrAllocationDomainTests.cpp"},
    {"jkr-archive", "OriginalJkrArchiveTests.cpp"},
    {"jmap-resource", "OriginalJMapResourceTests.cpp"},
    {"jmap-heap-lifetime", "JMapHeapLifetimeTests.cpp"},
    {"bck-ctrl", "OriginalBckCtrlTests.cpp"},
    {"resource-holder", "OriginalResourceHolderTests.cpp"},
    {"camera-vector-math", "OriginalCameraVectorMathTests.cpp"}
} do
    target("smg-pc-original-" .. fixture[1] .. "-tests")
        set_kind("binary")
        set_default(false)
        set_group("tests/aurora")
        if fixture[1] == "resource-holder" then
            if is_plat("macosx", "iphoneos") then
                add_ldflags("-Wl,-dead_strip", {force = true})
            else
                add_ldflags("-Wl,--gc-sections", {force = true})
            end
        end
        add_files(fixture[2])
        if fixture[1] == "message-holder" then add_files("OriginalTalkNodeTests.cpp") end
        if fixture[1] == "actor-broadcast" then add_deps("smg-pc-app", "aurora-main") end
        add_deps {
            "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
            "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
        }
        add_tests("original_" .. fixture[1]:gsub("-", "_"), {
            group = "aurora", rundir = os.projectdir(), realtime_output = true
        })
end

target("smg-pc-jut-texture-ownership-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("JutTextureOwnershipTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("jut_texture_ownership", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-runtime-context-construction-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("RuntimeContextConstructionTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("runtime_context_construction", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-kcollision-resource-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalKCollisionResourceTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("original_kcollision_resource", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-fixed-step-clock-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/runtime")
    add_includedirs("../src", "../aurora/include")
    add_defines("TARGET_PC")
    add_files("FixedStepClockTests.cpp", "../src/compat/J3DFrameCtrlCompat.cpp")
    add_deps("smg-pc-common")
    add_tests("fixed_step_clock", {
        group = "runtime",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-original-j3d-transform-animation-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "OriginalJ3DTransformAnimationTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("original_j3d_transform_animation", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-resource-table-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "ResourceTableTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("resource_table", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-hash-sort-table-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "HashSortTableTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("hash_sort_table", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-area-obj-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "AreaObjRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("area_obj_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-area-obj-core-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "AreaObjCoreTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("area_obj_core", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-npc-actor-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "NPCActorRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("npc_actor_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-game-data-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "GameDataRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("game_data_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-gravity-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "GravityRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("gravity_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-gravity-math-foundation-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "GravityMathFoundationTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("gravity_math_foundation", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-original-scenario-catalog-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("OriginalScenarioCatalogTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_scenario_catalog", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-scenario-publication-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("ScenarioPublicationTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("scenario_publication", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-jpa-manager-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("OriginalJpaManagerTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_jpa_manager", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-vi-render-mode-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("OriginalViRenderModeTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_vi_render_mode", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-camera-context-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("OriginalCameraContextTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_camera_context", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-console-nand-import-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("ConsoleNandImportTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("console_nand_import", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-system-config-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("OriginalSystemConfigTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_system_config", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-particle-resource-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("OriginalParticleResourceOwnerTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_particle_resource_owner", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-predraw-scheduler-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("OriginalPreDrawSchedulerTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_predraw_scheduler", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-scene-scheduler-heap-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("SceneSchedulerHeapTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("scene_scheduler_heap", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-game-math-rotation-tests")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "GameMathRotationTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("game_math_rotation", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-stage-zone-matrix-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "StageZoneMatrixRegistryTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("stage_zone_matrices", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-scene-movement-runtime-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "SceneMovementRuntimeTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("scene_movement_runtime", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-feedback-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "FeedbackRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("feedback_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-j3d-gx-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "J3dGxRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("j3d_gx_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-player-actor-bridge-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "PlayerActorBridgeTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("player_actor_bridge", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-runtime-event-ownership-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "RuntimeEventOwnershipTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("runtime_event_ownership", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-restart-stage-session-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "RestartStageSessionTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("restart_stage_session", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-mii-font-compat-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "MiiFontCompatTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("mii_font_compat", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-sphere-selector-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "SphereSelectorRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("sphere_selector_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-j-audio-playback-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "JAudioPlaybackTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("j_audio_playback", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-original-jai-sound-ownership-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "OriginalJaiSoundOwnershipTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("original_jai_sound_ownership", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-rfl-resource-archive-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "RflResourceArchiveTests.cpp",
        "../src/resource/RarcArchive.cpp",
        "../src/resource/Yaz0.cpp"
    }
    add_includedirs("../src")
    add_deps("aurora-base")
    add_tests("rfl_resource_archive", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-center-screen-blur-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "CenterScreenBlurRealOrAbsentTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("center_screen_blur_real_or_absent", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-planet-map-catalog-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "PlanetMapCatalogTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("planet_map_catalog", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-model-3d-for-2d-contract-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "Model3DFor2DContractTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("model_3d_for_2d_contract", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-bright-visibility-batch-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "BrightVisibilityBatchTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("bright_visibility_batch", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-brk-real-resource-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "BrkRealResourceTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("brk_real_resource", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-picture-font-tag-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "PictureFontTagTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("picture_font_tags", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-authored-placement-instantiator-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "AuthoredPlacementInstantiatorTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("authored_placement_instantiator", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-actor-event-camera-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "ActorEventCameraTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("actor_event_camera", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-original-shadow-controller-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "OriginalShadowControllerOwnerTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("original_shadow_controller_owner", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-point-light-runtime-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "PointLightRuntimeTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("point_light_runtime", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-upstream-component-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "UpstreamComponentTests.cpp"
    }
    add_deps {
        "smg-pc-game",
        "aurora-si"
    }
    add_tests("upstream_components", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })


target("smg-pc-msl-functional-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_includedirs("../aurora/include")
    add_files("MslFunctionalTests.cpp")
    add_tests("msl_functional", {group = "aurora", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-msl-printf-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("MslPrintfTests.cpp", "MslPrintfAliasTests.cpp")
    add_deps("smg-pc-common")
    add_cxxflags("-fno-builtin-sprintf", "-fno-builtin-snprintf", "-fno-builtin-vsprintf", "-fno-builtin-vsnprintf", {force = true})
    add_tests("msl_printf", {group = "aurora", rundir = os.projectdir(), realtime_output = true})


target("smg-pc-original-vector-integer-conversion-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_includedirs("../src", "../aurora/include")
    add_defines("TARGET_PC", "AURORA")
    add_files("OriginalVectorIntegerConversionTests.cpp")
    add_tests("original_vector_integer_conversion", {group = "aurora", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-text-encoding-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/resource")
    add_includedirs("../src", "../aurora/include")
    add_files("TextEncodingTests.cpp", "../src/resource/TextEncoding.cpp")
    if is_plat("macosx") then
        add_syslinks("iconv")
    end
    add_tests("text_encoding", {group = "resource", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-gx-misc-state-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("GxMiscStateTests.cpp")
    add_defines("TARGET_PC", "AURORA")
    add_deps("aurora-gx")
    add_tests("gx_misc_state", {group = "aurora", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-legacy-functional-adapters-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_includedirs("../aurora/include")
    add_files("LegacyFunctionalAdaptersTests.cpp")
    add_tests("legacy_functional_adapters", {group = "aurora", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-color8-byte-order-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_includedirs("../src", "../aurora/include")
    add_defines("TARGET_PC", "AURORA")
    add_files("Color8ByteOrderTests.cpp")
    add_tests("color8_byte_order", {group = "aurora", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-demo-start-request-holder-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("DemoStartRequestHolderTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("demo_start_request_holder", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-game-data-star-storage-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("GameDataStarStorageTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("game_data_star_storage", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-auto-effect-metadata-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("OriginalAutoEffectMetadataTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_auto_effect_metadata", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-effect-ownership-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalEffectOwnershipTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_effect_ownership", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-direct-draw-texture-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalDirectDrawTextureTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_direct_draw_texture", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-aurora-texture-object-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("../aurora/tests/gx_texture_object_test.cpp")
    add_deps {"aurora-core", "aurora-card", "aurora-dvd", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("aurora_texture_object", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-draw-sync-manager-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_includedirs("../src")
    add_files("OriginalDrawSyncManagerTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_draw_sync_manager", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-jkr-thread-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("OriginalJkrThreadTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_jkr_thread", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-game-system-startup-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("OriginalGameSystemStartupTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}

target("smg-pc-original-jkr-aram-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("OriginalJkrAramTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_jkr_aram", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-file-loader-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("OriginalFileLoaderTests.cpp")
    add_files("../src/Game/Util/FileUtil.cpp", {
        cxxflags = "-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp")
    })
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_file_loader", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-aurora-draw-sync-pass-render-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("../aurora/tests/gx_draw_sync_pass_render_test.cpp")
    add_deps {"aurora-core", "aurora-card", "aurora-dvd", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("aurora_draw_sync_pass_render", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-aurora-depth-snapshot-render-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("../aurora/tests/gx_depth_snapshot_render_test.cpp")
    add_deps {"aurora-core", "aurora-card", "aurora-dvd", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("aurora_depth_snapshot_render", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-star-pointer-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("StarPointerRealOrAbsentTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("star_pointer_real_or_absent", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-scene-initialization-state-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("SceneInitializationStateTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("scene_initialization_state", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-area-polygon-query-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("AreaPolygonQueryTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("area_polygon_query", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-fur-drawer-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("OriginalFurDrawerTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("original_fur_drawer", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-fur-shader-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalFurShaderTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("original_fur_shader", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-scene-counter-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalSceneCounterOwnerTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("original_scene_counter_owner", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-stage-camera-resource-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    add_files("StageCameraResourceTests.cpp")
    add_deps {
        "smg-pc-app", "smg-pc-common", "smg-pc-game", "aurora-main", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("stage_camera_resources", {
        group = "original-process", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-camera-holder-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalCameraHolderTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("original_camera_holder", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-camera-resource-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalCameraResourceTests.cpp")
    add_deps {
        "smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
        "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"
    }
    add_tests("original_camera_resource", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-aurora-clip-mode-render-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files("../aurora/tests/gx_clip_mode_render_test.cpp")
    add_deps {"aurora-core", "aurora-card", "aurora-dvd", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("aurora_clip_mode_render", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-jkr-exception-ownership-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("JkrExceptionOwnershipTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("jkr_exception_ownership", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-direct-draw-util-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalDirectDrawUtilTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_direct_draw_util", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-line-collision-query-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("LineCollisionQueryTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("line_collision_query", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-camera-director-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalCameraDirectorTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_camera_director", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-ppc-bitfield-abi-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_includedirs("../src", "../aurora/include")
    add_defines("TARGET_PC", "AURORA")
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), {force = true})
    add_files("PpcBitfieldAbiTests.cpp")
    add_tests("ppc_bitfield_abi", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-sound-permission-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("SoundPermissionTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("sound_permission", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-j2d-projection-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalJ2DProjectionTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_j2d_projection", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-pointer-input-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalPointerInputTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_pointer_input", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-wpad-acceleration-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalWPadAccelerationTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_wpad_acceleration", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-wpad-ownership-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalWPadOwnershipTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_wpad_ownership", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-wpad-gesture-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalWPadGestureTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_wpad_gesture", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-star-pointer-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalStarPointerOwnerTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_star_pointer_owner", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-event-sequence-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_includedirs("../src", "../aurora/include")
    add_defines("TARGET_PC", "AURORA")
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), {force = true})
    add_cxxflags("-ffunction-sections", "-fdata-sections", {force = true})
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
    add_files("OriginalEventSequenceTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_event_sequence", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-collision-parts-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalCollisionPartsOwnerTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_collision_parts_owner", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-j2d-projection-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalJ2DProjectionOwnerTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_j2d_projection_owner", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })


target("smg-pc-original-audio-category-volume-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalAudioCategoryVolumeTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_audio_category_volume", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })


target("smg-pc-original-layout-group-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalLayoutGroupTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_layout_group", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-image-effect-ownership-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalImageEffectOwnershipTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_image_effect_ownership", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-player-status-storage-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "OriginalPlayerStatusStorageTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("original_player_status_storage", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-name-obj-group-lifetime-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("NameObjGroupLifetimeTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("name_obj_group_lifetime", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-scene-wipe-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalSceneWipeOwnerTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_scene_wipe_owner", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })


target("smg-pc-scene-lifetime-binding-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("SceneLifetimeBindingTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("scene_lifetime_binding", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })


target("smg-pc-original-name-pos-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    add_files("OriginalNamePosOwnerTests.cpp")
    add_deps {"smg-pc-app", "smg-pc-common", "smg-pc-game", "aurora-main", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_name_pos_owner", {
        group = "original-process", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-scene-name-obj-registry-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("SceneNameObjRegistryTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("scene_name_obj_registry", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })


target("smg-pc-original-scene-execution-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalSceneExecutionOwnerTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_scene_execution_owner", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-aurora-z-texture-render-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("../aurora/tests/gx_z_texture_render_test.cpp")
    set_rundir(os.projectdir())
    add_deps {"aurora-core", "aurora-card", "aurora-dvd", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("aurora_z_texture_render", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-wpad-pause-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "OriginalWPadPauseTests.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-si",
        "aurora-vi"
    }
    add_tests("original_wpad_pause", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })


target("smg-pc-original-jsu-stream-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalJSUStreamTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_jsu_streams", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-save-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalSaveOwnerTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_save_owner", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })


target("smg-pc-original-jut-video-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_packages("abseil")
    set_rundir(os.projectdir())
    add_files("OriginalJutVideoTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("original_jut_video", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-frame-button-state-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("FrameButtonStateTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("frame_button_state", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-source-mirror-encoding-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/source-boundary")
    add_files("SourceMirrorEncodingTests.cpp")
    add_tests("source_mirror_encoding", {
        group = "source-boundary", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-language-ownership-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("LanguageOwnershipTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("language_ownership", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-j3d-command-scheduling-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("J3dCommandSchedulingTests.cpp")
    add_deps {"smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd",
              "aurora-gd", "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi"}
    add_tests("j3d_command_scheduling", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-process-collision-area-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalProcessCollisionAreaTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_process_collision_area", {
        group = "original-process", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-sensor-matrix-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalSensorMatrixTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_sensor_matrix_math", {
        group = "aurora", rundir = os.projectdir(), realtime_output = true
    })


target("smg-pc-original-process-shadow-line-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalProcessShadowVolumeLineTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_process_shadow_line", {
        group = "original-process", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-process-punching-kinoko-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalProcessPunchingKinokoTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_process_punching_kinoko", {
        group = "original-process", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-process-warp-pod-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalProcessWarpPodTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_process_warp_pod", {
        group = "original-process", rundir = os.projectdir(), realtime_output = true
    })

target("smg-pc-original-process-butterfly-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalProcessButterflyTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_process_butterfly", {group = "original-process", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-original-process-trample-jump-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalProcessTrampleJumpTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_process_trample_jump", {group = "original-process", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-original-process-crystal-cage-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalProcessCrystalCageTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_process_crystal_cage", {group = "original-process", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-original-process-star-piece-placement-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalProcessStarPiecePlacementTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_process_star_piece_placement", {group = "original-process", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-original-process-placement-transform-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    set_rundir(os.projectdir())
    add_files("OriginalProcessPlacementTransformTests.cpp")
    add_deps {"smg-pc-app", "aurora-main"}
    add_tests("original_process_placement_transform", {group = "original-process", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-original-actor-utility-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalActorUtilityTests.cpp")
    add_deps("smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd", "aurora-gd",
             "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi")
    add_tests("original_actor_utilities", {group = "aurora", rundir = os.projectdir(), realtime_output = true})


target("smg-pc-original-placement-coverage-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalPlacementCoverageTests.cpp")
    add_deps("smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd", "aurora-gd",
             "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi")
    add_tests("original_placement_coverage", {group = "aurora", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-original-process-player-owner-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    add_files("OriginalProcessPlayerOwnerTests.cpp", "OriginalProcessMarioCameraTests.cpp",
              "OriginalMarioStateTests.cpp", "OriginalPlayerUtilTests.cpp", "MarioWalkParameterTests.cpp")
    add_deps("smg-pc-app", "aurora-main")
    add_tests("original_process_player_owner", {group = "original-process", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-original-sphere-query-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalSphereQueryTests.cpp")
    add_deps("smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd", "aurora-gd",
             "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi")
    add_tests("original_sphere_queries", {group = "aurora", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-original-binder-sphere-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalBinderSphereTests.cpp")
    add_deps("smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd", "aurora-gd",
             "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi")
    add_tests("original_binder_spheres", {group = "aurora", rundir = os.projectdir(), realtime_output = true})

target("smg-pc-original-npc-orientation-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/original-process")
    add_files("OriginalNpcOrientationTests.cpp")
    add_deps("smg-pc-app", "aurora-main")
    add_tests("original_npc_orientation", {group = "original-process", rundir = os.projectdir(), realtime_output = true})
