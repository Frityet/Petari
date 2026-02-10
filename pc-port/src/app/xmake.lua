
target("smg-pc")
    set_kind("binary")
    add_files("**.cpp")
    add_deps {
        "smg-pc-render",
        "smg-pc-common"
    }
