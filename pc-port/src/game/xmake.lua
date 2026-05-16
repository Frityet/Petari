local game_runtime_sources = {
    "GameServices.cpp",
    "compat/FileSelectPreview.cpp",
    "compat/FileSelectPreviewTextures.cpp",
    "compat/FileSelectFuncCompat.cpp",
    "compat/FileSelectSkyJ3d.cpp",
    "compat/GamePadCompat.cpp",
    "compat/HostNandStorage.cpp",
    "compat/JKernelCompat.cpp",
    "compat/LayoutSceneCompat.cpp",
    "compat/LayoutTextureCompat.cpp",
    "compat/RuntimeContext.cpp",
    "compat/ScreenControllerCompat.cpp",
    "compat/SharedSkyBackground.cpp",
    "compat/TitleBackground.cpp",
    "layout/LayoutArchiveLoader.cpp",
    "layout/LayoutRuntimeActor.cpp",
    "Game/LiveActor/Nerve.cpp",
    "Game/LiveActor/Spine.cpp",
    "Game/Map/FileSelectFunc.cpp",
    "Game/Map/FileSelectIconID.cpp",
    "Game/Screen/ButtonPaneController.cpp",
    "Game/Screen/FileSelectButton.cpp",
    "Game/Screen/FileSelectInfo.cpp",
    "Game/Screen/FileSelectNumber.cpp",
    "Game/Screen/IconAButton.cpp",
    "Game/Screen/InformationMessage.cpp",
    "Game/Screen/BackButton.cpp",
    "Game/Screen/LayoutActor.cpp",
    "Game/Screen/Manual2P.cpp",
    "Game/Screen/ProloguePictureBook.cpp",
    "Game/Screen/SaveIcon.cpp",
    "Game/Screen/SimpleLayout.cpp",
    "Game/Screen/SysInfoWindow.cpp",
    "Game/Screen/TitleSequenceProduct.cpp",
    "Game/Screen/YesNoController.cpp",
    "Game/System/NerveExecutor.cpp",
    "Game/System/BinaryDataChunkHolder.cpp",
    "Game/System/BinaryDataContentAccessor.cpp",
    "Game/System/ConfigDataHolder.cpp",
    "Game/System/ConfigDataMii.cpp",
    "Game/System/ConfigDataMisc.cpp",
    "Game/System/GameDataHolder.cpp",
    "Game/System/GameSequenceFunction.cpp",
    "Game/System/NANDManager.cpp",
    "Game/System/SaveDataBannerCreator.cpp",
    "Game/System/SaveDataHandleSequence.cpp",
    "Game/System/SaveDataHandler.cpp",
    "Game/System/SysConfigFile.cpp",
    "Game/System/UserFile.cpp",
    "Game/Util/FileUtil.cpp",
    "Game/Util/GamePadUtil.cpp",
    "Game/Util/BitArray.cpp",
    "Game/Util/HashUtil.cpp",
    "Game/Util/LayoutUtil.cpp",
    "Game/Util/MessageUtil.cpp",
    "Game/Util/MemoryUtil.cpp",
    "Game/Util/NerveUtil.cpp",
    "Game/Util/ObjUtil.cpp",
    "Game/Util/SoundUtil.cpp",
    "Game/Util/StarPointerUtil.cpp",
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
