#include "runtime/RuntimeContext.hpp"
#include "runtime/ParticleResourceOwnership.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/ParticleCalcExecutor.hpp"
#include "Game/Effect/ParticleEmitterHolder.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include <aurora/dvd.h>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

class FixtureLogger final : public smgpc::logging::ILogger {
    void write(std::FILE*, std::source_location, smgpc::logging::Level,
               smgpc::logging::Category, std::string_view) override {}
};
static NameObj* original_effect_factory(int id, void*) {
    if (id == SceneObj_EffectSystem) return new EffectSystem("エフェクトシステム", true);
    return nullptr;
}
int main() {
    smgpc::render::AuroraWindow window({.width=640, .height=456, .title="Original EffectSystem ownership"});
    smgpc::render::AuroraRenderer renderer(window);
    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    if (!disc || !aurora_dvd_open(disc)) throw std::runtime_error("Actual disc required");
    struct Disc { ~Disc() { aurora_dvd_close(); } } disc_guard;
    DVDInit();
    smgpc::resource::GameResourceRuntime process({96U << 20, 32U << 20, 4U << 20});
    FixtureLogger logger;
    const auto nameobjs_before = smgpc::compat::name_obj_runtime_state_count();
    const auto free_before = process.host_heaps()->root_heap().getFreeSize();
    (void)renderer.begin_frame();
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        {
            smgpc::runtime::RuntimeContext runtime(logger, window, process);
            runtime.initialize_particle_resources(process);
            auto resources = runtime.retain_particle_resources();
            auto& scheduler = runtime.scheduler();
            smgpc::runtime::SceneSchedulerBinding scheduler_binding(scheduler);
            const auto marker = scheduler.registration_marker();
            const auto registrations_before = scheduler.snapshot().size();
            const auto runtime_nameobjs = smgpc::compat::name_obj_runtime_state_count();
            const auto runtime_free = process.host_heaps()->root_heap().getFreeSize();
            auto domain = smgpc::compat::JkrAllocationDomain::create(process.host_heaps(), 128U << 10);
            std::weak_ptr<smgpc::compat::JkrAllocationDomain> domain_weak = domain;
            SceneObjHolder holder;
            {
                smgpc::scene::SceneObjHolderBinding scene_binding(holder, original_effect_factory);
                EffectSystem* system;
                std::array<ParticleEmitter*, 4> emitters;
                {
                    smgpc::compat::JkrAllocationScope heap(domain);
                    system = static_cast<EffectSystem*>(MR::createSceneObj(SceneObj_EffectSystem));
                    assert(MR::getEffectSystem() == system && system);
                    const auto* screen = MR::getScreenResTIMG();
                    assert(screen && screen->mWidth && screen->mHeight);
                    const auto initial_free = domain->heap().getFreeSize();
                    system->entry(&resources->holder(), 32, 4);
                    assert(system->mEmitterManager->gidMax == 9);
                    assert(system->mEmitterManager->pResMgrAry[0] == resources->holder().mResourceMgr);
                    assert(system->mEmitterHolder->mEmitters.size() == 4);
                    bool found_screen = false;
                    auto* manager = resources->holder().mResourceMgr;
                    for (u16 i=0; i<manager->mTexNum; ++i) {
                        auto* texture = manager->mpTexArr[i];
                        if (std::strcmp(texture->getName(), "IndDummy") == 0) {
                            assert(texture->getJUTTexture()->getTexInfo() == screen);
                            found_screen = true;
                        }
                    }
                    assert(found_screen);
                    const char* authored_name = nullptr;
                    assert(resources->holder().mParticleNames->getValue(0, "name", &authored_name));
                    const auto resource_id = resources->holder().getUserIndex(authored_name);
                    assert(resource_id == 0);
                    const std::array<u8,4> groups = {0,1,7,8};
                    for (unsigned i=0; i<4; ++i) {
                        emitters[i] = system->createEmitter(resource_id, groups[i], 0);
                        assert(emitters[i] && emitters[i]->mEmitter->getGroupID() == groups[i]);
                    }
                    assert(system->createEmitter(resource_id, 0, 0) == nullptr);
                    for (unsigned frame=0; frame<8; ++frame) {
                        std::array<u32,4> ticks;
                        for (unsigned i=0;i<4;++i) ticks[i] = emitters[i]->mEmitter->mTick;
                        scheduler.execute_movement();
                        assert(system->mCalcExec->_15);
                        scheduler.execute_calc_anim();
                        assert(!system->mCalcExec->_15);
                        for (unsigned i=0;i<4;++i) assert(emitters[i]->mEmitter->mTick == ticks[i]+1);
                        scheduler.execute_calc_anim();
                        for (unsigned i=0;i<4;++i) assert(emitters[i]->mEmitter->mTick == ticks[i]+1);
                    }
                    std::cout << "cycle=" << cycle << " entry_and_emitters_bytes=" << initial_free-domain->heap().getFreeSize()
                              << " authored_resource=" << authored_name << " actual_screen_texture=pass groups=0,1,7,8 frames=8\n";
                    system->mEmitterHolder->forceDeleteAllEmitters();
                    for (auto* emitter:emitters) assert(!emitter->isValid());
                    auto* reused = system->createEmitter(resource_id, 0, 0);
                    assert(reused == emitters[0]);
                    system->mEmitterHolder->forceDeleteAllEmitters();
                }
                assert(scheduler.snapshot().size() == registrations_before + 11);
                // Remove category borrowers and their retained heap-domain leases
                // while the exact NameObjs and process resources are still alive.
                scheduler.remove_registrations_since(marker);
            }
            assert(!MR::getSceneObjHolder());
            assert(scheduler.snapshot().size() == registrations_before);
            assert(smgpc::compat::name_obj_runtime_state_count() == runtime_nameobjs);
            domain.reset();
            assert(domain_weak.expired());
            assert(process.host_heaps()->root_heap().getFreeSize() == runtime_free);
            // The RuntimeContext owns both the real capture texture and particle
            // resources and retires them after this complete scene borrower graph.
        }
        assert(!smgpc::runtime::ParticleResourceOwnership::active());
        assert(smgpc::compat::name_obj_runtime_state_count() == nameobjs_before);
        assert(process.host_heaps()->root_heap().getFreeSize() == free_before);
    }
    renderer.end_frame();
    std::cout << "actual_effect_entry_scheduler_calculation_cycles=2 ownership_reinitialization=pass drawing=not_invoked\n";
}
