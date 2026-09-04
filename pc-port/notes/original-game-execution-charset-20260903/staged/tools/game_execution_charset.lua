-- Proposed compiler-only rule. Apply to each native C++ consumer of original
-- Game headers; only explicitly admitted original files are mapped, so host
-- source/UI strings retain UTF-8. No edited/generated source enters Game/.
rule("smgpc.game_execution_charset")
    on_config(function (target)
        import("core.base.json")
        import("lib.detect.find_tool")
        local python = assert(find_tool("python3"), "python3 is required for Game execution encoding")
        local script = assert(target:values("game.charset.script"), "missing charset generator")
        local lexer = assert(target:values("game.charset.lexer"), "missing built Clang raw lexer")
        local roots = table.wrap(target:values("game.charset.roots"))
        assert(#roots > 0, "original Game provenance roots must be explicit")
        local output = path.join(target:autogendir(), "game-execution-charset")
        local args = {script, "--lexer", lexer, "--output", output}
        for _, root in ipairs(roots) do
            table.insert(args, "--root")
            table.insert(args, root)
        end
        local extracts = target:values("game.charset.extracts")
        if extracts then
            table.insert(args, "--extracts")
            table.insert(args, extracts)
        end
        -- on_config runs before dependency checking and compiler command
        -- caching on every invocation. Content hashes re-lex only changed
        -- files; the generated overlay is not rewritten when unchanged.
        os.vrunv(python.program, args)
        local report = json.loadfile(path.join(output, "report.json"))
        target:add("cxxflags", os.args({"-ivfsoverlay", report.overlay}), {force = true})
        -- xmake tracks compiler flags in each object dependency record. Tool
        -- or explicit extract-provenance changes therefore invalidate objects
        -- even when no original source/header mtime changed. A separate literal
        -- payload fingerprint closes coarse-mtime and external compiler-cache
        -- reuse when encoded literals change. Other source/comment edits retain
        -- original .d paths and normal selective recompilation.
        target:add("cxxflags", "-DSMGPC_GAME_EXECUTION_CHARSET_ID=0x" .. report.fingerprint:sub(1,16), {force = true})
        target:add("cxxflags", "-DSMGPC_GAME_EXECUTION_LITERALS_ID=0x" .. report.mapping_fingerprint:sub(1,16), {force = true})
        local stamp = path.join(output, "last-successful-target-fingerprint.txt")
        local fingerprint = report.fingerprint .. report.mapping_fingerprint
        target:data_set("game.charset.stamp", stamp)
        target:data_set("game.charset.fingerprint", fingerprint)
        if not os.isfile(stamp) or io.readfile(stamp) ~= fingerprint then
            -- xmake also uses coarse object mtimes for archive/link reuse.
            -- Its normal rebuild state invalidates both stages; persist only
            -- after the entire target succeeds, never during generation.
            target:data_set("rebuilt", true)
        end
    end)
    after_build(function (target)
        local stamp = target:data("game.charset.stamp")
        local fingerprint = target:data("game.charset.fingerprint")
        if not os.isfile(stamp) or io.readfile(stamp) ~= fingerprint then
            io.writefile(stamp, fingerprint)
        end
    end)
