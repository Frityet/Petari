set_project("pc_port")
set_xmakever("3.0.0")

add_rules("mode.debug", "mode.release")
set_languages("c++20")
add_cxxflags("-Wall", "-Wextra", "-Wpedantic", "-Werror", {tools = {"gcc", "clang"}})
add_includedirs("src")

option("pc_port_fetch_sdl2")
    set_default(true)
    set_showmenu(true)
    set_description("Fetch SDL2 from source when not installed")
option_end()

add_requires("libsdl2", {configs = {shared = false, wayland = false, x11 = true}})
add_requires("bgfx")
add_requires("stb")

includes("src", "tests")
