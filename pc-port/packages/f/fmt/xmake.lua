package("fmt")
    set_homepage("https://fmt.dev")
    set_description("A modern formatting library")
    set_license("MIT")

    add_urls("https://github.com/fmtlib/fmt/releases/download/$(version)/fmt-$(version).zip")
    add_versions("11.1.4", "49b039601196e1a765e81c5c9a05a61ed3d33f23b3961323d7322e4fe213d3e6")

    -- LLVM 23 libc++ no longer supplies malloc/free through unrelated headers.
    add_patches("11.1.4", "patches/11.1.4/cstdlib.patch",
                "fee74647f8b64585ab8aa1b153f000aba84e3bacfa553f13e14c4e2130f9f35f")

    add_configs("header_only", {description = "Use header only version", default = false, type = "boolean"})
    add_configs("unicode", {description = "Enable Unicode support", default = true, type = "boolean"})

    on_load(function (package)
        if package:config("header_only") then
            package:add("defines", "FMT_HEADER_ONLY=1")
            package:set("kind", "library", {headeronly = true})
        else
            package:add("deps", "cmake")
            package:add("links", "fmt")
        end
        if package:config("shared") then
            package:add("defines", "FMT_LIB_EXPORT")
        end
        if not package:config("unicode") then
            package:add("defines", "FMT_UNICODE=0")
        end
    end)

    on_install(function (package)
        if package:has_tool("cxx", "cl") and package:config("unicode") then
            package:add("cxxflags", "/utf-8")
        end
        if package:config("header_only") then
            os.cp("include/fmt", package:installdir("include"))
            return
        end
        io.gsub("CMakeLists.txt", "MASTER_PROJECT AND CMAKE_GENERATOR MATCHES \"Visual Studio\"", "0")
        local configs = {
            "-DFMT_TEST=OFF", "-DFMT_DOC=OFF", "-DFMT_FUZZ=OFF", "-DFMT_DEBUG_POSTFIX=",
            "-DCMAKE_CXX_VISIBILITY_PRESET=default",
            "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"),
            "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"),
            "-DFMT_UNICODE=" .. (package:config("unicode") and "ON" or "OFF"),
        }
        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <fmt/format.h>
            #include <string>
            #include <cassert>
            static void test() {
                std::string s = fmt::format("{}", "hello");
                assert(s == "hello");
            }
        ]]}, {configs = {languages = "c++14"}, includes = "fmt/format.h"}))
    end)
