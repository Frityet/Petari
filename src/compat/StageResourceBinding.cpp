#include "compat/StageResourceBinding.hpp"

#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/JMapResource.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/exception.hpp>
#include <exception>
#include <filesystem>
#include <limits>
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
            std::shared_ptr<const resource::RarcArchive> archive;
            std::optional<resource::JMapSourceRegistration> camera_registration;
            std::span<const std::uint8_t> camera;
            s32 camera_size = -1;
            bool camera_loaded = false;
        };

        runtime::DvdFileSystemService& dvd;
        std::vector<Zone> zones;

        State(runtime::DvdFileSystemService& source,
              std::span<const scene::StageHolderOccurrence> holders) : dvd(source) {
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
            zones.reserve(root->children.size() + 1);
            zones.push_back({.stage_name = root->stage_name, .zone_id = 0});
            // Original zone lookup examines only the root and its immediate
            // children, preserving the first occurrence of repeated zones.
            for (const auto child : root->children) {
                if (child >= holders.size() || holders[child].parent_instance_id != root->instance_id) {
                    aurora::throw_host_exception<std::invalid_argument>("Stage resource children have inconsistent ownership.");
                }
                zones.push_back({.stage_name = holders[child].stage_name, .zone_id = holders[child].zone_id});
            }
        }

        void camera_data(void** data, s32* size, s32 zone_id) {
            JkrHostAllocationScope host;
            for (auto& zone : zones) {
                if (zone.zone_id != zone_id) {
                    continue;
                }
                if (!zone.camera_loaded) {
                    const auto path = dvd.find_first({std::filesystem::path("StageData") / (zone.stage_name + ".arc")});
                    if (!path.has_value()) {
                        aurora::throw_host_exception<std::runtime_error>("Placed stage archive is absent: " + zone.stage_name);
                    }
                    zone.archive = dvd.retain_archive_for_path(*path);
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
                                               std::span<const scene::StageHolderOccurrence> holders) {
        JkrHostAllocationScope host;
        _state = std::make_unique<State>(dvd, holders);
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

    StageResourceBinding& require_stage_resources() {
        if (s_active_binding == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Stage resource queries require an active stage owner.");
        }
        return *s_active_binding;
    }
}

namespace MR {
    s32 getZoneNum() {
        return makeCurrentGalaxyStatusAccessor().getZoneNum();
    }

    void getStageCameraData(void** data, s32* size, s32 zone_id) {
        smgpc::compat::require_stage_resources().camera_data(data, size, zone_id);
    }
}
