#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Map/SwitchWatcher.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/AuthoredPlacementInstantiator.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    using smgpc::scene::AuthoredPlacementGroupLoadOrder;
    using smgpc::scene::AuthoredPlacementRetailPass;
    using smgpc::scene::StagePlacementLoadBatch;

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

    void write_be16(std::vector<std::uint8_t> &bytes,
                    std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    [[nodiscard]] JMapInfo make_fieldless_jmap(std::size_t entry_count) {
        auto bytes = std::vector<std::uint8_t>(0x10U, 0U);
        write_be32(bytes, 0x00U, static_cast<std::uint32_t>(entry_count));
        write_be32(bytes, 0x08U, 0x10U);
        return JMapInfo::from_bcsv(bytes);
    }

    [[nodiscard]] JMapInfo make_string_jmap(
        std::string_view field_name,
        std::span<const std::string_view> values) {
        constexpr auto field_count = std::size_t{1U};
        constexpr auto entry_size = std::size_t{4U};
        constexpr auto data_offset =
            std::size_t{0x10U + field_count * 0x0cU};
        auto string_bytes = std::size_t{};
        for (const auto value : values) {
            string_bytes += value.size() + 1U;
        }
        const auto string_offset = data_offset + values.size() * entry_size;
        auto bytes =
            std::vector<std::uint8_t>(string_offset + string_bytes, 0U);
        write_be32(bytes, 0x00U,
                   static_cast<std::uint32_t>(values.size()));
        write_be32(bytes, 0x04U,
                   static_cast<std::uint32_t>(field_count));
        write_be32(bytes, 0x08U,
                   static_cast<std::uint32_t>(data_offset));
        write_be32(bytes, 0x0cU,
                   static_cast<std::uint32_t>(entry_size));
        write_be32(bytes, 0x10U,
                   smgpc::resource::jmap_hash(field_name));
        write_be32(bytes, 0x14U, 0xffffffffU);
        write_be16(bytes, 0x18U, 0U);
        bytes[0x1aU] = 0U;
        bytes[0x1bU] = static_cast<std::uint8_t>(
            smgpc::resource::BcsvFieldType::StringOffset);

        auto next_string = std::size_t{};
        for (auto index = std::size_t{}; index < values.size(); ++index) {
            write_be32(
                bytes, data_offset + index * entry_size,
                static_cast<std::uint32_t>(next_string));
            std::copy(
                values[index].begin(), values[index].end(),
                bytes.begin() + static_cast<std::ptrdiff_t>(
                                    string_offset + next_string));
            next_string += values[index].size() + 1U;
        }
        return JMapInfo::from_bcsv(bytes);
    }

    [[nodiscard]] JMapInfo make_name_type_jmap(
        std::string_view raw_name, std::string_view type_name) {
        constexpr auto field_count = std::size_t{2U};
        constexpr auto entry_size = std::size_t{8U};
        constexpr auto data_offset =
            std::size_t{0x10U + field_count * 0x0cU};
        constexpr auto string_offset = data_offset + entry_size;
        auto bytes = std::vector<std::uint8_t>(
            string_offset + raw_name.size() + 1U + type_name.size() + 1U,
            0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, static_cast<std::uint32_t>(field_count));
        write_be32(bytes, 0x08U, static_cast<std::uint32_t>(data_offset));
        write_be32(bytes, 0x0cU, static_cast<std::uint32_t>(entry_size));
        const auto write_string_field = [&](std::size_t field_index,
                                            std::string_view field_name,
                                            std::uint16_t field_offset) {
            const auto descriptor = 0x10U + field_index * 0x0cU;
            write_be32(bytes, descriptor,
                       smgpc::resource::jmap_hash(field_name));
            write_be32(bytes, descriptor + 4U, 0xffffffffU);
            write_be16(bytes, descriptor + 8U, field_offset);
            bytes[descriptor + 10U] = 0U;
            bytes[descriptor + 11U] = static_cast<std::uint8_t>(
                smgpc::resource::BcsvFieldType::StringOffset);
        };
        write_string_field(0U, "name", 0U);
        write_string_field(1U, "type", 4U);
        write_be32(bytes, data_offset, 0U);
        write_be32(bytes, data_offset + 4U,
                   static_cast<std::uint32_t>(raw_name.size() + 1U));
        std::copy(raw_name.begin(), raw_name.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(string_offset));
        std::copy(
            type_name.begin(), type_name.end(),
            bytes.begin() + static_cast<std::ptrdiff_t>(
                                string_offset + raw_name.size() + 1U));
        return JMapInfo::from_bcsv(bytes);
    }

    [[nodiscard]] smgpc::scene::StagePlacementTable make_table(
        s32 zone_id, s32 layer_id, std::string category,
        std::string table_name, std::string table_path,
        u32 archive_entry_order, JMapInfo info = make_fieldless_jmap(0U)) {
        info.setName(table_name.c_str());
        info.setPlacedZoneId(zone_id);
        return smgpc::scene::StagePlacementTable{
            .stage_name = "SyntheticStage",
            .zone_name = "SyntheticZone",
            .category = std::move(category),
            .layer_name = layer_id == 0 ? "common" : "layera",
            .table_name = std::move(table_name),
            .archive_path = "Synthetic.arc",
            .table_path = std::move(table_path),
            .jmap_info = std::move(info),
            .zone_id = zone_id,
            .layer_id = layer_id,
            .layer_mask = 3U,
            .archive_entry_order = archive_entry_order,
        };
    }

    [[nodiscard]] smgpc::scene::StagePlacementObject make_placement(
        std::string raw_name, StagePlacementLoadBatch load_batch,
        std::string table_name, bool factory_supported,
        std::size_t attachment_order, s32 local_id,
        s32 shape_model_no = -1,
        std::optional<std::string> creator_identifier = std::nullopt,
        bool intentionally_ignored = false, s32 row = 0) {
        auto placement = smgpc::scene::StagePlacementObject{};
        placement.object_name = std::move(raw_name);
        placement.creator_identifier = creator_identifier.has_value()
                                             ? *creator_identifier
                                             : placement.object_name;
        if (creator_identifier.has_value()) {
            placement.type_name = *creator_identifier;
        }
        placement.stage_name = "SyntheticStage";
        placement.zone_name = "SyntheticZone";
        placement.category = "placement";
        placement.layer_name =
            load_batch == StagePlacementLoadBatch::CommonBootstrap
                ? "common"
                : "layera";
        placement.table_name = std::move(table_name);
        placement.table_path = "jmp/placement/" + placement.layer_name +
                               "/" + placement.table_name;
        placement.l_id = local_id;
        placement.zone_id = 3;
        placement.load_batch = load_batch;
        placement.placement_attachment_order = attachment_order;
        placement.shape_model_no = shape_model_no;
        placement.jmap_info = make_fieldless_jmap(
            static_cast<std::size_t>(row) + 1U);
        placement.jmap_info.setPlacedZoneId(placement.zone_id);
        placement.jmap_entry_index = row;
        placement.factory_supported = factory_supported;
        placement.intentionally_ignored = intentionally_ignored;
        placement.support_reason = intentionally_ignored
                                       ? "synthetic_non_actor"
                                       : factory_supported
                                             ? "synthetic_factory"
                                             : "synthetic_blocker";
        return placement;
    }

    [[nodiscard]] smgpc::scene::StageAuthoredData make_data(
        std::vector<smgpc::scene::StagePlacementObject> placements) {
        return smgpc::scene::StageAuthoredData(
            "SyntheticStage", 1, {}, std::move(placements), {},
            std::nullopt);
    }

    [[nodiscard]] std::string event_name(
        std::string_view prefix, std::string_view object_name,
        s32 local_id) {
        return std::string(prefix) + ':' + std::string(object_name) + '@' +
               std::to_string(local_id);
    }

    class SyntheticNameObj final : public NameObj {
    public:
        SyntheticNameObj(
            const char *name,
            std::vector<std::string> *destructor_events)
            : NameObj(name != nullptr ? name : "synthetic:null"),
              _destructor_events(destructor_events) {
        }

        ~SyntheticNameObj() override {
            if (_destructor_events != nullptr) {
                _destructor_events->push_back(
                    "delete:" + std::string(getName()));
            }
        }

    private:
        std::vector<std::string> *_destructor_events = nullptr;
    };

    class SyntheticDescendantRoot final : public NameObj {
    public:
        SyntheticDescendantRoot(
            const char *name,
            std::vector<std::string> *destructor_events)
            : NameObj(name != nullptr ? name : "synthetic:null"),
              _destructor_events(destructor_events) {
            (void)new SyntheticNameObj(
                "constructor-descendant", _destructor_events);
        }

        ~SyntheticDescendantRoot() override {
            if (_destructor_events != nullptr) {
                _destructor_events->push_back(
                    "delete:" + std::string(getName()));
            }
        }

        void create_init_descendant() {
            (void)new SyntheticNameObj(
                "init-descendant", _destructor_events);
        }

    private:
        std::vector<std::string> *_destructor_events = nullptr;
    };

    struct SyntheticSceneObjContext final {
        std::vector<std::string> events{};
        bool throw_outer_init = false;
        bool return_null_after_nested = false;
        bool create_nested_in_postpass = false;
    };

    class SyntheticNestedSceneObjB final : public NameObj {
    public:
        explicit SyntheticNestedSceneObjB(
            SyntheticSceneObjContext &context)
            : NameObj("nested-scene-b"), _context(&context) {
            _context->events.push_back("construct:B");
        }

        ~SyntheticNestedSceneObjB() override {
            _context->events.push_back("delete:B");
        }

        void init(const JMapInfoIter &) override {
            _context->events.push_back("init:B");
        }

        void initAfterPlacement() override {
            _context->events.push_back("after:B");
        }

    private:
        SyntheticSceneObjContext *_context;
    };

    class SyntheticNestedSceneObjA final : public NameObj {
    public:
        explicit SyntheticNestedSceneObjA(
            SyntheticSceneObjContext &context)
            : NameObj("nested-scene-a"), _context(&context) {
            _context->events.push_back("construct:A");
        }

        ~SyntheticNestedSceneObjA() override {
            _context->events.push_back("delete:A");
        }

        void init(const JMapInfoIter &) override {
            _context->events.push_back("init:A");
            if (!_context->create_nested_in_postpass) {
                require(MR::createSceneObj(SceneObj_35) != nullptr,
                        "recursive synthetic SceneObj B was not created");
            }
            if (_context->throw_outer_init) {
                throw std::runtime_error(
                    "synthetic outer SceneObj init failure");
            }
        }

        void initAfterPlacement() override {
            _context->events.push_back("after:A");
            if (_context->create_nested_in_postpass) {
                require(MR::createSceneObj(SceneObj_35) != nullptr,
                        "postpass synthetic SceneObj B was not created");
            }
        }

    private:
        SyntheticSceneObjContext *_context;
    };

    class SyntheticSwitchWatcherHolder final
        : public SwitchWatcherHolder {
    public:
        explicit SyntheticSwitchWatcherHolder(
            SyntheticSceneObjContext &context)
            : _context(&context) {
            _context->events.push_back("construct:watcher-holder");
        }

        ~SyntheticSwitchWatcherHolder() override {
            _context->events.push_back("delete:watcher-holder");
        }

        void initAfterPlacement() override {
            _context->events.push_back("after:watcher-holder");
        }

    private:
        SyntheticSceneObjContext *_context;
    };

    class SyntheticSwitchWatcher final : public SwitchWatcher {
    public:
        explicit SyntheticSwitchWatcher(
            SyntheticSceneObjContext &context)
            : SwitchWatcher(nullptr), _context(&context) {
            _context->events.push_back("construct:watcher");
        }

        ~SyntheticSwitchWatcher() override {
            _context->events.push_back("delete:watcher");
        }

        void initAfterPlacement() override {
            _context->events.push_back("after:watcher");
        }

    private:
        SyntheticSceneObjContext *_context;
    };

    [[nodiscard]] NameObj *synthetic_scene_obj_factory(
        int id, void *raw_context) {
        auto &context = *static_cast<SyntheticSceneObjContext *>(
            raw_context);
        switch (id) {
        case SceneObj_15:
            if (context.return_null_after_nested) {
                require(MR::createSceneObj(SceneObj_35) != nullptr,
                        "null synthetic SceneObj factory could not create its nested object");
                return nullptr;
            }
            return new SyntheticNestedSceneObjA(context);
        case SceneObj_35:
            return new SyntheticNestedSceneObjB(context);
        case SceneObj_SwitchWatcherHolder:
            return new SyntheticSwitchWatcherHolder(context);
        default:
            return nullptr;
        }
    }

    class RecordingLifecycle final
        : public smgpc::scene::AuthoredPlacementLifecycle {
    public:
        std::vector<smgpc::scene::nameobj::NameObjArchiveRequest>
        preload_archives(
            std::string_view object_name,
            const smgpc::scene::NameObjPlacementContext &placement) override {
            events.push_back(
                event_name("preload", object_name, placement.local_id));
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
                    .loaded = !unloaded_object.has_value() ||
                              object_name != *unloaded_object,
                },
            };
        }

        std::vector<smgpc::scene::nameobj::NameObjArchiveRequest>
        preload_model_changing_archive(
            std::string_view object_name, s32 shape_model_no,
            const smgpc::scene::NameObjPlacementContext &placement) override {
            auto model_name = std::array<char, 128U>{};
            const auto identifier = std::string(object_name);
            const auto written = std::snprintf(
                model_name.data(), model_name.size(), "%s%02d",
                identifier.c_str(), shape_model_no);
            require(written >= 0 &&
                        static_cast<std::size_t>(written) < model_name.size(),
                    "synthetic model-changing identifier overflowed");
            events.push_back(event_name(
                "preload-shaped", model_name.data(), placement.local_id));
            iter_infos.push_back(placement.iter.mInfo);
            iter_indices.push_back(placement.iter.mIndex);
            return {
                smgpc::scene::nameobj::NameObjArchiveRequest{
                    .archive_name = model_name.data(),
                    .disc_path = "ObjectData/" +
                                 std::string(model_name.data()) + ".arc",
                    .resolved_path = "/synthetic/" +
                                     std::string(model_name.data()) + ".arc",
                    .kind = smgpc::scene::nameobj::NameObjArchiveKind::Object,
                    .loaded = !unloaded_object.has_value() ||
                              std::string_view(model_name.data()) !=
                                  *unloaded_object,
                },
            };
        }

        std::unique_ptr<NameObj> construct(
            std::string_view object_name, const char *actor_name,
            const smgpc::scene::NameObjPlacementContext &placement) override {
            require(placement.iter.isValid(),
                    "construct received an invalid retained JMap iterator");
            events.push_back(
                event_name("construct", object_name, placement.local_id));
            constructed_actor_name_pointers.emplace_back(
                object_name, actor_name);
            if (throw_construct_object.has_value() &&
                object_name == *throw_construct_object) {
                throw std::runtime_error(
                    "synthetic authored construction failure");
            }
            constructed_actor_names.push_back(
                actor_name != nullptr
                    ? std::optional<std::string>{actor_name}
                    : std::nullopt);
            if (descendant_object.has_value() &&
                object_name == *descendant_object) {
                return std::make_unique<SyntheticDescendantRoot>(
                    actor_name, &destructor_events);
            }
            return std::make_unique<SyntheticNameObj>(
                actor_name, &destructor_events);
        }

        std::unique_ptr<NameObj> construct_model_changing(
            std::string_view object_name, s32 shape_model_no,
            const char *actor_name,
            const smgpc::scene::NameObjPlacementContext &placement) override {
            require(placement.iter.isValid(),
                    "model-changing construct received an invalid JMap iterator");
            auto model_name = std::array<char, 128U>{};
            const auto identifier = std::string(object_name);
            const auto written = std::snprintf(
                model_name.data(), model_name.size(), "%s%02d",
                identifier.c_str(), shape_model_no);
            require(written >= 0 &&
                        static_cast<std::size_t>(written) < model_name.size(),
                    "synthetic model-changing creator identifier overflowed");
            events.push_back(event_name(
                "construct-shaped", model_name.data(), placement.local_id));
            constructed_actor_name_pointers.emplace_back(
                object_name, actor_name);
            constructed_actor_names.push_back(
                actor_name != nullptr
                    ? std::optional<std::string>{actor_name}
                    : std::nullopt);
            return std::make_unique<SyntheticNameObj>(
                actor_name, &destructor_events);
        }

        void init(
            NameObj &object,
            const smgpc::scene::NameObjPlacementContext &) override {
            if (auto *root =
                    dynamic_cast<SyntheticDescendantRoot *>(&object);
                root != nullptr) {
                root->create_init_descendant();
            }
            if (lazy_scene_obj_object.has_value() &&
                std::string_view(object.getName()) ==
                    *lazy_scene_obj_object) {
                lazy_scene_obj = MR::createSceneObj(
                    lazy_scene_obj_id);
                require(lazy_scene_obj != nullptr,
                        "synthetic lifecycle could not create its lazy SceneObj");
            }
            if (descendant_after_lazy_name.has_value() &&
                lazy_scene_obj_object.has_value() &&
                std::string_view(object.getName()) ==
                    *lazy_scene_obj_object) {
                (void)new SyntheticNameObj(
                    descendant_after_lazy_name->c_str(),
                    &destructor_events);
            }
            if (init_hook) {
                init_hook(object);
            }
            if (throw_init_actor_name.has_value() &&
                std::string_view(object.getName()) ==
                    *throw_init_actor_name) {
                throw std::runtime_error(
                    "synthetic authored init failure");
            }
        }

        void init_after_placement(NameObj &object) override {
            events.push_back("after:" + std::string(object.getName()));
            if (throw_after_actor_name.has_value() &&
                std::string_view(object.getName()) ==
                    *throw_after_actor_name) {
                throw std::runtime_error(
                    "synthetic authored postpass failure");
            }
            object.initAfterPlacement();
        }

        void destroy(NameObj &object) override {
            events.push_back("destroy:" + std::string(object.getName()));
        }

        std::optional<std::string> unloaded_object{};
        std::optional<std::string> throw_construct_object{};
        std::optional<std::string> descendant_object{};
        std::optional<std::string> throw_init_actor_name{};
        std::optional<std::string> throw_after_actor_name{};
        std::optional<std::string> lazy_scene_obj_object{};
        std::optional<std::string> descendant_after_lazy_name{};
        s32 lazy_scene_obj_id = SceneObj_GroupCheckManager;
        NameObj *lazy_scene_obj = nullptr;
        std::function<void(NameObj &)> init_hook{};
        std::vector<std::string> events{};
        std::vector<std::string> destructor_events{};
        std::vector<std::optional<std::string>> constructed_actor_names{};
        std::vector<std::pair<std::string, const char *>>
            constructed_actor_name_pointers{};
        std::vector<const JMapInfo *> iter_infos{};
        std::vector<s32> iter_indices{};
    };

    [[nodiscard]] std::optional<std::string> synthetic_actor_name(
        const smgpc::scene::StagePlacementObject &placement) {
        return "actor:" +
               std::string(
                   smgpc::scene::authored_placement_identifier(placement)) +
               '@' + std::to_string(placement.l_id);
    }

    [[nodiscard]] smgpc::scene::AuthoredPlacementInstantiationOptions
    synthetic_ready_options() {
        return smgpc::scene::AuthoredPlacementInstantiationOptions{
            .mode = smgpc::scene::AuthoredPlacementMode::Strict,
            .actor_name_resolver = synthetic_actor_name,
            .deferred_archive_resolver = [](const auto &) {
                return false;
            },
            .group_load_order_resolver = [](const auto &) {
                return AuthoredPlacementGroupLoadOrder::ArchivesReady;
            },
            .creator_availability_resolver = [](const auto &) {
                return true;
            },
        };
    }

    [[nodiscard]] const smgpc::scene::AuthoredPlacementReportEntry &
    require_source_entry(
        const smgpc::scene::AuthoredPlacementInstantiationReport &report,
        std::size_t source_index) {
        const auto found = std::ranges::find_if(
            report.entries, [&](const auto &entry) {
                return entry.source_index == source_index;
            });
        require(found != report.entries.end(),
                "placement report omitted a synthetic source row");
        return *found;
    }

    [[nodiscard]] std::vector<std::size_t> report_source_order(
        const smgpc::scene::AuthoredPlacementInstantiationReport &report) {
        auto result = std::vector<std::size_t>{};
        result.reserve(report.entries.size());
        for (const auto &entry : report.entries) {
            result.push_back(entry.source_index);
        }
        return result;
    }

    void test_synthetic_default_construction_scope_needs_no_holder() {
        require(smgpc::scene::current_scene_obj_holder() == nullptr,
                "synthetic no-scope proof unexpectedly inherited a SceneObjHolder");
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto data = make_data({make_placement(
            "NoScope", StagePlacementLoadBatch::CommonBootstrap,
            "ObjInfo", true, 0U, 0)});
        auto lifecycle = RecordingLifecycle{};
        auto instantiator =
            smgpc::scene::AuthoredPlacementInstantiator(
                data, lifecycle, synthetic_ready_options());
        (void)instantiator.preload();
        require(instantiator.instantiate().created_count == 1U,
                "default synthetic construction scope did not remain a null token");
        (void)instantiator.init_after_placement();
        instantiator.clear();
        require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                "default synthetic construction scope changed registry lifetime");
    }

    void test_nameobj_adapter_retains_strict_zone_scope_source() {
        auto input = std::ifstream(
            "src/scene/AuthoredPlacementInstantiator.cpp",
            std::ios::binary);
        require(input.good(),
                "could not inspect the production NameObj placement adapter source");
        const auto source = std::string(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>());
        const auto adapter = source.find(
            "class NameObjPlacementLifecycleAdapter final");
        const auto strict_scope = source.find(
            "PlacementZoneNameScope _scope;", adapter);
        const auto adapter_factory = source.find(
            "std::make_unique<ConstructionScope>(placement)", adapter);
        const auto scope_use = source.find(
            "_lifecycle->begin_construction_scope(context)");
        const auto capture_use = source.find(
            "NameObjRuntimeRegistrationCapture{}", scope_use);
        require(adapter != std::string::npos &&
                    strict_scope != std::string::npos &&
                    adapter_factory != std::string::npos &&
                    scope_use != std::string::npos &&
                    capture_use != std::string::npos &&
                    scope_use < capture_use,
                "production adapter lost its strict zone scope or construct+init lifetime");
    }

    void test_model_changing_rows_require_their_exact_creator() {
        const auto placement = make_placement(
            "ShapeUsesOrdinaryName",
            StagePlacementLoadBatch::CommonBootstrap, "ObjInfo", true,
            0U, 0, 7);
        const auto support =
            smgpc::scene::classify_authored_placement(placement);
        require(support.kind ==
                        smgpc::scene::AuthoredPlacementSupportKind::Blocked &&
                    support.reason ==
                        "model_changing_creator_unavailable",
                "ShapeModelNo borrowed the ordinary NameObjFactory creator");
    }

    void test_holder_occurrence_discovery_preserves_duplicates_and_bounds_cycles() {
        const auto transform_at = [](f32 x) {
            auto transform = smgpc::scene::StageZoneTransform{};
            transform.matrix[3U] = x;
            return transform;
        };
        auto observed = std::vector<std::string>{};
        const auto holders =
            smgpc::scene::discover_stage_holder_occurrences(
                "Root", 0, transform_at(0.0F),
                [&](const smgpc::scene::StageHolderOccurrence &holder,
                    StagePlacementLoadBatch batch) {
                    auto children =
                        std::vector<smgpc::scene::
                                        StageHolderChildDescriptor>{};
                    if (batch != StagePlacementLoadBatch::CommonBootstrap) {
                        return children;
                    }
                    if (holder.stage_name == "Root") {
                        children.push_back({
                            .stage_name = "A",
                            .zone_id = 1,
                            .zone_transform = transform_at(10.0F),
                        });
                        children.push_back({
                            .stage_name = "A",
                            .zone_id = 1,
                            .zone_transform = transform_at(20.0F),
                        });
                    } else if (holder.stage_name == "A") {
                        children.push_back({
                            .stage_name = "B",
                            .zone_id = 2,
                            .zone_transform = transform_at(
                                holder.zone_transform.matrix[3U] + 1.0F),
                        });
                    } else if (holder.stage_name == "B") {
                        // A -> B -> A is rejected only on this ancestry path.
                        children.push_back({
                            .stage_name = "A",
                            .zone_id = 1,
                            .zone_transform = transform_at(999.0F),
                        });
                    }
                    return children;
                },
                [&](const smgpc::scene::StageHolderOccurrence &holder) {
                    observed.push_back(holder.stage_name);
                });

        require(holders.size() == 5U &&
                    observed ==
                        std::vector<std::string>{"Root", "A", "B", "A", "B"},
                "holder discovery collapsed a duplicate sibling or followed an ancestry cycle");
        require(holders[0U].children ==
                        std::vector<std::size_t>{1U, 3U} &&
                    holders[1U].parent_instance_id == 0U &&
                    holders[1U].sibling_order == 0U &&
                    holders[1U].children ==
                        std::vector<std::size_t>{2U} &&
                    holders[2U].parent_instance_id == 1U &&
                    holders[2U].depth == 2U &&
                    holders[2U].children.empty() &&
                    holders[3U].parent_instance_id == 0U &&
                    holders[3U].sibling_order == 1U &&
                    holders[3U].children ==
                        std::vector<std::size_t>{4U} &&
                    holders[4U].parent_instance_id == 3U &&
                    holders[4U].children.empty(),
                "holder occurrence parent/depth/sibling identity is not per StageObj row");
        require(holders[1U].zone_transform.matrix[3U] == 10.0F &&
                    holders[2U].zone_transform.matrix[3U] == 11.0F &&
                    holders[3U].zone_transform.matrix[3U] == 20.0F &&
                    holders[4U].zone_transform.matrix[3U] == 21.0F &&
                    std::ranges::equal(
                        holders |
                            std::views::transform([](const auto &holder) {
                                return holder.traversal_order;
                            }),
                        std::array<std::size_t, 5U>{0U, 1U, 2U, 3U, 4U}),
                "duplicate holder transforms or DFS traversal order were not retained");
    }

    void test_resolver_retains_missing_and_authored_empty_identifiers() {
        auto tables = std::vector<smgpc::scene::StagePlacementTable>{
            make_table(0, 0, "placement", "FieldlessObjInfo",
                       "jmp/placement/common/fieldlessobjinfo", 0U,
                       make_fieldless_jmap(1U)),
            make_table(0, 0, "placement", "TypedObjInfo",
                       "jmp/placement/common/typedobjinfo", 1U,
                       make_name_type_jmap("RawName", "")),
        };
        tables[0U].placement_attachment_order = 0U;
        tables[1U].placement_attachment_order = 1U;
        auto dvd = smgpc::runtime::DvdFileSystemService(
            "/synthetic/authored-placement-empty-id");
        const auto objects =
            smgpc::scene::resolve_stage_placement_objects(dvd, tables);
        require(objects.size() == 2U &&
                    objects[0U].object_name.empty() &&
                    objects[0U].creator_identifier.empty() &&
                    objects[1U].object_name == "RawName" &&
                    objects[1U].type_name.empty() &&
                    objects[1U].creator_identifier.empty(),
                "real BCSV resolution dropped an empty SameIdSet or fell back from authored empty type to raw name");
    }

    void test_archive_metadata_is_independent_of_creator_support() {
        const auto info = make_fieldless_jmap(1U);
        const auto iter = JMapInfoIter(&info, 0);
        require(NameObjFactory::getCreator("DinoPackun") == nullptr,
                "archive-metadata fixture unexpectedly gained a creator");
        auto collector = NameObjArchiveListCollector{};
        NameObjFactory::getMountObjectArchiveList(
            &collector, "DinoPackun", iter);
        const auto expected = std::array<std::string_view, 5U>{
            "DinoPackun", "DinoPackunDemoPosition",
            "DinoPackunEggShell", "DinoPackunEggShellBreak",
            "DinoPackunTailBall"};
        require(collector.mCount == static_cast<s32>(expected.size()),
                "unsupported static archive metadata was erased by creator policy");
        for (auto index = std::size_t{}; index < expected.size(); ++index) {
            require(collector.getArchive(static_cast<s32>(index)) ==
                        expected[index],
                    "unsupported static archive metadata order changed");
        }
    }

    void test_holder_provenance_and_attachment_ordinals() {
        const auto zone_names = std::array<std::string_view, 3U>{
            "Root", "BootstrapChild", "ScenarioChild"};
        const auto bootstrap_child = std::array<std::string_view, 1U>{
            "BootstrapChild"};
        const auto scenario_child = std::array<std::string_view, 1U>{
            "ScenarioChild"};
        const auto twin_actor = std::array<std::string_view, 1U>{
            "TwinActor"};
        const auto deep_actor = std::array<std::string_view, 1U>{
            "DeepActor"};
        auto zone_list = make_string_jmap("ZoneName", zone_names);

        auto tables = std::vector<smgpc::scene::StagePlacementTable>{
            // Deliberately scramble storage. Provenance and attachment policy
            // must derive from holders/layers/categories, not vector order.
            make_table(2, 0, "mapparts", "ScenarioMap",
                       "scenario/common/map", 0U),
            make_table(0, 1, "mapparts", "RootLayerAMap",
                       "root/layera/map", 1U),
            make_table(1, 0, "placement", "BootstrapObj",
                       "bootstrap/common/obj", 0U),
            make_table(0, 0, "mapparts", "RootCommonMap",
                       "root/common/map", 5U),
            make_table(0, 1, "placement", "StageObjInfo",
                       "root/layera/stage", 3U,
                       make_string_jmap("name", scenario_child)),
            make_table(0, 0, "placement", "RootCommonObj",
                       "root/common/obj", 20U),
            make_table(2, 0, "placement", "ScenarioObj",
                       "scenario/common/obj", 0U,
                       make_string_jmap("name", twin_actor)),
            make_table(1, 0, "mapparts", "BootstrapMap",
                       "bootstrap/common/map", 0U),
            make_table(0, 1, "placement", "RootLayerAObj",
                       "root/layera/obj", 4U),
            make_table(0, 0, "placement", "StageObjInfo",
                       "root/common/stage", 10U,
                       make_string_jmap("name", bootstrap_child)),
            make_table(2, 0, "placement", "ScenarioObj",
                       "scenario/common/obj", 0U,
                       make_string_jmap("name", twin_actor)),
            make_table(2, 0, "mapparts", "ScenarioMap",
                       "scenario/common/map", 0U),
            make_table(2, 0, "placement", "GrandchildObj",
                       "grandchild/common/obj", 0U,
                       make_string_jmap("name", deep_actor)),
        };

        const auto configure_holder = [&](std::string_view path,
                                          std::size_t holder_id,
                                          std::optional<std::size_t> parent_id,
                                          std::size_t depth,
                                          std::size_t sibling_order,
                                          StagePlacementLoadBatch discovery,
                                          StagePlacementLoadBatch batch,
                                          bool participates,
                                          std::size_t path_occurrence = 0U) {
            auto found = tables.end();
            auto seen = std::size_t{};
            for (auto table = tables.begin(); table != tables.end(); ++table) {
                if (table->table_path != path) {
                    continue;
                }
                if (seen++ == path_occurrence) {
                    found = table;
                    break;
                }
            }
            require(found != tables.end(),
                    "synthetic holder configuration lost a table");
            found->holder_instance_id = holder_id;
            found->parent_holder_instance_id = parent_id;
            found->holder_depth = depth;
            found->holder_sibling_order = sibling_order;
            found->holder_discovery_batch = discovery;
            found->load_batch = batch;
            found->participates_in_root_placement = participates;
            found->placement_attachment_order =
                std::numeric_limits<std::size_t>::max();
        };
        for (const auto path : {
                 "root/common/stage", "root/common/obj",
                 "root/common/map"}) {
            configure_holder(
                path, 0U, std::nullopt, 0U, 0U,
                StagePlacementLoadBatch::CommonBootstrap,
                StagePlacementLoadBatch::CommonBootstrap, true);
        }
        for (const auto path : {
                 "root/layera/stage", "root/layera/obj",
                 "root/layera/map"}) {
            configure_holder(
                path, 0U, std::nullopt, 0U, 0U,
                StagePlacementLoadBatch::CommonBootstrap,
                StagePlacementLoadBatch::ScenarioSelected, true);
        }
        for (const auto path : {
                 "bootstrap/common/obj", "bootstrap/common/map"}) {
            configure_holder(
                path, 1U, 0U, 1U, 0U,
                StagePlacementLoadBatch::CommonBootstrap,
                StagePlacementLoadBatch::CommonBootstrap, true);
        }
        for (const auto path : {
                 "scenario/common/obj", "scenario/common/map"}) {
            configure_holder(
                path, 2U, 0U, 1U, 1U,
                StagePlacementLoadBatch::ScenarioSelected,
                StagePlacementLoadBatch::ScenarioSelected, true);
        }
        for (const auto path : {
                 "scenario/common/obj",
                 "scenario/common/map"}) {
            configure_holder(
                path, 3U, 0U, 1U, 2U,
                StagePlacementLoadBatch::ScenarioSelected,
                StagePlacementLoadBatch::ScenarioSelected, true, 1U);
        }
        configure_holder(
            "grandchild/common/obj", 4U, 1U, 2U, 0U,
            StagePlacementLoadBatch::CommonBootstrap,
            StagePlacementLoadBatch::CommonBootstrap, false);
        auto first_duplicate_transform =
            smgpc::scene::StageZoneTransform{};
        first_duplicate_transform.matrix[3U] = 10.0F;
        auto second_duplicate_transform =
            smgpc::scene::StageZoneTransform{};
        second_duplicate_transform.matrix[3U] = 20.0F;
        for (auto &table : tables) {
            if (table.holder_instance_id == 2U) {
                table.zone_transform = first_duplicate_transform;
            } else if (table.holder_instance_id == 3U) {
                table.zone_transform = second_duplicate_transform;
            }
        }

        smgpc::scene::assign_stage_placement_provenance(
            tables, zone_list);
        const auto require_table = [&](std::string_view path,
                                       std::size_t path_occurrence = 0U)
            -> const smgpc::scene::StagePlacementTable & {
            auto found = tables.end();
            auto seen = std::size_t{};
            for (auto table = tables.begin(); table != tables.end(); ++table) {
                if (table->table_path != path) {
                    continue;
                }
                if (seen++ == path_occurrence) {
                    found = table;
                    break;
                }
            }
            require(found != tables.end(),
                    "synthetic provenance fixture lost a table");
            return *found;
        };
        const auto require_provenance = [&](std::string_view path,
                                            StagePlacementLoadBatch batch,
                                            std::size_t ordinal,
                                            std::size_t path_occurrence = 0U) {
            const auto &table = require_table(path, path_occurrence);
            require(table.load_batch == batch &&
                        table.placement_attachment_order == ordinal,
                    "StageDataHolder load batch or attachment ordinal diverged");
        };

        require_provenance(
            "root/common/stage",
            StagePlacementLoadBatch::CommonBootstrap, 0U);
        require_provenance(
            "root/common/obj",
            StagePlacementLoadBatch::CommonBootstrap, 1U);
        require_provenance(
            "root/common/map",
            StagePlacementLoadBatch::CommonBootstrap, 2U);
        require_provenance(
            "bootstrap/common/obj",
            StagePlacementLoadBatch::CommonBootstrap, 3U);
        require_provenance(
            "bootstrap/common/map",
            StagePlacementLoadBatch::CommonBootstrap, 4U);
        require_provenance(
            "root/layera/stage",
            StagePlacementLoadBatch::ScenarioSelected, 0U);
        require_provenance(
            "root/layera/obj",
            StagePlacementLoadBatch::ScenarioSelected, 1U);
        require_provenance(
            "root/layera/map",
            StagePlacementLoadBatch::ScenarioSelected, 2U);
        require_provenance(
            "scenario/common/obj",
            StagePlacementLoadBatch::ScenarioSelected, 3U);
        require_provenance(
            "scenario/common/map",
            StagePlacementLoadBatch::ScenarioSelected, 4U);
        require_provenance(
            "scenario/common/obj",
            StagePlacementLoadBatch::ScenarioSelected, 5U, 1U);
        require_provenance(
            "scenario/common/map",
            StagePlacementLoadBatch::ScenarioSelected, 6U, 1U);
        require(require_table("scenario/common/obj").zone_id ==
                        require_table("scenario/common/obj", 1U).zone_id &&
                    require_table("scenario/common/obj")
                            .holder_instance_id !=
                        require_table("scenario/common/obj", 1U)
                            .holder_instance_id &&
                    require_table("scenario/common/obj")
                            .zone_transform.matrix[3U] == 10.0F &&
                    require_table("scenario/common/obj", 1U)
                            .zone_transform.matrix[3U] == 20.0F,
                "same-path duplicate sibling zone rows collapsed holder identity or transforms");
        require(!require_table("grandchild/common/obj")
                     .participates_in_root_placement &&
                    require_table("grandchild/common/obj")
                            .placement_attachment_order ==
                        std::numeric_limits<std::size_t>::max(),
                "a grandchild holder leaked into root PlacementInfoOrdered");

        auto dvd = smgpc::runtime::DvdFileSystemService(
            "/synthetic/authored-placement-holder-occurrences");
        const auto objects =
            smgpc::scene::resolve_stage_placement_objects(dvd, tables);
        auto twin_holder_ids = std::vector<std::size_t>{};
        for (const auto &object : objects) {
            if (object.creator_identifier == "DeepActor") {
                throw std::runtime_error(
                    "grandchild holder object leaked into root placements");
            }
            if (object.creator_identifier == "TwinActor") {
                require(object.table_path == "scenario/common/obj",
                        "same-path duplicate object changed table identity");
                twin_holder_ids.push_back(object.holder_instance_id);
            }
        }
        require(twin_holder_ids ==
                        std::vector<std::size_t>{2U, 3U},
                "same-path duplicate holder objects collapsed during BCSV resolution");
    }

    void test_strict_preflight_is_support_only_and_non_mutating() {
        auto data = make_data({
            make_placement("Ready", StagePlacementLoadBatch::ScenarioSelected,
                           "ObjInfo", true, 0U, 10),
            make_placement("Blocked", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", false, 1U, 11),
            make_placement("Ignored", StagePlacementLoadBatch::CommonBootstrap,
                           "DemoObjInfo", false, 2U, 12, -1,
                           std::nullopt, true),
        });
        auto lifecycle = RecordingLifecycle{};
        auto actor_name_calls = std::size_t{};
        auto deferred_calls = std::size_t{};
        auto rank_calls = std::size_t{};
        auto creator_calls = std::size_t{};
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle,
            smgpc::scene::AuthoredPlacementInstantiationOptions{
                .mode = smgpc::scene::AuthoredPlacementMode::Strict,
                .actor_name_resolver = [&](const auto &placement) {
                    ++actor_name_calls;
                    return synthetic_actor_name(placement);
                },
                .deferred_archive_resolver = [&](const auto &) {
                    ++deferred_calls;
                    return false;
                },
                .group_load_order_resolver = [&](const auto &) {
                    ++rank_calls;
                    return AuthoredPlacementGroupLoadOrder::ArchivesReady;
                },
                .creator_availability_resolver = [&](const auto &) {
                    ++creator_calls;
                    return true;
                },
            });

        auto rejected = false;
        try {
            (void)instantiator.preload();
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        const auto &report = instantiator.report();
        require(rejected && lifecycle.events.empty() &&
                    actor_name_calls == 0U && deferred_calls == 0U &&
                    rank_calls == 0U && creator_calls == 0U &&
                    !report.preflight_accepted &&
                    report.ready_count == 1U && report.blocked_count == 1U &&
                    report.ignored_count == 1U && report.created_count == 0U &&
                    report.state ==
                        smgpc::scene::AuthoredPlacementRuntimeState::Failed,
                "Strict preflight consulted order/name policy or mutated lifecycle state");

        const auto context = data.placement_context(0U);
        require(context.iter.isValid() &&
                    context.iter.mInfo == &data.placements()[0U].jmap_info &&
                    context.iter.mIndex == 0,
                "StageAuthoredData did not retain its copied JMap row");
    }

    void test_five_pass_grouping_preload_and_lifecycle_order() {
        auto placements = std::vector<smgpc::scene::StagePlacementObject>{
            make_placement("CommonNormal", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 5U, 5),                         // 0
            make_placement("RawAliasLate", StagePlacementLoadBatch::CommonBootstrap,
                           "PlanetObjInfo", true, 2U, 2, -1, "SharedHigh"), // 1
            make_placement("ScenarioNormal", StagePlacementLoadBatch::ScenarioSelected,
                           "ObjInfo", true, 2U, 2),                         // 2
            make_placement("CommonHighSmall", StagePlacementLoadBatch::CommonBootstrap,
                           "PlanetObjInfo", true, 1U, 1),                  // 3
            make_placement("RawAliasEarly", StagePlacementLoadBatch::CommonBootstrap,
                           "PlanetObjInfo", true, 0U, 0, -1, "SharedHigh"), // 4
            // A scenario-discovered holder may contribute a common-layer table;
            // explicit provenance, not the layer spelling, selects pass 104.
            make_placement("ScenarioHigh", StagePlacementLoadBatch::ScenarioSelected,
                           "PlanetObjInfo", true, 0U, 0),                  // 5
            make_placement("Deferred", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 7U, 7),                         // 6
            make_placement("ShapeGroup", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 6U, 6, 3),                      // 7
            make_placement("ShapeGroup", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 4U, 4, 3),                      // 8
            make_placement("BlockedHigh", StagePlacementLoadBatch::CommonBootstrap,
                           "PlanetObjInfo", false, 3U, 3),                 // 9
            make_placement("BlockedHigh", StagePlacementLoadBatch::CommonBootstrap,
                           "PlanetObjInfo", false, 4U, 4),                 // 10
            make_placement("BlockedHigh", StagePlacementLoadBatch::CommonBootstrap,
                           "PlanetObjInfo", false, 5U, 5),                 // 11
            make_placement("PlayerGroup", StagePlacementLoadBatch::CommonBootstrap,
                           "PlanetObjInfo", true, 6U, 6),                  // 12
            make_placement("IgnoredHigh", StagePlacementLoadBatch::CommonBootstrap,
                           "PlanetObjInfo", false, 7U, 7, -1,
                           std::nullopt, true),                              // 13
            make_placement("CommonNormal", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 3U, 3),                         // 14
            make_placement("ShapeGroup", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 8U, 8, 4),                      // 15
            // Canonical empty `type` must not fall back to the raw `name`.
            make_placement("RawFallbackMustNotBeUsed",
                           StagePlacementLoadBatch::ScenarioSelected,
                           "ObjInfo", true, 1U, 1, -1, std::string{}),      // 16
            make_placement("SharedRaw", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 9U, 9, -1, "TypeA"),           // 17
            make_placement("SharedRaw", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 10U, 10, -1, "TypeB"),         // 18
            make_placement("MixedReady", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 11U, 11, -1, "MixedGroup"),    // 19
            make_placement("MixedBlocked", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", false, 12U, 12, -1, "MixedGroup"),   // 20
        };
        // Retain the exact scenario-discovered/common-layer provenance case.
        placements[5U].layer_name = "common";
        auto data = make_data(std::move(placements));

        auto lifecycle = RecordingLifecycle{};
        auto common_rank_count = std::size_t{};
        auto scenario_rank_count = std::size_t{};
        auto creator_check_count = std::size_t{};
        auto deferred_call_count = std::size_t{};
        const auto is_scenario_rank = [](std::string_view identifier) {
            return identifier == "ScenarioHigh" ||
                   identifier == "ScenarioNormal" || identifier.empty() ||
                   identifier == "Deferred";
        };
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle,
            smgpc::scene::AuthoredPlacementInstantiationOptions{
                .mode = smgpc::scene::AuthoredPlacementMode::
                    SupportedSubsetForDevelopment,
                .support_resolver = [](const auto &placement) {
                    if (placement.shape_model_no != -1) {
                        return smgpc::scene::AuthoredPlacementSupport{
                            .kind = smgpc::scene::
                                AuthoredPlacementSupportKind::Ready,
                            .reason =
                                "synthetic_model_changing_creator",
                        };
                    }
                    return smgpc::scene::classify_authored_placement(
                        placement);
                },
                .actor_name_resolver = synthetic_actor_name,
                .deferred_archive_resolver = [&](const auto &placement) {
                    ++deferred_call_count;
                    return smgpc::scene::authored_placement_identifier(
                               placement) == "Deferred";
                },
                .group_load_order_resolver = [&](const auto &placement) {
                    const auto identifier =
                        smgpc::scene::authored_placement_identifier(placement);
                    if (is_scenario_rank(identifier)) {
                        ++scenario_rank_count;
                        require(lifecycle.events.size() == 13U &&
                                    std::ranges::none_of(
                                        lifecycle.events, [](const auto &event) {
                                            return event.starts_with(
                                                "construct:");
                                        }),
                                "a scenario/deferred holder ranked before complete common preload or after construction");
                    } else {
                        ++common_rank_count;
                        require(lifecycle.events.empty(),
                                "the two common holders were not both ranked before common preload");
                    }
                    if (identifier == "PlayerGroup") {
                        return AuthoredPlacementGroupLoadOrder::
                            PlayerArchiveLoader;
                    }
                    if (placement.shape_model_no != -1) {
                        return AuthoredPlacementGroupLoadOrder::
                            ArchiveLoadRequired;
                    }
                    return AuthoredPlacementGroupLoadOrder::ArchivesReady;
                },
                .creator_availability_resolver = [&](const auto &placement) {
                    ++creator_check_count;
                    const auto identifier =
                        smgpc::scene::authored_placement_identifier(placement);
                    return identifier != "BlockedHigh";
                },
            });

        const auto &preflight = instantiator.preflight();
        require(preflight.preflight_accepted && lifecycle.events.empty() &&
                    common_rank_count == 0U && scenario_rank_count == 0U &&
                    preflight.state ==
                        smgpc::scene::AuthoredPlacementRuntimeState::Prepared,
                "development preflight consulted retail order or touched lifecycle state");
        const auto &preloaded = instantiator.preload();
        require(preloaded.state ==
                        smgpc::scene::AuthoredPlacementRuntimeState::Preloaded &&
                    preloaded.created_count == 0U &&
                    instantiator.instances().empty() &&
                    std::ranges::none_of(
                        lifecycle.events, [](const auto &event) {
                            return event.starts_with("construct:") ||
                                   event.starts_with("construct-shaped:");
                        }),
                "preload crossed the retail Mario boundary into actor construction");
        auto duplicate_preload_rejected = false;
        try {
            (void)instantiator.preload();
        } catch (const std::logic_error &) {
            duplicate_preload_rejected = true;
        }
        require(duplicate_preload_rejected,
                "a completed authored plan ranked or requested archives twice");
        const auto &created = instantiator.instantiate();
        require(created.preflight_accepted && created.ready_count == 16U &&
                    created.blocked_count == 4U &&
                    created.ignored_count == 1U &&
                    created.created_count == 16U &&
                    instantiator.instances().size() == 16U &&
                    common_rank_count == 11U && scenario_rank_count == 4U &&
                    deferred_call_count == 7U,
                "five-pass development subset lost support or ranking accounting");
        require(creator_check_count == 30U,
                "creator availability was not checked once for requestFileLoad and again for initPlacement for every SameIdSet");

        const auto expected_report_order = std::vector<std::size_t>{
            12U, 9U, 10U, 11U, 4U, 1U, 3U, 13U, // FC
            5U,                                    // 104
            14U, 0U, 19U, 20U, 17U, 18U, 8U, 7U, 15U, // 100
            16U, 2U,                               // 108
            6U,                                    // 10C
        };
        require(report_source_order(created) == expected_report_order,
                "five-pass SameIdSet order lost class/count/attachment semantics");

        const auto expected_preloads = std::vector<std::string>{
            "preload:PlayerGroup@6",
            "preload:SharedHigh@0",
            "preload:SharedHigh@2",
            "preload:CommonHighSmall@1",
            "preload:IgnoredHigh@7",
            "preload:CommonNormal@3",
            "preload:CommonNormal@5",
            "preload:MixedGroup@11",
            "preload:MixedGroup@12",
            "preload:TypeA@9",
            "preload:TypeB@10",
            "preload-shaped:ShapeGroup03@4",
            "preload-shaped:ShapeGroup04@8",
            "preload:ScenarioHigh@0",
            "preload:@1",
            "preload:ScenarioNormal@2",
            "preload:Deferred@7",
        };
        require(lifecycle.events.size() >= expected_preloads.size() &&
                    std::ranges::equal(
                        expected_preloads,
                        std::span(lifecycle.events).first(
                            expected_preloads.size())),
                "archive requests were not fully separated from construction or shaped once per SameIdSet");

        const auto expected_constructs = std::vector<std::string>{
            "construct:PlayerGroup@6",
            "construct:SharedHigh@0",
            "construct:SharedHigh@2",
            "construct:CommonHighSmall@1",
            "construct:ScenarioHigh@0",
            "construct:CommonNormal@3",
            "construct:CommonNormal@5",
            "construct:MixedGroup@11",
            "construct:TypeA@9",
            "construct:TypeB@10",
            "construct-shaped:ShapeGroup03@4",
            "construct-shaped:ShapeGroup03@6",
            "construct-shaped:ShapeGroup04@8",
            "construct:@1",
            "construct:ScenarioNormal@2",
            "construct:Deferred@7",
        };
        require(lifecycle.events.size() ==
                    expected_preloads.size() + expected_constructs.size() &&
                    std::ranges::equal(
                        expected_constructs,
                        std::span(lifecycle.events)
                            .subspan(expected_preloads.size())),
                "construction did not begin only after every preload or lost five-pass order");

        const auto &shape_first = require_source_entry(created, 8U);
        const auto &shape_peer = require_source_entry(created, 7U);
        const auto &shape_other = require_source_entry(created, 15U);
        require(shape_first.retail_group_index ==
                        shape_peer.retail_group_index &&
                    shape_first.retail_group_index !=
                        shape_other.retail_group_index &&
                    shape_first.archives.size() == 1U &&
                    shape_peer.archives.size() == 1U &&
                    shape_peer.archives.front().archive_name ==
                        shape_first.archives.front().archive_name &&
                    shape_peer.archives.front().resolved_path ==
                        shape_first.archives.front().resolved_path &&
                    shape_first.archives.front().archive_name ==
                        "ShapeGroup03" &&
                    shape_other.archives.size() == 1U &&
                    shape_other.archives.front().archive_name ==
                        "ShapeGroup04",
                "ShapeModelNo did not define separate generated archive groups");
        require(require_source_entry(created, 13U).archives.size() == 1U &&
                    require_source_entry(created, 13U).outcome ==
                        smgpc::scene::AuthoredPlacementOutcome::Ignored &&
                    require_source_entry(created, 9U).archives.empty() &&
                    require_source_entry(created, 20U).archives.size() == 1U &&
                    require_source_entry(created, 20U).outcome ==
                        smgpc::scene::AuthoredPlacementOutcome::Blocked,
                "creator-only request gating lost ignored/blocked/null-creator distinctions");
        const auto &empty_type = require_source_entry(created, 16U);
        require(empty_type.placement != nullptr &&
                    empty_type.placement->object_name ==
                        "RawFallbackMustNotBeUsed" &&
                    smgpc::scene::authored_placement_identifier(
                        *empty_type.placement)
                        .empty() &&
                    empty_type.archives.front().archive_name == "Archive" &&
                    empty_type.actor_name == "actor:@1",
                "an authored empty type incorrectly fell back to raw name");

        const auto events_before_after = lifecycle.events.size();
        const auto &after = instantiator.init_after_placement();
        require(after.initialized_after_placement_count == 16U &&
                    after.state == smgpc::scene::
                                       AuthoredPlacementRuntimeState::
                                           InitializedAfterPlacement &&
                    lifecycle.events.size() == events_before_after + 16U &&
                    std::ranges::all_of(
                        std::span(lifecycle.events).subspan(
                            events_before_after),
                        [](const auto &event) {
                            return event.starts_with("after:");
                        }),
                "initAfterPlacement was not one complete post-construction pass");

        auto expected_destroy = std::vector<std::string>{};
        expected_destroy.reserve(created.created_count);
        for (auto instance = instantiator.instances().rbegin();
             instance != instantiator.instances().rend(); ++instance) {
            expected_destroy.push_back(
                "destroy:" + std::string(instance->actor->getName()));
        }
        const auto events_before_clear = lifecycle.events.size();
        instantiator.clear();
        require(instantiator.instances().empty() &&
                    instantiator.report().destroyed_count == 16U &&
                    instantiator.report().state ==
                        smgpc::scene::AuthoredPlacementRuntimeState::Cleared &&
                    std::ranges::equal(
                        expected_destroy,
                        std::span(lifecycle.events).subspan(
                            events_before_clear)),
                "placement teardown was not reverse construction order");
        auto expected_delete = std::vector<std::string>{};
        expected_delete.reserve(expected_destroy.size());
        for (const auto &destroy : expected_destroy) {
            expected_delete.push_back(
                "delete:" + destroy.substr(std::string_view("destroy:").size()));
        }
        require(lifecycle.destructor_events == expected_delete,
                "actual NameObj destructors did not run at each reverse retirement point");
    }

    void test_group_creator_recheck_and_shared_actor_name_fallback() {
        auto data = make_data({
            make_placement("Multi", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 0U, 0),
            make_placement("Multi", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 1U, 1),
            make_placement("Absent", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 2U, 2),
            make_placement("Absent", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 3U, 3),
            make_placement("PresentEmpty",
                           StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 4U, 4),
            make_placement("PresentEmpty",
                           StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 5U, 5),
            make_placement("NullReady",
                           StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 6U, 6),
        });
        auto lifecycle = RecordingLifecycle{};
        auto creator_calls = std::vector<std::string>{};
        auto actor_name_calls = std::vector<std::string>{};
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle,
            smgpc::scene::AuthoredPlacementInstantiationOptions{
                .mode = smgpc::scene::AuthoredPlacementMode::
                    SupportedSubsetForDevelopment,
                .actor_name_resolver = [&](const auto &placement) {
                    const auto identifier = std::string(
                        smgpc::scene::authored_placement_identifier(
                            placement));
                    actor_name_calls.push_back(identifier);
                    if (identifier == "Multi") {
                        return std::optional<std::string>{"LocalizedOnce"};
                    }
                    if (identifier == "PresentEmpty") {
                        return std::optional<std::string>{std::string{}};
                    }
                    // MR::getJapaneseObjectName falls back to the input
                    // identifier when ObjNameTable has no matching row.
                    return std::optional<std::string>{identifier};
                },
                .deferred_archive_resolver = [](const auto &) {
                    return false;
                },
                .group_load_order_resolver = [](const auto &) {
                    return AuthoredPlacementGroupLoadOrder::ArchivesReady;
                },
                .creator_availability_resolver = [&](const auto &placement) {
                    const auto identifier = std::string(
                        smgpc::scene::authored_placement_identifier(
                            placement));
                    creator_calls.push_back(identifier);
                    return identifier != "NullReady";
                },
            });

        auto early_construct_rejected = false;
        try {
            (void)instantiator.instantiate();
        } catch (const std::logic_error &) {
            early_construct_rejected = true;
        }
        require(early_construct_rejected && lifecycle.events.empty(),
                "construction ran before a successful authored preload");

        const auto &preloaded = instantiator.preload();
        require(preloaded.state ==
                        smgpc::scene::AuthoredPlacementRuntimeState::Preloaded &&
                    preloaded.ready_count == 7U &&
                    preloaded.created_count == 0U &&
                    actor_name_calls.empty(),
                "preload resolved actor names or changed support accounting");
        const auto &report = instantiator.instantiate();
        require(report.ready_count == 6U && report.blocked_count == 1U &&
                    report.created_count == 6U &&
                    report.created_count == report.ready_count,
                "a Ready/null-creator row left dishonest creation accounting");

        const auto count_text = [](const auto &values,
                                   std::string_view value) {
            return static_cast<std::size_t>(
                std::ranges::count(values, std::string(value)));
        };
        require(count_text(creator_calls, "Multi") == 2U &&
                    count_text(creator_calls, "Absent") == 2U &&
                    count_text(creator_calls, "PresentEmpty") == 2U &&
                    count_text(creator_calls, "NullReady") == 2U,
                "each SameIdSet creator was not checked at request and init time");
        require(count_text(actor_name_calls, "Multi") == 1U &&
                    count_text(actor_name_calls, "Absent") == 1U &&
                    count_text(actor_name_calls, "PresentEmpty") == 1U &&
                    count_text(actor_name_calls, "NullReady") == 0U,
                "localized actor names were not resolved once after group creator success");

        const auto &multi_first = require_source_entry(report, 0U);
        const auto &multi_second = require_source_entry(report, 1U);
        const auto &absent_first = require_source_entry(report, 2U);
        const auto &absent_second = require_source_entry(report, 3U);
        const auto &empty_first = require_source_entry(report, 4U);
        const auto &empty_second = require_source_entry(report, 5U);
        const auto &null_creator = require_source_entry(report, 6U);
        require(multi_first.actor_name == "LocalizedOnce" &&
                    multi_second.actor_name == "LocalizedOnce" &&
                    absent_first.actor_name == "Absent" &&
                    absent_second.actor_name == "Absent" &&
                    empty_first.actor_name.has_value() &&
                    empty_first.actor_name->empty() &&
                    empty_second.actor_name.has_value() &&
                    empty_second.actor_name->empty() &&
                    null_creator.support.kind ==
                        smgpc::scene::AuthoredPlacementSupportKind::Blocked &&
                    null_creator.support.reason ==
                        "creator_unavailable_at_init_placement" &&
                    null_creator.outcome ==
                        smgpc::scene::AuthoredPlacementOutcome::Blocked &&
                    null_creator.actor == nullptr &&
                    null_creator.archives.empty(),
                "group actor-name reuse/null creator evidence was not retained");
        require(std::ranges::count(
                    lifecycle.constructed_actor_names,
                    std::optional<std::string>{"LocalizedOnce"}) == 2 &&
                    std::ranges::count(
                        lifecycle.constructed_actor_names,
                        std::optional<std::string>{"Absent"}) == 2 &&
                    std::ranges::count(
                        lifecycle.constructed_actor_names,
                        std::optional<std::string>{std::string{}}) == 2,
                "rows did not receive the group's stable localized/fallback/empty actor name");
        auto multi_name_pointers = std::vector<const char *>{};
        auto absent_name_pointers = std::vector<const char *>{};
        auto empty_name_pointers = std::vector<const char *>{};
        for (const auto &[identifier, pointer] :
             lifecycle.constructed_actor_name_pointers) {
            if (identifier == "Multi") {
                multi_name_pointers.push_back(pointer);
            } else if (identifier == "Absent") {
                absent_name_pointers.push_back(pointer);
            } else if (identifier == "PresentEmpty") {
                empty_name_pointers.push_back(pointer);
            }
        }
        require(multi_name_pointers.size() == 2U &&
                    multi_name_pointers[0U] != nullptr &&
                    multi_name_pointers[0U] == multi_name_pointers[1U] &&
                    absent_name_pointers.size() == 2U &&
                    absent_name_pointers[0U] != nullptr &&
                    absent_name_pointers[0U] ==
                        absent_name_pointers[1U] &&
                    std::string_view(absent_name_pointers[0U]) == "Absent" &&
                    empty_name_pointers.size() == 2U &&
                    empty_name_pointers[0U] != nullptr &&
                    empty_name_pointers[0U] == empty_name_pointers[1U] &&
                    std::string_view(empty_name_pointers[0U]).empty(),
                "SameIdSet rows did not share one stable localized/fallback/present-empty pointer");
        instantiator.clear();
    }

    void test_retail_shell_equal_key_permutation() {
        auto placements = std::vector<smgpc::scene::StagePlacementObject>{};
        for (auto index = std::size_t{}; index < 14U; ++index) {
            auto name = std::array<char, 8U>{};
            std::snprintf(name.data(), name.size(), "G%02zu", index);
            placements.push_back(make_placement(
                name.data(), StagePlacementLoadBatch::CommonBootstrap,
                "ObjInfo", true, index, static_cast<s32>(index)));
        }
        auto data = make_data(std::move(placements));
        auto lifecycle = RecordingLifecycle{};
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle,
            smgpc::scene::AuthoredPlacementInstantiationOptions{
                .mode = smgpc::scene::AuthoredPlacementMode::Strict,
                .actor_name_resolver = synthetic_actor_name,
                .deferred_archive_resolver = [](const auto &) {
                    return false;
                },
                .group_load_order_resolver = [](const auto &placement) {
                    return smgpc::scene::authored_placement_identifier(
                               placement) == "G13"
                               ? AuthoredPlacementGroupLoadOrder::
                                     PlayerArchiveLoader
                               : AuthoredPlacementGroupLoadOrder::
                                     ArchivesReady;
                },
                .creator_availability_resolver = [](const auto &) {
                    return true;
                },
            });
        (void)instantiator.preload();
        const auto &report = instantiator.instantiate();
        const auto expected = std::vector<std::size_t>{
            13U, 0U, 2U, 3U, 4U, 1U, 6U,
            7U, 8U, 5U, 10U, 11U, 12U, 9U,
        };
        require(report_source_order(report) == expected,
                "PlacementInfoOrdered Shell gaps lost the retail equal-key permutation");
        instantiator.clear();
    }

    void test_archive_failure_precedes_all_construction() {
        auto data = make_data({
            make_placement("First", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 0U, 0),
            make_placement("Second", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 1U, 1),
            make_placement("Unloaded", StagePlacementLoadBatch::ScenarioSelected,
                           "ObjInfo", true, 0U, 2),
        });
        auto lifecycle = RecordingLifecycle{};
        lifecycle.unloaded_object = "Unloaded";
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle,
            smgpc::scene::AuthoredPlacementInstantiationOptions{
                .mode = smgpc::scene::AuthoredPlacementMode::Strict,
                .actor_name_resolver = synthetic_actor_name,
                .deferred_archive_resolver = [](const auto &) {
                    return false;
                },
                .group_load_order_resolver = [](const auto &) {
                    return AuthoredPlacementGroupLoadOrder::ArchivesReady;
                },
                .creator_availability_resolver = [](const auto &) {
                    return true;
                },
            });

        auto failed = false;
        try {
            (void)instantiator.preload();
        } catch (const std::runtime_error &) {
            failed = true;
        }
        require(failed && instantiator.instances().empty() &&
                    instantiator.report().destroyed_count == 0U &&
                    require_source_entry(instantiator.report(), 2U).outcome ==
                        smgpc::scene::AuthoredPlacementOutcome::Failed,
                "archive failure advanced into actor construction");
        require(lifecycle.events ==
                    std::vector<std::string>{
                        "preload:First@0",
                        "preload:Second@1",
                        "preload:Unloaded@2",
                    },
                "archive failure was not detected before the five construction passes");
        auto construction_after_failure_rejected = false;
        try {
            (void)instantiator.instantiate();
        } catch (const std::logic_error &) {
            construction_after_failure_rejected = true;
        }
        require(construction_after_failure_rejected &&
                    std::ranges::none_of(
                        lifecycle.events, [](const auto &event) {
                            return event.starts_with("construct:");
                        }),
                "a failed preload plan advanced into construction");
    }

    void test_construction_failure_rolls_back_actual_actors_in_reverse() {
        auto data = make_data({
            make_placement("First", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 0U, 0),
            make_placement("Second", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 1U, 1),
            make_placement("Third", StagePlacementLoadBatch::CommonBootstrap,
                           "ObjInfo", true, 2U, 2),
        });
        auto lifecycle = RecordingLifecycle{};
        lifecycle.throw_construct_object = "Third";
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle,
            smgpc::scene::AuthoredPlacementInstantiationOptions{
                .mode = smgpc::scene::AuthoredPlacementMode::Strict,
                .actor_name_resolver = synthetic_actor_name,
                .deferred_archive_resolver = [](const auto &) {
                    return false;
                },
                .group_load_order_resolver = [](const auto &) {
                    return AuthoredPlacementGroupLoadOrder::ArchivesReady;
                },
                .creator_availability_resolver = [](const auto &) {
                    return true;
                },
            });
        (void)instantiator.preload();
        auto failed = false;
        try {
            (void)instantiator.instantiate();
        } catch (const std::runtime_error &) {
            failed = true;
        }
        require(failed && instantiator.instances().empty() &&
                    instantiator.report().state ==
                        smgpc::scene::AuthoredPlacementRuntimeState::Failed &&
                    instantiator.report().destroyed_count == 2U &&
                    lifecycle.destructor_events ==
                        std::vector<std::string>{
                            "delete:actor:Second@1",
                            "delete:actor:First@0"},
                "construction rollback did not delete actual actors in reverse order");
        auto destroy_events = std::vector<std::string>{};
        for (const auto &event : lifecycle.events) {
            if (event.starts_with("destroy:")) {
                destroy_events.push_back(event);
            }
        }
        require(destroy_events ==
                    std::vector<std::string>{
                        "destroy:actor:Second@1",
                        "destroy:actor:First@0"},
                "construction rollback lifecycle hooks were not reverse ordered");
    }

    void test_authored_descendants_share_global_postpass_and_reverse_teardown() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto data = make_data({
            make_placement(
                "Nested", StagePlacementLoadBatch::CommonBootstrap,
                "ObjInfo", true, 0U, 0),
            make_placement(
                "Plain", StagePlacementLoadBatch::CommonBootstrap,
                "ObjInfo", true, 1U, 1),
        });

        const auto run_generation = [&] {
            auto lifecycle = RecordingLifecycle{};
            lifecycle.descendant_object = "Nested";
            auto instantiator =
                smgpc::scene::AuthoredPlacementInstantiator(
                    data, lifecycle, synthetic_ready_options());
            (void)instantiator.preload();
            const auto &created = instantiator.instantiate();
            require(created.created_count == 2U &&
                        created.ready_count == 2U &&
                        created.descendants.size() == 2U &&
                        instantiator.descendants().size() == 2U,
                    "raw constructor/init descendants changed top-level authored accounting");

            const auto &constructor_child = created.descendants[0U];
            const auto &init_child = created.descendants[1U];
            require(constructor_child.parent_source_index == 0U &&
                        constructor_child.parent_report_index == 0U &&
                        constructor_child.construction_ordinal == 1U &&
                        constructor_child.parent_placement ==
                            &data.placements()[0U] &&
                        constructor_child.object != nullptr &&
                        std::string_view(
                            constructor_child.object->getName()) ==
                            "constructor-descendant" &&
                        init_child.parent_source_index == 0U &&
                        init_child.parent_report_index == 0U &&
                        init_child.construction_ordinal == 2U &&
                        init_child.parent_placement ==
                            &data.placements()[0U] &&
                        init_child.object != nullptr &&
                        std::string_view(init_child.object->getName()) ==
                            "init-descendant",
                    "authored descendant evidence lost its parent or exact construction ordinal");

            const auto events_before_after = lifecycle.events.size();
            const auto &after = instantiator.init_after_placement();
            require(after.initialized_after_placement_count == 2U &&
                        std::vector<std::string>(
                            lifecycle.events.begin() +
                                static_cast<std::ptrdiff_t>(
                                    events_before_after),
                            lifecycle.events.end()) ==
                            std::vector<std::string>{
                                "after:actor:Nested@0",
                                "after:constructor-descendant",
                                "after:init-descendant",
                                "after:actor:Plain@1",
                            } &&
                        std::ranges::all_of(
                            after.descendants, [](const auto &descendant) {
                                return descendant.outcome ==
                                    smgpc::scene::
                                        AuthoredPlacementOutcome::
                                            InitializedAfterPlacement;
                            }),
                    "scene-wide postpass did not visit roots and descendants once in construction order");

            const auto events_before_clear = lifecycle.events.size();
            instantiator.clear();
            const auto expected_reverse = std::vector<std::string>{
                "destroy:actor:Plain@1",
                "destroy:init-descendant",
                "destroy:constructor-descendant",
                "destroy:actor:Nested@0",
            };
            require(std::vector<std::string>(
                        lifecycle.events.begin() +
                            static_cast<std::ptrdiff_t>(events_before_clear),
                        lifecycle.events.end()) == expected_reverse &&
                        lifecycle.destructor_events ==
                            std::vector<std::string>{
                                "delete:actor:Plain@1",
                                "delete:init-descendant",
                                "delete:constructor-descendant",
                                "delete:actor:Nested@0",
                            } &&
                        instantiator.report().destroyed_count == 2U &&
                        std::ranges::all_of(
                            instantiator.report().descendants,
                            [](const auto &descendant) {
                                return descendant.object == nullptr &&
                                       descendant.outcome ==
                                           smgpc::scene::
                                               AuthoredPlacementOutcome::
                                                   Destroyed;
                            }) &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            baseline,
                    "root/descendant ownership was doubled or did not retire in exact reverse order");
        };

        run_generation();
        run_generation();
        require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                "authored descendant ownership could not recreate a clean second generation");
    }

    void test_authored_descendant_init_failure_rolls_back_exact_suffix() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto data = make_data({make_placement(
            "Failing", StagePlacementLoadBatch::CommonBootstrap,
            "ObjInfo", true, 0U, 0)});
        auto lifecycle = RecordingLifecycle{};
        lifecycle.descendant_object = "Failing";
        lifecycle.throw_init_actor_name = "actor:Failing@0";
        auto instantiator = smgpc::scene::AuthoredPlacementInstantiator(
            data, lifecycle, synthetic_ready_options());
        (void)instantiator.preload();

        auto preserved_failure = false;
        try {
            (void)instantiator.instantiate();
        } catch (const std::runtime_error &error) {
            preserved_failure = std::string_view(error.what()) ==
                                "synthetic authored init failure";
        }
        auto destroy_events = std::vector<std::string>{};
        for (const auto &event : lifecycle.events) {
            if (event.starts_with("destroy:")) {
                destroy_events.push_back(event);
            }
        }
        require(preserved_failure && instantiator.instances().empty() &&
                    instantiator.report().created_count == 0U &&
                    instantiator.report().destroyed_count == 0U &&
                    instantiator.report().descendants.empty() &&
                    require_source_entry(instantiator.report(), 0U).outcome ==
                        smgpc::scene::AuthoredPlacementOutcome::Failed &&
                    destroy_events ==
                        std::vector<std::string>{
                            "destroy:init-descendant",
                            "destroy:constructor-descendant",
                            "destroy:actor:Failing@0",
                        } &&
                    lifecycle.destructor_events ==
                        std::vector<std::string>{
                            "delete:init-descendant",
                            "delete:constructor-descendant",
                            "delete:actor:Failing@0",
                        } &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline,
                "init failure did not preserve its exception and reverse-roll back the complete live suffix");
    }

    void test_scene_obj_created_inside_capture_keeps_independent_owner() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto data = make_data({make_placement(
            "Lazy", StagePlacementLoadBatch::CommonBootstrap,
            "ObjInfo", true, 0U, 0)});
        auto holder = SceneObjHolder{};

        const auto run_generation = [&] {
            require(!holder.isExist(SceneObj_LensFlareDirector),
                    "SceneObj holder retained a stale prior generation");
            auto binding = smgpc::scene::SceneObjHolderBinding(holder);
            auto lifecycle = RecordingLifecycle{};
            lifecycle.lazy_scene_obj_object = "actor:Lazy@0";
            lifecycle.lazy_scene_obj_id = SceneObj_LensFlareDirector;
            lifecycle.descendant_after_lazy_name = "synthetic-sun";
            auto instantiator =
                smgpc::scene::AuthoredPlacementInstantiator(
                    data, lifecycle, synthetic_ready_options());
            (void)instantiator.preload();
            const auto &created = instantiator.instantiate();
            auto *lens_flare =
                dynamic_cast<LensFlareDirector *>(lifecycle.lazy_scene_obj);
            require(created.created_count == 1U &&
                        created.descendants.size() == 5U &&
                        std::ranges::all_of(
                            created.descendants |
                                std::views::take(4U),
                            [](const auto &descendant) {
                                return !descendant.owned_by_placement;
                            }) &&
                        created.descendants[4U].owned_by_placement &&
                        created.descendants[4U].construction_ordinal ==
                            5U &&
                        std::string_view(
                            created.descendants[4U].object->getName()) ==
                            "synthetic-sun" &&
                        lens_flare != nullptr &&
                        created.descendants[0U].object == lens_flare &&
                        created.descendants[0U].construction_ordinal ==
                            1U &&
                        created.descendants[1U].object ==
                            lens_flare->mRing &&
                        created.descendants[1U].construction_ordinal ==
                            2U &&
                        created.descendants[2U].object ==
                            lens_flare->mGlow &&
                        created.descendants[2U].construction_ordinal ==
                            3U &&
                        created.descendants[3U].object ==
                            lens_flare->mLine &&
                        created.descendants[3U].construction_ordinal ==
                            4U &&
                        holder.getObj(SceneObj_LensFlareDirector) ==
                            lifecycle.lazy_scene_obj &&
                        smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                lifecycle.lazy_scene_obj) &&
                        smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                lens_flare->mRing) &&
                        smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                lens_flare->mGlow) &&
                        smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                lens_flare->mLine),
                    "placement capture adopted a lazy SceneObj synchronous graph already owned by its holder");
            binding.init_after_placement();
            const auto events_before_after = lifecycle.events.size();
            (void)instantiator.init_after_placement();
            auto expected_postpass = std::vector<std::string>{
                "after:actor:Lazy@0",
            };
            for (const auto &descendant : created.descendants) {
                expected_postpass.push_back(
                    "after:" + std::string(descendant.object->getName()));
            }
            require(std::vector<std::string>(
                        lifecycle.events.begin() +
                            static_cast<std::ptrdiff_t>(
                                events_before_after),
                        lifecycle.events.end()) == expected_postpass &&
                        std::ranges::all_of(
                            created.descendants,
                            [](const auto &descendant) {
                                return descendant.outcome ==
                                    smgpc::scene::
                                        AuthoredPlacementOutcome::
                                            InitializedAfterPlacement;
                            }) &&
                        std::ranges::none_of(
                            created.descendants |
                                std::views::take(4U),
                            [](const auto &descendant) {
                                return smgpc::compat::
                                    name_obj_runtime_postpass_is_delegated(
                                        descendant.object);
                            }),
                    "lazy SceneObj graph was skipped or duplicated instead of delegated in registration order");
            instantiator.clear();
            require(holder.isExist(SceneObj_LensFlareDirector) &&
                        smgpc::compat::has_name_obj_runtime_state(
                            lifecycle.lazy_scene_obj) &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            baseline + 4U,
                    "placement teardown deleted the independently-owned lazy SceneObj graph");
        };

        run_generation();
        require(smgpc::compat::name_obj_runtime_state_count() == baseline &&
                    !holder.isExist(SceneObj_LensFlareDirector),
                "SceneObj owner did not retire and reset its first generation");
        run_generation();
        require(smgpc::compat::name_obj_runtime_state_count() == baseline &&
                    !holder.isExist(SceneObj_LensFlareDirector),
                "lazy SceneObj exclusion did not support clean recreation");
    }

    void test_failed_authored_root_preserves_lazy_scene_obj_graph() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto data = make_data({make_placement(
            "LazyFail", StagePlacementLoadBatch::CommonBootstrap,
            "ObjInfo", true, 0U, 0)});
        auto holder = SceneObjHolder{};

        const auto run_generation = [&] {
            auto binding = smgpc::scene::SceneObjHolderBinding(holder);
            auto lifecycle = RecordingLifecycle{};
            lifecycle.lazy_scene_obj_object = "actor:LazyFail@0";
            lifecycle.lazy_scene_obj_id = SceneObj_LensFlareDirector;
            lifecycle.throw_init_actor_name = "actor:LazyFail@0";
            auto instantiator =
                smgpc::scene::AuthoredPlacementInstantiator(
                    data, lifecycle, synthetic_ready_options());
            (void)instantiator.preload();

            auto preserved_failure = false;
            try {
                (void)instantiator.instantiate();
            } catch (const std::runtime_error &error) {
                preserved_failure = std::string_view(error.what()) ==
                                    "synthetic authored init failure";
            }
            auto *lens_flare = dynamic_cast<LensFlareDirector *>(
                holder.getObj(SceneObj_LensFlareDirector));
            require(preserved_failure && lens_flare != nullptr &&
                        instantiator.instances().empty() &&
                        instantiator.report().created_count == 0U &&
                        smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                lens_flare) &&
                        smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                lens_flare->mRing) &&
                        smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                lens_flare->mGlow) &&
                        smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                lens_flare->mLine) &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            baseline + 4U,
                    "authored root rollback deleted or detached the lazy SceneObj synchronous graph");
        };

        run_generation();
        require(smgpc::compat::name_obj_runtime_state_count() == baseline &&
                    !holder.isExist(SceneObj_LensFlareDirector),
                "failed-root SceneObj graph did not retire after its first holder generation");
        run_generation();
        require(smgpc::compat::name_obj_runtime_state_count() == baseline &&
                    !holder.isExist(SceneObj_LensFlareDirector),
                "failed-root SceneObj graph could not recreate and retire cleanly");
    }

    void test_cross_owner_postpass_stays_in_registration_order_once() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto data = make_data({make_placement(
            "CrossOwner", StagePlacementLoadBatch::CommonBootstrap,
            "ObjInfo", true, 0U, 0)});
        auto holder = SceneObjHolder{};

        const auto run_generation = [&] {
            auto context = SyntheticSceneObjContext{};
            {
                auto binding = smgpc::scene::SceneObjHolderBinding(
                    holder, synthetic_scene_obj_factory, &context);
                auto lifecycle = RecordingLifecycle{};
                lifecycle.descendant_object = "CrossOwner";
                lifecycle.lazy_scene_obj_object = "actor:CrossOwner@0";
                lifecycle.lazy_scene_obj_id = SceneObj_15;
                auto instantiator =
                    smgpc::scene::AuthoredPlacementInstantiator(
                        data, lifecycle, synthetic_ready_options());
                (void)instantiator.preload();
                const auto &created = instantiator.instantiate();
                require(created.descendants.size() == 4U &&
                            created.descendants[0U].construction_ordinal ==
                                1U &&
                            created.descendants[0U].owned_by_placement &&
                            created.descendants[1U].construction_ordinal ==
                                2U &&
                            created.descendants[1U].owned_by_placement &&
                            created.descendants[2U].construction_ordinal ==
                                3U &&
                            !created.descendants[2U].owned_by_placement &&
                            created.descendants[3U].construction_ordinal ==
                                4U &&
                            !created.descendants[3U].owned_by_placement &&
                            context.events ==
                                std::vector<std::string>{
                                    "construct:A", "init:A",
                                    "construct:B", "init:B"},
                        "cross-owner capture lost an owned or delegated registration identity");

                binding.init_after_placement();
                require(context.events ==
                            std::vector<std::string>{
                                "construct:A", "init:A",
                                "construct:B", "init:B"},
                        "SceneObj pre-pass duplicated a delegated placement identity");

                const auto events_before_after = lifecycle.events.size();
                (void)instantiator.init_after_placement();
                require(std::vector<std::string>(
                            lifecycle.events.begin() +
                                static_cast<std::ptrdiff_t>(
                                    events_before_after),
                            lifecycle.events.end()) ==
                            std::vector<std::string>{
                                "after:actor:CrossOwner@0",
                                "after:constructor-descendant",
                                "after:init-descendant",
                                "after:nested-scene-a",
                                "after:nested-scene-b"} &&
                            context.events ==
                                std::vector<std::string>{
                                    "construct:A", "init:A",
                                    "construct:B", "init:B",
                                    "after:A", "after:B"} &&
                            !smgpc::compat::
                                name_obj_runtime_postpass_is_delegated(
                                    created.descendants[2U].object) &&
                            !smgpc::compat::
                                name_obj_runtime_postpass_is_delegated(
                                    created.descendants[3U].object),
                        "cross-owner postpass did not follow exact global registration order once");
                instantiator.clear();
            }
            require(context.events ==
                        std::vector<std::string>{
                            "construct:A", "init:A",
                            "construct:B", "init:B",
                            "after:A", "after:B",
                            "delete:B", "delete:A"} &&
                        !holder.isExist(SceneObj_15) &&
                        !holder.isExist(SceneObj_35) &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            baseline,
                    "delegated cross-owner graph did not cleanly retire");
        };

        run_generation();
        run_generation();
    }

    void test_postpass_delegation_cleans_up_on_early_clear() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto data = make_data({make_placement(
            "EarlyClear", StagePlacementLoadBatch::CommonBootstrap,
            "ObjInfo", true, 0U, 0)});
        auto holder = SceneObjHolder{};
        auto context = SyntheticSceneObjContext{};
        {
            auto binding = smgpc::scene::SceneObjHolderBinding(
                holder, synthetic_scene_obj_factory, &context);
            auto lifecycle = RecordingLifecycle{};
            lifecycle.lazy_scene_obj_object = "actor:EarlyClear@0";
            lifecycle.lazy_scene_obj_id = SceneObj_15;
            auto instantiator =
                smgpc::scene::AuthoredPlacementInstantiator(
                    data, lifecycle, synthetic_ready_options());
            (void)instantiator.preload();
            const auto &created = instantiator.instantiate();
            require(created.descendants.size() == 2U &&
                        std::ranges::all_of(
                            created.descendants,
                            [](const auto &descendant) {
                                return smgpc::compat::
                                    name_obj_runtime_postpass_is_delegated(
                                        descendant.object);
                            }),
                    "early-clear fixture did not establish external postpass delegates");
            auto *outer = created.descendants[0U].object;
            auto *inner = created.descendants[1U].object;
            instantiator.clear();
            require(!smgpc::compat::
                        name_obj_runtime_postpass_is_delegated(outer) &&
                        !smgpc::compat::
                            name_obj_runtime_postpass_is_delegated(inner),
                    "clear-before-postpass retained a dangling instantiator delegate");

            binding.init_after_placement();
            require(context.events ==
                        std::vector<std::string>{
                            "construct:A", "init:A",
                            "construct:B", "init:B",
                            "after:A", "after:B"},
                    "released early-clear identities were not available to the holder fallback postpass");
        }
        require(context.events ==
                    std::vector<std::string>{
                        "construct:A", "init:A",
                        "construct:B", "init:B",
                        "after:A", "after:B",
                        "delete:B", "delete:A"} &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline,
                "early-clear delegation cleanup did not support clean teardown");
    }

    void test_postpass_failure_releases_current_and_pending_delegates() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto data = make_data({make_placement(
            "PostpassFail", StagePlacementLoadBatch::CommonBootstrap,
            "ObjInfo", true, 0U, 0)});
        auto holder = SceneObjHolder{};
        auto context = SyntheticSceneObjContext{};
        {
            auto binding = smgpc::scene::SceneObjHolderBinding(
                holder, synthetic_scene_obj_factory, &context);
            auto lifecycle = RecordingLifecycle{};
            lifecycle.lazy_scene_obj_object = "actor:PostpassFail@0";
            lifecycle.lazy_scene_obj_id = SceneObj_15;
            lifecycle.throw_after_actor_name = "nested-scene-a";
            auto instantiator =
                smgpc::scene::AuthoredPlacementInstantiator(
                    data, lifecycle, synthetic_ready_options());
            (void)instantiator.preload();
            const auto &created = instantiator.instantiate();
            auto *outer = created.descendants[0U].object;
            auto *inner = created.descendants[1U].object;
            binding.init_after_placement();

            auto preserved_failure = false;
            try {
                (void)instantiator.init_after_placement();
            } catch (const std::runtime_error &error) {
                preserved_failure = std::string_view(error.what()) ==
                                    "synthetic authored postpass failure";
            }
            require(preserved_failure &&
                        !smgpc::compat::
                            name_obj_runtime_postpass_is_delegated(outer) &&
                        smgpc::compat::
                            name_obj_runtime_postpass_is_delegated(inner),
                    "postpass failure did not release its attempted external identity exactly");
            instantiator.clear();
            require(!smgpc::compat::
                        name_obj_runtime_postpass_is_delegated(outer) &&
                        !smgpc::compat::
                            name_obj_runtime_postpass_is_delegated(inner),
                    "failed postpass clear retained a pending external delegate");
        }
        require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                "failed postpass delegation cleanup leaked its SceneObj graph");
    }

    void test_recursive_scene_obj_transaction_preserves_global_order() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto holder = SceneObjHolder{};

        const auto run_generation = [&] {
            auto context = SyntheticSceneObjContext{};
            {
                auto binding = smgpc::scene::SceneObjHolderBinding(
                    holder, synthetic_scene_obj_factory, &context);
                auto *outer = holder.create(SceneObj_15);
                auto *inner = holder.getObj(SceneObj_35);
                require(outer != nullptr && inner != nullptr &&
                            smgpc::scene::
                                current_scene_obj_holder_binding_owns(
                                    outer) &&
                            smgpc::scene::
                                current_scene_obj_holder_binding_owns(
                                    inner) &&
                            context.events ==
                                std::vector<std::string>{
                                    "construct:A", "init:A",
                                    "construct:B", "init:B"},
                        "recursive SceneObj transaction lost a root or construction order");
                binding.init_after_placement();
                require(context.events ==
                            std::vector<std::string>{
                                "construct:A", "init:A",
                                "construct:B", "init:B",
                                "after:A", "after:B"},
                        "recursive SceneObj postpass did not retain global A-to-B construction order");
            }
            require(context.events ==
                        std::vector<std::string>{
                            "construct:A", "init:A",
                            "construct:B", "init:B",
                            "after:A", "after:B",
                            "delete:B", "delete:A"} &&
                        !holder.isExist(SceneObj_15) &&
                        !holder.isExist(SceneObj_35) &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            baseline,
                    "recursive SceneObj graph did not retire in exact B-to-A reverse order");
        };

        run_generation();
        run_generation();
    }

    void test_scene_obj_postpass_consumes_callback_appends_once() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto holder = SceneObjHolder{};
        auto context = SyntheticSceneObjContext{
            .create_nested_in_postpass = true,
        };
        {
            auto binding = smgpc::scene::SceneObjHolderBinding(
                holder, synthetic_scene_obj_factory, &context);
            require(holder.create(SceneObj_15) != nullptr &&
                        !holder.isExist(SceneObj_35),
                    "postpass fixture created its nested SceneObj too early");
            binding.init_after_placement();
            binding.init_after_placement();
            require(context.events ==
                            std::vector<std::string>{
                                "construct:A", "init:A", "after:A",
                                "construct:B", "init:B", "after:B"} &&
                        holder.isExist(SceneObj_35),
                    "SceneObj postpass did not consume a callback-time append exactly once in order");
        }
        require(context.events ==
                        std::vector<std::string>{
                            "construct:A", "init:A", "after:A",
                            "construct:B", "init:B", "after:B", "delete:B",
                            "delete:A"} &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline,
                "callback-appended SceneObj generation did not retire in reverse order");
    }

    void test_recursive_scene_obj_failure_rolls_back_slots_and_recreates() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto holder = SceneObjHolder{};
        auto context = SyntheticSceneObjContext{
            .throw_outer_init = true,
        };
        {
            auto binding = smgpc::scene::SceneObjHolderBinding(
                holder, synthetic_scene_obj_factory, &context);
            auto preserved_failure = false;
            try {
                (void)holder.create(SceneObj_15);
            } catch (const std::runtime_error &error) {
                preserved_failure = std::string_view(error.what()) ==
                                    "synthetic outer SceneObj init failure";
            }
            require(preserved_failure &&
                        context.events ==
                            std::vector<std::string>{
                                "construct:A", "init:A",
                                "construct:B", "init:B",
                                "delete:B", "delete:A"} &&
                        !holder.isExist(SceneObj_15) &&
                        !holder.isExist(SceneObj_35) &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            baseline,
                    "outer SceneObj init failure left a nested slot, dangling owner, or changed the exception");

            context.events.clear();
            context.throw_outer_init = false;
            require(holder.create(SceneObj_15) != nullptr &&
                        holder.getObj(SceneObj_35) != nullptr,
                    "recursive SceneObj graph could not recreate after rollback");
            binding.init_after_placement();
            require(context.events ==
                        std::vector<std::string>{
                            "construct:A", "init:A",
                            "construct:B", "init:B",
                            "after:A", "after:B"},
                    "recreated recursive SceneObj graph lost its postpass order");
        }
        require(context.events ==
                    std::vector<std::string>{
                        "construct:A", "init:A",
                        "construct:B", "init:B",
                        "after:A", "after:B",
                        "delete:B", "delete:A"} &&
                    !holder.isExist(SceneObj_15) &&
                    !holder.isExist(SceneObj_35) &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        baseline,
                "recreated recursive SceneObj graph did not cleanly retire");
    }

    void test_recursive_scene_obj_null_factory_rolls_back_side_effects() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto holder = SceneObjHolder{};
        auto context = SyntheticSceneObjContext{
            .return_null_after_nested = true,
        };
        {
            auto binding = smgpc::scene::SceneObjHolderBinding(
                holder, synthetic_scene_obj_factory, &context);
            auto rejected_side_effecting_null = false;
            try {
                (void)holder.create(SceneObj_15);
            } catch (const std::logic_error& error) {
                rejected_side_effecting_null =
                    std::string_view(error.what()).find(
                        "returned null after creating nested") !=
                    std::string_view::npos;
            }
            require(rejected_side_effecting_null &&
                        context.events ==
                            std::vector<std::string>{
                                "construct:B", "init:B", "delete:B"} &&
                        !holder.isExist(SceneObj_15) &&
                        !holder.isExist(SceneObj_35) &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            baseline,
                    "a null outer SceneObj factory leaked its nested slot or registration");

            context.events.clear();
            context.return_null_after_nested = false;
            require(holder.create(SceneObj_15) != nullptr &&
                        holder.getObj(SceneObj_35) != nullptr,
                    "recursive SceneObj graph did not recreate after null-factory rollback");
            binding.init_after_placement();
        }
        require(context.events ==
                        std::vector<std::string>{
                            "construct:A", "init:A", "construct:B",
                            "init:B", "after:A", "after:B", "delete:B",
                            "delete:A"} &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline,
                "recreated null-factory SceneObj graph did not retire cleanly");
    }

    void test_scene_obj_holder_adopts_switch_watchers_without_parent_delete() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto data = make_data({make_placement(
            "SwitchRoot", StagePlacementLoadBatch::CommonBootstrap,
            "ObjInfo", true, 0U, 0)});
        auto holder = SceneObjHolder{};

        const auto run_generation = [&] {
            auto context = SyntheticSceneObjContext{};
            {
                auto binding = smgpc::scene::SceneObjHolderBinding(
                    holder, synthetic_scene_obj_factory, &context);
                auto *watcher_holder =
                    dynamic_cast<SyntheticSwitchWatcherHolder *>(
                        holder.create(SceneObj_SwitchWatcherHolder));
                require(watcher_holder != nullptr,
                        "synthetic SwitchWatcherHolder was not created");
                auto *watcher = static_cast<SyntheticSwitchWatcher *>(
                    nullptr);
                auto lifecycle = RecordingLifecycle{};
                lifecycle.init_hook = [&](NameObj &object) {
                    if (std::string_view(object.getName()) !=
                        "actor:SwitchRoot@0") {
                        return;
                    }
                    watcher = new SyntheticSwitchWatcher(context);
                    watcher_holder->addSwitchWatcher(watcher);
                };
                auto instantiator =
                    smgpc::scene::AuthoredPlacementInstantiator(
                        data, lifecycle, synthetic_ready_options());
                (void)instantiator.preload();
                const auto &created = instantiator.instantiate();
                require(watcher != nullptr &&
                            created.descendants.size() == 1U &&
                            created.descendants[0U].object == watcher &&
                            !created.descendants[0U].owned_by_placement &&
                            smgpc::scene::
                            current_scene_obj_holder_binding_owns(
                                watcher_holder) &&
                            smgpc::scene::
                                current_scene_obj_holder_binding_owns(
                                    watcher) &&
                            !smgpc::compat::
                                name_obj_runtime_ownership_is_claimed(
                                    watcher),
                        "SwitchWatcher remained parent-owned or escaped the SceneObj graph");
                binding.init_after_placement();
                require(context.events ==
                            std::vector<std::string>{
                                "construct:watcher-holder",
                                "construct:watcher",
                                "after:watcher-holder"},
                        "SceneObj pre-pass duplicated its delegated SwitchWatcher");
                const auto events_before_after = lifecycle.events.size();
                (void)instantiator.init_after_placement();
                const std::string watcher_lifecycle_event =
                    "after:" + std::string(watcher->getName());
                require(std::vector<std::string>(
                            lifecycle.events.begin() +
                                static_cast<std::ptrdiff_t>(
                                    events_before_after),
                            lifecycle.events.end()) ==
                            std::vector<std::string>{
                                "after:actor:SwitchRoot@0",
                                watcher_lifecycle_event} &&
                            context.events ==
                                std::vector<std::string>{
                                    "construct:watcher-holder",
                                    "construct:watcher",
                                    "after:watcher-holder",
                                    "after:watcher"} &&
                            !smgpc::compat::
                                name_obj_runtime_postpass_is_delegated(
                                    watcher),
                        "delegated SwitchWatcher lost registration order or ran twice");
                instantiator.clear();
            }
            require(context.events ==
                        std::vector<std::string>{
                            "construct:watcher-holder",
                            "construct:watcher",
                            "after:watcher-holder",
                            "after:watcher",
                            "delete:watcher",
                            "delete:watcher-holder"} &&
                        !holder.isExist(SceneObj_SwitchWatcherHolder) &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            baseline,
                    "SwitchWatcher was not retired before its retail-empty holder or was double-owned");
        };

        run_generation();
        run_generation();
    }

}  // namespace

