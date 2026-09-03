#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
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

        bool isExistInResource(u16* pIndex, const char* pName, s32 index) {
            char name[42];
            snprintf(name, sizeof(name), "%s%02d", pName, index);
            return MR::getParticleResourceHolder()->isExistInResource(name, pIndex);
        }
    };  // namespace Effect
};  // namespace MR
