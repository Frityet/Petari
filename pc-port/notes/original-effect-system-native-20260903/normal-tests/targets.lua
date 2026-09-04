-- Append to pc-port/tests/xmake.lua; copy these two tests alongside it.
-- These targets require no EffectSystem source or resource/runtime owner.
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
