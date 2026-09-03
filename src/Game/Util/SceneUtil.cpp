#include "Game/Util/SceneUtil.hpp"
#include "Game/NameObj/NameObjFinder.hpp"
#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/ScenePlayingResult.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/StringUtil.hpp"
#include <cstdio>

namespace {
    void getRailInfoFromRailId(JMapInfoIter* pIter, const JMapInfo** ppPointInfo, const StageDataHolder* pHolder, int railId) NO_INLINE {
        *pIter = pHolder->getCommonPathPointInfo(ppPointInfo, railId);
    }

    ScenePlayingResult* getScenePlayingResult() {
        return MR::getSceneObj< ScenePlayingResult >(SceneObj_ScenePlayingResult);
    }
};  // namespace

namespace MR {
    s32 getCurrentScenarioNo() {
        return SingletonHolder< GameSystem >::get()->mSceneController->getCurrentScenarioNo();
    }

    s32 getCurrentSelectedScenarioNo() {
        return SingletonHolder< GameSystem >::get()->mSceneController->getCurrentSelectedScenarioNo();
    }

    void setCurrentScenarioNo(s32 scenarioNo, s32 selectedScenarioNo) {
        return SingletonHolder< GameSystem >::get()->mSceneController->setCurrentScenarioNo(scenarioNo, selectedScenarioNo);
    }

    bool isScenarioDecided() {
        return SingletonHolder< GameSystem >::get()->mSceneController->isScenarioDecided();
    }

    const char* getCurrentStageName() {
        return SingletonHolder< GameSystem >::get()->mSceneController->mCurrSceneControlInfo.mStage;
    }

    bool isEqualSceneName(const char* pSceneName) {
        return isEqualStringCase(SingletonHolder< GameSystem >::get()->mSceneController->mCurrSceneControlInfo.mScene, pSceneName);
    }

    bool isEqualStageName(const char* pStageName) {
        const char* pStage = SingletonHolder< GameSystem >::get()->mSceneController->mCurrSceneControlInfo.mStage;

        if (pStage == nullptr) {
            return false;
        }

        return isEqualStringCase(pStage, pStageName);
    }

    bool isStageBeginPrologueEvent() {
        return isEqualStageName("PeachCastleGardenGalaxy");
    }

    bool isStageBeginFadeWipe() {
        return isEqualStageName("HeavensDoorGalaxy");
    }

    bool isStageBeginTitleWipe() {
        return isStageFileSelect();
    }

    bool isStageBeginWithoutWipe() {
        return isStageEpilogueDemo();
    }

    bool isStageDisablePauseMenu() {
        return isStageFileSelect() || isStageEpilogueDemo();
    }

    bool isStageAstroLocation() {
        return isEqualStageName("AstroGalaxy") || isEqualStageName("AstroDome") || isEqualStageName("LibraryRoom");
    }

    bool isStageSwimAngleLimit() {
        return isEqualStageName("OceanRingGalaxy");
    }

    bool isStageStarPieceFollowGroupLimit() {
        return isEqualStageName("EggStarGalaxy") && getCurrentScenarioNo() == 2;
    }

    bool isStageFileSelect() {
        return isEqualStageName("FileSelect");
    }

    bool isStageKoopaVs() {
        return isStageKoopaVs1() || isStageKoopaVs2() || isStageKoopaVs3();
    }

    bool isStageKoopaVs1() {
        return isEqualStageName("KoopaBattleVs3Galaxy");
    }

    bool isStageKoopaVs2() {
        return isEqualStageName("KoopaBattleVs2Galaxy");
    }

    bool isStageKoopaVs3() {
        return isEqualStageName("KoopaBattleVs3Galaxy");
    }

    bool isStageEpilogueDemo() {
        return isEqualStageName("EpilogueDemoStage");
    }

    bool isBeginScenarioStarter() {
        if (hasRetryGalaxySequence()) {
            return false;
        }

        return NameObjFinder::find("シナリオスターター");
    }

    bool isStageSuddenDeathDodoryu() {
        return isEqualStageName("CosmosGardenGalaxy") && getCurrentScenarioNo() == 4;
    }

    void setInitializeStatePlacementPlayer() {
        SingletonHolder< GameSystem >::get()->mSceneController->setSceneInitializeState(SceneInitializeState_PlacementPlayer);
    }

    void setInitializeStatePlacementHighPriority() {
        SingletonHolder< GameSystem >::get()->mSceneController->setSceneInitializeState(SceneInitializeState_PlacementHighPriority);
    }

    void setInitializeStatePlacement() {
        SingletonHolder< GameSystem >::get()->mSceneController->setSceneInitializeState(SceneInitializeState_Placement);
    }

    void setInitializeStateAfterPlacement() {
        SingletonHolder< GameSystem >::get()->mSceneController->setSceneInitializeState(SceneInitializeState_AfterPlacement);
    }

    bool isInitializeStateEnd() {
        return SingletonHolder< GameSystem >::get()->mSceneController->isSceneInitializeState(SceneInitializeState_End);
    }

    // isInitializeStatePlacementSomething
    // stopSceneForScenarioOpeningCamera
    // playSceneForScenarioOpeningCamera
    const JMapIdInfo& getCurrentMarioStartIdInfo() {
        return *SingletonHolder< GameSystem >::get()->mSceneController->mCurrSceneControlInfo.mStartIdInfo;
    }

