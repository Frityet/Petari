target("smg-pc-aurora-native-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files {
        "AuroraNativeTests.cpp",
        "../src/app/AuroraDolphinCompat.cpp"
    }
    add_deps {
        "smg-pc-common",
        "smg-pc-game",
        "smg-pc-render",
        "aurora-card",
        "aurora-dvd",
        "aurora-gd",
        "aurora-gx",
        "aurora-os",
        "aurora-pad",
        "aurora-vi"
    }
    add_tests("aurora_native", {
        group = "aurora",
        rundir = os.projectdir(),
        realtime_output = true
    })
