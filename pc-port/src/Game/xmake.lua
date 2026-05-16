target("smg-pc-game")
    set_kind("static")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_includedirs("../", { public = true })
    add_deps {
        "smg-pc-common",
        "smg-pc-render"
    }
