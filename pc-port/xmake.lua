set_project("smg-pc")
set_xmakever("3.0.0")

add_repositories("local-repo $(projectdir)")

option("aurora_enable_gx")
    set_default(true)
option_end()
option("aurora_enable_card")
    set_default(true)
option_end()
option("aurora_enable_dvd")
    set_default(true)
option_end()
option("aurora_dawn_version")
    set_default("v20260523.201736")
option_end()
set_config("aurora_enable_gx", "y")
set_config("aurora_enable_card", "y")
set_config("aurora_enable_dvd", "y")
set_config("aurora_dawn_version", "v20260523.201736")
add_requireconfs("dawn-build", {configs = {cxxflags = "-std=c++20"}})

includes("aurora")
set_project("smg-pc")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {lsp = "clangd"})
set_languages("c++23")

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
