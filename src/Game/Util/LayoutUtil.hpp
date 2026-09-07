#pragma once

#include <JSystem/JGeometry/TBox.hpp>
class LayoutHolder;
class TextBoxRecursiveOperation;
namespace nw4r::lyt { class DrawInfo; }

#include <JSystem/JGeometry/TVec.hpp>
#include <revolution/types.h>

class SimpleLayout;
class LayoutActor;
class J3DFrameCtrl;
class IconAButton;
class Nerve;

namespace nw4r::lyt {
    class TexMap;
    class Pane;
}

namespace nw4r::ut {
    class Font;
}

namespace MR {
    nw4r::lyt::Pane* getPane(const LayoutActor*, const char*);
    bool isDead(const SimpleLayout* pLayout);
    bool isDead(const LayoutActor* pLayout);
    void startAnim(SimpleLayout* pLayout, const char* pAnimName, u32 animLayer);
    void startAnim(LayoutActor* pLayout, const char* pAnimName, u32 animLayer);
    bool isAnimStopped(const LayoutActor* pLayout, u32 animLayer);
    void setAnimFrameAndStop(SimpleLayout* pLayout, f32 frame, u32 animLayer);
    void setAnimFrameAndStop(LayoutActor* pLayout, f32 frame, u32 animLayer);
    void setAnimFrame(SimpleLayout* pLayout, f32 frame, u32 animLayer);
    void setAnimFrame(LayoutActor* pLayout, f32 frame, u32 animLayer);
    f32 getAnimFrame(SimpleLayout* pLayout, u32 animLayer);
    f32 getAnimFrame(LayoutActor* pLayout, u32 animLayer);
    J3DFrameCtrl* getAnimCtrl(LayoutActor* pLayout, u32 animLayer);
    void setAnimRate(SimpleLayout* pLayout, f32 rate, u32 animLayer);
    void setAnimRate(LayoutActor* pLayout, f32 rate, u32 animLayer);
    void stopAnim(LayoutActor* pLayout, u32 animLayer);
    nw4r::lyt::TexMap* createLytTexMap(const char* pArchiveName, const char* pTextureName);
    void replacePaneTexture(LayoutActor* pLayout, const char* pPaneName, const nw4r::lyt::TexMap* pTexMap, u8 texMapIndex);
    void startAnimAtFirstStep(LayoutActor* pLayout, const char* pAnimName, u32 animLayer);
    void setAnimFrameAndStopAdjustTextHeight(LayoutActor* pLayout, const char* pPaneName, u32 animLayer);
    void setTextBoxNumberRecursive(LayoutActor* pLayout, const char* pPaneName, s32 number);
    void setTextBoxGameMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const char* pMessageId);
    void setTextBoxLayoutMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const char* pMessageId);
    void setTextBoxSystemMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const char* pMessageId);
    void setTextBoxMessageRecursive(LayoutActor* pLayout, const char* pPaneName, const wchar_t* pMessage);
    void setTextBoxFontRecursive(LayoutActor* pLayout, const char* pPaneName, nw4r::ut::Font* pFont);
    void clearTextBoxMessageRecursive(LayoutActor* pLayout, const char* pPaneName);
    void setTextBoxArgNumberRecursive(LayoutActor* pLayout, const char* pPaneName, s32 number, s32 argIndex);
    void setTextBoxArgStringRecursive(LayoutActor* pLayout, const char* pPaneName, const wchar_t* pMessage, s32 argIndex);
    void setTextBoxHorizontalPositionCenterRecursive(LayoutActor* pLayout, const char* pPaneName);
    void setTextBoxHorizontalPositionLeftRecursive(LayoutActor* pLayout, const char* pPaneName);
    void setTextBoxVerticalPositionTopRecursive(LayoutActor* pLayout, const char* pPaneName);
    void setTextBoxVerticalPositionCenterRecursive(LayoutActor* pLayout, const char* pPaneName);
    void setTextBoxVerticalPositionBottomRecursive(LayoutActor* pLayout, const char* pPaneName);
    void createAndAddPaneCtrl(LayoutActor* pLayout, const char* pPaneName, u32 animLayerNum);
    bool isExistPaneCtrl(LayoutActor* pLayout, const char* pPaneName);
    void showPane(LayoutActor* pLayout, const char* pPaneName);
    void showPaneRecursive(LayoutActor* pLayout, const char* pPaneName);
    void hidePane(LayoutActor* pLayout, const char* pPaneName);
    void hidePaneRecursive(LayoutActor* pLayout, const char* pPaneName);
    void setPaneAlphaFloat(LayoutActor* pLayout, const char* pPaneName, f32 alpha);
    void showLayout(LayoutActor* pLayout);
    void hideLayout(LayoutActor* pLayout);
    void convertScreenPosToLayoutPos(TVec2f* pLayoutPos, const TVec2f& rScreenPos);
    void convertLayoutPosToScreenPos(TVec2f* pScreenPos, const TVec2f& rLayoutPos);
    void setFollowPos(const TVec2f* pPos, const LayoutActor* pLayout, const char* pPaneName);
    void copyPaneTrans(TVec2f* pPos, const LayoutActor* pLayout, const char* pPaneName);
    void copyPaneScale(TVec2f* pScale, const LayoutActor* pLayout, const char* pPaneName);
    void setLayoutPosAtPaneTrans(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName);
    void setLayoutScaleAtPaneScale(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName);
    void setLayoutScalePosAtPaneScaleTrans(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName);
    void setLayoutScalePosAtPaneScaleTransIfExecCalcAnim(LayoutActor* pDst, const LayoutActor* pSrc, const char* pPaneName);
    void startPaneAnim(LayoutActor* pLayout, const char* pPaneName, const char* pAnimName, u32 animLayer);
    void stopPaneAnim(LayoutActor* pLayout, const char* pPaneName, u32 animLayer);
    void setPaneAnimFrame(LayoutActor* pLayout, const char* pPaneName, f32 frame, u32 animLayer);
    void setPaneAnimFrameAndStop(LayoutActor* pLayout, const char* pPaneName, f32 frame, u32 animLayer);
    void setPaneAnimRate(LayoutActor* pLayout, const char* pPaneName, f32 rate, u32 animLayer);
    f32 getPaneAnimFrame(LayoutActor* pLayout, const char* pPaneName, u32 animLayer);
    s16 getPaneAnimFrameMax(const LayoutActor* pLayout, const char* pPaneName, u32 animLayer);
    bool isPaneAnimStopped(const LayoutActor* pLayout, const char* pPaneName, u32 animLayer);
    bool isFirstStep(const LayoutActor* pActor);
    bool isStep(const LayoutActor* pActor, s32 step);
    bool isLessStep(const LayoutActor* pActor, s32 step);
    bool isGreaterStep(const LayoutActor* pActor, s32 step);
    bool isGreaterEqualStep(const LayoutActor* pActor, s32 step);
    f32 calcNerveRate(const LayoutActor* pActor, s32 stepMax);
    f32 calcNerveRate(const LayoutActor* pActor, s32 stepMin, s32 stepMax);
    void setNerveAtStep(LayoutActor* pLayout, const Nerve* pNerve, s32 step);
    void setNerveAtPaneAnimStopped(LayoutActor* pLayout, const char* pPaneName, const Nerve* pNerve, u32 animLayer);
    void setNerveAtAnimStopped(LayoutActor* pLayout, const Nerve* pNerve, u32 animLayer);
    void killAtAnimStopped(LayoutActor* pLayout, u32 animLayer);
    s16 getAnimFrameMax(LayoutActor* pLayout, const char* pAnimName);
    s16 getAnimFrameMax(LayoutActor* pLayout, u32 animLayer);
    void startAnimReverseOneTime(LayoutActor* pLayout, const char* pAnimName, u32 animLayer);
    void invalidateParentAnim(LayoutActor* pLayout);
    IconAButton* createAndSetupIconAButton(LayoutActor* pActor, bool connectToScene, bool connectToPause);
    void emitEffect(SimpleLayout* pLayout, const char* pEffectName);
    void emitEffect(LayoutActor* pLayout, const char* pEffectName);
    void deleteEffectAll(SimpleLayout* pLayout);
    void deleteEffectAll(LayoutActor* pLayout);
}  // namespace MR

