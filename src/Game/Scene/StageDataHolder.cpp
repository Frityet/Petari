#include "Game/Scene/StageDataHolder.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/PlacementInfoOrdered.hpp"
#include "Game/Scene/StageResourceLoader.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include <JSystem/JKernel/JKRFileFinder.hpp>
#include <JSystem/JKernel/JKRMemArchive.hpp>
#include <cstdio>

namespace {
    static const char* cLayerDirName[] = {
        "Common", "LayerA", "LayerB", "LayerC", "LayerD", "LayerE", "LayerF", "LayerG", "LayerH",
        "LayerI", "LayerJ", "LayerK", "LayerL", "LayerM", "LayerN", "LayerO", "LayerP",
    };

    static bool isPrioPlacementObjInfo(const char* pName) NO_INLINE {
        return MR::isEqualStringCase(pName, "AreaObjInfo") || MR::isEqualStringCase(pName, "PlanetObjInfo") ||
               MR::isEqualStringCase(pName, "DemoObjInfo") || MR::isEqualStringCase(pName, "CameraCubeInfo");
    }

    static void calcPlacementInfoNum(int* a1, int* a2, const MR::AssignableArray< JMapInfo >& rArray) NO_INLINE {
        *a1 = 0;
        *a2 = 0;

        for (const JMapInfo* pInfo = rArray.begin(); pInfo != rArray.end(); pInfo++) {
            if (::isPrioPlacementObjInfo(pInfo->getName())) {
                int size;

                if (pInfo->mData != nullptr) {
                    size = pInfo->mData->mNumEntries;
                } else {
                    size = 0;
                }

                *a1 += size;
            } else {
                int size;

                if (pInfo->mData != nullptr) {
                    size = pInfo->mData->mNumEntries;
                } else {
                    size = 0;
                }

                *a2 += size;
            }
        }
    }

    static void attachJmpInfoToPlacementInfoOrdered(PlacementInfoOrdered* a1, PlacementInfoOrdered* a2, PlacementInfoOrdered* a3,
                                                    const MR::AssignableArray< JMapInfo >& rArray) NO_INLINE {
        for (const JMapInfo* pInfo = rArray.begin(); pInfo != rArray.end(); pInfo++) {
            if (::isPrioPlacementObjInfo(pInfo->getName())) {
                a1->attach(pInfo, nullptr);
            } else {
                a2->attach(pInfo, a3);
            }
        }
    }

    static s32 getJMapInfoElementNum(const JMapInfo* pInfo) {
        if (pInfo != nullptr && pInfo->mData != nullptr) {
            return pInfo->mData->mNumEntries;
        }

        return 0;
    }

    static s32 countJmpFiles(JKRArchive* pArchive, const char* pDir) {
        s32 count = pArchive->countFile(pDir) - 2;
        return count > 0 ? count : 0;
    }

    static JMapInfoIter makeInvalidJMapInfoIter() {
        JMapInfoIter iter;
        iter.mInfo = nullptr;
        iter.mIndex = -1;
        return iter;
    }
};  // namespace

StageDataHolder::StageDataHolder(const char* pStageName, int zoneID, bool isScenarioRoot) : NameObj("StageDataHolder") {
    mPlacementObjs.mArr = nullptr;
    mPlacementObjs.mMaxSize = 0;
    mStartObjs.mArr = nullptr;
    mStartObjs.mMaxSize = 0;
    mGeneralPosObjs.mArr = nullptr;
    mGeneralPosObjs.mMaxSize = 0;
    mChildObjs.mArr = nullptr;
    mChildObjs.mMaxSize = 0;
    mListObjs.mArr = nullptr;
    mListObjs.mMaxSize = 0;
    mPathObjs.mArr = nullptr;
    mPathObjs.mMaxSize = 0;
    mObjNameTbl = nullptr;
    mStageDataHolderCount = 0;
    mArchive = MR::getStageArchive(pStageName);
    MR::zeroMemory(mStageDataArray, sizeof(mStageDataArray));
    _A8 = pStageName;
    PSMTXIdentity(mPlacementMtx);
    mZoneID = zoneID;
    _E0 = isScenarioRoot;
    _E4 = 0;
    _E8 = 0;
    _EC.mArr = nullptr;
    _EC.mMaxSize = 0;
    _F4.mArr = nullptr;
    _F4.mMaxSize = 0;
    _FC = nullptr;
    _100 = nullptr;
    _104 = nullptr;
    _108 = nullptr;
    _10C = nullptr;
}