int main() {
    try {
        test_synthetic_default_construction_scope_needs_no_holder();
        test_nameobj_adapter_retains_strict_zone_scope_source();
        test_model_changing_rows_require_their_exact_creator();
        test_holder_occurrence_discovery_preserves_duplicates_and_bounds_cycles();
        test_resolver_retains_missing_and_authored_empty_identifiers();
        test_archive_metadata_is_independent_of_creator_support();
        test_holder_provenance_and_attachment_ordinals();
        test_strict_preflight_is_support_only_and_non_mutating();
        test_five_pass_grouping_preload_and_lifecycle_order();
        test_group_creator_recheck_and_shared_actor_name_fallback();
        test_retail_shell_equal_key_permutation();
        test_archive_failure_precedes_all_construction();
        test_construction_failure_rolls_back_actual_actors_in_reverse();
        test_authored_descendants_share_global_postpass_and_reverse_teardown();
        test_authored_descendant_init_failure_rolls_back_exact_suffix();
        test_scene_obj_created_inside_capture_keeps_independent_owner();
        test_failed_authored_root_preserves_lazy_scene_obj_graph();
        test_cross_owner_postpass_stays_in_registration_order_once();
        test_postpass_delegation_cleans_up_on_early_clear();
        test_postpass_failure_releases_current_and_pending_delegates();
        test_recursive_scene_obj_transaction_preserves_global_order();
        test_scene_obj_postpass_consumes_callback_appends_once();
        test_recursive_scene_obj_failure_rolls_back_slots_and_recreates();
        test_recursive_scene_obj_null_factory_rolls_back_side_effects();
        test_scene_obj_holder_adopts_switch_watchers_without_parent_delete();
        std::cout << "[ok] shared authored placement instantiator contract passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] authored placement instantiator: "
                  << error.what() << '\n';
        return 1;
    }
}
