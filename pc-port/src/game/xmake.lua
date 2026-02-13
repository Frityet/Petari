local game_runtime_sources = {
    "GameServices.cpp",
    "compat/GamePadCompat.cpp",
    "compat/JKernelCompat.cpp",
    "compat/RuntimeContext.cpp",
    "layout/LayoutArchiveLoader.cpp",
    "layout/LayoutRuntimeActor.cpp",
    "Game/LiveActor/Nerve.cpp",
    "Game/LiveActor/Spine.cpp",
    "Game/Screen/LayoutActor.cpp",
    "Game/Screen/SimpleLayout.cpp",
    "Game/Screen/TitleSequenceProduct.cpp",
    "Game/System/NerveExecutor.cpp",
    "Game/Util/FileUtil.cpp",
    "Game/Util/GamePadUtil.cpp",
    "Game/Util/LayoutUtil.cpp",
    "Game/Util/NerveUtil.cpp",
    "Game/Util/ObjUtil.cpp",
    "Game/Util/SoundUtil.cpp",
    "Game/Util/SystemUtil.cpp",
    "Game/Util/TriggerChecker.cpp",
}

target("smg-pc-game")
    set_kind("static")
    add_files(game_runtime_sources)
    add_headerfiles("**.hpp")
    add_includedirs("./", {public = true})
    add_deps("smg-pc-render", "smg-pc-common", "smg-pc-assets")
    before_build(function (target)
        local project_dir = os.projectdir()
        local checker = import("tools.check_game_compliance", {rootdir = project_dir})
        local policy_module = import("tools.game_compliance_policy", {rootdir = project_dir})
        checker.check({
            policy = type(policy_module.policy) == "function" and policy_module.policy(),
            project_root = project_dir,
            repo_root = path.directory(project_dir)
        })
    end)
