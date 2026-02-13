add_requires("fmt")

target("smg-pc-common")
    set_kind("static")
    add_files("Logger.cpp")
    add_headerfiles("**.hpp")
    add_includedirs("./", {public = true})
    add_packages("fmt", { public = true })
