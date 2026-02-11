local test_sources = {
    "TestMain.cpp",
    "**/*Tests.cpp",
}

local test_cases = {
}

target("smg-pc-tests")
    set_kind("binary")
    set_default(false)
    set_rundir(path.directory(os.projectdir()))
    add_cxxflags("-Wno-dollar-in-identifier-extension", {tools = "clang"})
    add_files(test_sources)
    add_includedirs(
        "$(projectdir)",
        "$(projectdir)/src",
        "$(projectdir)/src/app",
        "$(projectdir)/src/assets",
        "$(projectdir)/src/common",
        "$(projectdir)/src/di",
        "$(projectdir)/src/game",
        "$(projectdir)/src/render"
    )
    add_deps(
        "smg-pc-app",
        "smg-pc-assets",
        "smg-pc-common",
        "smg-pc-di",
        "smg-pc-game",
        "smg-pc-render"
    )

    for _, case in ipairs(test_cases) do
        add_tests(case, {runargs = {"--case", case}})
    end
