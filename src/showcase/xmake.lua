target("smg-pc-showcase")
    set_kind("binary")
    add_files {
        "Showcase.cpp",
        "../../aurora/lib/compat.cpp"
    }
    add_deps {
        "smg-pc-app",
        "smg-pc-game",
        "aurora-main"
    }
    if is_plat("macosx", "iphoneos") then
        add_ldflags("-Wl,-dead_strip", {force = true})
    else
        add_ldflags("-Wl,--gc-sections", {force = true})
    end
