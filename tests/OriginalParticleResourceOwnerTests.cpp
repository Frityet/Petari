#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include "resource/JpcResource.hpp"
#include "resource/JMapResource.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include <cassert>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <strings.h>

namespace {
constexpr const char* archive_path = "/ParticleData/Effect.arc";
struct Backing {
    std::weak_ptr<const smgpc::resource::JpcResource> jpc;
    std::weak_ptr<const void> names, effects;
};
void verify_authored(Backing& backing) {
    auto* system = SingletonHolder<GameSystem>::get();
    auto* actual = MR::getParticleResourceHolder();
    assert(system && system->mObjHolder && actual == system->mObjHolder->mParticleResHolder);
    auto& holder = *actual;
    assert(holder.mResourceMgr->mResNum == 3327 && holder.mResourceMgr->mTexNum == 225);
    assert(holder.mParticleNames->getNumEntries() == 3327);
    assert(holder.mAutoEffectList->getNumEntries() == 2591 && holder.mNumParticles == 612);
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
    struct NoCase { bool operator()(const std::string& a, const std::string& b) const { return ::strcasecmp(a.c_str(), b.c_str()) < 0; } };
    std::map<std::string, int, NoCase> counts;
    std::map<std::string, int> exact_counts;
    for (std::size_t row = 0; row < static_cast<std::size_t>(holder.mAutoEffectList->getNumEntries()); ++row) {
        const char* name = nullptr;
        holder.mAutoEffectList->getValue(static_cast<s32>(row), "GroupName", &name);
        if (name) { ++counts[std::string(name)]; ++exact_counts[std::string(name)]; }
    }
    assert(counts.size() == 612 && exact_counts.size() == 614);
    for (const auto& [name, count] : counts) assert(holder.getAutoEffectNum(name.c_str()) == count && MR::Effect::getAutoEffectNum(name.c_str()) == count);
    // Borrow only weak native resource observers, then verify real process teardown.
    auto* archive = MR::receiveArchive(archive_path);
    assert(archive && MR::mountArchive(archive_path, nullptr) == archive);
    backing.jpc = smgpc::resource::resolve_jpc_source(archive->getResource("Particles.jpc"));
    backing.names = holder.mParticleNames->mResourceOwner;
    backing.effects = holder.mAutoEffectList->mResourceOwner;
    assert(!backing.jpc.expired() && !backing.names.expired() && !backing.effects.expired());
    std::fprintf(stderr, "PASS resources=3327 textures=225 names=3327 auto_effects=2591 groups=612 original_facades=pass\n");
}
}
int main() {
    Backing backing;
    const auto result = smgpc::test::run_stage_resource_process("particle-resource-owner", [&] { verify_authored(backing); });
    if (result != 0) return result;
    if (!backing.jpc.expired() || !backing.names.expired() || !backing.effects.expired()) {
        std::fprintf(stderr, "FAIL original process particle backing survived teardown\n");
        return 1;
    }
    std::fprintf(stderr, "PASS original process particle native resource retirement\n");
    return 0;
}
