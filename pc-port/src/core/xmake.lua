target("pc-port-core")
    set_kind("static")
    add_files("Logger.cpp")
    add_includedirs("$(projectdir)/src", {public = true})
