#include "Game/NameObj/NameObj.hpp"
#include "scene/AuthoredPlacementInstantiator.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void write_be32(std::vector<std::uint8_t> &bytes,
                    std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    [[nodiscard]] JMapInfo make_fieldless_jmap() {
        auto bytes = std::vector<std::uint8_t>(0x10U, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x08U, 0x10U);
        return JMapInfo::from_bcsv(bytes);
    }

    [[nodiscard]] smgpc::scene::StagePlacementObject make_placement(
        std::string object_name, std::string layer_name,
        std::string table_name, bool factory_supported,
        bool intentionally_ignored = false) {
        auto placement = smgpc::scene::StagePlacementObject{};
        placement.object_name = std::move(object_name);
        placement.stage_name = "SyntheticStage";
        placement.zone_name = "SyntheticZone";
        placement.layer_name = std::move(layer_name);
        placement.table_name = std::move(table_name);
        placement.table_path = "jmp/placement/" + placement.layer_name +
                               "/" + placement.table_name;
        placement.l_id = 10;
        placement.zone_id = 3;
        placement.jmap_info = make_fieldless_jmap();
        placement.jmap_info.setPlacedZoneId(placement.zone_id);
        placement.jmap_entry_index = 0;
        placement.factory_supported = factory_supported;
        placement.intentionally_ignored = intentionally_ignored;
        placement.support_reason = intentionally_ignored ? "synthetic_non_actor" : factory_supported ? "synthetic_factory" :
                                                                                                       "synthetic_blocker";
        return placement;
    }

    [[nodiscard]] smgpc::scene::StageAuthoredData make_data(
        std::vector<smgpc::scene::StagePlacementObject> placements) {
        return smgpc::scene::StageAuthoredData(
            "SyntheticStage", 1, {}, std::move(placements), {},
            std::nullopt);
    }

    class SyntheticNameObj final : public NameObj {
    public:
        explicit SyntheticNameObj(const char *name) : NameObj(name) {
        }
    };

    class RecordingLifecycle final
        : public smgpc::scene::AuthoredPlacementLifecycle {
    public:
        std::vector<smgpc::scene::nameobj::NameObjArchiveRequest>
        preload_archives(
            std::string_view object_name,
            const smgpc::scene::NameObjPlacementContext &placement) override {
            events.push_back("preload:" + std::string(object_name));
            iter_infos.push_back(placement.iter.mInfo);
            iter_indices.push_back(placement.iter.mIndex);
            return {
                smgpc::scene::nameobj::NameObjArchiveRequest{
                    .archive_name = std::string(object_name) + "Archive",
                    .disc_path = "ObjectData/" + std::string(object_name) +
                                 ".arc",
                    .resolved_path = "/synthetic/" +
                                     std::string(object_name) + ".arc",
                    .kind = smgpc::scene::nameobj::NameObjArchiveKind::Object,
                    .loaded = object_name != unloaded_object,
                },
            };
        }

        std::unique_ptr<NameObj> construct_and_init(
            std::string_view object_name, const char *actor_name,
            const smgpc::scene::NameObjPlacementContext &placement) override {
            require(placement.iter.isValid(),
                    "construct received an invalid retained JMap iterator");
            events.push_back("construct:" + std::string(object_name));
            return std::make_unique<SyntheticNameObj>(actor_name);
        }

        void init_after_placement(NameObj &object) override {
            events.push_back("after:" + std::string(object.getName()));
        }

        void destroy(NameObj &object) override {
            events.push_back("destroy:" + std::string(object.getName()));
        }

        std::string unloaded_object{};
        std::vector<std::string> events{};
        std::vector<const JMapInfo *> iter_infos{};
        std::vector<s32> iter_indices{};
    };

    [[nodiscard]] const smgpc::scene::AuthoredPlacementReportEntry &
    require_entry(
        const smgpc::scene::AuthoredPlacementInstantiationReport &report,
        std::string_view object_name) {
        const auto found = std::ranges::find_if(
            report.entries, [&](const auto &entry) {
                return entry.placement != nullptr &&
                       entry.placement->object_name == object_name;
            });
        require(found != report.entries.end(),
                "placement report omitted a synthetic source row");
        return *found;
    }

    void test_strict_preflight_is_non_mutating() {
        auto data = make_data({
            make_placement("Ready", "layera", "ObjInfo", true),
            make_placement("Blocked", "common", "ObjInfo", false),
            make_placement("Ignored", "common", "DemoObjInfo", false,
                           true),
        });
        auto lifecycle = RecordingLifecycle{};
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle,
            smgpc::scene::AuthoredPlacementInstantiationOptions{
                .mode = smgpc::scene::AuthoredPlacementMode::Strict,
                .actor_name_resolver = [](const auto &placement) {
                    return std::optional<std::string>(
                        "actor:" + placement.object_name);
                },
            });

        auto rejected = false;
        try {
            (void)instantiator.instantiate();
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        const auto &report = instantiator.report();
        require(rejected && lifecycle.events.empty() &&
                    !report.preflight_accepted && report.ready_count == 1U &&
                    report.blocked_count == 1U &&
                    report.ignored_count == 1U &&
                    report.created_count == 0U &&
                    report.state ==
                        smgpc::scene::AuthoredPlacementRuntimeState::Failed,
                "strict preflight mutated lifecycle state or lost blocker accounting");

        const auto context = data.placement_context(0U);
        require(context.iter.isValid() &&
                    context.iter.mInfo == &data.placements()[0U].jmap_info &&
                    context.iter.mIndex == 0,
                "StageAuthoredData did not retain its copied JMap row");
    }

    void test_development_subset_preserves_retail_order_and_lifecycle() {
        auto data = make_data({
            make_placement("LateLayer", "layera", "ObjInfo", true),
            make_placement("HighLayer", "layera", "PlanetObjInfo", true),
            make_placement("CommonNormal", "common", "ObjInfo", true),
            make_placement("HighCommon", "common", "CameraCubeInfo", true),
            make_placement("BlockedCommon", "common", "AreaObjInfo", false),
            make_placement("IgnoredDemo", "common", "DemoObjInfo", false,
                           true),
        });
        auto lifecycle = RecordingLifecycle{};
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle,
            smgpc::scene::AuthoredPlacementInstantiationOptions{
                .mode = smgpc::scene::AuthoredPlacementMode::
                    SupportedSubsetForDevelopment,
                .actor_name_resolver = [](const auto &placement) {
                    return std::optional<std::string>(
                        "actor:" + placement.object_name);
                },
            });

        const auto &created = instantiator.instantiate();
        require(created.preflight_accepted && created.ready_count == 4U &&
                    created.blocked_count == 1U &&
                    created.ignored_count == 1U &&
                    created.created_count == 4U &&
                    instantiator.instances().size() == 4U,
                "development subset did not report the complete authored set");

        const auto expected_create_events = std::vector<std::string>{
            "preload:HighCommon",
            "construct:HighCommon",
            "preload:HighLayer",
            "construct:HighLayer",
            "preload:CommonNormal",
            "construct:CommonNormal",
            "preload:LateLayer",
            "construct:LateLayer",
        };
        require(lifecycle.events == expected_create_events,
                "placement construction did not preserve retail phase order");

        for (auto index = std::size_t{};
             index < instantiator.instances().size(); ++index) {
            const auto &instance = instantiator.instances()[index];
            require(lifecycle.iter_infos[index] ==
                            &instance.placement->jmap_info &&
                        lifecycle.iter_indices[index] ==
                            instance.placement->jmap_entry_index,
                    "archive preload did not consume the retained placement JMap row");
            const auto &entry = require_entry(
                created, instance.placement->object_name);
            require(entry.actor == instance.actor &&
                        entry.actor_name.has_value() &&
                        std::string_view(instance.actor->getName()) ==
                            *entry.actor_name &&
                        entry.archives.size() == 1U &&
                        entry.archives.front().loaded,
                    "created actor lost stable actor-name or archive evidence");
        }

        const auto &after = instantiator.init_after_placement();
        require(after.initialized_after_placement_count == 4U &&
                    after.state == smgpc::scene::
                                       AuthoredPlacementRuntimeState::
                                           InitializedAfterPlacement,
                "shared post-placement lifecycle pass was incomplete");
        require(lifecycle.events ==
                    std::vector<std::string>{
                        "preload:HighCommon",
                        "construct:HighCommon",
                        "preload:HighLayer",
                        "construct:HighLayer",
                        "preload:CommonNormal",
                        "construct:CommonNormal",
                        "preload:LateLayer",
                        "construct:LateLayer",
                        "after:actor:HighCommon",
                        "after:actor:HighLayer",
                        "after:actor:CommonNormal",
                        "after:actor:LateLayer",
                    },
                "initAfterPlacement did not run after the full construction pass");

        instantiator.clear();
        require(instantiator.instances().empty() &&
                    instantiator.report().destroyed_count == 4U &&
                    instantiator.report().state ==
                        smgpc::scene::AuthoredPlacementRuntimeState::Cleared &&
                    require_entry(instantiator.report(), "BlockedCommon")
                            .outcome ==
                        smgpc::scene::AuthoredPlacementOutcome::Blocked &&
                    require_entry(instantiator.report(), "IgnoredDemo")
                            .outcome ==
                        smgpc::scene::AuthoredPlacementOutcome::Ignored,
                "placement teardown lost skipped-row report state");
        require(lifecycle.events ==
                    std::vector<std::string>{
                        "preload:HighCommon",
                        "construct:HighCommon",
                        "preload:HighLayer",
                        "construct:HighLayer",
                        "preload:CommonNormal",
                        "construct:CommonNormal",
                        "preload:LateLayer",
                        "construct:LateLayer",
                        "after:actor:HighCommon",
                        "after:actor:HighLayer",
                        "after:actor:CommonNormal",
                        "after:actor:LateLayer",
                        "destroy:actor:LateLayer",
                        "destroy:actor:CommonNormal",
                        "destroy:actor:HighLayer",
                        "destroy:actor:HighCommon",
                    },
                "normal placement teardown was not reverse construction order");
    }

    void test_archive_failure_cleans_previously_created_actors() {
        auto data = make_data({
            make_placement("First", "common", "ObjInfo", true),
            make_placement("Second", "common", "ObjInfo", true),
            make_placement("Unloaded", "layera", "ObjInfo", true),
        });
        auto lifecycle = RecordingLifecycle{};
        lifecycle.unloaded_object = "Unloaded";
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle,
            smgpc::scene::AuthoredPlacementInstantiationOptions{
                .mode = smgpc::scene::AuthoredPlacementMode::Strict,
                .actor_name_resolver = [](const auto &placement) {
                    return std::optional<std::string>(
                        "actor:" + placement.object_name);
                },
            });

        auto failed = false;
        try {
            (void)instantiator.instantiate();
        } catch (const std::runtime_error &) {
            failed = true;
        }
        require(failed && instantiator.instances().empty() &&
                    instantiator.report().destroyed_count == 2U &&
                    require_entry(instantiator.report(), "Unloaded")
                            .outcome ==
                        smgpc::scene::AuthoredPlacementOutcome::Failed,
                "archive failure left an earlier authored actor alive");
        require(lifecycle.events ==
                    std::vector<std::string>{
                        "preload:First",
                        "construct:First",
                        "preload:Second",
                        "construct:Second",
                        "preload:Unloaded",
                        "destroy:actor:Second",
                        "destroy:actor:First",
                    },
                "archive rollback was not reverse construction order");
    }

}  // namespace

int main() {
    try {
        test_strict_preflight_is_non_mutating();
        test_development_subset_preserves_retail_order_and_lifecycle();
        test_archive_failure_cleans_previously_created_actors();
        std::cout << "[ok] shared authored placement instantiator contract passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] authored placement instantiator: "
                  << error.what() << '\n';
        return 1;
    }
}
