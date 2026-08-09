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
        SourcePair{"../include/Game/Animation/AnmPlayer.hpp", "src/Game/Animation/AnmPlayer.hpp"},
        SourcePair{"../include/Game/AudioLib/AudBgmSetting.hpp", "src/Game/AudioLib/AudBgmSetting.hpp"},
        SourcePair{"../src/Game/AudioLib/AudBgmSetting.cpp", "src/Game/AudioLib/AudBgmSetting.cpp"},
        SourcePair{"../include/Game/LiveActor/MaterialCtrl.hpp", "src/Game/LiveActor/MaterialCtrl.hpp"},
        SourcePair{"../src/Game/LiveActor/MaterialCtrl.cpp", "src/Game/LiveActor/MaterialCtrl.cpp"},
        SourcePair{"../include/Game/MapObj/InvisiblePolygonObj.hpp", "src/Game/MapObj/InvisiblePolygonObj.hpp"},
        SourcePair{"../src/Game/MapObj/InvisiblePolygonObj.cpp", "src/Game/MapObj/InvisiblePolygonObj.cpp"},
        SourcePair{"../include/Game/MapObj/InvisiblePolygonObjGCapture.hpp", "src/Game/MapObj/InvisiblePolygonObjGCapture.hpp"},
        SourcePair{"../src/Game/MapObj/InvisiblePolygonObjGCapture.cpp", "src/Game/MapObj/InvisiblePolygonObjGCapture.cpp"},
        SourcePair{"../include/Game/Map/FIleSelectItem.hpp", "src/Game/Map/FileSelectItem.hpp"},
        SourcePair{"../src/Game/Map/FileSelectItem.cpp", "src/Game/Map/FileSelectItem.cpp"},
        SourcePair{"../include/Game/Map/FileSelectEffect.hpp", "src/Game/Map/FileSelectEffect.hpp"},
        SourcePair{"../src/Game/Map/FileSelectEffect.cpp", "src/Game/Map/FileSelectEffect.cpp"},
        SourcePair{"../include/Game/Map/FileSelectFunc.hpp", "src/Game/Map/FileSelectFunc.hpp"},
        SourcePair{"../src/Game/Map/FileSelectFunc.cpp", "src/Game/Map/FileSelectFunc.cpp"},
        SourcePair{"../include/Game/Map/FileSelector.hpp", "src/Game/Map/FileSelector.hpp"},
        SourcePair{"../src/Game/Map/FileSelector.cpp", "src/Game/Map/FileSelector.cpp"},
        SourcePair{"../include/Game/Map/FileSelectSky.hpp", "src/Game/Map/FileSelectSky.hpp"},
        SourcePair{"../src/Game/Map/FileSelectSky.cpp", "src/Game/Map/FileSelectSky.cpp"},
        SourcePair{"../include/Game/Map/SphereSelector.hpp", "src/Game/Map/SphereSelector.hpp"},
        SourcePair{"../src/Game/Map/SphereSelector.cpp", "src/Game/Map/SphereSelector.cpp"},
        SourcePair{"../include/Game/Map/SphereSelectorHandle.hpp", "src/Game/Map/SphereSelectorHandle.hpp"},
        SourcePair{"../src/Game/Map/SphereSelectorHandle.cpp", "src/Game/Map/SphereSelectorHandle.cpp"},
        SourcePair{"../include/Game/NPC/MiiFaceParts.hpp", "src/Game/NPC/MiiFaceParts.hpp"},
        SourcePair{"../src/Game/NPC/MiiFaceParts.cpp", "src/Game/NPC/MiiFaceParts.cpp"},
        SourcePair{"../include/Game/NPC/MiiFaceRecipe.hpp", "src/Game/NPC/MiiFaceRecipe.hpp"},
        SourcePair{"../src/Game/NPC/MiiFaceRecipe.cpp", "src/Game/NPC/MiiFaceRecipe.cpp"},
        SourcePair{"../include/Game/NPC/MiiFacePartsHolder.hpp", "src/Game/NPC/MiiFacePartsHolder.hpp"},
        SourcePair{"../src/Game/NPC/MiiFacePartsHolder.cpp", "src/Game/NPC/MiiFacePartsHolder.cpp"},
        SourcePair{"../include/Game/Scene/PlacementStateChecker.hpp", "src/Game/Scene/PlacementStateChecker.hpp"},
        SourcePair{"../src/Game/Scene/PlacementStateChecker.cpp", "src/Game/Scene/PlacementStateChecker.cpp"},
        SourcePair{"../include/Game/Screen/FileSelectInfo.hpp", "src/Game/Screen/FileSelectInfo.hpp"},
        SourcePair{"../src/Game/Screen/FileSelectInfo.cpp", "src/Game/Screen/FileSelectInfo.cpp"},
        SourcePair{"../include/Game/Screen/CenterScreenBlur.hpp", "src/Game/Screen/CenterScreenBlur.hpp"},
        SourcePair{"../src/Game/Screen/CenterScreenBlur.cpp", "src/Game/Screen/CenterScreenBlur.cpp"},
        SourcePair{"../include/Game/Screen/FullScreenBlur.hpp", "src/Game/Screen/FullScreenBlur.hpp"},
        SourcePair{"../src/Game/Screen/FullScreenBlur.cpp", "src/Game/Screen/FullScreenBlur.cpp"},
        SourcePair{"../include/Game/Util/JointController.hpp", "src/Game/Util/JointController.hpp"},
        SourcePair{"../src/Game/Util/JointController.cpp", "src/Game/Util/JointController.cpp"},
        SourcePair{"../include/Game/Util/LiveActorUtil.hpp", "src/Game/Util/LiveActorUtil.hpp"},
        SourcePair{"../src/Game/Util/LiveActorUtil.cpp", "src/Game/Util/LiveActorUtil.cpp"},
        SourcePair{"../include/Game/Util/ModelUtil.hpp", "src/Game/Util/ModelUtil.hpp"},
        SourcePair{"../include/Game/Util/Color.hpp", "src/Game/Util/Color.hpp"},
        SourcePair{"../include/Game/Util/DirectDraw.hpp", "src/Game/Util/DirectDraw.hpp"},
        SourcePair{"../include/Game/Util/NerveUtil.hpp", "src/Game/Util/NerveUtil.hpp"},
        SourcePair{"../src/Game/Util/NerveUtil.cpp", "src/Game/Util/NerveUtil.cpp"},
        SourcePair{"../include/Game/Util/PlayerUtil.hpp", "src/Game/Util/PlayerUtil.hpp"},
        SourcePair{"../src/Game/Util/PlayerUtil.cpp", "src/Game/Util/PlayerUtil.cpp"},
        SourcePair{"../include/Game/Util/SequenceUtil.hpp", "src/Game/Util/SequenceUtil.hpp"},
        SourcePair{"../src/Game/Util/SequenceUtil.cpp", "src/Game/Util/SequenceUtil.cpp"},
        SourcePair{"../include/Game/Util/ScreenUtil.hpp", "src/Game/Util/ScreenUtil.hpp"},
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
