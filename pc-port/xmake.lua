set_project("smg-pc")
set_xmakever("3.0.0")

add_rules("mode.debug", "mode.release")
set_languages("c++23")
add_cxxflags("-Wall", "-Wextra", "-Wpedantic", "-Werror", {tools = {"gcc", "clang"}})
add_cxxflags("-Wno-dollar-in-identifier-extension", {tools = "clang"})

add_repositories("local-repo $(projectdir)")

add_requires("glfw", {configs = {shared = false, wayland = false, x11 = true}})
add_requires("bgfx 8752")
add_requires("stb")

includes {
    "src",
    "tests"
}
