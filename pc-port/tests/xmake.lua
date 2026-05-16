target("smg-pc-compat-tests")
    set_kind("binary")
    add_files("CompatLayerTests.cpp")
    add_deps {
        "smg-pc-game",
        "smg-pc-render"
    }
