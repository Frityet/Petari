#include "OriginalSaveDataSnapshot.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GalaxyMoveArgument.hpp"
#include "Game/System/StorySequenceExecutor.hpp"
#include "resource/TextEncoding.hpp"
#include <stdexcept>
#include <string_view>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void verify_story_sequence() {
    auto& sequence = smgpc::test::original_save_sequence();
    auto& file = *sequence.mCurrentUserFile;
    const smgpc::test::OriginalUserFileSnapshot restore_current(file);
    auto executor = StorySequenceExecutor{};
    auto independent = StorySequenceExecutor{};
    const auto* first = executor.addDynamicDemoSequenceInfo(6, 12, "first");
    const auto* second = executor.addDynamicDemoSequenceInfo(10, 17, "second");
    require(executor._6C.size() == 2 && independent._6C.size() == 0 &&
                first == &executor._6C[0] && second == &executor._6C[1] &&
                first->_0 == 6 && first->_2 == 12 && std::string_view(first->_4) == "first" &&
                second->_0 == 10 && second->_2 == 17 && std::string_view(second->_4) == "second",
            "original dynamic sequence records append in the executor's own stable fixed array");
    file.mGameDataHolder->resetAllData();
    file.mIsPlayerMario = true;
    const JMapIdInfo start(7, 2);
    auto move = GalaxyMoveArgument(6, nullptr, 1, &start);
    executor.overwriteGalaxyNameAfterLoading(&move);
    require(std::string_view(move.mStageName) == "PeachCastleGardenGalaxy" && move.mScenarioNo == 1 &&
                executor.getCurrentDemoInfo() && executor.getCurrentDemoInfo()->_0 == 8 &&
                std::string_view(executor.getCurrentDemoInfo()->_4) == "Prologue" && !executor.hasNextDemo(),
            "a fresh original Mario file selects the authored opening stage and prologue sequence");
    const auto event = smgpc::resource::encode_cp932("ピーチ城浮上後");
    GameDataFunction::followStoryEventByName(event.c_str());
    executor.overwriteGalaxyNameAfterLoading(&move);
    require(std::string_view(move.mStageName) == "HeavensDoorGalaxy" && move.mScenarioNo == 1,
            "after-loading stage selection reads the actual saved story milestone");
    file.mGameDataHolder->resetAllData();
    std::array<u8, 4096> luigi_data{};
    const auto luigi_size = file.mGameDataHolder->makeFileBinary(luigi_data.data(), luigi_data.size());
    file.loadFromGameDataBinary("luigi1", luigi_data.data(), luigi_size);
    independent.overwriteGalaxyNameAfterLoading(&move);
    require(std::string_view(move.mStageName) == "HeavensDoorGalaxy" && !independent.hasNextDemo(),
            "original Luigi save selection bypasses the Mario opening without invented story state");
}
}
int main() { return smgpc::test::run_stage_resource_process("story-sequence-owner", verify_story_sequence); }
