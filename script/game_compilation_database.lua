-- Source editors parse the unchanged UTF-8 files with the actual compiler.
-- Builds continue through the original Game execution-encoding wrapper.
rule("smgpc.compilation_database")
    set_kind("project")
    after_build(function ()
        if os.getenv("XMAKE_IN_XREPO") then return end
        import("core.base.task")
        import("lib.detect.find_tool")
        task.run("project", {kind = "compile_commands", lsp = "clangd"})
        local python = assert(find_tool("python3"))
        os.vrunv(python.program, {path.join(os.projectdir(), "script/game_compilation_database.py"),
                                 path.join(os.projectdir(), "compile_commands.json")})
    end)
rule_end()
