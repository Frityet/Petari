package("dawn-build")
    set_homepage("https://github.com/encounter/dawn-build")
    set_description("Prebuilt Dawn/WebGPU install trees used by Aurora")
    set_license("BSD-3-Clause")

    add_configs("shared", {description = "Use a shared Dawn library when one is provided", default = false, type = "boolean"})

    local hashes = {
        ["v20260523.201736"] = {
            ["linux-x86_64"] = "f8d0886fe7ddd05227781d3d4feb73eb4ee2484d1d1ae9ea29d1adbb665a3457",
        },
        ["v20260423.175430"] = {
            ["darwin-arm64"] = "8c0518bc143bd53dbef6f238c00cc9874099882f41f647cd2baf49b9fe8e5c60",
            ["darwin-x86_64"] = "deab8a47deec3ae17d8315026d38d05bbb83c405d073f3dd8ab2b9443993db58",
            ["linux-aarch64"] = "10ce5ed7df7a7234c37d983038c66180c3fb87ff2b3319fd1d644ced39f4f4dd",
            ["linux-x86_64"] = "9e45b00d3c7863349cdde8a6d6b2227ba6ea9da8deac25f021af54d7e417c111",
            ["windows-amd64"] = "8969fb7390af8c194677c5d81caff6037e67c79b03c409b0e94b55520b647899",
            ["windows-arm64"] = "66785d47a3f8b84114053226404fe9a35a5e48982cd3546a7df5e16cb1fa72b0",
        },
    }

    local function platform_key(package)
        local system
        if package:is_plat("windows", "mingw", "msys") then
            system = "windows"
        elseif package:is_plat("macosx") then
            system = "darwin"
        elseif package:is_plat("linux") then
            system = "linux"
        else
            return nil
        end

        local arch
        if package:is_arch("x64", "x86_64") then
            arch = system == "windows" and "amd64" or "x86_64"
        elseif package:is_arch("arm64", "aarch64") then
            arch = "arm64"
            if system == "linux" then
                arch = "aarch64"
            end
        end

        if system and arch then
            return system .. "-" .. arch
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
                raise("package(dawn-build): no prebuilt Dawn package for %s/%s", package:plat(), package:arch())
            end
            local version_hashes = hashes[version]
            if not version_hashes or not version_hashes[key] then
                raise("package(dawn-build): no hash for Dawn %s on %s", version, key)
            end
            package:add("urls", "https://github.com/encounter/dawn-build/releases/download/$(version)/dawn-" .. key .. ".tar.gz")
            package:add("versions", version, version_hashes[key])
        end)
    end

    on_load(function (package)
        package:add("includedirs", "include")
        package:add("cxxflags", "-std=c++20")
        package:add("links", "webgpu_dawn")

        if package:is_plat("linux") then
            package:add("syslinks", "dl", "rt", "pthread")
        elseif package:is_plat("windows", "mingw", "msys") then
            package:add("syslinks", "advapi32", "dbghelp")
        elseif package:is_plat("macosx") then
            package:add("frameworks", "CoreFoundation", "Foundation", "IOKit", "Metal", "QuartzCore")
        end
    end)

    on_install(function (package)
        os.cp("*", package:installdir())
    end)

    on_test(function (package)
        assert(package:has_cxxincludes("webgpu/webgpu_cpp.h"))
    end)
