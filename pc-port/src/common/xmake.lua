add_requires("fmt")

target("smg-pc-common")
    set_kind("static")
    add_files("**.cpp")
    add_headerfiles("**.hpp")
    add_includedirs("./", {public = true})
    add_packages("fmt", { public = true })
