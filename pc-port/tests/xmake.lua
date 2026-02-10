local test_sources = {
    "TestMain.cpp",
}

local test_cases = {
}

target("smg-pc-tests")
    set_kind("binary")
    set_default(false)
    set_rundir(path.directory(os.projectdir()))
    add_cxxflags("-Wno-dollar-in-identifier-extension", {tools = "clang"})
    add_files(test_sources)
    add_includedirs("$(projectdir)", "$(projectdir)/src")
    add_deps(
        "smg-pc"
    )

    for _, case in ipairs(test_cases) do
        add_tests(case, {runargs = {"--case", case}})
    end
