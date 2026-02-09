local test_sources = {
    "TestMain.cpp",
    "AssetPipelineTests.cpp",
    "LayoutParserTests.cpp",
    "TplDecoderTests.cpp",
    "MenuRenderSmokeTests.cpp",
    "WiiStubTests.cpp",
    "CliContractTests.cpp"
}

local test_cases = {
    "AssetPipelineRootExists",
    "AssetPipelinePressStartLayoutExists",
    "AssetPipelineFileSelectLayoutExists",
    "AssetPipelineTitleLogoLayoutExists",
    "AssetPipelinePressStartAnimationsExist",
    "AssetPipelineFileSelectAnimationsExist",
    "AssetPipelineTitleLogoAnimationsExist",
    "CliInvalidGameRootFails",
    "LayoutParserPressStartLoads",
    "LayoutParserTitleLogoLoads",
    "LayoutParserFileSelectLoads",
    "LayoutParserPressStartRequiredPanes",
    "LayoutParserTitleLogoRequiredPanes",
    "LayoutParserFileSelectRequiredPanes",
    "MenuRenderReachesExpectedStates",
    "MenuRenderProducesPhaseHashes",
    "MenuRenderHashesChangeAcrossPhases",
    "MenuRenderCapturesAllLayersAsPngAndPpm",
    "TplDecoderDecodesFileSelectTextures",
    "TplDecoderDecodesTitleLogoTextures",
    "TplDecoderDecodesAllMenuTextures",
    "WiiStubsAreDeterministic"
}

target("pc_port_tests")
    set_kind("binary")
    set_default(false)
    set_rundir(path.directory(os.projectdir()))
    add_cxxflags("-Wno-dollar-in-identifier-extension", {tools = "clang"})
    add_files(test_sources)
    add_includedirs("$(projectdir)", "$(projectdir)/src")
    add_deps(
        "pc-port-core",
        "pc-port-assets",
        "pc-port-image",
        "pc-port-layout",
        "pc-port-game",
        "pc-port-render",
        "pc-port-menu",
        "pc-port-platform",
        "pc-port"
    )

    for _, case in ipairs(test_cases) do
        add_tests(case, {runargs = {"--case", case}})
    end
