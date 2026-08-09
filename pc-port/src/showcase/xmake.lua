target("smg-pc-mario-gateway-walk-slice")
    set_kind("static")
    set_default(false)
    set_group("showcase")
    set_toolset("cxx", "clang++")
    add_includedirs(path.join(os.projectdir(), "src"), {before = true})
    add_includedirs(path.join(os.projectdir(), "..", "include"))
    add_includedirs(path.join(os.projectdir(), "..", "libs", "JSystem", "include"))
    add_cxxflags("-include " .. path.join(os.projectdir(), "src/compat/MetrowerksStdCompat.hpp"), {force = true})
    add_cxxflags("-ffunction-sections", "-fdata-sections", {force = true})
    add_files {
        "../Game/Player/Mario.cpp",
        "../Game/Player/MarioActor.cpp",
        "../Game/Player/MarioActorCamera.cpp",
        "../Game/Player/MarioActorDraw.cpp",
        "../Game/Player/MarioActorInit.cpp",
        "../Game/Player/MarioActorPad.cpp",
        "../Game/Player/MarioAnimator.cpp",
        "../Game/Player/MarioConst.cpp",
        "../Game/Player/MarioInit.cpp",
        "../Game/Player/MarioMapCode.cpp",
        "../Game/Player/MarioModule.cpp",
        "../Game/Player/MarioMove.cpp",
        "../Game/Player/MarioMove2D.cpp",
        "../Game/Player/MarioSlip.cpp",
        "../Game/Player/MarioTask.cpp",
        "../Game/Player/MarioWalk.cpp"
    }
    add_deps("smg-pc-game")

target("smg-pc-showcase")
    set_kind("binary")
    add_files {
        "Showcase.cpp",
        "../../aurora/lib/compat.cpp"
    }
    add_deps {
        "smg-pc-app",
        "smg-pc-mario-gateway-walk-slice",
        "aurora-main"
    }
    add_ldflags("-Wl,--gc-sections", {force = true})
