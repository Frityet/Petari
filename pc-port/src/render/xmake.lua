rule("pc-port-bgfx-shaders")
    set_extensions(".sc")
    on_buildcmd_file(function (target, batchcmds, shaderfile, opt)
        import("core.base.option")

        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.bgfx-shader %s", shaderfile)

        local fileconfig = target:fileconfig(shaderfile) or {}
        local bgfx_pkg = target:pkg("bgfx")
        assert(bgfx_pkg, "bgfx package is required to compile shaders")

        local bgfx_bindir = path.join(bgfx_pkg:installdir(), "bin")
        local shaderc_candidates = {
            path.join(bgfx_bindir, "shadercRelease"),
            path.join(bgfx_bindir, "shadercDebug")
        }

        local shaderc = nil
        for _, candidate in ipairs(shaderc_candidates) do
            if os.isexec(candidate) then
                shaderc = candidate
                break
            end
        end
        assert(shaderc, "bgfx shaderc not found in package bin dir: " .. bgfx_bindir)

        local shader_type = fileconfig.type
        if not shader_type then
            if shaderfile:match("^vs_.*%.sc$") then
                shader_type = "vertex"
            elseif shaderfile:match("^fs_.*%.sc$") then
                shader_type = "fragment"
            elseif shaderfile:match("^cs_.*%.sc$") then
                shader_type = "compute"
            end
        end
        assert(shader_type, "cannot determine shader type for " .. shaderfile)

        local output_name = fileconfig.output_name
        assert(output_name, "output_name is required for " .. shaderfile)

        local output_dir = fileconfig.output_dir or "shaders"
        local profile = fileconfig.profile or "120"

        local bgfx_platform_map = {
            windows = "windows",
            linux = "linux",
            macosx = "osx"
        }
        local bgfx_platform = bgfx_platform_map[target:plat()]
        assert(bgfx_platform, "unsupported platform for shaderc: " .. target:plat())

        local varyingdef = fileconfig.vardef or path.join(path.directory(shaderfile), "varying.def.sc")

        local outputdir = path.join(target:targetdir(), output_dir, "glsl")
        batchcmds:mkdir(outputdir)
        local binary = path.join(outputdir, output_name)

        local args = {
            "-f", shaderfile,
            "--type", shader_type,
            "--varyingdef", varyingdef,
            "--platform", bgfx_platform,
            "-i", path.directory(shaderfile),
            "-o", binary,
            "--profile", profile
        }

        if option.get("verbose") then
            batchcmds:show(shaderc .. " " .. os.args(args))
        end
        batchcmds:vrunv(shaderc, args)
        batchcmds:add_depfiles(shaderfile, varyingdef)
        batchcmds:set_depmtime(os.mtime(binary))
    end)

target("pc-port-image")
    set_kind("static")
    add_files("TplDecoder.cpp")
    add_includedirs("$(projectdir)/src", {public = true})
    add_deps("pc-port-core")

target("pc-port-render")
    set_kind("static")
    add_files("BgfxRenderer.cpp")
    add_includedirs("$(projectdir)/src", {public = true})
    add_deps("pc-port-core", "pc-port-layout")
    add_packages("bgfx", "libsdl2", {public = true})
    add_packages("stb")
    add_rules("pc-port-bgfx-shaders")
    add_files("shaders/vs_layout.sc", {
        type = "vertex",
        output_dir = "shaders",
        output_name = "vs_layout.bin",
        profile = "120"
    })
    add_files("shaders/fs_layout.sc", {
        type = "fragment",
        output_dir = "shaders",
        output_name = "fs_layout.bin",
        profile = "120"
    })
