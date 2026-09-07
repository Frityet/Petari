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
        SourcePair{"decomp/include/Game/Animation/AnmPlayer.hpp", "src/Game/Animation/AnmPlayer.hpp"},
        SourcePair{"decomp/include/Game/AudioLib/AudBgmSetting.hpp", "src/Game/AudioLib/AudBgmSetting.hpp"},
        SourcePair{"decomp/src/Game/AudioLib/AudBgmSetting.cpp", "src/Game/AudioLib/AudBgmSetting.cpp"},
        SourcePair{"decomp/include/Game/Camera/Camera.hpp", "src/Game/Camera/Camera.hpp"},
        SourcePair{"decomp/src/Game/Camera/Camera.cpp", "src/Game/Camera/Camera.cpp"},
        SourcePair{"decomp/include/Game/Camera/CameraMan.hpp", "src/Game/Camera/CameraMan.hpp"},
        SourcePair{"decomp/src/Game/Camera/CameraMan.cpp", "src/Game/Camera/CameraMan.cpp"},
        SourcePair{"decomp/include/Game/Camera/CameraParallel.hpp", "src/Game/Camera/CameraParallel.hpp"},
        SourcePair{"decomp/src/Game/Camera/CameraParallel.cpp", "src/Game/Camera/CameraParallel.cpp"},
        SourcePair{"decomp/include/Game/Camera/CameraHeightArrange.hpp", "src/Game/Camera/CameraHeightArrange.hpp"},
        SourcePair{"decomp/src/Game/Camera/CameraHeightArrange.cpp", "src/Game/Camera/CameraHeightArrange.cpp"},
        SourcePair{"decomp/include/Game/Camera/CameraPoseParam.hpp", "src/Game/Camera/CameraPoseParam.hpp"},
        SourcePair{"decomp/src/Game/Camera/CameraPoseParam.cpp", "src/Game/Camera/CameraPoseParam.cpp"},
        SourcePair{"decomp/include/Game/Camera/CamTranslatorParallel.hpp", "src/Game/Camera/CamTranslatorParallel.hpp"},
        SourcePair{"decomp/src/Game/Camera/CamTranslatorParallel.cpp", "src/Game/Camera/CamTranslatorParallel.cpp"},
        SourcePair{"decomp/include/Game/Camera/CameraCalc.hpp", "src/Game/Camera/CameraCalc.hpp"},
        SourcePair{"decomp/src/Game/Camera/CameraCalc.cpp", "src/Game/Camera/CameraCalc.cpp"},
        SourcePair{"decomp/include/Game/LiveActor/MaterialCtrl.hpp", "src/Game/LiveActor/MaterialCtrl.hpp"},
        SourcePair{"decomp/src/Game/LiveActor/MaterialCtrl.cpp", "src/Game/LiveActor/MaterialCtrl.cpp"},
        SourcePair{"decomp/include/Game/LiveActor/ActorStateBase.hpp", "src/Game/LiveActor/ActorStateBase.hpp"},
        SourcePair{"decomp/src/Game/LiveActor/ActorStateBase.cpp", "src/Game/LiveActor/ActorStateBase.cpp"},
        SourcePair{"decomp/include/Game/LiveActor/ActorStateKeeper.hpp", "src/Game/LiveActor/ActorStateKeeper.hpp"},
        SourcePair{"decomp/src/Game/LiveActor/ActorStateKeeper.cpp", "src/Game/LiveActor/ActorStateKeeper.cpp"},
        SourcePair{"decomp/include/Game/LiveActor/Spine.hpp", "src/Game/LiveActor/Spine.hpp"},
        SourcePair{"decomp/src/Game/LiveActor/Spine.cpp", "src/Game/LiveActor/Spine.cpp"},
        SourcePair{"decomp/include/Game/MapObj/InvisiblePolygonObj.hpp", "src/Game/MapObj/InvisiblePolygonObj.hpp"},
        SourcePair{"decomp/src/Game/MapObj/InvisiblePolygonObj.cpp", "src/Game/MapObj/InvisiblePolygonObj.cpp"},
        SourcePair{"decomp/include/Game/MapObj/InvisiblePolygonObjGCapture.hpp", "src/Game/MapObj/InvisiblePolygonObjGCapture.hpp"},
        SourcePair{"decomp/src/Game/MapObj/InvisiblePolygonObjGCapture.cpp", "src/Game/MapObj/InvisiblePolygonObjGCapture.cpp"},
        SourcePair{"decomp/include/Game/System/NerveExecutor.hpp", "src/Game/System/NerveExecutor.hpp"},
        SourcePair{"decomp/src/Game/System/NerveExecutor.cpp", "src/Game/System/NerveExecutor.cpp"},
        SourcePair{"decomp/include/Game/System/StationedFileInfo.hpp", "src/Game/System/StationedFileInfo.hpp"},
        SourcePair{"decomp/src/Game/System/StationedFileInfo.cpp", "src/Game/System/StationedFileInfo.cpp"},
        SourcePair{"decomp/include/Game/Map/FIleSelectItem.hpp", "src/Game/Map/FileSelectItem.hpp"},
        SourcePair{"decomp/src/Game/Map/FileSelectItem.cpp", "src/Game/Map/FileSelectItem.cpp"},
        SourcePair{"decomp/include/Game/Map/FileSelectEffect.hpp", "src/Game/Map/FileSelectEffect.hpp"},
        SourcePair{"decomp/src/Game/Map/FileSelectEffect.cpp", "src/Game/Map/FileSelectEffect.cpp"},
        SourcePair{"decomp/include/Game/Map/FileSelectFunc.hpp", "src/Game/Map/FileSelectFunc.hpp"},
        SourcePair{"decomp/src/Game/Map/FileSelectFunc.cpp", "src/Game/Map/FileSelectFunc.cpp"},
        SourcePair{"decomp/include/Game/Map/FileSelector.hpp", "src/Game/Map/FileSelector.hpp"},
        SourcePair{"decomp/src/Game/Map/FileSelector.cpp", "src/Game/Map/FileSelector.cpp"},
        SourcePair{"decomp/include/Game/Map/FileSelectSky.hpp", "src/Game/Map/FileSelectSky.hpp"},
        SourcePair{"decomp/src/Game/Map/FileSelectSky.cpp", "src/Game/Map/FileSelectSky.cpp"},
        SourcePair{"decomp/include/Game/Map/SphereSelector.hpp", "src/Game/Map/SphereSelector.hpp"},
        SourcePair{"decomp/src/Game/Map/SphereSelector.cpp", "src/Game/Map/SphereSelector.cpp"},
        SourcePair{"decomp/include/Game/Map/SphereSelectorHandle.hpp", "src/Game/Map/SphereSelectorHandle.hpp"},
        SourcePair{"decomp/src/Game/Map/SphereSelectorHandle.cpp", "src/Game/Map/SphereSelectorHandle.cpp"},
        SourcePair{"decomp/include/Game/NPC/MiiFaceParts.hpp", "src/Game/NPC/MiiFaceParts.hpp"},
        SourcePair{"decomp/src/Game/NPC/MiiFaceParts.cpp", "src/Game/NPC/MiiFaceParts.cpp"},
        SourcePair{"decomp/include/Game/NPC/MiiFaceRecipe.hpp", "src/Game/NPC/MiiFaceRecipe.hpp"},
        SourcePair{"decomp/src/Game/NPC/MiiFaceRecipe.cpp", "src/Game/NPC/MiiFaceRecipe.cpp"},
        SourcePair{"decomp/include/Game/NPC/MiiFacePartsHolder.hpp", "src/Game/NPC/MiiFacePartsHolder.hpp"},
        SourcePair{"decomp/src/Game/NPC/MiiFacePartsHolder.cpp", "src/Game/NPC/MiiFacePartsHolder.cpp"},
        SourcePair{"decomp/include/Game/Player/MarioHolder.hpp", "src/Game/Player/MarioHolder.hpp"},
        SourcePair{"decomp/src/Game/Player/MarioHolder.cpp", "src/Game/Player/MarioHolder.cpp"},
        SourcePair{"decomp/include/Game/Scene/PlacementStateChecker.hpp", "src/Game/Scene/PlacementStateChecker.hpp"},
        SourcePair{"decomp/src/Game/Scene/PlacementStateChecker.cpp", "src/Game/Scene/PlacementStateChecker.cpp"},
        SourcePair{"decomp/include/Game/Screen/FileSelectInfo.hpp", "src/Game/Screen/FileSelectInfo.hpp"},
        SourcePair{"decomp/src/Game/Screen/FileSelectInfo.cpp", "src/Game/Screen/FileSelectInfo.cpp"},
        SourcePair{"decomp/include/Game/Screen/BackButton.hpp", "src/Game/Screen/BackButton.hpp"},
        SourcePair{"decomp/src/Game/Screen/BackButton.cpp", "src/Game/Screen/BackButton.cpp"},
        SourcePair{"decomp/include/Game/Screen/BrosButton.hpp", "src/Game/Screen/BrosButton.hpp"},
        SourcePair{"decomp/src/Game/Screen/BrosButton.cpp", "src/Game/Screen/BrosButton.cpp"},
        SourcePair{"decomp/include/Game/Screen/InformationMessage.hpp", "src/Game/Screen/InformationMessage.hpp"},
        SourcePair{"decomp/src/Game/Screen/InformationMessage.cpp", "src/Game/Screen/InformationMessage.cpp"},
        SourcePair{"decomp/include/Game/Screen/InformationObserver.hpp", "src/Game/Screen/InformationObserver.hpp"},
        SourcePair{"decomp/src/Game/Screen/InformationObserver.cpp", "src/Game/Screen/InformationObserver.cpp"},
        SourcePair{"decomp/include/Game/Screen/MiiConfirmIcon.hpp", "src/Game/Screen/MiiConfirmIcon.hpp"},
        SourcePair{"decomp/src/Game/Screen/MiiConfirmIcon.cpp", "src/Game/Screen/MiiConfirmIcon.cpp"},
        SourcePair{"decomp/include/Game/Screen/SysInfoWindow.hpp", "src/Game/Screen/SysInfoWindow.hpp"},
        SourcePair{"decomp/src/Game/Screen/SysInfoWindow.cpp", "src/Game/Screen/SysInfoWindow.cpp"},
        SourcePair{"decomp/include/Game/Screen/CenterScreenBlur.hpp", "src/Game/Screen/CenterScreenBlur.hpp"},
        SourcePair{"decomp/src/Game/Screen/CenterScreenBlur.cpp", "src/Game/Screen/CenterScreenBlur.cpp"},
        SourcePair{"decomp/include/Game/Screen/FullScreenBlur.hpp", "src/Game/Screen/FullScreenBlur.hpp"},
        SourcePair{"decomp/src/Game/Screen/FullScreenBlur.cpp", "src/Game/Screen/FullScreenBlur.cpp"},
        SourcePair{"decomp/include/Game/Util/JointController.hpp", "src/Game/Util/JointController.hpp"},
        SourcePair{"decomp/src/Game/Util/JointController.cpp", "src/Game/Util/JointController.cpp"},
        SourcePair{"decomp/include/Game/Util/LiveActorUtil.hpp", "src/Game/Util/LiveActorUtil.hpp"},
        SourcePair{"decomp/src/Game/Util/LiveActorUtil.cpp", "src/Game/Util/LiveActorUtil.cpp"},
        SourcePair{"decomp/include/Game/Util/ModelUtil.hpp", "src/Game/Util/ModelUtil.hpp"},
        SourcePair{"decomp/include/Game/Util/Color.hpp", "src/Game/Util/Color.hpp"},
        SourcePair{"decomp/include/Game/Util/DirectDraw.hpp", "src/Game/Util/DirectDraw.hpp"},
        SourcePair{"decomp/include/Game/Util/GamePadUtil.hpp", "src/Game/Util/GamePadUtil.hpp"},
        SourcePair{"decomp/src/Game/Util/GamePadUtil.cpp", "src/Game/Util/GamePadUtil.cpp"},
        SourcePair{"decomp/include/Game/Util/NerveUtil.hpp", "src/Game/Util/NerveUtil.hpp"},
        SourcePair{"decomp/src/Game/Util/NerveUtil.cpp", "src/Game/Util/NerveUtil.cpp"},
        SourcePair{"decomp/include/Game/Util/PlayerUtil.hpp", "src/Game/Util/PlayerUtil.hpp"},
        SourcePair{"decomp/src/Game/Util/PlayerUtil.cpp", "src/Game/Util/PlayerUtil.cpp"},
        SourcePair{"decomp/include/Game/Util/SequenceUtil.hpp", "src/Game/Util/SequenceUtil.hpp"},
        SourcePair{"decomp/src/Game/Util/SequenceUtil.cpp", "src/Game/Util/SequenceUtil.cpp"},
        SourcePair{"decomp/include/Game/Util/ScreenUtil.hpp", "src/Game/Util/ScreenUtil.hpp"},
        SourcePair{"decomp/include/Game/Util/SystemUtil.hpp", "src/Game/Util/SystemUtil.hpp"},
        SourcePair{"decomp/src/Game/Util/SystemUtil.cpp", "src/Game/Util/SystemUtil.cpp"},
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
