#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/CubeCamera.hpp"
#include "Game/AreaObj/LightArea.hpp"
#include "Game/AreaObj/LightAreaHolder.hpp"
#include "Game/AreaObj/MessageArea.hpp"
#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/AreaObj/MercatorTransformCube.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/LightZoneDataHolder.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "render/light/LightData.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageLightSceneBinding.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template <typename Exception, typename Function>
    void require_throws(Function &&function, std::string_view message_fragment) {
        try {
            function();
        } catch (const Exception &error) {
            require(std::string_view(error.what()).find(message_fragment) != std::string_view::npos,
                    "the rejected operation must explain its missing real dependency");
            return;
        }
        throw std::runtime_error("the unavailable operation silently returned instead of rejecting the query");
    }

    void test_area_queries_reject_missing_scene_owner() {
        constexpr auto position = TVec3f{10.0F, 20.0F, 30.0F};

        require_throws<std::logic_error>([] { (void)MR::getAreaObjContainer(); }, "active scene-owned");
        require_throws<std::logic_error>([&] { (void)MR::isInDeath(position); }, "active scene-owned");
        require_throws<std::logic_error>([&] { (void)MR::isInDarkMatter(position); }, "active scene-owned");
    }

    [[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) {
        return value.size() >= prefix.size() && value.substr(0U, prefix.size()) == prefix;
    }

    [[nodiscard]] std::string read_file(const std::filesystem::path &path) {
        auto input = std::ifstream(path, std::ios::binary);
        require(input.is_open(), "the source-boundary fixture must be readable");
        return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }

    [[nodiscard]] std::optional<std::filesystem::path> find_pc_port_root() {
        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        if (error) {
            return std::nullopt;
        }
        while (true) {
            const auto source = directory / "src/Game/AreaObj/CubeCamera.cpp";
            const auto retail_source = directory.parent_path() / "src/Game/AreaObj/CubeCamera.cpp";
            if (std::filesystem::is_regular_file(source, error) && !error &&
                std::filesystem::is_regular_file(retail_source, error) && !error) {
                return directory;
            }
            error.clear();
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::filesystem::path> find_real_disc() {
        if (const auto *configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            return std::filesystem::path(configured);
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        if (error) {
            return std::nullopt;
        }
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) && !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        return std::nullopt;
    }

    void test_area_obj_source_boundaries_are_exact() {
        const auto pc_port_root = find_pc_port_root();
        require(pc_port_root.has_value(), "the source-boundary test must locate the pc-port source root");
        const auto decomp_root = pc_port_root->parent_path();
        constexpr auto source_pairs = std::array{
            std::pair{"include/Game/AreaObj/CubeCamera.hpp", "src/Game/AreaObj/CubeCamera.hpp"},
            std::pair{"src/Game/AreaObj/CubeCamera.cpp", "src/Game/AreaObj/CubeCamera.cpp"},
            std::pair{"include/Game/AreaObj/MessageArea.hpp", "src/Game/AreaObj/MessageArea.hpp"},
            std::pair{"src/Game/AreaObj/MessageArea.cpp", "src/Game/AreaObj/MessageArea.cpp"},
            std::pair{"include/Game/AreaObj/LightArea.hpp", "src/Game/AreaObj/LightArea.hpp"},
            std::pair{"src/Game/AreaObj/LightArea.cpp", "src/Game/AreaObj/LightArea.cpp"},
            std::pair{"include/Game/AreaObj/LightAreaHolder.hpp", "src/Game/AreaObj/LightAreaHolder.hpp"},
            std::pair{"src/Game/AreaObj/LightAreaHolder.cpp", "src/Game/AreaObj/LightAreaHolder.cpp"},
        };
        for (const auto &[retail_source, host_source] : source_pairs) {
            require(read_file(decomp_root / retail_source) == read_file(*pc_port_root / host_source),
                    "completed PC AreaObj sources must remain byte-identical to the decompiled source");
        }
    }

    void verify_installed_descriptor_managers(AreaObjContainer &container) {
        const auto descriptors = smgpc::scene::complete_area_obj_placement_descriptors();
        auto installed_manager_names = std::vector<std::string_view>{};
        for (const auto &descriptor : descriptors) {
            require(!descriptor.object_name.empty() && descriptor.object_creator != nullptr &&
                        !descriptor.manager_name.empty() && descriptor.retail_manager_order >= 0 &&
                        descriptor.manager_capacity > 0 &&
                        descriptor.manager_creator != nullptr,
                    "the public AreaObj registry must expose complete descriptors only");
            require(smgpc::scene::find_complete_area_obj_placement_descriptor(descriptor.object_name) ==
                        &descriptor,
                    "descriptor lookup must return the canonical registry entry");

            const auto manager_name = std::string(descriptor.manager_name);
            auto *manager = container.getManager(manager_name.c_str());
            require(manager != nullptr && manager->_18 == descriptor.manager_capacity,
                    "the container must construct each descriptor's real manager with retail capacity");

            if (std::ranges::find(installed_manager_names, descriptor.manager_name) ==
                installed_manager_names.end()) {
                installed_manager_names.push_back(descriptor.manager_name);
            }

            const auto derived_name = manager_name + "PlacementVariant";
            const auto expected = std::ranges::find_if(installed_manager_names, [&](const auto &installed_name) {
                return starts_with(derived_name, installed_name);
            });
            require(expected != installed_manager_names.end() &&
                        std::string_view(container.getManager(derived_name.c_str())->mName) == *expected,
                    "manager lookup must preserve retail prefix and first-match ordering");
        }
    }

    void test_retail_prefix_collision_uses_first_manager() {
        auto short_prefix = AreaObjMgr{4, "Prefix"};
        auto long_prefix = AreaObjMgr{4, "PrefixLong"};
        const auto query = std::string_view{"PrefixLongCube"};

        auto retail_order = std::array<AreaObjMgr *, 2U>{&short_prefix, &long_prefix};
        require(smgpc::scene::find_area_obj_manager_by_retail_prefix(retail_order, query) ==
                    &short_prefix,
                "a prefix collision must select the first manager in retail descriptor order");

        auto reversed_order = std::array<AreaObjMgr *, 2U>{&long_prefix, &short_prefix};
        require(smgpc::scene::find_area_obj_manager_by_retail_prefix(reversed_order, query) ==
                    &long_prefix,
                "prefix selection must follow registry order rather than longest-name preference");
    }

    class LifecycleManager final : public AreaObjMgr {
    public:
        LifecycleManager(int id, std::vector<int> &events, int &destroyed)
            : AreaObjMgr(4, "LifecycleManager"), _id(id), _events(events), _destroyed(destroyed) {
        }

        ~LifecycleManager() override {
            ++_destroyed;
        }

        void initAfterPlacement() override {
            _events.push_back(_id);
        }

        void finalize() {
            _events.push_back(100 + _id);
        }

    private:
        int _id;
        std::vector<int> &_events;
        int &_destroyed;
    };

    void finalize_lifecycle_manager(AreaObjMgr &manager) {
        dynamic_cast<LifecycleManager &>(manager).finalize();
    }

    void test_manager_lifecycle_is_owned_ordered_and_once() {
        auto events = std::vector<int>{};
        auto destroyed = 0;
        {
            auto runtime = smgpc::scene::AreaObjRuntime{};
            (void)runtime.adopt_manager(
                std::make_unique<LifecycleManager>(1, events, destroyed),
                finalize_lifecycle_manager);
            (void)runtime.adopt_manager(
                std::make_unique<LifecycleManager>(2, events, destroyed),
                finalize_lifecycle_manager);
            runtime.init_after_placement();
            runtime.init_after_placement();
            require(events == std::vector<int>({1, 2, 101, 102}),
                    "scene-owned AreaObj managers must initialize then finalize in creation order exactly once");
            require_throws<std::logic_error>(
                [&] {
                    (void)runtime.adopt_manager(
                        std::make_unique<LifecycleManager>(3, events, destroyed));
                },
                "after the scene post-placement phase");
        }
        require(destroyed == 3,
                "AreaObjRuntime must destroy every adopted manager, including a rejected late adoption");
    }

    void configure_unit_cube(CubeCameraArea &area, s32 priority, s32 category_mask) {
        area.mObjArg2 = priority;
        area._3C = category_mask;
        auto *form = dynamic_cast<AreaFormCube *>(area.mForm);
        require(form != nullptr, "the priority fixture must use the exact CubeCamera cube form");
        form->mTranslation.set(0.0F, 0.0F, 0.0F);
        form->mRotation.set(0.0F, 0.0F, 0.0F);
        form->mScale.set(1.0F, 1.0F, 1.0F);
        form->updateBoxParam();
    }

    void test_cube_camera_manager_finalizes_priority_and_reverse_query() {
        auto holder = SceneObjHolder{};
        auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        auto *container = dynamic_cast<AreaObjContainer *>(holder.create(SceneObj_AreaObjContainer));
        require(container != nullptr, "the CubeCamera fixture requires the real scene-owned container");

        auto *manager = dynamic_cast<CubeCameraMgr *>(container->getManager("CubeCamera"));
        require(manager != nullptr && manager->_18 == 0xA0,
                "all CubeCamera forms must share the exact retail manager and capacity");
        for (const auto name : {"CubeCameraBox", "CubeCameraCylinder", "CubeCameraSphere", "CubeCameraBowl"}) {
            require(container->getManager(name) == manager,
                    "retail prefix lookup must route every CubeCamera form to one deduplicated manager");
        }

        auto high = CubeCameraArea(AreaForm::Type_Cube1, "CubeCamera");
        auto low = CubeCameraArea(AreaForm::Type_Cube1, "CubeCamera");
        auto middle = CubeCameraArea(AreaForm::Type_Cube1, "CubeCamera");
        configure_unit_cube(high, 9, 1);
        configure_unit_cube(low, 1, 1);
        configure_unit_cube(middle, 5, 1);
        manager->entry(&high);
        manager->entry(&low);
        manager->entry(&middle);

        binding.init_after_placement();
        require(manager->getAreaObj(0) == &low && manager->getAreaObj(1) == &middle &&
                    manager->getAreaObj(2) == &high,
                "the generalized manager-finalize callback must run CubeCameraMgr::initAfterLoad priority sorting");
        binding.init_after_placement();
        require(manager->getAreaObj(0) == &low && manager->getAreaObj(1) == &middle &&
                    manager->getAreaObj(2) == &high,
                "the CubeCamera manager finalizer must not run a second time");

        constexpr auto origin = TVec3f{0.0F, 0.0F, 0.0F};
        CubeCameraArea::setCurrentCategory(0);
        require(manager->find_in(origin) == &high,
                "the sorted retail manager must reverse-query the highest-priority overlapping camera");
        CubeCameraArea::setCurrentCategory(1);
        require(manager->find_in(origin) == nullptr,
                "CubeCamera queries must reject areas outside the current category mask");
        high._3C = 2;
        require(manager->find_in(origin) == &high,
                "CubeCamera queries must accept the matching current category bit");
        CubeCameraArea::setCurrentCategory(0);
    }

    void configure_light_cube(LightArea &area, const TVec3f &translation, s32 zone_id,
                              s32 light_id, s32 priority) {
        area.mPlacedZoneID = zone_id;
        area.mObjArg0 = light_id;
        area.mObjArg1 = priority;
        auto *form = dynamic_cast<AreaFormCube *>(area.mForm);
        require(form != nullptr, "the light-area fixture must use the exact base-origin cube form");
        form->mTranslation = translation;
        form->mRotation.set(0.0F, 0.0F, 0.0F);
        form->mScale.set(1.0F, 1.0F, 1.0F);
        form->updateBoxParam();
    }

    void test_light_area_priority_and_stable_zone_identity() {
        auto holder = SceneObjHolder{};
        auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        auto *container = dynamic_cast<AreaObjContainer *>(holder.create(SceneObj_AreaObjContainer));
        require(container != nullptr, "the LightArea fixture requires the real scene-owned container");
        auto *manager = dynamic_cast<LightAreaHolder *>(container->getManager("LightArea"));
        require(manager != nullptr && manager->_18 == 0x80,
                "both exact LightCtrl forms must share the retail LightArea manager and capacity");

        auto lower = LightArea(AreaForm::Type_Cube2, "LightCtrlCube");
        auto higher = LightArea(AreaForm::Type_Cube2, "LightCtrlCube");
        configure_light_cube(lower, TVec3f{0.0F, 0.0F, 0.0F}, 2, 4, 3);
        configure_light_cube(higher, TVec3f{0.0F, 0.0F, 0.0F}, 5, 7, 10);
        manager->entry(&higher);
        manager->entry(&lower);
        binding.init_after_placement();
        require(manager->getAreaObj(0) == &lower && manager->getAreaObj(1) == &higher,
                "LightAreaHolder must sort authored priorities before reverse-volume lookup");

        auto light_id = ZoneLightID{};
        require(manager->tryFindLightID(TVec3f{0.0F, 100.0F, 0.0F}, &light_id) &&
                    light_id._0 == 5 && light_id.mLightID == 7,
                "overlapping LightCtrl volumes must select the highest authored priority and zone ID");
        require(!manager->tryFindLightID(TVec3f{0.0F, 100.0F, 0.0F}, &light_id),
                "ZoneLightID::isTargetArea must suppress a repeated change inside the same authored area");
        require(manager->tryFindLightID(TVec3f{5000.0F, 5000.0F, 5000.0F}, &light_id) &&
                    light_id._0 == -1 && light_id.mLightID == -1,
                "leaving a LightCtrl volume must clear the exact zone/light identity once");
        require(!manager->tryFindLightID(TVec3f{5000.0F, 5000.0F, 5000.0F}, &light_id),
                "remaining outside every LightCtrl volume must not retrigger a light transition");

        auto moving_actor = LiveActor{"moving LightArea fixture"};
        auto scheduler = smgpc::runtime::SceneScheduler{};
        const auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding{scheduler};
        scheduler.register_live_actor_model(
            moving_actor, MR::MovementType_NPC, MR::CalcAnimType_NPC,
            MR::DrawBufferType_NPC, -1);
        auto light_ctrl = std::make_unique<ActorLightCtrl>(&moving_actor);
        moving_actor.mActorLightCtrl = light_ctrl.get();
        light_ctrl->init(-1, false);
        require(light_ctrl->_4 == MR::LightType_Strong,
                "a controller created after connectToScene must inherit the draw-buffer's retained retail light type");
        moving_actor.makeActorAppeared();
        moving_actor.mPosition.set(0.0F, 100.0F, 0.0F);
        moving_actor.movement();
        require(light_ctrl->mLightID._0 == 5 && light_ctrl->mLightID.mLightID == 7,
                "LiveActor movement must update its controller into the highest-priority authored LightArea");
        moving_actor.mPosition.set(5000.0F, 5000.0F, 5000.0F);
        moving_actor.movement();
        require(light_ctrl->mLightID._0 == -1 && light_ctrl->mLightID.mLightID == -1,
                "LiveActor movement must update its controller when leaving authored LightAreas");
        scheduler.unregister_live_actor_model(moving_actor);
        moving_actor.mActorLightCtrl = nullptr;
    }

    void test_scene_holder_owns_real_container_and_managers() {
        {
            auto holder = SceneObjHolder{};
            auto binding = smgpc::scene::SceneObjHolderBinding(holder);
            auto *object = holder.create(SceneObj_AreaObjContainer);
            auto *container = dynamic_cast<AreaObjContainer *>(object);

            require(container != nullptr && holder.isExist(SceneObj_AreaObjContainer) &&
                        MR::getAreaObjContainer() == container,
                    "a bound scene must install the real AreaObjContainer SceneObj");
            require(holder.create(SceneObj_AreaObjContainer) == container,
                    "SceneObj creation must retain one container per scene");
            verify_installed_descriptor_managers(*container);
            require_throws<std::logic_error>(
                [&] { (void)container->getManager("__SMGPC_missing_area_manager__"); },
                "No complete retail AreaObj manager");
            require_throws<std::invalid_argument>([&] { (void)container->getManager(nullptr); },
                                                  "non-null retail name");

            binding.init_after_placement();
            binding.init_after_placement();
        }

        auto detached_light = ZoneLightID{};
        require(!LightFunction::tryFindNewAreaLightID(TVec3f{}, &detached_light) &&
                    detached_light._0 == -1 && detached_light.mLightID == -1,
                "destroying the scene-owned LightArea manager must detach its non-owning lookup before reuse");

        auto second_holder = SceneObjHolder{};
        const auto second_binding = smgpc::scene::SceneObjHolderBinding(second_holder);
        auto *second_container = dynamic_cast<AreaObjContainer *>(
            second_holder.create(SceneObj_AreaObjContainer));
        require(second_container != nullptr &&
                    dynamic_cast<LightAreaHolder *>(second_container->getManager("LightArea")) != nullptr,
                "destroying a scene binding must release container and manager ownership for the next scene");
    }

    void test_descriptor_registry_and_strict_area_preflight() {
        require(smgpc::scene::is_area_obj_placement_table("jmp/placement/common/areaobjinfo") &&
                    smgpc::scene::is_area_obj_placement_table("Jmp/Placement/Common/AreaObjInfo.bcsv") &&
                    !smgpc::scene::is_area_obj_placement_table("jmp/placement/common/planetobjinfo"),
                "AreaObj table classification must be path- and case-stable without stage-name policy");
        require(smgpc::scene::find_complete_area_obj_placement_descriptor(
                    "__SMGPC_missing_area_descriptor__") == nullptr,
                "unknown AreaObj placements must not acquire a synthetic descriptor");

        require(smgpc::scene::placement_has_complete_area_obj_runtime(
                    "GlobalPointGravity", "jmp/placement/common/planetobjinfo", true),
                "a complete non-area factory route must remain eligible for stage preflight");
        require(!smgpc::scene::placement_has_complete_area_obj_runtime(
                    "GlobalPointGravity", "jmp/placement/common/planetobjinfo", false),
                "a missing actor creator must remain blocked regardless of placement table");
        require(!smgpc::scene::placement_has_complete_area_obj_runtime(
                    "__SMGPC_missing_area_descriptor__", "jmp/placement/common/areaobjinfo", true),
                "an AreaObj creator cannot bypass preflight without its registered retail manager closure");

        const auto descriptors = smgpc::scene::complete_area_obj_placement_descriptors();
        constexpr auto expected_descriptors = std::array{
            std::tuple{"CubeCameraBox", "CubeCamera", 4, 0xA0, AreaForm::Type_Cube1, true},
            std::tuple{"CubeCameraCylinder", "CubeCamera", 4, 0xA0, AreaForm::Type_Cylinder, true},
            std::tuple{"CubeCameraSphere", "CubeCamera", 4, 0xA0, AreaForm::Type_Sphere, true},
            std::tuple{"CubeCameraBowl", "CubeCamera", 4, 0xA0, AreaForm::Type_Bowl, true},
            std::tuple{"PullBackCylinder", "PullBackCylinder", 17, 0x40, AreaForm::Type_Cylinder, false},
            std::tuple{"ViewGroupCtrlCube", "ViewGroupCtrlCube", 32, 0x40, AreaForm::Type_Cube2, false},
            std::tuple{"LensFlareArea", "LensFlareArea", 33, 0x40, AreaForm::Type_Cube2, false},
            std::tuple{"LightCtrlCube", "LightArea", 35, 0x80, AreaForm::Type_Cube2, false},
            std::tuple{"LightCtrlCylinder", "LightArea", 35, 0x80, AreaForm::Type_Cylinder, false},
            std::tuple{"BlueStarGuidanceCube", "BlueStarGuidanceCube", 40, 0x10, AreaForm::Type_Cube2, false},
            std::tuple{"MessageAreaCube", "MessageArea", 42, 0x10, AreaForm::Type_Cube2, false},
            std::tuple{"MessageAreaCylinder", "MessageArea", 42, 0x10, AreaForm::Type_Cylinder, false},
        };
        require(descriptors.size() == expected_descriptors.size(),
                "the registry must contain only the completed exact and passive AreaObj closures");
        for (auto index = std::size_t{}; index < expected_descriptors.size(); ++index) {
            const auto &[object_name, manager_name, retail_order, capacity, form_type, has_finalize] =
                expected_descriptors[index];
            const auto &descriptor = descriptors[index];
            require(descriptor.object_name == object_name && descriptor.manager_name == manager_name &&
                        descriptor.retail_manager_order == retail_order &&
                        descriptor.manager_capacity == capacity &&
                        (descriptor.manager_finalize != nullptr) == has_finalize,
                    "the completed descriptor subset must retain exact retail manager-table data");
            auto actor = std::unique_ptr<NameObj>(descriptor.object_creator(object_name));
            const auto *area = dynamic_cast<const AreaObj *>(actor.get());
            require(area != nullptr && area->mFormType == form_type,
                    "each descriptor must retain its exact retail AreaForm creator");
        }
        for (auto index = std::size_t{1U}; index < descriptors.size(); ++index) {
            require(descriptors[index - 1U].retail_manager_order <=
                        descriptors[index].retail_manager_order,
                    "the complete descriptor subset must preserve retail cCreateTable order");
        }
        if (!descriptors.empty()) {
            require(smgpc::scene::placement_has_complete_area_obj_runtime(
                        descriptors.front().object_name, "jmp/placement/common/areaobjinfo", true),
                    "a descriptor-backed AreaObj creator must pass the shared stage preflight predicate");
        }
    }

    void test_rmgk01_cube_camera_rows_construct_exactly() {
        const auto disc_path = find_real_disc();
        if (!disc_path.has_value()) {
            std::cout << "[skip] RMGK01 CubeCamera placement test (set SMGPC_REAL_DISC or place RMGK01.iso in a workspace ancestor)\n";
            return;
        }

        aurora_dvd_close();
        const auto disc_path_string = disc_path->string();
        require(aurora_dvd_open(disc_path_string.c_str()),
                "the RMGK01 CubeCamera fixture must be a readable SMG disc image");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        const auto placements = smgpc::scene::resolve_stage_placement_objects(
            dvd, "HeavensDoorGalaxy", 1);
        auto camera_rows = std::vector<const smgpc::scene::StagePlacementObject *>{};
        for (const auto &placement : placements) {
            if (starts_with(placement.object_name, "CubeCamera")) {
                camera_rows.push_back(&placement);
            }
        }
        require(camera_rows.size() == 16U,
                "RMGK01 HeavensDoor scenario 1 must expose the known 16 CubeCamera rows");

        const auto count_name = [&](std::string_view name) {
            return std::ranges::count_if(camera_rows, [&](const auto *placement) {
                return placement->object_name == name;
            });
        };
        require(count_name("CubeCameraBox") == 2 && count_name("CubeCameraCylinder") == 8 &&
                    count_name("CubeCameraSphere") == 6 && count_name("CubeCameraBowl") == 0,
                "the real-disc CubeCamera form counts must match the RMGK01 placement frontier");
        require(std::ranges::all_of(camera_rows, [](const auto *placement) {
                    return placement->factory_supported && !placement->intentionally_ignored &&
                           placement->follow_id == -1 && placement->parent_id == -1 &&
                           placement->group_id == -1 && placement->clipping_group_id == -1 &&
                           placement->view_group_id == -1 && placement->object_args[3] == -1;
                }),
                "all 16 real CubeCamera rows must be factory-backed, standalone, and use the default category");

        require(std::ranges::count_if(camera_rows, [](const auto *placement) {
                    return placement->switch_appear_id == 1015;
                }) == 3 &&
                    std::ranges::count_if(camera_rows, [](const auto *placement) {
                        return placement->switch_a_id == 1125;
                    }) == 1 &&
                    std::ranges::count_if(camera_rows, [](const auto *placement) {
                        return placement->switch_a_id == 1127;
                    }) == 1 &&
                    std::ranges::all_of(camera_rows, [](const auto *placement) {
                        return (placement->switch_appear_id == -1 || placement->switch_appear_id == 1015) &&
                               (placement->switch_a_id == -1 || placement->switch_a_id == 1125 ||
                                placement->switch_a_id == 1127) &&
                               placement->switch_b_id == -1 && placement->switch_dead_id == -1 &&
                               placement->switch_sleep_id == -1;
                    }),
                "the exact RMGK01 CubeCamera switch frontier must remain three appear and two A-switch rows");

        auto holder = SceneObjHolder{};
        auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        for (const auto scene_obj : {SceneObj_StageSwitchContainer, SceneObj_SwitchWatcherHolder,
                                     SceneObj_SleepControllerHolder, SceneObj_AreaObjContainer}) {
            require(holder.create(scene_obj) != nullptr,
                    "real CubeCamera placement init requires each retail scene service");
        }
        auto *manager = dynamic_cast<CubeCameraMgr *>(MR::getAreaObjContainer()->getManager("CubeCamera"));
        require(manager != nullptr, "the real-disc rows must enter the exact CubeCamera manager");

        auto objects = std::vector<std::unique_ptr<NameObj>>{};
        objects.reserve(camera_rows.size());
        for (const auto *placement : camera_rows) {
            const auto *descriptor = smgpc::scene::find_complete_area_obj_placement_descriptor(
                placement->object_name);
            require(descriptor != nullptr, "each real CubeCamera row must have one complete descriptor");
            auto object = std::unique_ptr<NameObj>(descriptor->object_creator(placement->object_name.c_str()));
            auto *camera = dynamic_cast<CubeCameraArea *>(object.get());
            require(camera != nullptr, "each CubeCamera descriptor must construct the exact retail actor");
            const auto iter = JMapInfoIter(&placement->jmap_info, placement->jmap_entry_index);
            object->init(iter);

            const auto has_appear = placement->switch_appear_id >= 0;
            const auto has_a = placement->switch_a_id >= 0;
            const char *validity = nullptr;
            const auto explicitly_invalid = iter.getValue("Validity", &validity) &&
                                            std::string_view(validity) == "Invalid";
            require(camera->mObjArg2 == placement->object_args[2] && camera->mZoneID == placement->zone_id &&
                        camera->_3C == 1 && camera->mSwitchCtrl->isValidSwitchAppear() == has_appear &&
                        camera->isValidSwitchA() == has_a &&
                        camera->mIsValid == !(has_appear || has_a || explicitly_invalid),
                    "exact CubeCamera init must preserve priority/zone/category and switch-gated validity");
            objects.push_back(std::move(object));
        }

        binding.init_after_placement();
        require(manager->mArray.size() == camera_rows.size(),
                "all 16 real rows must enter the one scene-owned CubeCamera manager");
        constexpr auto expected_priorities = std::array<s32, 16U>{
            -1, 0, 0, 0, 1, 2, 5, 8, 8, 8, 9, 10, 10, 11, 12, 13,
        };
        for (auto index = std::size_t{}; index < expected_priorities.size(); ++index) {
            require(manager->getAreaObj(static_cast<int>(index))->mObjArg2 == expected_priorities[index],
                    "CubeCameraMgr::initAfterLoad must sort all real rows by retail priority");
        }
    }

    void test_rmgk01_message_area_rows_construct_exactly() {
        const auto disc_path = find_real_disc();
        if (!disc_path.has_value()) {
            std::cout << "[skip] RMGK01 MessageArea placement test (set SMGPC_REAL_DISC or place RMGK01.iso in a workspace ancestor)\n";
            return;
        }

        aurora_dvd_close();
        const auto disc_path_string = disc_path->string();
        require(aurora_dvd_open(disc_path_string.c_str()),
                "the RMGK01 MessageArea fixture must be a readable SMG disc image");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        const auto placements = smgpc::scene::resolve_stage_placement_objects(
            dvd, "HeavensDoorGalaxy", 1);
        constexpr auto expected_rows = std::array{
            std::tuple{"MessageAreaCube", "HeavensDoorSmallZone", 1, 1, 0,
                       AreaForm::Type_Cube2},
            std::tuple{"MessageAreaCylinder", "HeavensDoorMysteriousZone", 3, 14, 1,
                       AreaForm::Type_Cylinder},
        };

        auto holder = SceneObjHolder{};
        auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        for (const auto scene_obj : {SceneObj_StageSwitchContainer, SceneObj_SwitchWatcherHolder,
                                     SceneObj_SleepControllerHolder, SceneObj_AreaObjContainer}) {
            require(holder.create(scene_obj) != nullptr,
                    "real MessageArea placement init requires each retail scene service");
        }
        auto *manager = MR::getAreaObjContainer()->getManager("MessageArea");
        require(manager != nullptr && manager->_18 == 0x10 &&
                    MR::getAreaObjContainer()->getManager("MessageAreaCube") == manager &&
                    MR::getAreaObjContainer()->getManager("MessageAreaCylinder") == manager,
                "both MessageArea forms must share the retail order-42 manager and capacity");

        auto objects = std::vector<std::unique_ptr<NameObj>>{};
        objects.reserve(expected_rows.size());
        for (const auto &[object_name, zone_name, row_index, l_id, arg0, form_type] : expected_rows) {
            const auto placement = std::ranges::find_if(placements, [&](const auto &candidate) {
                return candidate.object_name == object_name && candidate.zone_name == zone_name &&
                       candidate.table_path == "jmp/placement/common/areaobjinfo" &&
                       candidate.jmap_entry_index == row_index;
            });
            require(placement != placements.end(),
                    "RMGK01 HeavensDoor scenario 1 must retain both exact MessageArea rows");
            require(placement->stage_name == zone_name && placement->layer_name == "common" &&
                        placement->l_id == l_id && placement->object_args[0] == arg0 &&
                        placement->factory_supported && !placement->intentionally_ignored &&
                        placement->switch_appear_id == -1 && placement->switch_dead_id == -1 &&
                        placement->switch_a_id == -1 && placement->switch_b_id == -1 &&
                        placement->switch_sleep_id == -1 && placement->follow_id == -1 &&
                        placement->group_id == -1 && placement->clipping_group_id == -1,
                    "each real MessageArea row must preserve its retail zone, argument, and switch metadata");
            for (auto index = std::size_t{1U}; index < placement->object_args.size(); ++index) {
                require(placement->object_args[index] == -1,
                        "unused real MessageArea arguments must remain at the retail sentinel");
            }

            const auto *descriptor = smgpc::scene::find_complete_area_obj_placement_descriptor(
                placement->object_name);
            require(descriptor != nullptr,
                    "each real MessageArea row must have one complete creator-manager descriptor");
            auto object = std::unique_ptr<NameObj>(descriptor->object_creator(object_name));
            auto *message_area = dynamic_cast<MessageArea *>(object.get());
            require(message_area != nullptr && message_area->mFormType == form_type,
                    "each MessageArea descriptor must construct the exact retail actor and form");
            object->init(JMapInfoIter(&placement->jmap_info, placement->jmap_entry_index));
            require(message_area->mZoneID == placement->zone_id && message_area->mObjArg0 == arg0 &&
                        message_area->mObjArg1 == -1 && message_area->mObjArg2 == -1 &&
                        message_area->mObjArg3 == -1 && message_area->mObjArg4 == -1 &&
                        message_area->mObjArg5 == -1 && message_area->mObjArg6 == -1 &&
                        message_area->mObjArg7 == -1 &&
                        !message_area->mSwitchCtrl->isValidSwitchAppear() &&
                        !message_area->isValidSwitchA() && !message_area->isValidSwitchB(),
                    "exact MessageArea init must retain placed-zone, argument, and absent-switch state");
            objects.push_back(std::move(object));
        }

        binding.init_after_placement();
        require(manager->mArray.size() == expected_rows.size(),
                "both real MessageArea rows must enter their one scene-owned manager");
        for (const auto &object : objects) {
            const auto *area = dynamic_cast<const MessageArea *>(object.get());
            require(std::ranges::find(manager->mArray, area) != manager->mArray.end(),
                    "the shared MessageArea manager must own each exact constructed placement");
        }
    }

    void test_rmgk01_zone_light_data_resolves_child_tables() {
        const auto disc_path = find_real_disc();
        if (!disc_path.has_value()) {
            std::cout << "[skip] RMGK01 zone-light data test (set SMGPC_REAL_DISC or place RMGK01.iso in a workspace ancestor)\n";
            return;
        }

        aurora_dvd_close();
        const auto disc_path_string = disc_path->string();
        require(aurora_dvd_open(disc_path_string.c_str()),
                "the RMGK01 zone-light fixture must be a readable SMG disc image");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        auto &light_data = smgpc::render::light::StageLightData::instance();
        light_data.reset();
        const auto light_tables = std::array{
            smgpc::scene::StagePlacementTable{
                .stage_name = "HeavensDoorGalaxy",
                .zone_name = "HeavensDoorMysteriousZone",
                .zone_id = 5,
            },
            smgpc::scene::StagePlacementTable{
                .stage_name = "HeavensDoorGalaxy",
                .zone_name = "HeavensDoorGalaxy",
                .zone_id = 0,
            },
            // Duplicate table metadata is intentionally accepted; resolved
            // scenario tables repeat zones across placement categories.
            smgpc::scene::StagePlacementTable{
                .stage_name = "HeavensDoorGalaxy",
                .zone_name = "HeavensDoorMysteriousZone",
                .zone_id = 5,
            },
        };
        auto light_binding = std::make_unique<smgpc::scene::StageLightSceneBinding>(
            dvd, "HeavensDoorGalaxy", light_tables);
        require(light_data.stage_zones().size() == 2U &&
                    light_data.stage_zones()[0].zone_id == 0 &&
                    light_data.stage_zones()[1].zone_id == 5,
                "scene-owned stage lighting must deduplicate and order authored placement-zone IDs");

        auto root_id = ZoneLightID{};
        auto *root = light_data.area_light_info(root_id);
        require(root != nullptr && root->mAreaLightName != nullptr &&
                    std::string_view(root->mAreaLightName) == "[共通]宇宙の星" &&
                    std::string_view(light_data.default_area_light_name()) == "[共通]宇宙の星",
                "the clear ZoneLightID must resolve the exact root-zone default row");

        auto child_id = ZoneLightID{};
        child_id._0 = 5;
        child_id.mLightID = 0;
        auto *child = light_data.area_light_info(child_id);
        require(child != nullptr && child != root && child->mAreaLightName != nullptr &&
                    std::string_view(child->mAreaLightName) == "ロゼッタ出会い" &&
                    child->mPlayerLight.mInfo0.mColor.r == 90U &&
                    child->mPlayerLight.mInfo0.mColor.g == 90U &&
                    child->mPlayerLight.mInfo0.mColor.b == 90U &&
                    !child->mPlayerLight.mInfo0.mIsFollowCamera &&
                    child->mPlayerLight.mInfo1.mIsFollowCamera &&
                    child->mPlayerLight.mColor.r == 90U &&
                    child->mPlayerLight.mColor.g == 90U &&
                    child->mPlayerLight.mColor.b == 115U &&
                    child->mPlayerLight.mColor.a == 60U,
                "zone 5/light 0 must resolve exact child-zone Rosetta player lighting and ambient data");

        child_id.mLightID = 1;
        auto *observatory = light_data.area_light_info(child_id);
        require(observatory != nullptr && observatory->mAreaLightName != nullptr &&
                    std::string_view(observatory->mAreaLightName) == "天文台（ロゼッタ）",
                "zone 5/light 1 must resolve the second exact child-zone light row");

        child_id.mLightID = 999;
        require(light_data.area_light_info(child_id) == root,
                "an absent child light ID must use the exact root-stage default rather than another child row");

        const auto conflicting = std::array{
            smgpc::render::light::StageLightZone{.zone_id = 5, .zone_name = "HeavensDoorMysteriousZone"},
            smgpc::render::light::StageLightZone{.zone_id = 5, .zone_name = "HeavensDoorGalaxy"},
        };
        require_throws<std::invalid_argument>(
            [&] { light_data.configure_stage_zones(conflicting); },
            "cannot name multiple authored zones");

        light_binding.reset();
        require(light_data.stage_zones().empty() &&
                    light_data.area_light_info(ZoneLightID{}) == nullptr,
                "stage-light scene teardown must release every cached zone and AreaLight row");

        const auto next_scene_tables = std::array{
            smgpc::scene::StagePlacementTable{
                .stage_name = "HeavensDoorGalaxy",
                .zone_name = "HeavensDoorGalaxy",
                .zone_id = 0,
            },
        };
        {
            const auto next_scene_binding = smgpc::scene::StageLightSceneBinding{
                dvd, "HeavensDoorGalaxy", next_scene_tables};
            auto stale_child = ZoneLightID{};
            stale_child._0 = 5;
            stale_child.mLightID = 0;
            const auto* next_scene_light = light_data.area_light_info(stale_child);
            require(light_data.stage_zones().size() == 1U &&
                        next_scene_light != nullptr &&
                        std::string_view(next_scene_light->mAreaLightName) ==
                            "[共通]宇宙の星",
                    "a recreated scene without child metadata must not inherit the previous scene's child light table");
        }
        require(light_data.stage_zones().empty(),
                "recreated stage-light ownership must also reset on teardown");
    }

    class RecordingDivideInfo final : public DivideMercatorRailPosInfo {
    public:
        void setPosition(s32, const TVec3f &) override {
            ++writes;
        }

        int writes = 0;
    };

    void test_water_and_mercator_do_not_fabricate_results() {
        constexpr auto position = TVec3f{};
        require_throws<std::logic_error>([&] { (void)MR::isInWater(position); }, "WaterArea");

        require_throws<std::logic_error>(
            [] { MR::getDivideMercatorRailPosition(nullptr, nullptr, 0, 10.0F, 10); },
            "retail transformation routine");

        auto result = RecordingDivideInfo{};
        require_throws<std::logic_error>(
            [&] { MR::getDivideMercatorRailPosition(&result, nullptr, 8, 10.0F, 10); },
            "retail transformation routine");
        require(result.writes == 0, "unavailable Mercator placement must not emit invented rail positions");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"AreaObj source boundaries are exact", test_area_obj_source_boundaries_are_exact},
        TestCase{"area queries reject missing scene owner", test_area_queries_reject_missing_scene_owner},
        TestCase{"scene holder owns real container and managers", test_scene_holder_owns_real_container_and_managers},
        TestCase{"retail prefix collision uses first manager", test_retail_prefix_collision_uses_first_manager},
        TestCase{"manager lifecycle owned ordered and once", test_manager_lifecycle_is_owned_ordered_and_once},
        TestCase{"CubeCamera manager finalizes priority and reverse query", test_cube_camera_manager_finalizes_priority_and_reverse_query},
        TestCase{"LightArea priority and stable zone identity", test_light_area_priority_and_stable_zone_identity},
        TestCase{"descriptor registry and strict area preflight", test_descriptor_registry_and_strict_area_preflight},
        TestCase{"RMGK01 CubeCamera rows construct exactly", test_rmgk01_cube_camera_rows_construct_exactly},
        TestCase{"RMGK01 MessageArea rows construct exactly", test_rmgk01_message_area_rows_construct_exactly},
        TestCase{"RMGK01 zone-light data resolves child tables", test_rmgk01_zone_light_data_resolves_child_tables},
        TestCase{"water and Mercator do not fabricate results", test_water_and_mercator_do_not_fabricate_results},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " AreaObj runtime test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " AreaObj runtime test(s) passed\n";
    return 0;
}