void StageDataHolder::init(const JMapInfoIter& rIter) {
    if (!mZoneID) {
        initTableData();
    }

    if (_E0) {
        u32 commonLayer = ScenarioDataFunction::getCurrentCommonLayers(_A8);
        initLayerJmpInfo(&_EC, "/jmp/Placement", "/jmp/MapParts", commonLayer);
    }

    createLocalStageDataHolder(_EC, 1);

    if (!mZoneID) {
        initPlacementInfoOrderedCommon();
    }
}

void StageDataHolder::initAfterScenarioSelected() {
    s32 curScenarioNo = MR::getCurrentScenarioNo();
    u32 curLayers = ScenarioDataFunction::getCurrentScenarioLayers(_A8, curScenarioNo);

    if (!_E0) {
        curLayers |= ScenarioDataFunction::getCurrentCommonLayers(_A8);
    }

    initLayerJmpInfo(&_F4, "/jmp/Placement", "/jmp/MapParts", curLayers);
    createLocalStageDataHolder(_F4, false);

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        mStageDataArray[i]->initAfterScenarioSelected();
    }

    if (!mZoneID) {
        initPlacementInfoOrderedScenario();
    }

    initAllLayerJmpInfo(&mPlacementObjs, "/jmp/Placement", "/jmp/MapParts");
    initAllLayerJmpInfo(&mStartObjs, "/jmp/Start");

    s32 generalPosFileCount = mArchive->countFile("/jmp/GeneralPos");
    generalPosFileCount -= 2;
    s32 isValidGeneralPosCount = (generalPosFileCount > 0) ? generalPosFileCount : 0;

    if (isValidGeneralPosCount) {
        initAllLayerJmpInfo(&mGeneralPosObjs, "/jmp/GeneralPos");
    }

    s32 childObjFileCount = mArchive->countFile("/jmp/ChildObj");
    childObjFileCount -= 2;
    s32 isValidChildObjCount = (childObjFileCount > 0) ? childObjFileCount : 0;

    if (isValidChildObjCount) {
        initAllLayerJmpInfo(&mChildObjs, "/jmp/ChildObj");
    }

    initJmpInfo(&mListObjs, "/jmp/List");
    initJmpInfo(&mPathObjs, "/jmp/Path");
    calcDataAddress();
}

void StageDataHolder::requestFileLoadCommon() {
    if (!MR::tryRequestLoadStageResource()) {
        _FC->requestFileLoad();
        _100->requestFileLoad();
    }
}

void StageDataHolder::requestFileLoadScenario() {
    if (MR::isLoadStageScenarioResource()) {
        _104->requestFileLoad();
        _108->requestFileLoad();
        _10C->requestFileLoad();
    }
}

void StageDataHolder::initPlacement() {
    MR::setInitializeStatePlacementPlayer();
    initPlacementMario();
    MR::setInitializeStatePlacementHighPriority();
    _FC->initPlacement();
    _104->initPlacement();
    MR::setInitializeStatePlacement();
    _100->initPlacement();
    _108->initPlacement();
    _10C->initPlacement();
}

JMapInfo StageDataHolder::getCommonPathPointInfo(const JMapInfo** ppOut, int idx) const {
    const JMapInfo* pInfo = findJmpInfoFromArray(&mPathObjs, "CommonPathInfo");
    JMapInfoIter pathIter = pInfo->findElement< s32 >("l_id", idx, 0);
    return getCommonPathPointInfoFromRailDataIndex(ppOut, pathIter.mIndex);
}

JMapInfo StageDataHolder::getCommonPathPointInfoFromRailDataIndex(const JMapInfo** ppInfo, int idx) const {
    const JMapInfo* pInfo = findJmpInfoFromArray(&mPathObjs, "CommonPathInfo");
    char buf[128];
    snprintf(buf, sizeof(buf), "CommonPathPointInfo.%d", idx);
    *ppInfo = findJmpInfoFromArray(&mPathObjs, buf);
    return *pInfo;
}

s32 StageDataHolder::getCommonPathInfoElementNum() const {
    return ::getJMapInfoElementNum(findJmpInfoFromArray(&mPathObjs, "CommonPathInfo"));
}

