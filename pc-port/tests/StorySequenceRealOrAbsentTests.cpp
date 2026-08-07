#include "Game/System/GalaxyMoveArgument.hpp"
#include "Game/System/StorySequenceExecutor.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "compat/StorySequencePlatformCompat.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

[[nodiscard]] std::string read_file(const std::filesystem::path& path) {
    auto stream = std::ifstream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Could not open source evidence: " + path.string());
    }
    auto buffer = std::ostringstream{};
    buffer << stream.rdbuf();
    return buffer.str();
}

[[nodiscard]] std::filesystem::path find_project_root() {
    for (auto path = std::filesystem::current_path(); !path.empty(); path = path.parent_path()) {
        if (std::filesystem::is_regular_file(path / "src/Game/System/StorySequenceExecutor.cpp") &&
            std::filesystem::is_regular_file(path / "../src/Game/System/StorySequenceExecutor.cpp")) {
            return path;
        }
        if (path == path.root_path()) {
            break;
        }
    }
    throw std::runtime_error("Could not locate the pc-port project root");
}

void test_initial_file_select_comes_from_retail_executor() {
    auto executor = StorySequenceExecutor{};
    const auto scene_state = smgpc::compat::story_sequence::SceneStateBinding("Game", "", 0);
    const auto start = JMapIdInfo(19, 4);
    auto move = GalaxyMoveArgument(7, nullptr, 9, &start);

    executor.moveGalaxy(&move, true);

    require(move.mStageName != nullptr && std::string_view(move.mStageName) == "FileSelect" && move.mScenarioNo == 1,
            "retail StorySequenceExecutor move type 7 must choose FileSelect scenario 1");
    require(move.mIDInfo._0 == 19 && move.mIDInfo.mZoneID == 4,
            "the retail FileSelect move must preserve an explicit start ID");
}

void test_after_loading_requires_retail_save_backing() {
    auto executor = StorySequenceExecutor{};
    const auto start = JMapIdInfo(7, 2);
    auto move = GalaxyMoveArgument(6, nullptr, 1, &start);
    auto unavailable = false;
    try {
        executor.overwriteGalaxyNameAfterLoading(&move);
    } catch (const std::logic_error&) {
        unavailable = true;
    }
    require(unavailable,
            "after-loading route selection must stop when the retail save sequence has no real backing");
}

void test_source_is_exact_and_scene_shims_are_absent() {
    const auto project = find_project_root();
    const auto port_executor = read_file(project / "src/Game/System/StorySequenceExecutor.cpp");
    const auto decomp_executor = read_file(project / "../src/Game/System/StorySequenceExecutor.cpp");
    require(port_executor == decomp_executor,
            "pc-port StorySequenceExecutor.cpp must remain byte-exact with the decompiled source");

    const auto boot = read_file(project / "src/scene/SequenceBootService.cpp");
    require(boot.find("ACTMES_AUTORUSH_BEGIN") == std::string::npos &&
                boot.find("sendMsgToAllLiveActor") == std::string::npos,
            "SequenceBootService must not broadcast a fabricated global auto-rush message");

    const auto transitions = read_file(project / "src/scene/SceneTransitionRequestService.cpp");
    require(transitions.find("\"PeachCastleGardenGalaxy\"") == std::string::npos &&
                transitions.find("\"PrologueDirector\"") == std::string::npos,
            "the scene service must not own a hardcoded after-save route");
    require(transitions.find("GalaxyMoveArgument(7") != std::string::npos &&
                transitions.find("GalaxyMoveArgument(6") != std::string::npos,
            "the scene service must delegate initial and after-load moves to the retail executor");
}

struct TestCase {
    std::string_view name;
    void (*run)();
};
}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"initial FileSelect comes from retail executor", test_initial_file_select_comes_from_retail_executor},
        TestCase{"after-loading requires retail save backing", test_after_loading_requires_retail_save_backing},
        TestCase{"exact source and absent scene shims", test_source_is_exact_and_scene_shims_are_absent},
    };

    auto failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " StorySequence real-or-absent test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " StorySequence real-or-absent test(s) passed\n";
    return 0;
}
