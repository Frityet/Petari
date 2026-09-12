#include "compat/StageResourceBinding.hpp"

#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "camera/CameraAnimation.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/JMapResource.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/exception.hpp>
#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    thread_local smgpc::compat::StageResourceBinding* s_active_binding = nullptr;
}

namespace smgpc::compat {
    struct StageResourceBinding::State {
        struct Zone {
            std::string stage_name;
            s32 zone_id;
            std::size_t holder_id;
            std::shared_ptr<const resource::RarcArchive> archive;
            std::optional<resource::JMapSourceRegistration> camera_registration;
            std::span<const std::uint8_t> camera;
            s32 camera_size = -1;
            bool camera_loaded = false;
        };

        runtime::DvdFileSystemService& dvd;
        std::vector<Zone> zones;
        struct Holder {
            std::vector<JMapInfo> starts;
            std::vector<JMapInfo> paths;
            std::vector<JMapInfo> general_positions;
            std::vector<std::size_t> children;
        };
        std::vector<Holder> catalogs;
        std::size_t root_id = 0;
        std::map<s32, std::optional<camera::NativeCameraAnimationData>> scenario_animations;

        State(runtime::DvdFileSystemService& source,
              std::span<const scene::StageHolderOccurrence> holders,
              std::span<const scene::StagePlacementTable> tables) : dvd(source) {
            const scene::StageHolderOccurrence* root = nullptr;
            for (const auto& holder : holders) {
                if (holder.instance_id >= holders.size() || &holders[holder.instance_id] != &holder) {
                    aurora::throw_host_exception<std::invalid_argument>("Stage resources require ordered holder occurrences.");
                }
                if (!holder.parent_instance_id.has_value()) {
                    if (root != nullptr || holder.zone_id != 0) {
                        aurora::throw_host_exception<std::invalid_argument>("Stage resources require one root holder with zone ID zero.");
                    }
                    root = &holder;
                }
            }
            if (root == nullptr) {
                aurora::throw_host_exception<std::invalid_argument>("Stage resources require a retained root holder.");
            }
            root_id = root->instance_id;
            zones.reserve(root->children.size() + 1);
            zones.push_back({.stage_name = root->stage_name, .zone_id = 0, .holder_id = root_id});
            // Original zone lookup examines only the root and its immediate
            // children, preserving the first occurrence of repeated zones.
            for (const auto child : root->children) {
                if (child >= holders.size() || holders[child].parent_instance_id != root->instance_id) {
                    aurora::throw_host_exception<std::invalid_argument>("Stage resource children have inconsistent ownership.");
                }
                zones.push_back({.stage_name = holders[child].stage_name, .zone_id = holders[child].zone_id, .holder_id = child});
            }
            catalogs.resize(holders.size());
            for (const auto& holder : holders) {
                auto& catalog = catalogs[holder.instance_id];
                catalog.children = holder.children;
                for (const auto child : holder.children) {
                    if (child >= holders.size() || holders[child].parent_instance_id != holder.instance_id) {
                        aurora::throw_host_exception<std::invalid_argument>("Stage catalog children have inconsistent ownership.");
                    }
                }
                std::vector<const scene::StagePlacementTable*> ordered;
                for (const auto& table : tables) {
                    if (table.holder_instance_id == holder.instance_id &&
                        (table.category == "start" || table.category == "path" || table.category == "generalpos")) {
                        ordered.push_back(&table);
                    }
                }
                // StageDataHolder attaches active layers in mask-bit order,
                // then files in the archive directory's original order.
                std::ranges::stable_sort(ordered, [](const auto* left, const auto* right) {
                    if (left->layer_id != right->layer_id) return left->layer_id < right->layer_id;
                    return left->archive_entry_order < right->archive_entry_order;
                });
                for (const auto* table : ordered) {
                    auto& destination = table->category == "start" ? catalog.starts :
                                        table->category == "path" ? catalog.paths : catalog.general_positions;
                    destination.push_back(table->jmap_info);
                    if (table->category == "generalpos" && !root->children.empty()) {
                        // Original isPlacementLocalStage tests the root's child
                        // count. Native readers consume transformed fields.
                        scene::apply_stage_zone_transform(destination.back(), table->zone_transform);
                    }
                }
            }
        }

        const Holder* zone_catalog(s32 zone_id) const {
            for (const auto& zone : zones) {
                if (zone.zone_id == zone_id) return &catalogs[zone.holder_id];
            }
            return nullptr;
        }

        static const JMapInfo* find_path(const Holder& holder, const char* name) {
            for (const auto& path : holder.paths) {
                if (MR::isEqualStringCase(path.getName(), name)) return &path;
            }
            return nullptr;
        }

