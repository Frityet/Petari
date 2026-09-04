#include "runtime/ParticleResourceOwnership.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/Util/FileUtil.hpp"
#include "resource/JpcResource.hpp"
#include "resource/JMapResource.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <strings.h>

namespace aurora { extern AuroraConfig g_config; }
using smgpc::runtime::ParticleResourceOwnership;
constexpr const char* archive_path = "/ParticleData/Effect.arc";
constexpr std::size_t measured_budget = 1692960;

struct Backing {
    std::weak_ptr<const smgpc::resource::JpcResource> jpc;
    std::weak_ptr<JMapInfo::DataCompat> names, effects;
    std::weak_ptr<const smgpc::runtime::MountedArchive> archive;
    void assert_retired() const {
        assert(jpc.expired() && names.expired() && effects.expired() && archive.expired());
    }
};
static Backing backing_for(const std::shared_ptr<const smgpc::runtime::MountedArchive>& archive) {
    return {
        smgpc::resource::resolve_jpc_source(archive->archive().getResource("Particles.jpc")),
        smgpc::resource::find_jmap_resource(archive->archive().getResource("ParticleNames.bcsv"))->mData,
        smgpc::resource::find_jmap_resource(archive->archive().getResource("AutoEffectList.bcsv"))->mData,
        archive,
    };
}
static void assert_unpublished() {
    assert(ParticleResourceOwnership::active() == nullptr);
    bool rejected = false;
    try { MR::getParticleResourceHolder(); }
    catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
}
static void verify_authored(ParticleResourceOwnership& owner, smgpc::runtime::ArchiveMountService& mounts, std::size_t budget = measured_budget) {
    auto& holder = owner.holder();
    assert(MR::getParticleResourceHolder() == &holder);
    assert(holder.mResourceMgr->mResNum == 3327 && holder.mResourceMgr->mTexNum == 225);
    assert(holder.mParticleNames->getNumEntries() == 3327);
    assert(holder.mAutoEffectList->getNumEntries() == 2591 && holder.mNumParticles == 612);
    assert(owner.construction_bytes() == 1692720 && holder.mResourceMgr->mpHeap->getFreeSize() == budget - measured_budget);
    assert(JKRHeap::findFromRoot(&holder) == holder.mResourceMgr->mpHeap);
    assert(JKRHeap::findFromRoot(holder.mResourceMgr) == holder.mResourceMgr->mpHeap);
    assert(JKRHeap::findFromRoot(holder.mParticleNames) == holder.mResourceMgr->mpHeap);
    bool numbered_query = false;
    for (int i = 0; i < holder.mParticleNames->getNumEntries(); ++i) {
        const char* name = nullptr;
        assert(holder.mParticleNames->getValue(i, "name", &name));
        assert(holder.getUserIndex(name) == i);
        assert(holder.mResourceMgr->getResource(i) != nullptr);
        u16 index = 0xffff;
        assert(MR::Effect::isExistInResource(&index, name) && index == i);
        const std::string text(name);
        if (!numbered_query && text.ends_with("00")) {
            assert(MR::Effect::isExistInResource(&index, text.substr(0, text.size() - 2).c_str(), 0) && index == i);
            numbered_query = true;
        }
    }
    assert(numbered_query && MR::Effect::getAutoEffectListBinary() == holder.mAutoEffectList);
    u16 missing = 0x1234;
    assert(!MR::Effect::isExistInResource(&missing, "not-an-authored-particle") && missing == 0x1234);
    auto archive = mounts.retain(archive_path);
    auto raw = smgpc::resource::BcsvTable::from_bytes(archive->source().resource_data("/AutoEffectList.bcsv"));
    struct NoCase { bool operator()(const std::string& a, const std::string& b) const { return ::strcasecmp(a.c_str(), b.c_str()) < 0; } };
    std::map<std::string, int, NoCase> counts;
    std::map<std::string, int> exact_counts;
    for (std::size_t row = 0; row < raw.entry_count(); ++row) {
        auto name = raw.get_string(row, "GroupName");
        if (name) { ++counts[std::string(*name)]; ++exact_counts[std::string(*name)]; }
    }
    assert(counts.size() == 612 && exact_counts.size() == 614);
    for (const auto& [name, count] : counts) assert(holder.getAutoEffectNum(name.c_str()) == count && MR::Effect::getAutoEffectNum(name.c_str()) == count);
}

