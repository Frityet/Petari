#pragma once

#include "Game/Scene/SceneFunction.hpp"
#include "JSystem/JGeometry/TVec.hpp"

class LiveActor;
class LayoutActor;
class NameObj;
class JMapInfoIter;
class JMapInfo;
class ResourceHolder;
class StageSwitchCtrl;
struct ResTIMG;

namespace MR {
    bool isExistFileInArc(const ResourceHolder*, const char*, ...);
    JMapInfo* createCsvParser(const ResourceHolder*, const char*, ...);
    JMapInfo* tryCreateCsvParser(const LiveActor*, const char*, ...);
    JMapInfo* tryCreateCsvParser(const ResourceHolder*, const char*, ...);
    s32 getCsvDataElementNum(const JMapInfo*);
    void getCsvDataStr(const char**, const JMapInfo*, const char*, s32);
    void getCsvDataStrOrNULL(const char**, const JMapInfo*, const char*, s32);
    void getCsvDataS32(s32*, const JMapInfo*, const char*, s32);
    void getCsvDataU8(u8*, const JMapInfo*, const char*, s32);
    void getCsvDataF32(f32*, const JMapInfo*, const char*, s32);
    void getCsvDataBool(bool*, const JMapInfo*, const char*, s32);
    void getCsvDataVec(Vec*, const JMapInfo*, const char*, s32);

    const ResTIMG* loadTexFromArc(const char*, const char*);
    const ResTIMG* loadTexFromArc(const char*);
    void connectToSceneMapObjDecorationMovement(NameObj* pObj);
    class FunctorBase;

    bool isInWater(const TVec3f&);
    bool isInDeath(const TVec3f&);
    bool isInDarkMatter(const TVec3f&);
    void requestMovementOn(NameObj* pObj);
    void requestMovementOff(NameObj* pObj);
    void connectToSceneMapObj(LiveActor* pActor);
    void connectToScenePlanet(LiveActor* pActor);
    void connectToSceneAir(LiveActor* pActor);
    void connectToSceneSun(LiveActor* pActor);
    void connectToScene3DModelFor2D(LiveActor* pActor);
    void connectToSceneMapObjMovement(NameObj* pObj);
    void connectToSceneNoSilhouettedMapObj(LiveActor* pActor);
    void connectToSceneItemStrongLight(LiveActor* pActor);
    void connectToSceneAreaObj(NameObj* pObj);
    void connectToSceneNpc(LiveActor* pActor);
    void connectToSceneNpcMovement(NameObj* pObj);
    void connectToSceneLayout(LayoutActor* pLayout);
    void connectToSceneLayoutDecoration(LayoutActor* pLayout);
    void connectToSceneTalkLayout(LayoutActor* pLayout);
    void connectToSceneLayoutOnPause(LayoutActor* pLayout);
    void connectToSceneLayoutMovement(NameObj* pObj);
    bool isExistResourceInArc(const char* pArcName, const char* pResourceName);
    bool tryRumblePad(const void*, const char*, s32);
    bool tryRumblePadVeryStrongLong(const void*, s32);
    bool tryRumblePadVeryStrong(const void*, s32);
    bool tryRumblePadStrong(const void*, s32);
    bool tryRumblePadMiddle(const void*, s32);
    bool tryRumblePadWeak(const void*, s32);
    bool tryRumblePadVeryWeak(const void*, s32);
    bool tryRumbleDefaultHit(const void*, s32);
    void shakeCameraVeryStrong();
    void shakeCameraStrong();
    void shakeCameraNormalStrong();
    void shakeCameraNormal();
    void shakeCameraNormalWeak();
    void shakeCameraWeak();
    void shakeCameraVeryWeak();
    void declarePowerStarCoin100();
    void declareStarPiece(const NameObj* pObj, s32 num);
    void listenNameObjStageSwitchOnAppear(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnOffAppear(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnA(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnOffA(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnB(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&);
    void listenNameObjStageSwitchOffB(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnOffB(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&, const MR::FunctorBase&);
}  // namespace MR
