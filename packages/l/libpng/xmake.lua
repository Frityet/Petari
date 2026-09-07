package("libpng")
    set_homepage("https://www.libpng.org/pub/png/libpng.html")
    set_description("The official PNG reference library")
    set_license("libpng-2.0")

    add_urls("https://github.com/glennrp/libpng/archive/refs/tags/$(version).tar.gz")
    add_versions("v1.6.58", "a9d4df463d36a6e5f9c29bd6f4967312d17e996c1854f3511f833924eb1993cf")
    add_deps("zlib")

    if is_plat("wasm") then
        add_configs("shared", {description = "Build shared library", default = false, type = "boolean", readonly = true})
    elseif is_plat("linux") then
        add_syslinks("m")
    end

    on_install(function (package)
        if package:is_plat("android") and package:is_arch("armeabi-v7a") then
            io.replace("arm/filter_neon.S", ".func", ".hidden", {plain = true})
            io.replace("arm/filter_neon.S", ".endfunc", "", {plain = true})
        end
        os.cp("scripts/pnglibconf.h.prebuilt", "pnglibconf.h")
        io.writefile("xmake.lua", [[
            add_rules("mode.debug", "mode.release")
            add_requires("zlib")
            target("png")
                set_kind("$(kind)")
                add_files("*.c|example.c|pngtest.c")
                if is_arch("x86", "x64", "i386", "x86_64") then
                    add_files("intel/*.c")
                    add_defines("PNG_INTEL_SSE_OPT=1")
                    add_vectorexts("sse", "sse2")
                elseif is_arch("arm.*") then
                    add_files("arm/*.c")
                    if is_plat("windows") then
                        add_defines("PNG_ARM_NEON_OPT=1")
                        add_defines("PNG_ARM_NEON_IMPLEMENTATION=1")
                    else
                        add_files("arm/*.S")
                        add_defines("PNG_ARM_NEON_OPT=2")
                    end
                elseif is_arch("mips.*") then
                    add_files("mips/*.c")
                    add_defines("PNG_MIPS_MSA_OPT=2")
                elseif is_arch("ppc.*") then
                    add_files("powerpc/*.c")
                    add_defines("PNG_POWERPC_VSX_OPT=2")
                end
                add_headerfiles("*.h")
                add_packages("zlib")
                if is_kind("shared") and is_plat("windows") then
                    add_defines("PNG_BUILD_DLL")
                end
        ]])
        -- xmake 3.1.1 drops explicit archiver overrides for host LLVM sub-builds.
        -- Forward the selected tool so Homebrew's llvm-ar survives the Apple setup.
        import("package.tools.xmake").install(package, {ar = get_config("ar")})
    end)

    on_test(function (package)
        assert(package:has_cfuncs("png_create_read_struct", {includes = "png.h"}))
    end)
