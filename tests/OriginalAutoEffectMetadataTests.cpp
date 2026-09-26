#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/Effect/AutoEffectGroup.hpp"
#include "Game/Effect/AutoEffectGroupHolder.hpp"
#include "Game/Effect/AutoEffectInfo.hpp"
#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JKernel/JKRExpHeap.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include "NativeHeapFixture.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"

#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr const char* archive_path = "/ParticleData/Effect.arc";
constexpr std::array<std::string_view, 9> draw_orders{
    "3D", "PAUSE_IGNORE", "INDIRECT", "AFTER_INDIRECT", "BLOOM_EFFECT",
    "AFTER_IMAGE_EFFECT", "2D", "2D_PAUSE_IGNORE", "FOR_2D_MODEL"
};

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string(message));
}

std::string folded(std::string name) {
    for (char& letter : name) {
        if (letter >= 'A' && letter <= 'Z') letter += 'a' - 'A';
    }
    return name;
}

std::string changed_case(std::string name) {
    for (char& letter : name) {
        if (letter >= 'a' && letter <= 'z') letter -= 'a' - 'A';
        else if (letter >= 'A' && letter <= 'Z') letter += 'a' - 'A';
    }
    return name;
}

struct Coverage {
    std::size_t rows = 0;
    std::size_t colors = 0;
    std::size_t animations = 0;
    std::size_t continued_animations = 0;
    std::size_t parents = 0;
    std::array<std::size_t, 9> draw_counts{};
};

void verify_info(const AutoEffectInfo& info, const smgpc::resource::BcsvTable& raw,
                 int row, Coverage* coverage = nullptr, u16 reserved_flags = 0) {
    const auto check = [row](bool value, const char* field) {
        if (!value) throw std::runtime_error("AutoEffectList row " + std::to_string(row) + " differs in " + field);
    };
    const auto string = [&](const char* field) { return raw.get_string(row, field).value(); };
    const std::array<std::pair<const char*, const char*>, 6> strings{{
        {"GroupName", info.mGroupName}, {"AnimName", info.mAnimName}, {"UniqueName", info.mUniqueName},
        {"EffectName", info.mEffectName}, {"ParentName", info.mParentName}, {"JointName", info.mJointName}
    }};
    for (const auto& [field, actual] : strings) {
        const auto expected = string(field);
        check(expected.empty() ? actual == nullptr : actual != nullptr && actual == expected, field);
    }
    check(info.getName() == (info.mUniqueName ? info.mUniqueName : info.mEffectName), "effective name");
    const auto follow = string("Follow");
    const auto affect = string("Affect");
    u16 flags = string("ContinueAnimEnd") == "on" ? 0x40 : 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (follow.find("TRS"[axis]) != std::string::npos) flags |= 1U << axis;
        if (affect.find("TRS"[axis]) != std::string::npos) flags |= 8U << axis;
    }
    check(info.mFlag == (flags | reserved_flags), "Follow/Affect/ContinueAnimEnd flags");
    const std::array<std::pair<const char*, float>, 6> floats{{
        {"OffsetX", info.mOffset.x}, {"OffsetY", info.mOffset.y}, {"OffsetZ", info.mOffset.z},
        {"ScaleValue", info.mScaleValue}, {"RateValue", info.mRateValue}, {"LightAffectValue", info.mLightAffectValue}
    }};
    for (const auto& [field, actual] : floats) check(actual == raw.get_float(row, field).value(), field);
    check(info.mStartFrame == raw.get_s32(row, "StartFrame").value(), "StartFrame");
    check(info.mEndFrame == raw.get_s32(row, "EndFrame").value(), "EndFrame");
    const auto color = [&](const char* field, const Color8& actual, bool valid) {
        const auto text = string(field);
        check(valid == !text.empty(), field);
        if (!valid) return;
        check(text.size() == 7 && text.front() == '#', "authored RGB notation");
        const auto rgb = static_cast<u32>(std::stoul(text.substr(1), nullptr, 16));
        check(actual.r == ((rgb >> 16) & 255) && actual.g == ((rgb >> 8) & 255) && actual.b == (rgb & 255), field);
        check(actual.a == 0 && static_cast<u32>(actual) == (rgb << 8), "native RGBA byte order");
        if (coverage) ++coverage->colors;
    };
    color("PrmColor", info.mPrmColor, info.mIsValidPrmColor);
    color("EnvColor", info.mEnvColor, info.mIsValidEnvColor);
    s32 order = 0;
    const auto authored_order = string("DrawOrder");
    for (std::size_t index = 0; index < draw_orders.size(); ++index) {
        if (authored_order == draw_orders[index]) order = static_cast<s32>(index);
    }
    check(info.mDrawOrder == order, "DrawOrder");
    if (coverage) {
        ++coverage->rows;
        coverage->animations += info.mAnimName != nullptr;
        coverage->continued_animations += (info.mFlag & 0x40) != 0;
        coverage->parents += info.mParentName != nullptr;
        ++coverage->draw_counts[order];
    }
}

