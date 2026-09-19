-- Builds and editors use the same compiler arguments and explicit CP932 literals.
rule("smgpc.compilation_database")
    set_kind("project")
    after_build(function ()
        if os.getenv("XMAKE_IN_XREPO") then return end
        import("core.base.task")
        task.run("project", {kind = "compile_commands", lsp = "clangd"})
    end)
rule_end()
