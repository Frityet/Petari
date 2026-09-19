set_project("smg-pc")
set_xmakever("3.1.0")
set_defaultmode("debug")
set_policy("run.autobuild", true)
-- Explicit `xmake f` options take precedence over these project defaults.
set_config("toolchain", "llvm")
set_config("runtimes", "c++_shared")
if is_plat("macosx") or (not get_config("plat") and is_host("macosx")) then
    set_config("target_minver", "26.0")
end
-- Build only the game and its dependency closure by default.
set_default(false)

add_repositories("local-repo $(projectdir)")

option("disc")
    set_showmenu(true)
    set_description("Local game disc image used by xmake run")
option_end()

option("optimize_debug")
    set_default(true)
    set_showmenu(true)
    set_description("Optimize debug builds while retaining symbols and runtime checks")
option_end()

-- Apply before including Aurora so native and Game targets use the same
-- optimization policy. Keep debug guards and per-file floating-point rules.
if is_mode("debug") and has_config("optimize_debug") then
    set_optimize("faster")
end

includes("aurora")
set_project("smg-pc")

add_rules("mode.debug", "mode.release")
includes("scripts/build/compilation_database.lua")
add_rules("smgpc.compilation_database")
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

if is_mode("debug") then
    add_defines("SMGPC_DEBUG_BUILD")
end

local include_dirs = {"src"}

if is_mode("debug") then
    table.insert(include_dirs, "tests")
end

includes(include_dirs)
