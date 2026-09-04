#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/AutoEffectGroupHolder.hpp"
#include "Game/Effect/AutoEffectInfo.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/MultiEmitter.hpp"
#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Scene/MultiSceneActor.hpp"
#include "Game/Scene/MultiSceneEffectKeeper.hpp"
#include "Game/Screen/PaneEffectKeeper.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Effect/ParticleEmitter.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include <cstdio>
#include <cstring>

namespace {
    void setupMultiEmitter(MultiEmitter* pEmitter, const AutoEffectInfo* pInfo) {
        pEmitter->_28 = pInfo;
        pEmitter->setDrawOrder(pInfo->mDrawOrder);
        pEmitter->setOffset(pInfo->mOffset);
        if (!MR::isNearZero(pInfo->mScaleValue - 1.0f)) {
            pEmitter->setBaseScale(pInfo->mScaleValue);
        }
        if (!MR::isNearZero(pInfo->mRateValue - 1.0f)) {
            pEmitter->setRate(pInfo->mRateValue, -1);
        }
        if (pInfo->mIsValidPrmColor) {
            Color8 color = pInfo->mPrmColor;
            pEmitter->setGlobalPrmColor(color.r, color.g, color.b, -1);
        }
        if (pInfo->mIsValidEnvColor) {
            Color8 color = pInfo->mEnvColor;
            pEmitter->setGlobalEnvColor(color.r, color.g, color.b, -1);
        }
        if (!MR::isNearZero(pInfo->mLightAffectValue)) {
            pEmitter->_2C = pInfo->mLightAffectValue;
        }
    }
}  // namespace

namespace MR {
    namespace Effect {
        bool isEffect2D(const MultiEmitter* pEmitter) {
            if (pEmitter->_28 == nullptr) {
                return false;
            }

            return pEmitter->_28->mDrawOrder == 6 || pEmitter->_28->mDrawOrder == 7;
        }

        void initEffectSyncBck(EffectKeeper* pKeeper, const ModelManager* pModelManager, const char* pEffectName,
                               const char* pAnimName, s32 count, f32 startFrame, f32 endFrame, bool deleteOnEnd) {
            pKeeper->registerSyncBckEffect(pModelManager->mXanimePlayer, pEffectName, pAnimName, count, startFrame, endFrame, deleteOnEnd);
        }

        void addEffectSyncBck(MultiEmitter* pEmitter, const ModelManager* pModelManager, const char* pAnimName) {
            pEmitter->addSyncBck(pModelManager->mXanimePlayer, pAnimName);
        }

        void setupMultiEmitter(EffectKeeper* pKeeper, const ModelManager* pModelManager, const AutoEffectInfo* pInfo) {
            MultiEmitter* pEmitter = pKeeper->getEmitter(pInfo->getName());
            setupMultiEmitterSyncBck(pKeeper, pModelManager, pInfo);
            ::setupMultiEmitter(pEmitter, pInfo);
            pEmitter->scanParticleEmitter(MR::getEffectSystem());
            if (pInfo->mParentName != nullptr) {
                pKeeper->getEmitter(pInfo->mParentName)->addChildEmitter(pEmitter);
            }
        }

        void setupMultiEmitterSyncBck(EffectKeeper* pKeeper, const ModelManager* pModelManager, const AutoEffectInfo* pInfo) {
            if (pInfo->mAnimName == nullptr) {
                return;
            }

            MultiEmitter* pEmitter = pKeeper->getEmitter(pInfo->getName());
            if (!MR::hasStringSpace(pInfo->mAnimName)) {
                initEffectSyncBck(pKeeper, pModelManager, pInfo->getName(), pInfo->mAnimName, 1,
                                 pInfo->mStartFrame, pInfo->mEndFrame, false);
            } else {
                const char* pAnimNames = pInfo->mAnimName;
                char name[256];
                MR::zeroMemory(name, sizeof(name));
                s32 length = 0;
                for (u32 i = 0; i <= strlen(pAnimNames); i++) {
                    if (pAnimNames[i] == ' ' || pAnimNames[i] == '\0') {
                        name[length] = '\0';
                        const char* pName = nullptr;
                        MR::findBckNameStringInResource(&pName, pModelManager->getResourceHolder(), name);
                        if (pEmitter->_24 == nullptr) {
                            s32 count = 0;
                            for (u32 j = 0; j <= strlen(pAnimNames); j++) {
                                if (pAnimNames[j] == ' ' || pAnimNames[j] == '\0') {
                                    count++;
                                }
                            }
                            initEffectSyncBck(pKeeper, pModelManager, pInfo->getName(), pName, count, 0.0f, -1.0f, false);
                        } else {
                            addEffectSyncBck(pEmitter, pModelManager, pName);
                        }
                        length = 0;
                        MR::zeroMemory(name, sizeof(name));
                    } else {
                        name[length++] = pAnimNames[i];
                    }
                }
            }
            pEmitter->setContinueBckEnd((pInfo->mFlag & 0x40) == 0x40);
        }

        void registerAutoEffectInfoGroup(EffectKeeper* pKeeper, const LiveActor* pActor, const char* pName) {
            createAndAddAutoEffectGroup(MR::getEffectSystem()->mGroupHolder, pName);
            registerAutoEffectInfos(MR::getEffectSystem()->mGroupHolder, pKeeper, pActor, pName);
        }

