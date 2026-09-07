set_project("smg-pc")
set_xmakever("3.0.0")

add_repositories("local-repo $(projectdir)")

includes("aurora")
set_project("smg-pc")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {lsp = "clangd"})
set_languages("c++23")
-- Keep complete original translation units while linking the reachable native
-- closure. Referenced functions must still have concrete native providers.
if is_plat("macosx", "iphoneos", "linux", "mingw") then
    add_cxflags("-ffunction-sections", "-fdata-sections")
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip")
    else
        add_ldflags("-Wl,--gc-sections")
    end
end
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