int main() {
    const char* disc = std::getenv("SMGPC_REAL_DISC");
    if (!disc || !aurora_dvd_open(disc)) throw std::runtime_error("SMGPC_REAL_DISC must name the actual supplied disc");
    struct DiscGuard { ~DiscGuard() { aurora_dvd_close(); } } disc_guard;
    DVDInit();
    aurora::g_config.mem1Size = 24U * 1024U * 1024U;
    smgpc::runtime::DvdFileSystemService dvd({});
    smgpc::runtime::ArchiveMountService mounts(dvd);
    auto runtime = smgpc::compat::JkrHeapRuntime::create(4U * 1024U * 1024U);
    const auto root_free = runtime->root_heap().getFreeSize();
    const auto name_objects = smgpc::compat::name_obj_runtime_state_count();
    assert_unpublished();
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        Backing backing;
        {
            const auto budget = cycle == 0 ? measured_budget : ParticleResourceOwnership::default_byte_budget;
            ParticleResourceOwnership owner(runtime, budget, mounts);
            verify_authored(owner, mounts, budget);
            backing = backing_for(mounts.retain(archive_path));
            auto* identity = MR::mountArchive(archive_path, nullptr);
            assert(identity == &mounts.retain(archive_path)->archive());
            const auto free_while_owned = runtime->root_heap().getFreeSize();
            bool duplicate_rejected = false;
            try { ParticleResourceOwnership duplicate(runtime, measured_budget, mounts); }
            catch (const std::logic_error&) { duplicate_rejected = true; }
            assert(duplicate_rejected && MR::getParticleResourceHolder() == &owner.holder());
            assert(runtime->root_heap().getFreeSize() == free_while_owned && mounts.size() == 1);
            if (cycle == 1) {
                mounts.remove_for_heap(owner.holder().mResourceMgr->mpHeap);
                assert(mounts.size() == 0 && !backing.archive.expired());
                const char* name = nullptr;
                assert(owner.holder().mParticleNames->getValue(3326, "name", &name));
                assert(owner.holder().getUserIndex(name) == 3326 && owner.holder().getAutoEffectNum("Mario") > 0);
            }
        }
        assert_unpublished();
        assert(mounts.size() == 0 && runtime->root_heap().getFreeSize() == root_free);
        backing.assert_retired();
    }
    std::cout << "fresh_owner_cycles=2 minimum_and_default_budget=pass literal_resource_queries=pass original_disposers_jpa_finalizer_archive_release=pass mount_unpublication_lease=pass\n";

    // A prior process mount keeps its actual identity and publication. Its heap
    // tag is not retagged or removed by the particle owner.
    {
        auto* prior = mounts.mount(archive_path, nullptr);
        auto lease = mounts.retain(archive_path);
        auto backing = backing_for(lease);
        {
            ParticleResourceOwnership owner(runtime, measured_budget, mounts);
            verify_authored(owner, mounts);
            assert(MR::mountArchive(archive_path, nullptr) == prior && lease->heap() == nullptr);
        }
        assert_unpublished();
        assert(mounts.size() == 1 && mounts.receive(archive_path) == prior);
        mounts.remove_for_heap(nullptr);
        lease.reset();
        backing.assert_retired();
        assert(runtime->root_heap().getFreeSize() == root_free);
    }
    std::cout << "borrowed_mount_identity_and_publication=pass\n";

    for (const auto budget : {std::size_t(10240), measured_budget - 32}) {
        bool failed = false;
        try { ParticleResourceOwnership constrained(runtime, budget, mounts); }
        catch (const std::bad_alloc&) { failed = true; }
        assert(failed);
        assert_unpublished();
        assert(mounts.size() == 0 && runtime->root_heap().getFreeSize() == root_free);
    }
    // Decode weak observers through the real registered archive before a late
    // original-constructor failure. If its JMap disposers or completed JPA
    // manager finalizer are missed, these weak owners cannot expire afterward.
    {
        mounts.mount(archive_path, nullptr);
        auto lease = mounts.retain(archive_path);
        auto backing = backing_for(lease);
        bool failed = false;
        try { ParticleResourceOwnership constrained(runtime, measured_budget - 32, mounts); }
        catch (const std::bad_alloc&) { failed = true; }
        assert(failed);
        assert_unpublished();
        assert(mounts.size() == 1 && runtime->root_heap().getFreeSize() == root_free);
        mounts.remove_for_heap(nullptr);
        lease.reset();
        backing.assert_retired();
    }
    assert(smgpc::compat::name_obj_runtime_state_count() == name_objects);
    std::cout << "early_and_late_constructor_failure_rollback=pass failure_jmap_jpa_backing_expiration=pass no_nameobj_registration=pass\n";
    std::cout << "domain_budget=1692960 construction_bytes=1692720 native_heap_header=240 remaining=0 resources=3327 textures=225 names=3327 auto_effects=2591 groups_case_insensitive=612 groups_case_sensitive=614\n";
}