        void requestMovementOn(EffectKeeper* pKeeper) {
            for (s32 i = 0; i < pKeeper->_C.size(); i++) {
                MultiEmitter* pEmitter = pKeeper->getEmitter(i);
                if (pEmitter != nullptr && pEmitter->isValid()) {
                    pEmitter->pauseOff(-1);
                }
            }
        }

        void registerAutoEffectInfoGroup(PaneEffectKeeper* pKeeper, const LayoutActor* pActor, const char* pName) {
            registerAutoEffectInfoGroup(pKeeper, MR::getEffectSystem(), pActor, pName);
        }

        void registerAutoEffectInfoGroup(PaneEffectKeeper* pKeeper, const EffectSystem* pSystem, const LayoutActor* pActor, const char* pName) {
            createAndAddAutoEffectGroup(pSystem->mGroupHolder, pName);
            registerAutoEffectInfos(pSystem->mGroupHolder, pKeeper, pActor, pName);
        }

        void addAutoEffect(EffectKeeper* pKeeper, const LiveActor* pActor, const AutoEffectInfo* pInfo) {
            if (pInfo->mJointName != nullptr) {
                const char* pEffectName = pInfo->mEffectName;
                pKeeper->registerEffect(pEffectName, MR::getJointMtx(pActor, pInfo->mJointName), pInfo->mUniqueName, nullptr);
            } else if (pActor->getBaseMtx() != nullptr) {
                const char* pEffectName = pInfo->mEffectName;
                pKeeper->registerEffect(pEffectName, pActor->getBaseMtx(), &pActor->mScale, pInfo->mUniqueName, nullptr);
            } else {
                pKeeper->registerEffect(pInfo->mEffectName, &pActor->mPosition, &pActor->mRotation, &pActor->mScale, pInfo->mUniqueName);
            }
            setupMultiEmitter(pKeeper, pActor->mModelManager, pInfo);
        }

        void addAutoEffect(PaneEffectKeeper* pKeeper, const LayoutActor*, const AutoEffectInfo* pInfo) {
            pKeeper->add(pInfo->mJointName, pInfo->mEffectName, pInfo->mUniqueName);
            ::setupMultiEmitter(pKeeper->getEmitter(pInfo->mUniqueName), pInfo);
        }

        void addAutoEffect(MultiSceneEffectKeeper* pKeeper, const MultiSceneActor* pActor, const AutoEffectInfo* pInfo) {
            if (pInfo->mJointName != nullptr) {
                const char* pEffectName = pInfo->mEffectName;
                pKeeper->add(pEffectName, MultiScene::getJointMtx(pActor, pInfo->mJointName), pInfo->mUniqueName);
            } else {
                pKeeper->add(pInfo->mEffectName, &pActor->mTranslation, &pActor->mRotation, &pActor->mScale, pInfo->mUniqueName);
            }
            ::setupMultiEmitter(pKeeper->get(pInfo->mUniqueName), pInfo);
        }

        void registerAutoEffectInfoGroup(MultiSceneEffectKeeper* pKeeper, const EffectSystem* pSystem, const MultiSceneActor* pActor, const char* pName) {
            createAndAddAutoEffectGroup(pSystem->mGroupHolder, pName);
            registerAutoEffectInfos(pSystem->mGroupHolder, pKeeper, pActor, pName);
        }

        bool isExistInResource(u16* pIndex, const char* pName) {
            return MR::getParticleResourceHolder()->isExistInResource(pName, pIndex);
        }

        int getAutoEffectNum(const char* pName) {
            return MR::getParticleResourceHolder()->getAutoEffectNum(pName);
        }

        JMapInfo* getAutoEffectListBinary() {
            return MR::getParticleResourceHolder()->getAutoEffectListBinary();
        }

        void deleteParticleEmitter(ParticleEmitter* pEmitter) {
            if (pEmitter->mEmitter != nullptr) {
                pEmitter->mEmitter->playCalcEmitter();
                pEmitter->mEmitter->becomeInvalidEmitter();
            }
        }

        void setLinkSingleEmitter(ParticleEmitter* pEmitter, SingleEmitter* pSingleEmitter) {
            pEmitter->mEmitter->setUserWork(reinterpret_cast<uintptr_t>(pSingleEmitter));
        }

        SingleEmitter* getLinkSingleEmitter(const JPABaseEmitter* pEmitter) {
            return reinterpret_cast<SingleEmitter*>(pEmitter->getUserWork());
        }

        void createParticleEmitter(ParticleEmitter* pEmitter, JPAEmitterManager* pManager, const TVec3f& rPosition,
                                   u16 userIndex, u8 groupId, u8 resourceManagerId) {
            JPABaseEmitter* emitter = pManager->createSimpleEmitterID(rPosition, userIndex, groupId, resourceManagerId, nullptr, nullptr);
            if (emitter != nullptr) {
                pEmitter->mEmitter = emitter;
                pEmitter->init(userIndex);
            }
        }

        bool isExistInResource(u16* pIndex, const char* pName, s32 index) {
            char name[42];
            snprintf(name, sizeof(name), "%s%02d", pName, index);
            return MR::getParticleResourceHolder()->isExistInResource(name, pIndex);
        }
    };  // namespace Effect
};  // namespace MR
