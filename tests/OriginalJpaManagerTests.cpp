#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "resource/JpcResource.hpp"
#include "resource/RarcArchive.hpp"
#include "NativeHeapFixture.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/JParticle/JPAResource.hpp>
#include <JSystem/JParticle/JPAEmitterManager.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPADynamicsBlock.hpp>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string_view>
#include <cstdlib>
#include <stdexcept>

namespace {
void verify_jpa(bool ownershipOnly, std::weak_ptr<const smgpc::resource::JpcResource>& backing) {
    auto& holder = *MR::getParticleResourceHolder();
    auto* manager = holder.mResourceMgr;
    assert(holder.mParticleNames->getNumEntries() == 3327 && holder.mNumParticles == 612);
    for (int i = 0; i < holder.mParticleNames->getNumEntries(); ++i) {
        const char* name = nullptr;
        assert(holder.mParticleNames->getValue(i, "name", &name));
        assert(holder.getUserIndex(name) == i);
    }
    auto domain = smgpc::test::create_native_solid_heap(MR::getSceneObjHolder()->nativeAllocationHeap(), 2U << 20);
    const std::weak_ptr<JKRHeap> retired = domain;
    backing = manager->mNativeResource;
    assert(manager->mResNum == 3327 && manager->mTexNum == 225);
    size_t functions = 0, frames = 0, particleObservations = 0;
    {
        const JKRHeap::CurrentHeapScope scope(*(domain));
        const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
        JPAEmitterManager emitters(32, 4, &(*domain), 1, 1);
        emitters.entryResourceManager(manager, 0);
        // Exercise the recovered camera helper directly; drawing remains outside this CPU fixture.
        PSMTXIdentity(emitters.pWd->mPosCamMtx);
        emitters.pWd->mPosCamMtx[1][1] = 3.0f;
        emitters.pWd->mPosCamMtx[2][1] = 4.0f;
        emitters.pWd->mPosCamMtx[0][3] = 2.0f;
        emitters.calcYBBCam();
        assert(std::abs(emitters.pWd->mYBBCamMtx[1][1] - 0.6f) < 0.00001f);
        assert(std::abs(emitters.pWd->mYBBCamMtx[2][1] - 0.8f) < 0.00001f);
        assert(emitters.pWd->mYBBCamMtx[1][2] == -emitters.pWd->mYBBCamMtx[2][1]);
        assert(emitters.pWd->mYBBCamMtx[0][3] == 2.0f);
        for (u32 i = 0; i < manager->mResNum; ++i) {
            auto* resource = manager->mpResArr[i];
            assert(manager->getResource(resource->mUsrIdx) == resource);
            auto check = [&](auto* list, u8 count) {
                for (u8 j = 0; j < count; ++j) { assert(list[j]); ++functions; }
            };
            check(resource->mpCalcEmitterFuncList, resource->mpCalcEmitterFuncListNum);
            check(resource->mpDrawEmitterFuncList, resource->mpDrawEmitterFuncListNum);
            check(resource->mpDrawEmitterChildFuncList, resource->mpDrawEmitterChildFuncListNum);
            check(resource->mpCalcParticleFuncList, resource->mpCalcParticleFuncListNum);
            check(resource->mpDrawParticleFuncList, resource->mpDrawParticleFuncListNum);
            check(resource->mpCalcParticleChildFuncList, resource->mpCalcParticleChildFuncListNum);
            check(resource->mpDrawParticleChildFuncList, resource->mpDrawParticleChildFuncListNum);
            auto* emitter = emitters.createSimpleEmitterID(TVec3f(1, 2, 3), resource->mUsrIdx, 0, 0, nullptr, nullptr);
            assert(emitter && emitter->mpRes == resource && emitter->mRate == resource->getDyn()->getRate());
            assert(emitter->mGlobalTrs.x == 1 && emitter->mGlobalTrs.y == 2 && emitter->mGlobalTrs.z == 3);
            assert(emitters.getEmitterNumber() == 1);
            for (int frame = 0; !ownershipOnly && frame < 16 && emitters.getEmitterNumber(); ++frame) {
                emitters.calc(0);
                ++frames;
                particleObservations += 32 - emitters.mPtclPool.getNum();
            }
            if (emitters.getEmitterNumber()) emitters.forceDeleteEmitter(emitter);
            assert(emitters.getEmitterNumber() == 0 && emitters.mPtclPool.getNum() == 32);
        }
        JPABaseEmitter* full[4];
        for (auto& emitter : full) emitter = emitters.createSimpleEmitterID(TVec3f(0, 0, 0), manager->mpResArr[0]->mUsrIdx, 0, 0, nullptr, nullptr);
        assert(emitters.getEmitterNumber() == 4);
        assert(emitters.createSimpleEmitterID(TVec3f(0, 0, 0), manager->mpResArr[0]->mUsrIdx, 0, 0, nullptr, nullptr) == nullptr);
        for (auto* emitter : full) emitters.forceDeleteEmitter(emitter);
        assert(emitters.getEmitterNumber() == 0 && emitters.mPtclPool.getNum() == 32);
    }
    domain.reset();
    assert(retired.expired() && !backing.expired());
    std::cout << "Actual original manager loaded 3327 resources and 225 textures; " << functions
              << " function entries; all 3327 emitter IDs constructed; " << frames << " CPU frames; "
              << particleObservations << " particle-frame observations; pool reuse, exhaustion, and emitter heap retirement passed\n";
}
}
int main(int argc, char** argv) {
    if (argc > 2 || (argc == 2 && std::string_view(argv[1]) != "--ownership-only")) return 2;
    std::weak_ptr<const smgpc::resource::JpcResource> backing;
    const auto result = smgpc::test::run_stage_resource_process("original-jpa-manager", [&] { verify_jpa(argc == 2, backing); });
    if (result != 0) return result;
    if (!backing.expired()) throw std::runtime_error("Original particle resource backing survived process retirement");
    return 0;
}
