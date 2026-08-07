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

target("smg-pc-stage-player-runtime-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "StagePlayerRuntimeTests.cpp",
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
    add_tests("stage_player_runtime", {
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

target("smg-pc-lod-ctrl-compat-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
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
    add_tests("lod_ctrl_compat", {
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