namespace MR {
    void setTextBoxFormatRecursive(LayoutActor*, const char*, const wchar_t*, ...);
    bool isHiddenPane(const LayoutActor*, const char*);
    bool isHiddenLayout(const LayoutActor*);
    void setFollowTypeAdd(const LayoutActor*, const char*);
    f32 getPaneTransX(const LayoutActor*, const char*);
    f32 getPaneTransY(const LayoutActor*, const char*);
    void startAnimAndSetFrameAndStop(LayoutActor*, const char*, f32, u32);
    bool isIntervalStep(const LayoutActor*, s32);
    f32 calcNerveEaseInRate(const LayoutActor*, s32);
    bool isExecuteCalcAnimLayout(const LayoutActor*);
    bool isExecuteDrawLayout(const LayoutActor*);
    void setFollowTypeReplace(const LayoutActor*, const char*);
}

namespace MR {
    u32 getTextLineNumMaxRecursive(const LayoutActor*, const char*);
    void clearTextBoxMessageRecursive(LayoutActor*, const char*);
}

namespace MR {
    LayoutHolder* createAndAddLayoutHolder(const char*);
    LayoutHolder* createAndAddLayoutHolderRawData(const char*);
    void createAndAddGroupCtrl(LayoutActor*, const char*, u32);
    u8 getPaneAlpha(const LayoutActor*, const char*);
    void setInfluencedAlphaToChild(const LayoutActor*);
    void setLayoutAlpha(const LayoutActor*, u8);
    void setLayoutAlphaFloat(const LayoutActor*, f32);
    void setPaneAlpha(const LayoutActor*, const char*, u8);
    void executeTextBoxRecursive(LayoutActor*, const char*, const TextBoxRecursiveOperation&);
    void setTextBoxArgGameMessageRecursive(LayoutActor*, const char*, const char*, s32);
    void updateClearTimeTextBox(LayoutActor*, const char*, u32);
    void updateMinuteAndSecondTextBox(LayoutActor*, const char*, u32);
    void showScreen(LayoutActor*);
    void hideScreen(LayoutActor*);
    void setPaneScale(const LayoutActor*, f32, f32, const char*);
    void copyPaneRotate(TVec3f*, const LayoutActor*, const char*);
    void setPaneRotate(const LayoutActor*, f32, f32, f32, const char*);
    nw4r::lyt::TexMap* getLytTexMap(LayoutActor*, const char*, u8);
    void startPaneAnimAtStep(LayoutActor*, const char*, const char*, s32, u32);
    void startPaneAnimAtFirstStep(LayoutActor*, const char*, const char*, u32);
    void startPaneAnimReverseOneTime(LayoutActor*, const char*, const char*, u32);
    void setAnimFrameAndStopAtEnd(LayoutActor*, u32);
    void setAnimFrameAndStopAdjustTextWidth(LayoutActor*, const char*, u32);
    void setPaneAnimFrameAndStopAtEnd(LayoutActor*, const char*, u32);
    J3DFrameCtrl* getPaneAnimCtrl(const LayoutActor*, const char*, u32);
    void deleteEffect(LayoutActor*, const char*);
    void forceDeleteEffect(LayoutActor*, const char*);
    void forceDeleteEffectAll(LayoutActor*);
    void setEffectHostMtx(LayoutActor*, const char*, MtxPtr);
    void setEffectRate(LayoutActor*, const char*, f32);
    void setEffectDirectionalSpeed(LayoutActor*, const char*, f32);
    void pauseOffEffectAll(LayoutActor*);
    bool isRegisteredEffect(const LayoutActor*, const char*);
    void copyLayoutDrawInfoWithAspect(nw4r::lyt::DrawInfo*, const LayoutActor*, bool);
    void calcAnimLayoutWithDrawInfo(const LayoutActor*, const nw4r::lyt::DrawInfo&);
    void drawLayoutWithDrawInfoWithoutProjectionSetup(const LayoutActor*, const nw4r::lyt::DrawInfo&);
    f32 calcNerveEaseInValue(const LayoutActor*, s32, s32, f32, f32);
    bool isStopAnimFrame(const LayoutActor*);
    void stopAnimFrame(LayoutActor*);
    void releaseAnimFrame(LayoutActor*);
    void onCalcAnim(LayoutActor*);
    void offCalcAnim(LayoutActor*);
    SimpleLayout* createSimpleLayout(const char*, const char*, u32);
    SimpleLayout* createSimpleLayoutTalkParts(const char*, const char*, u32);
    nw4r::lyt::Pane* getRootPane(const LayoutActor*);
    void calcTextBoxRectRecursive(TBox2f*, const LayoutActor*, const char*);
    void setCometPaneAnimFromId(LayoutActor*, const char*, int, u32);
    void setCometAnimFromId(LayoutActor*, int, u32);
}
