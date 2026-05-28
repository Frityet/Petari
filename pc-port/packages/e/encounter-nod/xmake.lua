package("encounter-nod")
    set_homepage("https://github.com/encounter/nod")
    set_description("GameCube and Wii disc image library from encounter/nod")
    set_license("MIT")

    add_configs("shared", {description = "Use the shared nod library when one is provided", default = false, type = "boolean"})

    local hashes = {
        ["v2.0.0-alpha.8"] = {
            ["linux-x86_64"] = "d9812d435600333a13bfacd9043d027e308fc29ba0bd65801858fe35ee77e584",
            ["macos-arm64"] = "1969f4311bb3e44ff7e772f2d660937cdc629f8ab0937f05b12f041195f07c41",
            ["windows-x86_64"] = "7c49f8f0701855602f9e3f32a0003b6eaa71a8a520afa4bc34d6302e2508eddf",
        },
    }

    local function platform_key(package)
        if package:is_plat("linux") and package:is_arch("x64", "x86_64") then
            return "linux-x86_64"
        elseif package:is_plat("macosx") and package:is_arch("arm64", "aarch64") then
            return "macos-arm64"
        elseif package:is_plat("windows", "mingw", "msys") and package:is_arch("x64", "x86_64") then
            return "windows-x86_64"
        end
    end

    if on_source then
        on_source(function (package)
            local version = package:requireinfo().version
            if type(version) ~= "string" then
                version = tostring(version)
            end
            local key = platform_key(package)
            if not key then
                raise("package(encounter-nod): no prebuilt nod package for %s/%s", package:plat(), package:arch())
            end
            local version_hashes = hashes[version]
            if not version_hashes or not version_hashes[key] then
                raise("package(encounter-nod): no hash for nod %s on %s", version, key)
            end
            package:add("urls", "https://github.com/encounter/nod/releases/download/$(version)/libnod-" .. key .. ".tar.gz")
            package:add("versions", version, version_hashes[key])
        end)
    end

    on_load(function (package)
        package:add("includedirs", "include")
        package:add("links", "nod")
        if package:is_plat("windows", "mingw", "msys") and not package:config("shared") then
            package:add("syslinks", "ntdll", "userenv", "ws2_32", "dbghelp", "bcrypt")
        end
    end)

    on_install(function (package)
        os.cp("*", package:installdir())
        if not package:config("shared") then
            os.tryrm(path.join(package:installdir("lib"), "libnod.so*"))
            os.tryrm(path.join(package:installdir("lib"), "nod.dll"))
            os.tryrm(path.join(package:installdir("lib"), "libnod.dylib"))
        end
    end)

    on_test(function (package)
        assert(package:has_cxxincludes("nod.h"))
    end)