        s32 count_starts(std::size_t holder_id) const {
            const auto& holder = catalogs[holder_id];
            s32 count = 0;
            for (const auto& table : holder.starts) count += table.getNumEntries();
            for (const auto child : holder.children) count += count_starts(child);
            return count;
        }

        JMapInfoIter start_iter(std::size_t holder_id, int index) const {
            const auto& holder = catalogs[holder_id];
            for (const auto& table : holder.starts) {
                if (index < table.getNumEntries()) return JMapInfoIter(&table, index);
                index -= table.getNumEntries();
            }
            for (const auto child : holder.children) {
                const auto count = count_starts(child);
                if (index < count) return start_iter(child, index);
                index -= count;
            }
            return JMapInfoIter();
        }

        s32 count_general_positions(std::size_t holder_id) const {
            const auto& holder = catalogs[holder_id];
            s32 count = 0;
            for (const auto& table : holder.general_positions) count += table.getNumEntries();
            for (const auto child : holder.children) count += count_general_positions(child);
            return count;
        }

        JMapInfoIter general_position_iter(std::size_t holder_id, int index) const {
            const auto& holder = catalogs[holder_id];
            for (const auto& table : holder.general_positions) {
                if (index < table.getNumEntries()) return JMapInfoIter(&table, index);
                index -= table.getNumEntries();
            }
            for (const auto child : holder.children) {
                const auto count = count_general_positions(child);
                if (index < count) return general_position_iter(child, index);
                index -= count;
            }
            return JMapInfoIter();
        }

        JMapInfoIter mario_start_iter(const JMapIdInfo& start_id) const {
            const auto* holder = zone_catalog(start_id.mZoneID);
            if (holder == nullptr) {
                aurora::throw_host_exception<std::logic_error>("Mario start lookup requires a placed zone.");
            }
            for (const auto& table : holder->starts) {
                const auto iter = table.findElement<s32>("MarioNo", start_id._0, 0);
                if (!(iter == table.end())) return iter;
            }
            return JMapInfoIter();
        }

        void retain_archive(Zone& zone) {
            if (zone.archive) return;
            const auto path = dvd.find_first({std::filesystem::path("StageData") / (zone.stage_name + ".arc")});
            if (!path.has_value()) {
                aurora::throw_host_exception<std::runtime_error>("Placed stage archive is absent: " + zone.stage_name);
            }
            zone.archive = dvd.retain_archive_for_path(*path);
        }

        void rail_at_index(JMapInfoIter* path, const JMapInfo** points, int index, s32 zone_id) const {
            const auto* holder = zone_catalog(zone_id);
            if (holder == nullptr) {
                aurora::throw_host_exception<std::logic_error>("Camera rail queries require a placed zone.");
            }
            const auto* table = find_path(*holder, "CommonPathInfo");
            if (table == nullptr || index < 0 || index >= table->getNumEntries()) {
                aurora::throw_host_exception<std::out_of_range>("Camera rail index does not identify an authored row.");
            }
            char name[128];
            std::snprintf(name, sizeof(name), "CommonPathPointInfo.%d", index);
            *points = find_path(*holder, name);
            *path = JMapInfoIter(table, index);
        }

        void camera_data(void** data, s32* size, s32 zone_id) {
            JkrHostAllocationScope host;
            for (auto& zone : zones) {
                if (zone.zone_id != zone_id) {
                    continue;
                }
                if (!zone.camera_loaded) {
                    retain_archive(zone);
                    if (const auto* entry = zone.archive->find_resource("CameraParam.bcam")) {
                        zone.camera = zone.archive->file_data(*entry);
                        if (zone.camera.size() > static_cast<std::size_t>(std::numeric_limits<s32>::max())) {
                            aurora::throw_host_exception<std::runtime_error>("Placed stage camera resource has invalid bounds: " + zone.stage_name);
                        }
                        if (!zone.camera.empty()) {
                            zone.camera_registration.emplace(resource::register_jmap_source(zone.camera, zone.archive));
                        }
                        zone.camera_size = static_cast<s32>(zone.camera.size());
                    }
                    zone.camera_loaded = true;
                }
                // JKRArchive::getResSize returns -1 for an absent resource;
                // only an unplaced zone takes SceneUtil's explicit zero path.
                *data = zone.camera_size < 0 ? nullptr : const_cast<std::uint8_t*>(zone.camera.data());
                *size = zone.camera_size;
                return;
            }
            *data = nullptr;
            *size = 0;
        }
    };

    StageResourceBinding::StageResourceBinding(runtime::DvdFileSystemService& dvd,
                                               std::span<const scene::StageHolderOccurrence> holders,
                                               std::span<const scene::StagePlacementTable> tables) {
        JkrHostAllocationScope host;
        _state = std::make_unique<State>(dvd, holders, tables);
        _previous = s_active_binding;
        s_active_binding = this;
    }