s32 StageDataHolder::getStartPosNum() const {
    s32 num = 0;

    for (const JMapInfo* pInfo = mStartObjs.begin(); pInfo != mStartObjs.end(); pInfo++) {
        num += ::getJMapInfoElementNum(pInfo);
    }

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        num += mStageDataArray[i]->getStartPosNum();
    }

    return num;
}

s32 StageDataHolder::getCurrentStartZoneId() const {
    JMapInfoIter marioIter = makeCurrentMarioJMapInfoIter();
    const StageDataHolder* pHolder = findPlacedStageDataHolder(marioIter);
    return pHolder->mZoneID;
}

s32 StageDataHolder::getCurrentStartCameraId() const {
    JMapInfoIter marioIter = makeCurrentMarioJMapInfoIter();
    s32 cameraID;
    bool ret = marioIter.getValue< s32 >("Camera_id", &cameraID);

    if (ret) {
        return cameraID;
    }

    return -1;
}

void StageDataHolder::getStartCameraIdInfoFromStartDataIndex(JMapIdInfo* pInfo, int startDataIdx) const {
    JMapInfoIter startIter = getStartJMapInfoIterFromStartDataIndex(startDataIdx);
    s32 cameraID;
    startIter.getValue< s32 >("Camera_id", &cameraID);
    pInfo->initialize(cameraID, startIter);
}

s32 StageDataHolder::getGeneralPosNum() const {
    s32 num = 0;

    for (const JMapInfo* pInfo = mGeneralPosObjs.begin(); pInfo != mGeneralPosObjs.end(); pInfo++) {
        num += ::getJMapInfoElementNum(pInfo);
    }

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        num += mStageDataArray[i]->getGeneralPosNum();
    }

    return num;
}

JMapInfoIter StageDataHolder::getGeneralPosInfoFromDataIndex(int idx_) const {
    int idx = idx_;

    for (const JMapInfo* pInfo = mGeneralPosObjs.begin(); pInfo != mGeneralPosObjs.end(); pInfo++) {
        s32 curIdx = ::getJMapInfoElementNum(pInfo);

        if (idx < curIdx) {
            return JMapInfoIter(pInfo, idx);
        }

        idx -= curIdx;
    }

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        StageDataHolder* pHolder = mStageDataArray[i];
        s32 posNum = pHolder->getGeneralPosNum();

        if (idx < posNum) {
            return pHolder->getGeneralPosInfoFromDataIndex(idx);
        }

        idx -= posNum;
    }

    return ::makeInvalidJMapInfoIter();
}

s32 StageDataHolder::getChildObjNum(const JMapInfoIter& rIter) const {
    s32 parentID = -1;
    MR::getJMapInfoLinkID(rIter, &parentID);

    s32 num = 0;
    for (const JMapInfo* pInfo = mChildObjs.begin(); pInfo != mChildObjs.end(); pInfo++) {
        for (s32 i = 0; i < ::getJMapInfoElementNum(pInfo); i++) {
            s32 curParentID = -1;
            pInfo->getValue< s32 >(i, "ParentID", &curParentID);

            if (curParentID == parentID) {
                num++;
            }
        }
    }

    return num;
}

JMapInfoIter StageDataHolder::getChildObjInfoFromDataIndex(const JMapInfoIter& rIter, int idx) const {
    s32 parentID = -1;
    MR::getJMapInfoLinkID(rIter, &parentID);

    s32 curCount = 0;
    for (const JMapInfo* pInfo = mChildObjs.begin(); pInfo != mChildObjs.end(); pInfo++) {
        for (s32 i = 0; i < ::getJMapInfoElementNum(pInfo); i++) {
            s32 curParentID = -1;
            pInfo->getValue< s32 >(i, "ParentID", &curParentID);

            if (curParentID == parentID) {
                if (curCount == idx) {
                    return JMapInfoIter(pInfo, i);
                }

                curCount++;
            }
        }
    }

    return ::makeInvalidJMapInfoIter();
}

const StageDataHolder* StageDataHolder::findPlacedStageDataHolder(const JMapInfoIter& rIter) const {
    s32 data = (s32)rIter.mInfo->mData + rIter.mInfo->mData->mDataOffset + rIter.mInfo->mData->mEntrySize * rIter.mIndex;

    if (_E4 <= data && data < _E8) {
        return this;
    }

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        const StageDataHolder* pHolder = mStageDataArray[i]->findPlacedStageDataHolder(rIter);

        if (pHolder != nullptr) {
            return pHolder;
        }
    }

    return nullptr;
}

