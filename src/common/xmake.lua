add_requires("fmt 11.1.4")
add_requires("nlohmann_json")

if is_mode("debug") then
    add_requires("sqlite3 3.53.0+0")
end

target("smg-pc-common")
    set_kind("static")
    add_files("Logger.cpp", "DumpJson.cpp", "MarkdownWriter.cpp", "BinaryChunkFile.cpp", "../compat/MslPrintfCompat.cpp")
    if is_mode("debug") then
        add_files("Sqlite.cpp", "TraceStore.cpp", "TraceAnalysis.cpp")
    end
    add_headerfiles("**.hpp")
    add_includedirs("./", {public = true})
    add_includedirs("../", {public = true})
    add_packages("fmt", { public = true })
    add_packages("nlohmann_json", { public = true })
    if is_mode("debug") then
        add_packages("sqlite3", { public = true })
    end
