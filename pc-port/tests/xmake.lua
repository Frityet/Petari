local function add_smgpc_test_target(name, main_file, suite_file, test_name, test_group)
    target(name)
        set_kind("binary")
        set_default(false)
        set_group("tests/" .. test_group)
        add_files(main_file, suite_file)
        add_headerfiles("TestSupport.hpp", "TestSuites.hpp")
        add_deps {
            "smg-pc-app",
            "smg-pc-game",
            "smg-pc-render"
        }
        add_tests(test_name, {
            group = test_group,
            rundir = os.projectdir(),
            realtime_output = true
        })
end

target("smg-pc-compat-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests/aggregate")
    add_files {
        "AllTests.cpp",
        "ResourceLayoutTests.cpp",
        "J3dGxTests.cpp",
        "RuntimeSceneTests.cpp",
        "RenderCoreTests.cpp"
    }
    add_headerfiles("TestSupport.hpp", "TestSuites.hpp")
    add_deps {
        "smg-pc-app",
        "smg-pc-game",
        "smg-pc-render"
    }

add_smgpc_test_target("smg-pc-resource-layout-tests", "ResourceLayoutTestMain.cpp", "ResourceLayoutTests.cpp", "resource_layout", "assets")
add_smgpc_test_target("smg-pc-j3d-gx-tests", "J3dGxTestMain.cpp", "J3dGxTests.cpp", "j3d_gx", "compat")
add_smgpc_test_target("smg-pc-runtime-scene-tests", "RuntimeSceneTestMain.cpp", "RuntimeSceneTests.cpp", "runtime_scene", "compat")
add_smgpc_test_target("smg-pc-render-core-tests", "RenderCoreTestMain.cpp", "RenderCoreTests.cpp", "render_core", "render")