const StageDataHolder* StageDataHolder::getStageDataHolderFromZoneId(int zoneID) const {
    if (zoneID == 0) {
        return this;
    }

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        StageDataHolder* pHolder = mStageDataArray[i];
        s32 curZoneID = pHolder->mZoneID;

        if (zoneID == curZoneID) {
            return pHolder;
        }
    }

    return nullptr;
}

const StageDataHolder* StageDataHolder::getStageDataHolderFromZoneId(int zoneID) {
    return static_cast< const StageDataHolder* >(this)->getStageDataHolderFromZoneId(zoneID);
}

bool StageDataHolder::isPlacedZone(int zoneID) const {
    if (!zoneID) {
        return true;
    }

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        if (zoneID == mStageDataArray[i]->mZoneID) {
            return true;
        }
    }

    return false;
}

const char* StageDataHolder::getJapaneseObjectName(const char* pName) const {
    const JMapInfoIter englishName = mObjNameTbl->findElement< const char* >("en_name", pName, 0);

    if (englishName == mObjNameTbl->end()) {
        return nullptr;
    }

    const char* japaneseName;
    englishName.getValue< const char* >("jp_name", &japaneseName);
    return japaneseName;
}

void* StageDataHolder::getStageArchiveResource(const char* pName) {
    return mArchive->getResource('????', pName);
}

s32 StageDataHolder::getStageArchiveResourceSize(void* pData) {
    return mArchive->getResSize(pData);
}

void StageDataHolder::initPlacementMario() {
    JMapInfoIter iter = makeCurrentMarioJMapInfoIter();
    MR::setCurrentPlacementZoneId(MR::getPlacedZoneId(iter));
    const char* objName = "";
    MR::getObjectName(&objName, iter);
    CreationFuncPtr funcPtr = NameObjFactory::getCreator(objName);

    NameObj* obj = funcPtr("マリオアクター");
    obj->init(iter);
    MR::clearCurrentPlacementZoneId();
}

void StageDataHolder::initTableData() {
    void* tableFile = MR::receiveArchive("/StageData/ObjTableTable.arc")->getResource('????', "ObjNameTable.tbl");

    mObjNameTbl = new JMapInfo();
    mObjNameTbl->attach(tableFile);
}

JMapInfoIter StageDataHolder::makeMarioJMapInfoIter(const JMapIdInfo& rInfo) const {
    const StageDataHolder* pHolder = getStageDataHolderFromZoneId(rInfo.mZoneID);
    if (pHolder == nullptr) {
        return ::makeInvalidJMapInfoIter();
    }

    for (const JMapInfo* pInfo = pHolder->mStartObjs.begin(); pInfo != pHolder->mStartObjs.end(); pInfo++) {
        JMapInfoIter iter = pInfo->findElement< s32 >("MarioNo", rInfo._0, 0);

        if (iter != pInfo->end()) {
            return iter;
        }
    }

    return ::makeInvalidJMapInfoIter();
}

JMapInfoIter StageDataHolder::makeCurrentMarioJMapInfoIter() const {
    JMapInfoIter iter = makeMarioJMapInfoIter(MR::getCurrentMarioStartIdInfo());

    if (iter.isValid()) {
        return iter;
    }

    return ::makeInvalidJMapInfoIter();
}

void StageDataHolder::initPlacementInfoOrderedCommon() {
    int v12, v11;
    ::calcPlacementInfoNum(&v12, &v11, _EC);

    s32 v2 = 0;
    s32 v3 = 0;

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        int v10, v9;
        ::calcPlacementInfoNum(&v10, &v9, mStageDataArray[i]->_EC);
        v12 += v10;
        v11 += v9;
    }

    _FC = new PlacementInfoOrdered(v12);
    _100 = new PlacementInfoOrdered(v11);
    _10C = new PlacementInfoOrdered(0x20);
    ::attachJmpInfoToPlacementInfoOrdered(_FC, _100, _10C, _EC);

    s32 v7 = 0;
    s32 v8 = 0;

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        ::attachJmpInfoToPlacementInfoOrdered(_FC, _100, _10C, mStageDataArray[i]->_EC);
    }

    _FC->sort();
    _100->sort();
}

