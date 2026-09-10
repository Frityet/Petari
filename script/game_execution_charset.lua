-- Original Game sources stay UTF-8 on disk. Clang's expanded-token provenance
-- selects ordinary Game literals for the Wii execution character set.
local configurations = {}

rule("smgpc.game_execution_charset")
    on_config(function (target)
        -- This rule is installed at project scope; SDK/dependency targets must
        -- retain their own compiler and UTF-8 source contract.
        if not target:name():startswith("smg-pc-") or not target:has_sourcekind("cxx") then return end
        import("core.base.bytes")
        import("lib.detect.find_tool")
        local compiler, toolname = target:tool("cxx")
        assert(toolname == "clang" or toolname == "clangxx", "Original Game execution encoding requires Clang with LLVM development libraries")
        local root = os.projectdir()
        local wrapper = path.join(root, "script/game_execution_charset.py")
        -- Xmake persists target tool lookup results across invocations. Decode
        -- only our own prior wrapper command, never an arbitrary compiler shim.
        local previous = os.argv(compiler)
        if table.contains(previous, wrapper) then
            for index, argument in ipairs(previous) do
                if argument == "--compiler" then
                    compiler = assert(previous[index + 1], "Cached Game compiler wrapper has no compiler")
                    break
                end
            end
        end
        local key = root .. "\0" .. compiler
        local config = configurations[key]
        if not config then
            local python = assert(find_tool("python3"), "python3 is required for original Game execution encoding")
            local llvmconfig = path.join(path.directory(compiler), "llvm-config")
            assert(os.isfile(llvmconfig), "Use an LLVM Clang toolchain containing llvm-config and libclang-cpp")
            local includedir = os.iorunv(llvmconfig, {"--includedir"}):trim()
            local libdir = os.iorunv(llvmconfig, {"--libdir"}):trim()
            local version = os.iorunv(llvmconfig, {"--version"}):trim()
            local source = path.join(root, "script/game_literal_preprocessor.cpp")
            local rulefile = path.join(root, "script/game_execution_charset.lua")
            -- A bytes object hashes this manifest's contents, rather than
            -- interpreting a concatenated string as a filesystem path.
            local fingerprint = hash.sha256(bytes(table.concat({
                hash.sha256(wrapper), hash.sha256(source), hash.sha256(rulefile),
                compiler, includedir, libdir, version
            }, "\0")))
            local directory = path.join(root, "build/.tools/game-execution-charset", fingerprint)
            local helper = path.join(directory, "game-literal-preprocessor")
            if not os.isfile(helper) then
                os.mkdir(directory)
                local temporary = helper .. ".new"
                os.vrunv(compiler, {"-std=c++23", "-O2", source, "-I" .. includedir,
                    "-L" .. libdir, "-Wl,-rpath," .. libdir, "-lclang-cpp", "-lLLVM", "-o", temporary})
                os.mv(temporary, helper)
            end
            local args = {python.program, wrapper, "--compiler", compiler, "--helper", helper,
                          "--game-root", path.join(root, "src/Game")}
            -- These complete providers contain extracted original Game
            -- behavior. Mixed services encode their Game API inputs explicitly.
            for _, file in ipairs({
                "src/compat/EventUtilCompat.cpp",
                "src/compat/OriginalSceneWipeUtil.cpp",
                "src/compat/OriginalMarioSound.cpp"
            }) do
                table.insert(args, "--game-file")
                table.insert(args, path.join(root, file))
            end
            table.insert(args, "--")
            config = {tool = "clangxx@" .. os.args(args), fingerprint = fingerprint}
            configurations[key] = config
        end
        target:set("toolset", "cxx", config.tool)
        -- Xmake's preprocessor cache would erase provenance before the wrapper
        -- receives tokens. The wrapper emits normal original-file depfiles.
        target:set("policy", "build.ccache", false)
        target:add("cxxflags", "-DSMGPC_GAME_EXECUTION_CHARSET_ID=0x" .. config.fingerprint:sub(1,16), {force = true})
        local stamp = path.join(target:autogendir(), "game-execution-charset-fingerprint.txt")
        target:data_set("game.charset.stamp", stamp)
        target:data_set("game.charset.fingerprint", config.fingerprint)
        if not os.isfile(stamp) or io.readfile(stamp) ~= config.fingerprint then
            -- Tool edits must invalidate archive/link steps too, even when
            -- object mtimes fall within Xmake's coarse timestamp resolution.
            target:data_set("rebuilt", true)
        end
    end)
    after_build(function (target)
        local stamp = target:data("game.charset.stamp")
        if not stamp then return end
        local fingerprint = target:data("game.charset.fingerprint")
        if not os.isfile(stamp) or io.readfile(stamp) ~= fingerprint then
            os.mkdir(path.directory(stamp))
            io.writefile(stamp, fingerprint)
        end
    end)
rule_end()
