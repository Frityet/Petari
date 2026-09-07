#include "Game/Map/NamePosHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapLinkInfo.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/StageResourceBinding.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StagePlacementResolver.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace smgpc;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void put32(std::vector<u8>& data, std::size_t offset, u32 value) {
    for (int i = 0; i < 4; ++i) data[offset + i] = value >> (24 - i * 8);
}
struct Row { const char* name; float x; s32 link = -1; };
JMapInfo positions(std::initializer_list<Row> rows) {
    constexpr std::array fields{"PosName", "pos_x", "pos_y", "pos_z", "dir_x", "dir_y", "dir_z", "MapParts_ID", "Obj_ID", "ChildObjId"};
    constexpr u32 data_start = 16 + fields.size() * 12, stride = fields.size() * 4;
    const auto string_start = data_start + rows.size() * stride;
    std::vector<u8> bytes(string_start);
    put32(bytes, 0, rows.size()); put32(bytes, 4, fields.size()); put32(bytes, 8, data_start); put32(bytes, 12, stride);
    for (std::size_t f = 0; f < fields.size(); ++f) {
        const auto offset = 16 + f * 12;
        put32(bytes, offset, resource::jmap_hash(fields[f])); put32(bytes, offset + 4, 0xffffffff);
        bytes[offset + 9] = f * 4;
        bytes[offset + 11] = f == 0 ? 6 : f < 7 ? 2 : 0;
    }
    std::size_t index = 0;
    for (const auto& row : rows) {
        const auto offset = data_start + index++ * stride;
        put32(bytes, offset, bytes.size() - string_start);
        bytes.insert(bytes.end(), row.name, row.name + std::strlen(row.name) + 1);
        put32(bytes, offset + 4, std::bit_cast<u32>(row.x));
        put32(bytes, offset + 28, row.link); put32(bytes, offset + 32, -1); put32(bytes, offset + 36, -1);
    }
    return JMapInfo::from_bcsv(bytes);
}
JMapInfo linked_object(s32 zone, const char* name, s32 id) {
    std::vector<u8> bytes(32);
    put32(bytes, 0, 1); put32(bytes, 4, 1); put32(bytes, 8, 28); put32(bytes, 12, 4);
    put32(bytes, 16, resource::jmap_hash("l_id")); put32(bytes, 20, 0xffffffff); put32(bytes, 28, id);
    auto result = JMapInfo::from_bcsv(bytes); result.setPlacedZoneId(zone); result.setName(name); return result;
}
scene::StageHolderOccurrence holder(std::size_t id, std::optional<std::size_t> parent, s32 zone) {
    scene::StageHolderOccurrence value;
    value.instance_id = id; value.parent_instance_id = parent; value.zone_id = zone; value.stage_name = "Fixture";
    return value;
}
void cycle(const std::shared_ptr<compat::JkrHeapRuntime>& heaps) {
    runtime::DvdFileSystemService dvd("/");
    std::optional<compat::StageResourceBinding> catalog;
    {
        auto holders = std::vector{holder(0, {}, 0), holder(1, 0, 4), holder(2, 0, 4), holder(3, 1, 9)};
        holders[0].children = {1, 2}; holders[1].children = {3};
        std::vector<scene::StagePlacementTable> tables;
        auto add = [&](std::size_t owner, s32 layer, u32 order, JMapInfo info, const scene::StageZoneTransform& transform = {}) {
            info.setPlacedZoneId(holders[owner].zone_id); info.setName("generalposinfo");
            scene::StagePlacementTable table;
            table.holder_instance_id = owner; table.category = "generalpos"; table.layer_id = layer;
            table.archive_entry_order = order; table.jmap_info = std::move(info); table.zone_transform = transform;
            tables.push_back(std::move(table));
        };
        add(2, 0, 0, positions({{"Repeated", 60}}));
        add(0, 2, 0, positions({{"Higher", 3}}));
        add(3, 0, 0, positions({{"Nested", 50}}));
        add(0, 0, 2, positions({{"Late", 2}}));
        add(1, 0, 0, positions({{"Child", 10}}), scene::StageZoneTransform::from_translation_rotation({100, 0, 0}, {0, 0, 90}));
        add(0, 0, 1, positions({{"Duplicate", 1}, {"Bound", 7, 7}, {"Bound", 8, 7}}));
        catalog.emplace(dvd, holders, tables);
    }
    require(MR::getGeneralPosNum() == 8, "catalog retains every row after input tables and holder descriptions retire");
    const auto root_free = heaps->root_heap().getFreeSize();
    std::weak_ptr<compat::JkrAllocationDomain> weak;
    {
        auto domain = compat::JkrAllocationDomain::create(heaps, 64U << 10); weak = domain;
        SceneObjHolder slots;
        scene::SceneObjHolderBinding binding(slots, nullptr, nullptr, domain);
        auto* owner = dynamic_cast<NamePosHolder*>(MR::createSceneObj(SceneObj_NamePosHolder));
        require(owner && owner == MR::getNamePosHolder() && owner->mPosNum == 8, "factory publishes the actual original NamePosHolder");
        constexpr std::array names{"Duplicate", "Bound", "Bound", "Late", "Higher", "Child", "Nested", "Repeated"};
        for (int i = 0; i < names.size(); ++i) {
            require(std::strcmp(owner->mInfos[i].mName, names[i]) == 0, "original catalog preserves holder recursion, layer, archive and row order");
            require(JKRHeap::findFromRoot(owner->mInfos[i].mLinkInfo) == &domain->heap(), "original per-position link records belong to the real Game domain");
            require(!JKRHeap::findFromRoot(const_cast<char*>(owner->mInfos[i].mName)), "borrowed catalog strings escape Game arena allocation");
        }
        require(JKRHeap::findFromRoot(owner->mInfos) == &domain->heap(), "original position array shares its actual scene allocation owner");
        TVec3f pos, rot;
        require(MR::tryFindNamePos("Child", &pos, &rot) && std::abs(pos.x - 100) < 0.001F && std::abs(pos.y - 10) < 0.001F &&
                std::abs(rot.z - 90) < 0.001F, "general query uses the same real zone transform as native actor placement exactly once");
        NameObj actor("Linked"), other("Other");
        require(!MR::tryFindLinkNamePos(&actor, "Bound", &pos, nullptr), "valid but unregistered authored links do not match arbitrary actors");
        auto wrong_zone = linked_object(4, "mappartsinfo", 7);
        auto wrong_type = linked_object(0, "objinfo", 7);
        auto valid = linked_object(0, "mappartsinfo", 7);
        require(!MR::tryRegisterNamePosLinkObj(&actor, {}) && !MR::tryRegisterNamePosLinkObj(&actor, JMapInfoIter(&wrong_zone, 0)) &&
                !MR::tryRegisterNamePosLinkObj(&actor, JMapInfoIter(&wrong_type, 0)), "invalid and mismatching zone/type links remain unregistered");
        require(MR::tryRegisterNamePosLinkObj(&actor, JMapInfoIter(&valid, 0)) && owner->mInfos[1]._20 == &actor && !owner->mInfos[2]._20,
                "original registration updates only the first matching link record");
        require(MR::tryFindLinkNamePos(&actor, "Bound", &pos, nullptr) && pos.x == 7 &&
                !MR::tryFindLinkNamePos(&other, "Bound", &pos, nullptr), "linked lookup preserves actor identity and first matching name");
        require(MR::tryFindLinkNamePos(&other, "Duplicate", &pos, nullptr) && pos.x == 1,
                "unlinked positions remain visible to every actor as in the original contract");
        pos.set(101, 102, 103); rot.set(201, 202, 203);
        require(!MR::tryFindNamePos("Missing", &pos, &rot) && pos.x == 101 && rot.x == 201, "missing positions leave output vectors unchanged");
        TPos3f matrix; matrix.identity(); matrix.mMtx[0][3] = 4321;
        require(!MR::tryFindNamePos("Missing", matrix.toMtxPtr()) && matrix.mMtx[0][3] == 4321,
                "missing matrix lookup also leaves the destination unchanged");
        require(MR::findNamePos("Child", matrix.toMtxPtr()) && std::abs(matrix.mMtx[0][3] - 100) < 0.001F,
                "original matrix wrapper constructs transform from the actual retained position record");
        bool invalid = false;
        try { (void)catalog->general_position_iter(8); } catch (const std::out_of_range&) { invalid = true; }
        require(invalid, "native catalog rejects out-of-range input instead of inventing a row");
    }
    require(weak.expired() && heaps->root_heap().getFreeSize() == root_free,
            "typed original owner retirement and arena release reclaim arrays and raw link records");
    const char* retained = nullptr;
    catalog->general_position_iter(0).getValue("PosName", &retained);
    require(retained && std::strcmp(retained, "Duplicate") == 0, "catalog strings remain valid after original Game arena retirement");
}
}
int main() {
    try {
        auto heaps = compat::JkrHeapRuntime::create(1U << 20);
        const auto baseline = compat::name_obj_runtime_state_count();
        for (int i = 0; i < 16; ++i) {
            cycle(heaps);
            require(compat::name_obj_runtime_state_count() == baseline, "repeated NamePos ownership returns the native registry to baseline");
        }
        std::cout << "Original NamePos catalog, zone transforms, links and repeated scene ownership passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