void StageDataHolder::initPlacementInfoOrderedScenario() {
    int prioNum, normalNum;
    ::calcPlacementInfoNum(&prioNum, &normalNum, _F4);

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        int curPrioNum, curNormalNum;
        ::calcPlacementInfoNum(&curPrioNum, &curNormalNum, mStageDataArray[i]->_F4);
        prioNum += curPrioNum;
        normalNum += curNormalNum;
    }

    _104 = new PlacementInfoOrdered(prioNum);
    _108 = new PlacementInfoOrdered(normalNum);
    ::attachJmpInfoToPlacementInfoOrdered(_104, _108, nullptr, _F4);

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        ::attachJmpInfoToPlacementInfoOrdered(_104, _108, nullptr, mStageDataArray[i]->_F4);
    }

    _104->sort();
    _108->sort();
    _10C->sort();
}

void StageDataHolder::initJmpInfo(MR::AssignableArray< JMapInfo >* pArray, const char* pDir) {
    s32 count = ::countJmpFiles(mArchive, pDir);

    if (count) {
        pArray->init(count);
        attachJmpInfoToArray(pArray->mArr, pDir);
    }
}

void StageDataHolder::initAllLayerJmpInfo(MR::AssignableArray< JMapInfo >* pArray, const char* pDir) {
    s32 curScenarioNo = MR::getCurrentScenarioNo();
    u32 layers = ScenarioDataFunction::getCurrentCommonLayers(_A8) |
                 ScenarioDataFunction::getCurrentScenarioLayers(_A8, curScenarioNo);
    s32 count = 0;
    char path[0x40];

    for (s32 i = 0; i < 17; i++) {
        if (layers & (1 << i)) {
            snprintf(path, sizeof(path), "%s/%s", pDir, ::cLayerDirName[i]);
            count += ::countJmpFiles(mArchive, path);
        }
    }

    if (count) {
        pArray->init(count);
        JMapInfo* pInfo = pArray->mArr;

        for (s32 i = 0; i < 17; i++) {
            if (layers & (1 << i)) {
                snprintf(path, sizeof(path), "%s/%s", pDir, ::cLayerDirName[i]);
                pInfo = attachJmpInfoToArray(pInfo, path);
            }
        }
    }
}

void StageDataHolder::initAllLayerJmpInfo(MR::AssignableArray< JMapInfo >* pArray, const char* pDir1, const char* pDir2) {
    s32 curScenarioNo = MR::getCurrentScenarioNo();
    u32 layers = ScenarioDataFunction::getCurrentCommonLayers(_A8) |
                 ScenarioDataFunction::getCurrentScenarioLayers(_A8, curScenarioNo);
    initLayerJmpInfo(pArray, pDir1, pDir2, layers);
}

void StageDataHolder::initLayerJmpInfo(MR::AssignableArray< JMapInfo >* pArray, const char* pDir1, const char* pDir2, u32 layers) {
    s32 count = 0;
    char path[0x40];

    for (s32 i = 0; i < 17; i++) {
        if (layers & (1 << i)) {
            snprintf(path, sizeof(path), "%s/%s", pDir1, ::cLayerDirName[i]);
            count += ::countJmpFiles(mArchive, path);
            snprintf(path, sizeof(path), "%s/%s", pDir2, ::cLayerDirName[i]);
            count += ::countJmpFiles(mArchive, path);
        }
    }

    if (count) {
        pArray->init(count);
        JMapInfo* pInfo = pArray->mArr;

        for (s32 i = 0; i < 17; i++) {
            if (layers & (1 << i)) {
                snprintf(path, sizeof(path), "%s/%s", pDir1, ::cLayerDirName[i]);
                pInfo = attachJmpInfoToArray(pInfo, path);
                snprintf(path, sizeof(path), "%s/%s", pDir2, ::cLayerDirName[i]);
                pInfo = attachJmpInfoToArray(pInfo, path);
            }
        }
    }
}

JMapInfo* StageDataHolder::attachJmpInfoToArray(JMapInfo* pInfo, const char* pDir) {
    s32 count = ::countJmpFiles(mArchive, pDir);
    if (!count) {
        return pInfo;
    }

    JKRArcFinder* pFinder = mArchive->getFirstFile(pDir);

    for (s32 i = 0; i < count; i++) {
        void* pResource = mArchive->getIdxResource(pFinder->mFileID);
        pInfo->attach(pResource);
        pInfo->setName(pFinder->mName);
        pFinder->findNextFile();
        pInfo++;
    }

    delete pFinder;
    return pInfo;
}

