#pragma once

#include "compat/Types.hpp"

class LayoutActor;
class IconAButton;
struct ResTIMG;

namespace nw4r::lyt {
class TexMap;
}

struct J3DFrameCtrl {
    f32 mFrame {};
    f32 mEnd {};
    f32 mRate {1.0F};
};

namespace MR {

void startAnim(LayoutActor *pActor, const char *pAnimationName, u32 layer);
void invalidateParentAnim(LayoutActor *pActor);
[[nodiscard]] bool isAnimStopped(const LayoutActor *pActor, u32 layer);
void setAnimFrameAndStop(LayoutActor *pActor, f32 frame, u32 layer);
void setAnimFrame(LayoutActor *pActor, f32 frame, u32 layer);
void setAnimRate(LayoutActor *pActor, f32 rate, u32 layer);
[[nodiscard]] f32 getAnimFrame(const LayoutActor *pActor, u32 layer);
[[nodiscard]] f32 getAnimRate(const LayoutActor *pActor, u32 layer);
[[nodiscard]] f32 getAnimFrameMax(const LayoutActor *pActor, const char *pAnimationName);
[[nodiscard]] J3DFrameCtrl *getAnimCtrl(const LayoutActor *pActor, u32 layer);
[[nodiscard]] J3DFrameCtrl *getPaneAnimCtrl(const LayoutActor *pActor, const char *pPaneName, u32 slot);
void emitEffect(LayoutActor *pActor, const char *pEffectName);
void deleteEffectAll(LayoutActor *pActor);
void setLayoutScalePosAtPaneScaleTransIfExecCalcAnim(LayoutActor *pActor, const LayoutActor *pParent, const char *pPaneName);
void setLayoutPosAtPaneTrans(LayoutActor *pActor, const LayoutActor *pParent, const char *pPaneName);
void showLayout(LayoutActor *pActor);
void hideLayout(LayoutActor *pActor);
void showPane(LayoutActor *pActor, const char *pPaneName);
void hidePane(LayoutActor *pActor, const char *pPaneName);
void showPaneRecursive(LayoutActor *pActor, const char *pPaneName);
void hidePaneRecursive(LayoutActor *pActor, const char *pPaneName);
void setFollowPos(const TVec2f *pFollowPos, const LayoutActor *pActor, const char *pPaneName);
void copyPaneTrans(TVec2f *pOut, const LayoutActor *pActor, const char *pPaneName);
void setLayoutScalePosAtPaneScaleTrans(LayoutActor *pActor, const LayoutActor *pParent, const char *pPaneName);
void setTextBoxGameMessageRecursive(LayoutActor *pActor, const char *pPaneName, const char *pMessageName);
void setTextBoxLayoutMessageRecursive(LayoutActor *pActor, const char *pPaneName, const char *pMessageName);
void clearTextBoxMessageRecursive(LayoutActor *pActor, const char *pPaneName);
void setTextBoxNumberRecursive(LayoutActor *pActor, const char *pPaneName, s32 number);
void setTextBoxMessageRecursive(LayoutActor *pActor, const char *pPaneName, const wchar_t *pMessage);
void setTextBoxMessageRecursive(LayoutActor *pActor, const char *pPaneName, const u16 *pMessage);
void setTextBoxFormatRecursive(LayoutActor *pActor, const char *pPaneName, const wchar_t *pFormat, ...);
void setTextBoxArgNumberRecursive(LayoutActor *pActor, const char *pPaneName, s32 number, s32 index);
void setTextBoxArgStringRecursive(LayoutActor *pActor, const char *pPaneName, const wchar_t *pMessage, s32 index);
[[nodiscard]] const wchar_t *getGameMessageDirect(const char *pMessageName);
[[nodiscard]] bool isExistPaneCtrl(const LayoutActor *pActor, const char *pPaneName);
void createAndAddPaneCtrl(LayoutActor *pActor, const char *pPaneName, u32 slotCount);
void startPaneAnim(LayoutActor *pActor, const char *pPaneName, const char *pAnimName, u32 slot);
[[nodiscard]] bool isPaneAnimStopped(const LayoutActor *pActor, const char *pPaneName, u32 slot);
void setPaneAnimFrameAndStop(LayoutActor *pActor, const char *pPaneName, f32 frame, u32 slot);
void setPaneAnimFrame(LayoutActor *pActor, const char *pPaneName, f32 frame, u32 slot);
[[nodiscard]] f32 getPaneAnimFrame(const LayoutActor *pActor, const char *pPaneName, u32 slot);
void setPaneAnimRate(LayoutActor *pActor, const char *pPaneName, f32 rate, u32 slot);
[[nodiscard]] nw4r::lyt::TexMap *createLytTexMap(const char *pArchiveName, const char *pTextureName);
[[nodiscard]] nw4r::lyt::TexMap *createLytTexMap(ResTIMG *pImage);
[[nodiscard]] nw4r::lyt::TexMap *getLytTexMap(LayoutActor *pActor, const char *pPaneName, u8 slot);
void replacePaneTexture(LayoutActor *pActor, const char *pPaneName, const nw4r::lyt::TexMap *pTexMap, u8 slot);
IconAButton *createAndSetupIconAButton(LayoutActor *pParent, bool connectToScene, bool connectToPause);

[[nodiscard]] bool isDead(const LayoutActor *pActor);

}  // namespace MR
