#include "Game/Screen/SceneWipeHolder.hpp"
#include "Game/Util/ScreenUtil.hpp"

namespace MR {
    void closeWipeCircle(s32 frame) {
        SceneWipeHolderFunction::closeWipe("円ワイプ", frame);
    }

    void forceOpenWipeCircle() {
        SceneWipeHolderFunction::forceOpenWipe("円ワイプ");
    }

    void forceCloseWipeCircle() {
        SceneWipeHolderFunction::forceCloseWipe("円ワイプ");
    }

    void closeWipeFade(s32 frame) {
        SceneWipeHolderFunction::closeWipe("フェードワイプ", frame);
    }

    void forceOpenWipeFade() {
        SceneWipeHolderFunction::forceOpenWipe("フェードワイプ");
    }

    void forceCloseWipeFade() {
        SceneWipeHolderFunction::forceCloseWipe("フェードワイプ");
    }

    void closeWipeWhiteFade(s32 frame) {
        SceneWipeHolderFunction::closeWipe("白フェードワイプ", frame);
    }

    void forceOpenWipeWhiteFade() {
        SceneWipeHolderFunction::forceOpenWipe("白フェードワイプ");
    }

    void forceCloseWipeWhiteFade() {
        SceneWipeHolderFunction::forceCloseWipe("白フェードワイプ");
    }

    bool isWipeActive() {
        return SceneWipeHolderFunction::getSceneWipeHolder()->isWipeIn() || SceneWipeHolderFunction::getSceneWipeHolder()->isWipeOut();
    }

    bool isWipeBlank() {
        return SceneWipeHolderFunction::getSceneWipeHolder()->isClose();
    }

    bool isWipeOpen() {
        return SceneWipeHolderFunction::getSceneWipeHolder()->isOpen();
    }

    void openWipeCircle(s32 frame) {
        SceneWipeHolderFunction::openWipe("円ワイプ", frame);
    }

    void openWipeFade(s32 frame) {
        SceneWipeHolderFunction::openWipe("フェードワイプ", frame);
    }

    void openWipeWhiteFade(s32 frame) {
        SceneWipeHolderFunction::openWipe("白フェードワイプ", frame);
    }

    void startGameOverWipe() {
        SceneWipeHolderFunction::getSceneWipeHolder()->wipe("ゲームオーバー", -1);
    }

    void startDownWipe() {
        SceneWipeHolderFunction::getSceneWipeHolder()->wipe("クッパ", -1);
    }

} // namespace MR
