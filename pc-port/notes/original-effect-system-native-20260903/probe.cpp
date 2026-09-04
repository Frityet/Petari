#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/ParticleCalcExecutor.hpp"
#include "Game/Effect/ParticleDrawExecutor.hpp"
#include "Game/Effect/ParticleEmitterHolder.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Effect/AutoEffectGroupHolder.hpp"
#include "Game/Effect/AutoEffectInfo.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/Color.hpp"
#include <cstring>
#include "Game/NameObj/NameObjAdaptor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/ParticleResourceOwnership.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>

namespace aurora { extern AuroraConfig g_config; }
static NameObj* original_effect_factory(int id, void*) {
    if (id == SceneObj_EffectSystem) return new EffectSystem("エフェクトシステム", true);
    return nullptr;
}
int main() {
    const char* disc = std::getenv("SMGPC_REAL_DISC");
    if (!disc || !aurora_dvd_open(disc)) throw std::runtime_error("Actual disc required");
    struct DiscGuard { ~DiscGuard() { aurora_dvd_close(); } } disc_guard;
    DVDInit();
    aurora::g_config.mem1Size = 24U * 1024U * 1024U;
    smgpc::runtime::DvdFileSystemService dvd({});
    smgpc::runtime::ArchiveMountService mounts(dvd);
    auto heaps = smgpc::compat::JkrHeapRuntime::create(8U << 20);
    smgpc::runtime::ParticleResourceOwnership resources(heaps, resources.default_byte_budget, mounts);
    smgpc::runtime::SceneScheduler scheduler;
    smgpc::runtime::SceneSchedulerBinding scheduler_binding(scheduler);
    const auto base_objects = smgpc::compat::name_obj_runtime_state_count();
    unsigned authored_colors = 0;
    const auto* auto_table = MR::Effect::getAutoEffectListBinary();
    for (int i = 0; i < auto_table->getNumEntries(); ++i) {
        JMapInfoIter iter(auto_table, i);
        AutoEffectInfo info;
        info.init(iter);
        auto check_color = [&](const char* key, const Color8& color, bool valid) {
            if (!valid) return;
            const char* value = nullptr;
            assert(iter.getValue(key, &value) && value && value[0] == '#');
            const auto expected = static_cast<u32>(std::strtoul(value + 1, nullptr, 16)) << 8;
            assert(color.r == (expected >> 24) && color.g == ((expected >> 16) & 255));
            assert(color.b == ((expected >> 8) & 255) && color.a == (expected & 255));
            assert(static_cast<u32>(color) == expected);
            ++authored_colors;
        };
        check_color("PrmColor", info.mPrmColor, info.mIsValidPrmColor);
        check_color("EnvColor", info.mEnvColor, info.mIsValidEnvColor);
    }
    assert(authored_colors > 0);
    std::size_t constructor_bytes = 0;
    const auto root_free = heaps->root_heap().getFreeSize();
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 64U << 10);
        const auto marker = scheduler.registration_marker();
        SceneObjHolder holder;
        {
            smgpc::scene::SceneObjHolderBinding binding(holder, original_effect_factory);
            EffectSystem* system;
            {
                smgpc::compat::JkrAllocationScope heap(domain);
                const auto before_constructor = domain->heap().getFreeSize();
                system = static_cast<EffectSystem*>(MR::createSceneObj(SceneObj_EffectSystem));
                constructor_bytes = before_constructor - domain->heap().getFreeSize();
                assert(system && MR::getEffectSystem() == system && holder.getObj(SceneObj_EffectSystem) == system);
                assert(JKRHeap::findFromRoot(system) == &domain->heap());
                assert(JKRHeap::findFromRoot(system->mDrawExec) == &domain->heap());
                assert(JKRHeap::findFromRoot(system->mCalcExec) == &domain->heap());
                assert(system->mDrawExec->mHost == system && system->mCalcExec->mHost == system);
                assert(MR::Effect::createAndAddAutoEffectGroup(system->mGroupHolder, "Mario"));
                assert(system->mGroupHolder->find("Mario")->mInfos.size() == MR::Effect::getAutoEffectNum("Mario"));
                assert(!MR::Effect::createAndAddAutoEffectGroup(system->mGroupHolder, "Mario"));
            }
            const auto registered = scheduler.snapshot();
            assert(registered.size() == 11);
            std::set<int> draw_categories;
            unsigned normal_calc = 0, pause_calc = 0, update = 0;
            for (const auto& row : registered) {
                if (row.draw_type >= 0) draw_categories.insert(row.draw_type);
                normal_calc += row.calc_anim_type == 19;
                pause_calc += row.calc_anim_type == 20;
                update += row.movement_type == 20;
            }
            assert((draw_categories == std::set<int>{71,72,73,74,75,76,77}));
            assert(normal_calc == 1 && pause_calc == 2 && update == 1);
            assert(smgpc::compat::name_obj_runtime_state_count() == base_objects + 12);
            // Keep entry in the complete linked chain, but only invoke it when
            // the existing real screen owner is available. This CPU process has none.
            if (MR::getScreenResTIMG()) {
                smgpc::compat::JkrAllocationScope heap(domain);
                system->entry(MR::getParticleResourceHolder(), 32, 4);
                assert(system->mEmitterManager->gidMax == 9 && system->mEmitterHolder->mEmitters.size() == 4);
                system->mEmitterHolder->forceDeleteAllEmitters();
            } else {
                assert(system->mEmitterManager == nullptr);
            }
            scheduler.remove_registrations_since(marker);
        }
        assert(MR::getSceneObjHolder() == nullptr);
        assert(scheduler.snapshot().empty());
        assert(smgpc::compat::name_obj_runtime_state_count() == base_objects);
        domain.reset();
        assert(heaps->root_heap().getFreeSize() == root_free);
    }
    {
        auto measurement = smgpc::compat::JkrAllocationDomain::create(heaps, 64U << 10);
        const auto header_bytes = (64U << 10) - measurement->heap().getFreeSize();
        measurement.reset();
        auto failure = smgpc::compat::JkrAllocationDomain::create(heaps, header_bytes + constructor_bytes - 32);
        const auto marker = scheduler.registration_marker();
        SceneObjHolder holder;
        {
            smgpc::scene::SceneObjHolderBinding binding(holder, original_effect_factory);
            bool rejected = false;
            try {
                smgpc::compat::JkrAllocationScope heap(failure);
                (void)MR::createSceneObj(SceneObj_EffectSystem);
            } catch (const std::bad_alloc&) { rejected = true; }
            assert(rejected && !holder.isExist(SceneObj_EffectSystem));
            assert(smgpc::compat::name_obj_runtime_state_count() == base_objects);
            assert(scheduler.snapshot().empty());
            scheduler.remove_registrations_since(marker);
        }
        failure.reset();
        assert(heaps->root_heap().getFreeSize() == root_free);
    }
    std::cout << "constructor_bytes=" << constructor_bytes << " authored_color_fields=" << authored_colors << " constructor_failure_rollback=pass\n";
    std::cout << "actual_effect_system_scene_binding_cycles=2 nameobjs=12 draw_categories=7 calc_adaptors=3 update_adaptors=1 actual_auto_effect_group=pass scheduler_rollback=pass arena_retirement=pass entry_screen_owner=absent_not_invoked\n";
}
