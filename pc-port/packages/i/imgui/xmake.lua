package("imgui")
    set_homepage("https://github.com/ocornut/imgui")
    set_description("Bloat-free Immediate Mode Graphical User interface for C++ with minimal dependencies")
    set_license("MIT")

    add_urls("https://github.com/ocornut/imgui.git", {alias = "git"})
    add_versions("git:v1.91.9-docking", "v1.91.9-docking")

    add_configs("sdl3",          {description = "Enable the SDL3 platform backend", default = false, type = "boolean"})
    add_configs("sdl3_renderer", {description = "Enable the SDL3 renderer backend", default = false, type = "boolean"})
    add_configs("wgpu",          {description = "Enable the WebGPU backend", default = false, type = "boolean"})
    add_configs("wgpu_backend",  {description = "Use specific WebGPU backend", default = "wgpu", type = "string", values = {"wgpu", "dawn"}})
    add_configs("dawn_version",  {description = "Dawn prebuilt version for the Dawn WebGPU backend", default = "v20260523.201736", type = "string"})
    add_configs("freetype",      {description = "Use FreeType to build and rasterize the font atlas", default = false, type = "boolean"})
    add_configs("user_config",   {description = "Use user config", default = nil, type = "string"})

    add_includedirs("include", "include/imgui", "include/backends", "include/misc/cpp")

    on_load(function (package)
        if package:config("sdl3") or package:config("sdl3_renderer") then
            package:add("deps", "libsdl3", {
                configs = {shared = false, x11 = package:is_plat("linux"), x11_shared = true, wayland = false},
            })
        end
        if package:config("wgpu") then
            if package:config("wgpu_backend") == "dawn" then
                package:add("deps", "dawn-build " .. package:config("dawn_version"), {configs = {shared = false}})
                package:add("defines", "IMGUI_IMPL_WEBGPU_BACKEND_DAWN")
            else
                package:add("deps", "wgpu-native")
                package:add("defines", "IMGUI_IMPL_WEBGPU_BACKEND_WGPU")
            end
        end
        if package:config("freetype") then
            package:add("deps", "freetype")
        end
    end)

    on_install(function (package)
        local configs = {
            sdl3 = package:config("sdl3"),
            sdl3_renderer = package:config("sdl3_renderer"),
            wgpu = package:config("wgpu"),
            wgpu_backend = package:config("wgpu_backend"),
            dawn_version = package:config("dawn_version"),
            freetype = package:config("freetype"),
            user_config = package:config("user_config"),
        }
        os.cp(path.join(package:scriptdir(), "port", "xmake.lua"), "xmake.lua")
        import("package.tools.xmake").install(package, configs)
    end)

    on_test(function (package)
        if package:config("user_config") ~= nil then
            return
        end
        assert(package:check_cxxsnippets({test = [[
            void test() {
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGui::DestroyContext();
            }
        ]]}, {configs = {languages = "c++14"}, includes = {"imgui.h"}}))
    end)
