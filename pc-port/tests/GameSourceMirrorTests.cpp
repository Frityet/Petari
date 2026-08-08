#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    struct SourcePair {
        std::string_view decomp;
        std::string_view port;
    };

    constexpr auto cSourcePairs = std::array{
        SourcePair{"../include/Game/Screen/FileSelectInfo.hpp", "src/Game/Screen/FileSelectInfo.hpp"},
        SourcePair{"../src/Game/Screen/FileSelectInfo.cpp", "src/Game/Screen/FileSelectInfo.cpp"},
        SourcePair{"../include/Game/Util/JointController.hpp", "src/Game/Util/JointController.hpp"},
        SourcePair{"../src/Game/Util/JointController.cpp", "src/Game/Util/JointController.cpp"},
        SourcePair{"../include/Game/Util/PlayerUtil.hpp", "src/Game/Util/PlayerUtil.hpp"},
        SourcePair{"../src/Game/Util/PlayerUtil.cpp", "src/Game/Util/PlayerUtil.cpp"},
        SourcePair{"../include/Game/Util/SequenceUtil.hpp", "src/Game/Util/SequenceUtil.hpp"},
        SourcePair{"../src/Game/Util/SequenceUtil.cpp", "src/Game/Util/SequenceUtil.cpp"},
        SourcePair{"../include/Game/Util/SystemUtil.hpp", "src/Game/Util/SystemUtil.hpp"},
        SourcePair{"../src/Game/Util/SystemUtil.cpp", "src/Game/Util/SystemUtil.cpp"},
    };

    [[nodiscard]] std::string readFile(std::string_view path) {
        auto stream = std::ifstream(std::string(path), std::ios::binary);
        if (!stream.is_open()) {
            throw std::runtime_error(std::string("could not open source-boundary file: ") + std::string(path));
        }

        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }

}  // namespace

int main() {
    auto failures = 0;

    for (const auto &pair : cSourcePairs) {
        try {
            if (readFile(pair.decomp) != readFile(pair.port)) {
                throw std::runtime_error(std::string("PC Game mirror is not byte-identical: ") +
                                         std::string(pair.port));
            }

            std::cout << "[ok] " << pair.port << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << pair.port << ": " << error.what() << '\n';
        }
    }

    return failures == 0 ? 0 : 1;
}
