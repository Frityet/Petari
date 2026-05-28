package("libsdl3")
    set_homepage("https://www.libsdl.org/")
    set_description("Simple DirectMedia Layer")
    set_license("zlib")

    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::SDL3")
    elseif is_plat("linux") then
        add_extsources("pacman::sdl3", "apt::libsdl3-dev")
    elseif is_plat("macosx") then
        add_extsources("brew::sdl3")
    end

    add_urls("https://github.com/libsdl-org/SDL/releases/download/release-$(version)/SDL3-$(version).tar.gz")
    add_versions("3.4.8", "e9fff7467fb60f037e6708da18b25560649e4c63edc2a69bb871b960d9cbfbba")

    add_deps("cmake", "egl-headers", "opengl-headers")

    if is_plat("linux", "bsd", "cross") then
        add_configs("x11", {description = "Enables X11 support", default = true, type = "boolean"})
        add_configs("x11_shared", {description = "Dynamically load X11 support", default = true, type = "boolean"})
        add_configs("wayland", {description = "Enables Wayland support", default = false, type = "boolean"})
        add_configs("wayland_shared", {description = "Dynamically load Wayland support", default = true, type = "boolean"})
    end

    if is_plat("wasm") then
        add_cxflags("-sUSE_SDL=0")
    end

    on_load(function (package)
        if package:is_plat("windows") then
            package:add("deps", "ninja")
            package:set("policy", "package.cmake_generator.ninja", true)
        end
        if package:is_plat("linux", "bsd", "cross") and package:config("x11") then
            local deplibs = {"libx11", "libxcb", "libxext", "libxcursor", "libxfixes", "libxi", "libxrandr", "libxrender"}
            local depconfig = package:config("x11_shared") and {private = true, configs = {shared = true}} or nil
            for _, lib in ipairs(deplibs) do
                package:add("deps", lib, depconfig)
            end
        end
        if package:is_plat("linux", "bsd", "cross") and package:config("wayland") then
            if package:config("wayland_shared") then
                package:add("deps", "wayland", {private = true, configs = {shared = true}})
            else
                package:add("deps", "wayland")
            end
        end
        if not package:config("shared") then
            if package:is_plat("windows", "mingw") then
                package:add("syslinks", "user32", "gdi32", "winmm", "imm32", "ole32", "oleaut32", "version", "uuid", "advapi32", "setupapi", "shell32")
            elseif package:is_plat("linux", "bsd") then
                package:add("syslinks", "pthread", "dl")
                if package:is_plat("bsd") then
                    package:add("syslinks", "usbhid")
                end
            elseif package:is_plat("android") then
                package:add("syslinks", "dl", "log", "android", "GLESv1_CM", "GLESv2", "OpenSLES")
            elseif package:is_plat("iphoneos", "macosx") then
                package:add("frameworks", "AudioToolbox", "AVFoundation", "CoreAudio", "CoreHaptics", "CoreMedia", "CoreVideo", "Foundation", "GameController", "Metal", "QuartzCore", "CoreFoundation", "UniformTypeIdentifiers")
                package:add("syslinks", "iconv")
                if package:is_plat("macosx") then
                    package:add("frameworks", "Cocoa", "Carbon", "ForceFeedback", "IOKit")
                else
                    package:add("frameworks", "CoreBluetooth", "CoreGraphics", "CoreMotion", "OpenGLES", "UIKit")
                end
            end
        end
    end)

    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DSDL_TEST_LIBRARY=OFF")
        table.insert(configs, "-DSDL_EXAMPLES=OFF")
        if package:is_plat("linux", "bsd", "cross") then
            table.insert(configs, "-DSDL_X11=" .. (package:config("x11") and "ON" or "OFF"))
            table.insert(configs, "-DSDL_X11_SHARED=" .. (package:config("x11_shared") and "ON" or "OFF"))
            table.insert(configs, "-DSDL_X11_XTEST=OFF")
            table.insert(configs, "-DSDL_X11_XSCRNSAVER=OFF")
            table.insert(configs, "-DSDL_WAYLAND=" .. (package:config("wayland") and "ON" or "OFF"))
            table.insert(configs, "-DSDL_WAYLAND_SHARED=" .. (package:config("wayland_shared") and "ON" or "OFF"))
            if not package:config("x11") and not package:config("wayland") then
                table.insert(configs, "-DSDL_UNIX_CONSOLE_BUILD=ON")
            end
        end

        local cflags
        local packagedeps
        if not package:is_plat("wasm") then
            packagedeps = table.join2(packagedeps or {}, {"egl-headers", "opengl-headers"})
        end

        if package:is_plat("linux", "bsd", "cross") then
            packagedeps = table.join2(packagedeps or {}, {"libxcursor", "libxext", "libxfixes", "libxcb", "libx11", "libxi", "libxrandr", "libxrender", "xorgproto", "wayland"})
        elseif package:is_plat("wasm") then
            cflags = {"-sUSE_SDL=0"}
        end

        local includedirs = {}
        for _, depname in ipairs(packagedeps) do
            local dep = package:dep(depname)
            if dep then
                local depfetch = dep:fetch()
                if depfetch then
                    for _, includedir in ipairs(depfetch.includedirs or depfetch.sysincludedirs or {}) do
                        table.insert(includedirs, includedir)
                    end
                end
            end
        end
        if #includedirs > 0 then
            includedirs = table.unique(includedirs)
            table.insert(configs, "-DCMAKE_INCLUDE_PATH=" .. table.concat(includedirs, ";"))
            cflags = cflags or {}
            for _, includedir in ipairs(includedirs) do
                table.insert(cflags, "-I" .. includedir)
            end
        end
        import("package.tools.cmake").install(package, configs, {cflags = cflags})
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <SDL3/SDL.h>
            int main(int argc, char** argv) {
                SDL_Init(0);
                SDL_Quit();
                return 0;
            }
        ]]}))
    end)
