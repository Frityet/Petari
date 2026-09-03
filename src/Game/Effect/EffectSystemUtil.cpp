#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Effect/ParticleEmitter.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include <cstdio>

namespace MR {
    namespace Effect {
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
