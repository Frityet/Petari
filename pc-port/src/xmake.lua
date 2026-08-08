local subdirs = {
    "common",
    "render",
    "Game",
    "app",
    "showcase"
}

if is_mode("debug") then
    table.insert(subdirs, "debug")
end

includes(subdirs)
