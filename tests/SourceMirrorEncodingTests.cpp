#include "SourceMirrorEncoding.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

int main(int argc, char **argv) {
    using smgpc::test::source_matches_with_cp932;
    int cases = 0;
    const auto check = [&](std::string_view original, std::string_view native, bool expected) {
        ++cases;
        if (source_matches_with_cp932(original, native) != expected) {
            throw std::runtime_error("CP932 source boundary failed case " + std::to_string(cases));
        }
    };
    check("f(\"日本語\");\n", "#include \"resource/TextEncoding.hpp\"\nf(CP932(\"日本語\"));\n", true);
    check("f(\"日\" \"本\");", "f(CP932(\"日\" \"本\"));", true);
    check("f(\"日\" /* retained */\n\"本\");", "f(CP932(\"日\" /* retained */\n\"本\"));", true);
    check("f(\"日\\\"本\\n\");", "f(CP932(\"日\\\"本\\n\"));", true);
    check("f(\"日本\");", "f(CP932 ( \"日本\" ));", true);
    check("f(value);", "f(CP932(value));", false);
    check("f(\"日\" + suffix);", "f(CP932(\"日\" + suffix));", false);
    check("f(\"日\", 2);", "f(CP932(\"日\", 2));", false);
    check("f(\"日\");", "f(CP932(\"日\");", false);
    check("f(\"日\");", "f(CP932(CP932(\"日\")));", false);
    check("f(\"日\");", "g(CP932(\"日\"));", false);
    check("f(\"日\", 1);", "f(CP932(\"日\"), 2);", false);
    check("f(\"日\");", "f(CP932(\"本\"));", false);
    check("f(\"日\");", "f(CP932(\"\\x93\\xfa\"));", false);
    check("f(\"日\");", "f(MY_CP932(\"日\"));", false);
    check("f(\"日\");", "f(CP932_extra(\"日\"));", false);
    check("f(\"日\"_tag);", "f(CP932(\"日\"_tag));", false);
    check("f(u8\"日\");", "f(CP932(u8\"日\"));", false);
    check("f(L\"日\");", "f(CP932(L\"日\"));", false);
    check("// \"日\"\nf();", "// CP932(\"日\")\nf();", false);
    check("/* \"日\" */", "/* CP932(\"日\") */", false);
    check("// \\\n\"日\"\nf();", "// \\\nCP932(\"日\")\nf();", false);
    check("auto s = R\"tag(\"日\")tag\";", "auto s = R\"tag(CP932(\"日\"))tag\";", false);
    check("f(\"CP932(\\\"日\\\")\");", "f(\"CP932(\\\"日\\\")\");", true);
    check("int n = 1;\n", "#include \"compat/Other.hpp\"\nint n = 1;\n", false);
    check("int n = 1;\n", "#include \"resource/TextEncoding.hpp\"\n#include \"resource/TextEncoding.hpp\"\nint n = 1;\n", false);
    check("int n = 1;\n", "int n = 1;\n#include \"resource/TextEncoding.hpp\"\n", false);
    check("int n = 1;\n", "#include \"resource/TextEncoding.hpp\" // unexpected\nint n = 1;\n", false);
    check("int n = 1;\n", "#include \"resource/TextEncoding.hpp\"\n\nint n = 1;\n", false);
    check("f(\"日\"); // original", "f(CP932(\"日\")); // changed", false);
    check("f(\"日\");", "f(CP932(\"日\" /* unexpected */));", false);
    std::cout << "Passed " << cases << " strict CP932 source-boundary cases\n";
    if (argc == 3) {
        const auto original_root = std::filesystem::path(argv[1]);
        const auto native_root = std::filesystem::path(argv[2]);
        const auto read = [](const auto &path) {
            std::ifstream file(path, std::ios::binary);
            if (!file)
                throw std::runtime_error("Cannot read source " + path.string());
            return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        };
        std::size_t files = 0;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(original_root)) {
            if (!entry.is_regular_file())
                continue;
            const auto relative = entry.path().lexically_relative(original_root);
            if (!source_matches_with_cp932(read(entry.path()), read(native_root / relative))) {
                throw std::runtime_error("Encoding migration changed other source bytes: " + relative.string());
            }
            ++files;
        }
        if (!files)
            throw std::runtime_error("Source snapshot must not be empty");
        std::cout << "Recovered all " << files << " snapshot files byte-for-byte\n";
    } else if (argc != 1) {
        throw std::runtime_error("Usage: source-mirror-encoding-tests [original-directory native-directory]");
    }
}
