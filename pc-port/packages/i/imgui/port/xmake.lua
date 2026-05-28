add_rules("mode.debug", "mode.release")
add_rules("utils.install.cmake_importfiles")
set_languages("cxx14")

option("sdl3",          {showmenu = true, default = false})
option("sdl3_renderer", {showmenu = true, default = false})
option("wgpu",          {showmenu = true, default = false})
option("wgpu_backend",  {showmenu = true, default = "wgpu", type = "string", values = {"wgpu", "dawn"}})
option("dawn_version",  {showmenu = true, default = "v20260523.201736", type = "string"})
option("freetype",      {showmenu = true, default = false})
option("user_config",   {showmenu = true, default = nil, type = "string"})

if has_config("sdl3") or has_config("sdl3_renderer") then
    add_requires("libsdl3", {
        configs = {shared = false, x11 = is_plat("linux"), x11_shared = true, wayland = false},
    })
end

if has_config("wgpu") then
    if get_config("wgpu_backend") == "dawn" then
        add_requires("dawn-build " .. get_config("dawn_version"), {configs = {shared = false}})
    else
        add_requires("wgpu-native")
    end
end

if has_config("freetype") then
    add_requires("freetype")
end

target("imgui")
    set_kind("$(kind)")
    add_files("*.cpp", "misc/cpp/*.cpp")
    add_headerfiles("*.h", "(backends/*.h)", "(misc/cpp/*.h)")
    add_includedirs(".", "backends", "misc/cpp")

    if has_config("sdl3") then
        add_files("backends/imgui_impl_sdl3.cpp")
        add_packages("libsdl3")
    end

    if has_config("sdl3_renderer") then
        add_files("backends/imgui_impl_sdlrenderer3.cpp")
        add_packages("libsdl3")
    end

    if has_config("wgpu") then
        add_files("backends/imgui_impl_wgpu.cpp")
        if get_config("wgpu_backend") == "dawn" then
            add_packages("dawn-build")
            add_defines("IMGUI_IMPL_WEBGPU_BACKEND_DAWN")
        else
            add_packages("wgpu-native")
            add_defines("IMGUI_IMPL_WEBGPU_BACKEND_WGPU")
        end
    end

    if has_config("freetype") then
        add_files("misc/freetype/imgui_freetype.cpp")
        add_headerfiles("(misc/freetype/imgui_freetype.h)")
        add_packages("freetype")
        add_defines("IMGUI_ENABLE_FREETYPE")
    end

    if has_config("user_config") then
        add_defines("IMGUI_USER_CONFIG=\"" .. get_config("user_config") .. "\"")
    end
