rule("smg-pc-bgfx-shaders")
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
        assert(shader_type, "cannot determine shader type for " .. shaderfile)

        local bgfx_platform_map = {
            windows = "windows",
            linux = "linux",
            macosx = "osx"
        }
        local bgfx_platform = bgfx_platform_map[target:plat()]
        assert(bgfx_platform, "unsupported platform for shaderc: " .. target:plat())

        local varyingdef = fileconfig.vardef or path.join(path.directory(shaderfile), "varying.def.sc")

        local backend_variants = fileconfig.backends
        if backend_variants == nil then
            backend_variants = {{
                output_name = fileconfig.output_name,
                output_dir = fileconfig.output_dir,
                backend_dir = fileconfig.backend_dir,
                profile = fileconfig.profile
            }}
        end

        local last_binary = nil
        for _, backend in ipairs(backend_variants) do
            local output_name = backend.output_name or fileconfig.output_name
            assert(output_name, "output_name is required for " .. shaderfile)

            local output_dir = backend.output_dir or fileconfig.output_dir or "shaders"
            local backend_dir = backend.backend_dir or fileconfig.backend_dir or "glsl"
            local profile = backend.profile or fileconfig.profile or "120"

            local outputdir = path.join(target:targetdir(), output_dir, backend_dir)
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
            last_binary = binary
        end

        batchcmds:add_depfiles(shaderfile, varyingdef)
        if last_binary ~= nil then
            batchcmds:set_depmtime(os.mtime(last_binary))
        end
    end)

target("smg-pc-render")
    set_kind("static")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_includedirs("./", { public = true })
    add_packages("bgfx", "stb", "glfw", { public = true })
    add_deps("smg-pc-common")
    add_rules("smg-pc-bgfx-shaders")
    add_files("layout/shaders/vs_layout.sc", {
        type = "vertex",
        backends = {
            { output_dir = "shaders", backend_dir = "glsl", output_name = "vs_layout.bin", profile = "120" },
            { output_dir = "shaders", backend_dir = "spirv", output_name = "vs_layout.bin", profile = "spirv" }
        }
    })
    add_files("layout/shaders/fs_layout.sc", {
        type = "fragment",
        backends = {
            { output_dir = "shaders", backend_dir = "glsl", output_name = "fs_layout.bin", profile = "120" },
            { output_dir = "shaders", backend_dir = "spirv", output_name = "fs_layout.bin", profile = "spirv" }
        }
    })
