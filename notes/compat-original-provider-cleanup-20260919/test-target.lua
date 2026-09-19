-- Append to tests/xmake.lua after the coordinated production cleanup.
target("smg-pc-original-actor-utility-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aurora")
    add_files("OriginalActorUtilityTests.cpp", "../aurora/lib/compat.cpp")
    add_deps("smg-pc-common", "smg-pc-game", "aurora-card", "aurora-dvd", "aurora-gd",
             "aurora-gx", "aurora-os", "aurora-pad", "aurora-si", "aurora-vi")
    add_tests("original_actor_utilities", {group = "aurora", rundir = os.projectdir(), realtime_output = true})