    s32 getStartPosNum() {
        return getStageDataHolder()->getStartPosNum();
    }

    s32 getCurrentStartZoneId() {
        return getStageDataHolder()->getCurrentStartZoneId();
    }

    // getInitializeStartIdInfo
    // getStageArchive
    // getGeneralPosNum
    // getGeneralPosData
    // getChildObjNum
    // getChildObjName
    // initChildObj

    const char* getAppearPowerStarObjName(s32 scenarioNo) {
        return makeCurrentGalaxyStatusAccessor().getAppearPowerStarObjName(scenarioNo);
    }

    s32 getCurrentStageNormalScenarioNum() {
        return makeCurrentGalaxyStatusAccessor().getNormalScenarioNum();
    }

    s32 getCurrentStagePowerStarNum() {
        return makeCurrentGalaxyStatusAccessor().getPowerStarNum();
    }

    s32 getZoneNum() {
        return makeCurrentGalaxyStatusAccessor().getZoneNum();
    }

    const char* getZoneNameFromZoneId(s32 zoneId) {
        return makeCurrentGalaxyStatusAccessor().getZoneName(zoneId);
    }

    // getPlacedHiddenStarScenarioNo
    // getRailInfo
    // getNextLinkRailInfo
    s32 getCurrentStartCameraId() {
        return getStageDataHolder()->getCurrentStartCameraId();
    }

    void getStartCameraIdInfoFromStartDataIndex(JMapIdInfo* pInfo, int startDataIndex) {
        getStageDataHolder()->getStartCameraIdInfoFromStartDataIndex(pInfo, startDataIndex);
    }

    s32 getPlacedRailNum(s32 zoneId) {
        if (getStageDataHolder()->isPlacedZone(zoneId)) {
            return getStageDataHolder()->getStageDataHolderFromZoneId(zoneId)->getCommonPathInfoElementNum();
        }

        return 0;
    }

    void getCameraRailInfo(JMapInfoIter* pIter, const JMapInfo** ppPointInfo, s32 railId, s32 zoneId) {
        const StageDataHolder* pHolder = getStageDataHolder()->getStageDataHolderFromZoneId(zoneId);
        ::getRailInfoFromRailId(pIter, ppPointInfo, pHolder, railId);
    }

    bool getCameraRailInfoFromRailDataIndex(JMapInfoIter* pIter, const JMapInfo** ppPointInfo, int index, s32 zoneId) {
        const StageDataHolder* pHolder = getStageDataHolder()->getStageDataHolderFromZoneId(zoneId);
        *pIter = pHolder->getCommonPathPointInfoFromRailDataIndex(ppPointInfo, index);
        return isEqualRailUsage(*pIter, "Camera");
    }

    void getStageCameraData(void** ppData, s32* pSize, s32 zoneId) {
        if (!getStageDataHolder()->isPlacedZone(zoneId)) {
            *ppData = nullptr;
            *pSize = 0;
            return;
        }

        StageDataHolder* pHolder = getStageDataHolder()->getStageDataHolderFromZoneId(zoneId);
        *ppData = pHolder->getStageArchiveResource("CameraParam.bcam");
        *pSize = pHolder->getStageArchiveResourceSize(*ppData);
    }

    void getCurrentScenarioStartAnimCameraData(void** ppData, s32* pSize) {
        StageDataHolder* pHolder = getStageDataHolder();
        char filename[64];
        snprintf(filename, sizeof(filename), "StartScenario%d.canm", getCurrentScenarioNo());
        *ppData = pHolder->getStageArchiveResource(filename);
        if (*ppData != nullptr) {
            *pSize = pHolder->getStageArchiveResourceSize(*ppData);
        } else {
            *pSize = 0;
        }
    }

    void incCoin(int term) {
        ::getScenePlayingResult()->incCoin(term);
        incPlayerLife(term);
    }

    void incPurpleCoin() {
        ::getScenePlayingResult()->incPurpleCoin();
    }

    s32 getCoinNum() {
        return ::getScenePlayingResult()->getCoinNum();
    }

    s32 getPurpleCoinNum() {
        return ::getScenePlayingResult()->mPurpleCoinNum;
    }

    s32 getPowerStarNum() {
        return GameDataFunction::calcCurrentPowerStarNum();
    }

    // isPlacementLocalStage
    // getPlacedZoneId
    TPos3f* getZonePlacementMtx(const JMapInfoIter& rIter) {
        const StageDataHolder* holder = getStageDataHolder()->findPlacedStageDataHolder(rIter);
        return const_cast< TPos3f* >(&holder->mPlacementMtx);
    }

    TPos3f* getZonePlacementMtx(s32 zoneId) {
        const StageDataHolder* holder = getStageDataHolder()->getStageDataHolderFromZoneId(zoneId);
        return const_cast< TPos3f* >(&holder->mPlacementMtx);
    }

    // getJapaneseObjectName

    void setCurrentPlacementZoneId(s32 zoneId) {
        getPlacementStateChecker()->setCurrentPlacementZoneId(zoneId);
    }

    void clearCurrentPlacementZoneId() {
        getPlacementStateChecker()->clearCurrentPlacementZoneId();
    }

    s32 getCurrentPlacementZoneId() {
        return getPlacementStateChecker()->getCurrentPlacementZoneId();
    }

    const char* getCurrentPlacementZoneName() {
        return getZoneNameFromZoneId(getCurrentPlacementZoneId());
    }
};  // namespace MR
