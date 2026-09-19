local subdirs = {
    "common",
    "render",
    "Game",
    "app"
}

if is_mode("debug") then
    table.insert(subdirs, "debug")
end

includes(subdirs)
