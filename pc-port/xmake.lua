set_project("smg-pc")
set_xmakever("3.0.0")

add_repositories("local-repo $(projectdir)")

includes("aurora")
set_project("smg-pc")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {lsp = "clangd"})
set_languages("c++23")
includes("scripts")

if not is_mode("debug") then
    add_defines("NDEBUG")
end

if is_mode("debug") then
    add_defines("SMGPC_DEBUG_BUILD")
end

local include_dirs = {"src"}

if is_mode("debug") then
    table.insert(include_dirs, "tests")
end

includes(include_dirs)
