target("smg-pc-showcase")
    set_kind("binary")
    add_files {
        "Showcase.cpp",
        "../../aurora/lib/compat.cpp"
    }
    add_deps {
        "smg-pc-app",
        "aurora-main"
    }
