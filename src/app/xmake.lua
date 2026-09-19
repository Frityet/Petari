target("smg-pc-app")
    set_kind("static")
    add_files("Application.cpp", "OriginalGameApplication.cpp")
    add_headerfiles("Application.hpp", "SimulationClock.hpp")
    add_includedirs("./", {public = true})
    add_deps {
        "smg-pc-render",
        "smg-pc-common",
        "smg-pc-game"
    }

target("smg-pc")
    set_kind("binary")
    set_default(true)
    set_group("applications")
    set_rundir(os.projectdir())
    on_run("build.run")
    if is_plat("macosx") then
        add_rules("xcode.application")
        add_files("Info.plist")
        set_values("xcode.bundle_identifier", "org.petari.smg-pc.game")
    end
    add_files("main.cpp")
    add_headerfiles("**.hpp")
    add_deps {
        "smg-pc-app",
        "aurora-main"
    }