const JMapInfo* StageDataHolder::findJmpInfoFromArray(const MR::AssignableArray< JMapInfo >* pInfoArr, const char* pName) const {
    for (const JMapInfo* pInfo = pInfoArr->begin(); pInfo != pInfoArr->end(); pInfo++) {
        if (MR::isEqualStringCase(pInfo->getName(), pName)) {
            return pInfo;
        }
    }

    return nullptr;
}

JMapInfoIter StageDataHolder::getStartJMapInfoIterFromStartDataIndex(int idx_) const {
    int idx = idx_;

    for (JMapInfo* pInfo = mStartObjs.mArr; pInfo != mStartObjs.end(); pInfo++) {
        const JMapData* curData = pInfo->mData;
        bool isValid = curData;
        int curIdx = isValid ? curData->mNumEntries : 0;

        if (idx < curIdx) {
            JMapInfoIter iter;
            iter.mInfo = pInfo;
            iter.mIndex = idx;

            return iter;
        }

        curIdx = isValid ? curData->mNumEntries : 0;

        idx -= curIdx;
    }

    for (s32 i = 0; i < mStageDataHolderCount; i++) {
        StageDataHolder* pHolder = mStageDataArray[i];
        int startPosNum = pHolder->getStartPosNum();

        if (idx < startPosNum) {
            return pHolder->getStartJMapInfoIterFromStartDataIndex(idx);
        }

        idx -= startPosNum;
    }

    JMapInfoIter iter;
    iter.mInfo = nullptr;
    iter.mIndex = -1;

    return iter;
}

void StageDataHolder::calcDataAddress() {
    _E4 = -1;
    _E8 = 0;
    updateDataAddress(&mPlacementObjs);
    updateDataAddress(&mStartObjs);
    updateDataAddress(&mGeneralPosObjs);
    updateDataAddress(&mChildObjs);
    updateDataAddress(&mPathObjs);
}

void StageDataHolder::calcPlacementMtx(const JMapInfoIter& rIter) {
    TVec3f pos;
    rIter.getValue< f32 >("pos_x", &pos.x);
    rIter.getValue< f32 >("pos_y", &pos.y);
    rIter.getValue< f32 >("pos_z", &pos.z);

    TVec3f rot;
    rIter.getValue< f32 >("dir_x", &rot.x);
    rIter.getValue< f32 >("dir_y", &rot.y);
    rIter.getValue< f32 >("dir_z", &rot.z);

    MR::makeMtxTR(mPlacementMtx, pos, rot);
}

void StageDataHolder::updateDataAddress(const MR::AssignableArray< JMapInfo >* pInfoArray) {
    for (const JMapInfo* pInfo = pInfoArray->begin(); pInfo != pInfoArray->end(); pInfo++) {
        if ((u32)pInfo->mData < _E4) {
            _E4 = (u32)pInfo->mData;
        }

        u32 addr = (pInfo->mData->mEntrySize * pInfo->mData->mNumEntries) + ((s32)pInfo->mData + pInfo->mData->mDataOffset);

        if (_E8 < addr) {
            _E8 = addr;
        }
    }
}

void StageDataHolder::createLocalStageDataHolder(const MR::AssignableArray< JMapInfo >& rArray, bool isScenarioRoot) {
    for (const JMapInfo* pInfo = rArray.begin(); pInfo != rArray.end(); pInfo++) {
        if (MR::isEqualStringCase(pInfo->getName(), "StageObjInfo")) {
            for (s32 i = 0; i < ::getJMapInfoElementNum(pInfo); i++) {
                const char* pZoneName = nullptr;
                JMapInfoIter iter(pInfo, i);
                MR::getObjectName(&pZoneName, iter);

                s32 zoneID = MR::makeCurrentGalaxyStatusAccessor().getZoneId(pZoneName);
                StageDataHolder* pHolder = new StageDataHolder(pZoneName, zoneID, isScenarioRoot);
                mStageDataArray[mStageDataHolderCount] = pHolder;
                pHolder->initWithoutIter();
                pHolder->calcPlacementMtx(iter);
                mStageDataHolderCount++;
            }
        }
    }
}

namespace MR {
    StageDataHolder* getStageDataHolder() {
        return getSceneObj< StageDataHolder >(SceneObj_StageDataHolder);
    }
};  // namespace MR
