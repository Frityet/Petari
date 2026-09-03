package("dawn-build")
    set_homepage("https://github.com/encounter/dawn")
    set_description("Prebuilt Dawn/WebGPU install trees used by Aurora")
    set_license("BSD-3-Clause")

    add_configs("shared", {description = "Use a shared Dawn library when one is provided", default = false, type = "boolean"})

    local hashes = {
        ["v20260618.032059"] = {
            ["android-aarch64"] = "ee54be2311b714a7c079629654317b06737909aa30829c44c694907dec361060",
            ["darwin-arm64"] = "a9cc9903761e60cf70d7d771bd0c482be1943e273717782d71c33313afeb6080",
            ["darwin-x86_64"] = "d9cb3fd6f59d6fb69a356c0f832db9a6792a610e9de9ad2b0254f1533cf18e2e",
            ["ios-arm64"] = "ada0bafc173152d80eba7c3b2f9609a71185d5809cbd5dd3251b91a0803a7ae2",
            ["linux-aarch64"] = "4c02f50500831d1e4bf568f4aa7db34a8286842ee156443c9dcc3ac1a44fb1b0",
            ["linux-x86_64"] = "650a5479d8ecdfaad1a74079715010427fc30dab6c266fe46320948340897962",
            ["windows-amd64"] = "d26d3107142c5a123d2102979c7403308668038372616fbd33a537e45c22f0dc",
            ["windows-arm64"] = "489838fb247a5b35b16da42100099cea0b06e6071159014a411d35330ab1745b",
        },
    }

    local function platform_key(package)
        local system
        if package:is_plat("android") then
            system = "android"
        elseif package:is_plat("iphoneos") then
            system = "ios"
        elseif package:is_plat("windows", "mingw", "msys") then
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
            if system == "linux" or system == "android" then
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
            package:add("urls", "https://github.com/encounter/dawn/releases/download/$(version)/dawn-" .. key .. ".tar.gz")
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
            package:add("frameworks", "CoreFoundation", "Foundation", "IOKit", "IOSurface", "Metal", "QuartzCore")
        end
    end)

    on_install(function (package)
        os.cp("*", package:installdir())
    end)

    on_test(function (package)
        assert(package:has_cxxincludes("webgpu/webgpu_cpp.h"))
    end)
