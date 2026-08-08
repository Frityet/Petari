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
