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
        "MarioModelDemoSurfaceTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-binder-kcl-mario-walk-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "BinderKclMarioWalkTests.cpp",
        "../src/Game/Player/MarioMapCode.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("binder_kcl_mario_walk", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-gateway-demo-scene-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "GatewayDemoSceneTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("gateway_demo_scene", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-information-observer-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "InformationObserverTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("information_observer", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-gateway-spin-checkpoint-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    set_toolset("cxx", "clang++")
    add_files {
        "GatewaySpinCheckpointTests.cpp",
        "../aurora/lib/compat.cpp"
    }
    add_ldflags("-Wl,--gc-sections", {force = true})
    add_deps {
        "smg-pc-mario-gateway-walk-slice",
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
    add_tests("gateway_spin_checkpoint", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-mario-gateway-walk-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    set_toolset("cxx", "clang++")
    add_files {
        "MarioGatewayWalkTests.cpp",
        "../aurora/lib/compat.cpp"
    }
    add_ldflags("-Wl,--gc-sections", {force = true})
    add_deps {
        "smg-pc-mario-gateway-walk-slice",
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
    add_tests("mario_gateway_walk", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-file-select-exact-source-compile")
    set_kind("static")
    set_default(false)
    set_group("tests/source-boundary")
    set_toolset("cxx", "clang++")
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), { force = true })
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
        "AuroraNativeTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "StageStartCameraTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "StationedArchiveRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-jpc-billboard-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "JpcBillboardTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("jpc_billboard", {
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
        "LodCtrlCompatTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-demo-sheet-runtime-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "DemoSheetRuntimeTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("demo_sheet_runtime", {
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
        "ObjectNameTableTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-demo-scene-runtime-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "DemoSceneRuntimeTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("demo_scene_runtime", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-nameobj-factory-placement-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "NameObjFactoryPlacementTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "StageCollisionRegistrationTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "MessageRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "RflRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "FixedPositionRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "SceneObjHolderRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "CameraUtilRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "PlayerUtilRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "StorySequenceRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "LiveActorUtilRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "BtpRealResourceTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "SaveDataCoreRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "SaveConfigRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "LayoutRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "TalkRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "GameActorPhysicsRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "ActorSensorRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "ActorRuntimeRegistryTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "FileSelectNameRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "J3DFrameCtrlTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-area-obj-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "AreaObjRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "AreaObjCoreTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "NPCActorRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "GameDataRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "GravityRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "GravityMathFoundationTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-feedback-real-or-absent-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "FeedbackRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "J3dGxRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-restart-stage-session-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "RestartStageSessionTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "MiiFontCompatTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "SphereSelectorRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "JAudioPlaybackTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "CenterScreenBlurRealOrAbsentTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-sky-actor-route-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "SkyActorRouteTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("sky_actor_route", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-air-actor-route-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "AirActorRouteTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("air_actor_route", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-title-file-select-visual-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "TitleFileSelectVisualTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("title_file_select_visual", {
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
        "PlanetMapCatalogTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-planet-map-actor-route-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "PlanetMapActorRouteTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("planet_map_actor_route", {
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
        "Model3DFor2DContractTests.cpp",
        "../aurora/lib/compat.cpp"
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
        "BrightVisibilityBatchTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-bright-sun-route-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "BrightSunRouteTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("bright_sun_route", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-brk-real-resource-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "BrkRealResourceTests.cpp",
        "../aurora/lib/compat.cpp"
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

target("smg-pc-file-select-far-visual-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "FileSelectFarVisualTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("file_select_far_visual", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })

target("smg-pc-title-file-select-route-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    set_rundir(os.projectdir())
    add_files {
        "TitleFileSelectRouteTests.cpp",
        "../aurora/lib/compat.cpp"
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
    add_tests("title_file_select_route", {
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
        "PictureFontTagTests.cpp",
        "../aurora/lib/compat.cpp"
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
