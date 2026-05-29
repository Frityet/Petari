target("smg-pc-app")
    set_kind("static")
    add_files("Application.cpp")
    add_headerfiles("Application.hpp")
    add_includedirs("./", {public = true})
    add_deps {
        "smg-pc-render",
        "smg-pc-common",
        "smg-pc-game"
    }

target("smg-pc")
    set_kind("binary")
    add_files("main.cpp")
    add_files("../../aurora/lib/compat.cpp")
    add_headerfiles("**.hpp")
    add_deps {
        "smg-pc-app",
        "aurora-main"
    }