    StageResourceBinding::~StageResourceBinding() {
        if (s_active_binding != this) {
            std::terminate();
        }
        s_active_binding = _previous;
    }

    void StageResourceBinding::camera_data(void** data, s32* size, s32 zone_id) {
        _state->camera_data(data, size, zone_id);
    }

    s32 StageResourceBinding::start_count() const {
        return _state->count_starts(_state->root_id);
    }

    s32 StageResourceBinding::general_position_count() const {
        return _state->count_general_positions(_state->root_id);
    }

    JMapInfoIter StageResourceBinding::general_position_iter(int index) const {
        if (index < 0 || index >= general_position_count()) {
            aurora::throw_host_exception<std::out_of_range>("General position index does not identify an authored row.");
        }
        return _state->general_position_iter(_state->root_id, index);
    }

    void StageResourceBinding::start_camera_id(JMapIdInfo* output, int index) const {
        const auto iter = _state->start_iter(_state->root_id, index);
        s32 camera_id;
        if (!iter.getValue("Camera_id", &camera_id)) {
            aurora::throw_host_exception<std::out_of_range>("Start camera query does not identify an authored Camera_id.");
        }
        output->initialize(camera_id, iter);
    }

    s32 StageResourceBinding::rail_count(s32 zone_id) const {
        const auto* holder = _state->zone_catalog(zone_id);
        if (holder == nullptr) return 0;
        const auto* table = State::find_path(*holder, "CommonPathInfo");
        if (table == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Placed zone has no authored CommonPathInfo table.");
        }
        return table->getNumEntries();
    }

    void StageResourceBinding::rail_by_id(JMapInfoIter* path, const JMapInfo** points, s32 rail_id, s32 zone_id) const {
        const auto* holder = _state->zone_catalog(zone_id);
        const auto* table = holder != nullptr ? State::find_path(*holder, "CommonPathInfo") : nullptr;
        if (table == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Camera rail ID lookup requires an authored path table.");
        }
        const auto iter = table->findElement<s32>("l_id", rail_id, 0);
        _state->rail_at_index(path, points, iter.mIndex, zone_id);
    }

    bool StageResourceBinding::camera_rail_by_index(JMapInfoIter* path, const JMapInfo** points, int index, s32 zone_id) const {
        _state->rail_at_index(path, points, index, zone_id);
        return MR::isEqualRailUsage(*path, "Camera");
    }

    s32 StageResourceBinding::start_zone(const JMapIdInfo& start_id) const {
        const auto iter = _state->mario_start_iter(start_id);
        if (iter.mInfo == nullptr) {
            aurora::throw_host_exception<std::out_of_range>("Current Mario start does not identify an authored row.");
        }
        return iter.mInfo->getPlacedZoneId();
    }

    s32 StageResourceBinding::start_camera(const JMapIdInfo& start_id) const {
        const auto iter = _state->mario_start_iter(start_id);
        s32 camera_id;
        return iter.getValue("Camera_id", &camera_id) ? camera_id : -1;
    }

    void StageResourceBinding::scenario_start_camera(void** data, s32* size, s32 scenario_no) {
        JkrHostAllocationScope host;
        auto found = _state->scenario_animations.find(scenario_no);
        if (found == _state->scenario_animations.end()) {
            auto& root = _state->zones.front();
            _state->retain_archive(root);
            char name[64];
            std::snprintf(name, sizeof(name), "StartScenario%d.canm", scenario_no);
            std::optional<camera::NativeCameraAnimationData> native;
            if (const auto* entry = root.archive->find_resource(name)) {
                // Original CameraAnim dereferences native u32/f32 fields.
                // Decode once at the archive boundary and retain its aligned
                // block for every camera borrowing it during this stage.
                native = camera::CameraAnimation::from_bytes(root.archive->file_data(*entry)).native_data();
            }
            found = _state->scenario_animations.emplace(scenario_no, std::move(native)).first;
        }
        if (!found->second.has_value()) {
            *data = nullptr;
            *size = 0;
            return;
        }
        const auto bytes = found->second->bytes();
        if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<s32>::max())) {
            aurora::throw_host_exception<std::runtime_error>("Stage animation camera resource exceeds original address bounds.");
        }
        *data = const_cast<std::uint8_t*>(bytes.data());
        *size = static_cast<s32>(bytes.size());
    }

    StageResourceBinding& require_stage_resources() {
        if (s_active_binding == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Stage resource queries require an active stage owner.");
        }
        return *s_active_binding;
    }
}