// The original holder owns a fixed pointer array. This fixture retires its
// constructed metadata before releasing the containing native allocation domain.
struct GroupBatch {
    AutoEffectGroupHolder holder;
    ~GroupBatch() { clear(); }
    void clear() {
        for (auto* group : holder.mGroups) {
            delete group;
        }
        holder.mGroups.clear();
    }
};
}

namespace {
struct Backing {
    std::weak_ptr<JKRHeap> root;
    std::weak_ptr<JKRHeap> scene;
    std::weak_ptr<JKRHeap> metadata;
    std::weak_ptr<const void> effects;
    std::weak_ptr<const void> source;
};

void verify_authored(Backing& backing) {
    require(JKRHeap::sRootHeap != nullptr, "the original process has created its actual SDK root heap");
    const auto root = (*JKRHeap::sRootHeap).retainNativeLifetime();
    backing.root = root;
    const auto scene = MR::getSceneObjHolder()->nativeAllocationHeap();
    require(scene != nullptr, "the original GameScene has its actual allocation owner");
    backing.scene = scene;
    Coverage coverage;
    std::size_t exact_count = 0;
    std::size_t folded_count = 0;
    std::size_t unique_count = 0;
    std::size_t full_batches = 0;
    {
        auto* system = SingletonHolder<GameSystem>::get();
        auto* particles = MR::getParticleResourceHolder();
        require(system && system->mObjHolder && particles && particles == system->mObjHolder->mParticleResHolder,
                "the original process owns the queried particle resource holder");
        auto* archive = MR::receiveArchive(archive_path);
        require(archive != nullptr && MR::mountArchive(archive_path, nullptr) == archive,
                "the original FileLoader retains the process Effect.arc mount");
        const auto* source = static_cast<const u8*>(archive->getResource("AutoEffectList.bcsv"));
        const auto size = archive->getResSize(source);
        require(source && size > 0, "the original mounted archive exposes bounded authored metadata bytes");
        const auto raw = smgpc::resource::BcsvTable::from_bytes(std::span<const u8>(source, size));
        auto* map = MR::Effect::getAutoEffectListBinary();
        require(map == particles->mAutoEffectList && map->getData() == source && map->getNumEntries() == raw.entry_count(),
                "original metadata retains the actual mounted byte identity and complete row set");
        auto* process_heap = JKRHeap::findFromRoot(particles);
        require(process_heap && process_heap == particles->mResourceMgr->mpHeap &&
                    JKRHeap::findFromRoot(particles->mResourceMgr) == process_heap &&
                    JKRHeap::findFromRoot(map) == process_heap,
                "the actual particle holder, resource manager and metadata table belong to their original process heap");
        backing.effects = map->mResourceOwner;
        backing.source = archive->retainSource();
        require(!backing.effects.expired() && !backing.source.expired(),
                "weak observers witness the real process metadata and retained archive source");
        require(raw.entry_count() > 0, "the real auto-effect table is populated");
        std::map<std::string, std::vector<int>> groups;
        std::set<std::string> exact_names;
        std::map<std::string, int> unique_first_rows;
        for (int row = 0; row < map->getNumEntries(); ++row) {
            const auto name = raw.get_string(row, "GroupName").value();
            groups[folded(name)].push_back(row);
            exact_names.insert(name);
            unique_first_rows.try_emplace(raw.get_string(row, "UniqueName").value(), row);
        }
        exact_count = exact_names.size();
        folded_count = groups.size();
        unique_count = unique_first_rows.size();
        require(particles->mNumParticles == groups.size(),
                "original resource counts collapse case variants of group names");
        // Original scene heaps are solid: individual frees are deferred until
        // scene retirement. Use a reclaiming child to verify metadata deletes.
        std::unique_ptr<JKRExpHeap, void (*)(JKRExpHeap*)> metadata_heap(
            JKRExpHeap::create(1U * 1024U * 1024U, &(*scene), false),
            +[](JKRExpHeap* heap) { heap->destroy(); });
        require(metadata_heap != nullptr, "the original scene heap has space for the bounded metadata test arena");
        auto metadata = (*metadata_heap).retainNativeLifetime();
        backing.metadata = metadata;
        const auto metadata_free = metadata_heap->getTotalFreeSize();
        GroupBatch batch;
        require(batch.holder.mGroups.size() == 0 && batch.holder.mGroups.capacity() == 256,
                "original group holder starts empty with its authored fixed capacity");
        constexpr const char* missing = "__unpublished_metadata_group__";
        require(!groups.contains(missing), "missing lookup fixture is absent from the actual resource");
        for (const auto& [name, rows] : groups) {
            if (batch.holder.mGroups.size() == batch.holder.mGroups.capacity()) {
                ++full_batches;
                batch.clear();
            }
            const int previous_count = batch.holder.mGroups.size();
            {
                const JKRHeap::CurrentHeapScope scope(*(metadata));
                const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
                require(MR::Effect::createAndAddAutoEffectGroup(&batch.holder, name.c_str()),
                        "every authored group constructs through the original registration surface");
            }
            auto* group = batch.holder.find(name.c_str());
            require(group != nullptr && group->getName() == name.c_str() && batch.holder.isExist(name.c_str()),
                    "registered group preserves its caller-owned name and lookup identity");
            require(batch.holder.mGroups.size() == previous_count + 1 && batch.holder.mGroups[previous_count] == group,
                    "group insertion preserves original append order");
            require(group->mInfos.size() == rows.size() && group->mInfos.capacity() == rows.size() &&
                        MR::Effect::getAutoEffectNum(name.c_str()) == rows.size(),
                    "group allocation uses the exact case-insensitive authored record count");
            require(JKRHeap::findFromRoot(group) == &(*metadata) &&
                        JKRHeap::findFromRoot(group->mInfos.mArray.mArr) == &(*metadata),
                    "original group and pointer array use the native metadata allocation domain");
            for (std::size_t i = 0; i < rows.size(); ++i) {
                const auto* info = group->mInfos[static_cast<int>(i)];
                require(JKRHeap::findFromRoot(const_cast<AutoEffectInfo*>(info)) == &(*metadata),
                        "original metadata records use the same native allocation domain");
                verify_info(*info, raw, rows[i], &coverage);
            }
            const auto variant = changed_case(name);
            const auto free_before_lookup = (*metadata).getFreeSize();
            require(batch.holder.find(variant.c_str()) == group && batch.holder.isExist(variant.c_str()),
                    "case variants retrieve the existing original group");
            {
                const JKRHeap::CurrentHeapScope scope(*(metadata));
                const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
                require(!MR::Effect::createAndAddAutoEffectGroup(&batch.holder, variant.c_str()) &&
                            !MR::Effect::createAndAddAutoEffectGroup(&batch.holder, missing),
                        "duplicate and missing groups do not consume another slot, including at capacity");
            }
            require(batch.holder.find(missing) == nullptr && !batch.holder.isExist(missing) &&
                        batch.holder.mGroups.size() == previous_count + 1 && (*metadata).getFreeSize() == free_before_lookup,
                    "unsuccessful registration leaves original count and storage unchanged");
        }
        require(coverage.rows == raw.entry_count() && full_batches > 0,
                "every authored row is checked while respecting the original fixed group capacity");
        batch.clear();
        require(batch.holder.mGroups.size() == 0 && batch.holder.mGroups.capacity() == 256,
                "retiring metadata empties the holder without changing its fixed capacity");

        AutoEffectInfo reused;
        require(reused.mGroupName == nullptr && reused.mAnimName == nullptr && reused.mUniqueName == nullptr &&
                    reused.mEffectName == nullptr && reused.mParentName == nullptr && reused.mJointName == nullptr &&
                    reused.mFlag == 0 && !reused.mIsValidPrmColor && !reused.mIsValidEnvColor &&
                    static_cast<u32>(reused.mPrmColor) == 0 && static_cast<u32>(reused.mEnvColor) == 0 &&
                    reused.mStartFrame == 0 && reused.mEndFrame == -1 && reused.mScaleValue == 1.0f &&
                    reused.mRateValue == 1.0f && reused.mLightAffectValue == 0.0f && reused.mDrawOrder == 0,
                "original constructor initializes all documented metadata defaults");
        reused.mFlag = 0xa580;
        for (int row = 0; row < map->getNumEntries(); ++row) {
            reused.init(JMapInfoIter(map, row));
            verify_info(reused, raw, row, nullptr, 0xa580);
        }
        for (const auto& [name, first_row] : unique_first_rows) {
            std::unique_ptr<AutoEffectInfo> first;
            std::unique_ptr<AutoEffectInfo> second;
            {
                const JKRHeap::CurrentHeapScope scope(*(metadata));
                const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
                first.reset(MR::Effect::createAutoEffect("unused-first-argument", name.c_str()));
                second.reset(MR::Effect::createAutoEffect("different-unused-first-argument", name.c_str()));
            }
            require(first != nullptr && second != nullptr && first.get() != second.get(),
                    "original UniqueName lookup constructs a fresh metadata record on each call");
            verify_info(*first, raw, first_row);
            verify_info(*second, raw, first_row);
        }
        require(coverage.colors > 0 && coverage.animations > 0 && coverage.parents > 0,
                "the actual fixture covers authored colors, animation bindings, and parent relationships");
        require(metadata_heap->getTotalFreeSize() == metadata_free,
                "all original metadata records, groups and arrays reclaim their actual child heap storage");
        // retain_heap does not own this child: release routing before the
        // scoped actual heap owner destroys it. Its solid parent retires later.
        metadata.reset();
        require(backing.metadata.expired(), "the temporary metadata routing owner retires before its actual heap");
        metadata_heap.reset();
    }
    require(backing.metadata.expired(), "the temporary metadata allocation domain retires within the live process");
    require(!backing.effects.expired() && !backing.source.expired(),
            "temporary metadata retirement leaves the original process resource owner alive");
    std::cout << "records=" << coverage.rows << " groups_case_sensitive=" << exact_count
              << " groups_case_insensitive=" << folded_count << " unique_name_queries=" << unique_count
              << " full_256_group_batches=" << full_batches << " authored_colors=" << coverage.colors
              << " animations=" << coverage.animations << " continued_animations=" << coverage.continued_animations
              << " parents=" << coverage.parents << '\n';
    for (std::size_t i = 0; i < draw_orders.size(); ++i) std::cout << draw_orders[i] << '=' << coverage.draw_counts[i] << ' ';
    std::cout << "\noriginal_auto_effect_metadata=pass case_insensitive_capacity=pass constructor_reinit=pass fresh_unique_lookup=pass metadata_domain_retirement=pass\n";
}
}

int main() {
    Backing backing;
    const auto result = smgpc::test::run_stage_resource_process("original-auto-effect-metadata", [&] { verify_authored(backing); });
    if (result != 0) return result;
    if (!backing.root.expired() || !backing.scene.expired() || !backing.metadata.expired() || !backing.effects.expired() || !backing.source.expired()) {
        std::cerr << "original process effect metadata or native backing survived teardown\n";
        return 1;
    }
    std::cout << "original_auto_effect_process_retirement=pass\n";
    return 0;
}
