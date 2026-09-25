#include "NativeHeapFixture.hpp"
#include "OriginalLightFixture.hpp"
#include "SourceMirrorEncoding.hpp"
#include "resource/TextEncoding.hpp"
#include "SceneExecutionFixture.hpp"
#include "OriginalSceneControllerFixture.hpp"
#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/CubeCamera.hpp"
#include "Game/AreaObj/GlaringLightArea.hpp"
#include "Game/AreaObj/LightArea.hpp"
#include "Game/AreaObj/LightAreaHolder.hpp"
#include "Game/AreaObj/MessageArea.hpp"
#include "Game/AreaObj/SwitchArea.hpp"
#include "Game/AreaObj/WarpCube.hpp"
#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/AreaObj/MercatorTransformCube.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/LightZoneDataHolder.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "resource/BcsvTable.hpp"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <bit>
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

namespace aurora { extern AuroraConfig g_config; }

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

    struct AreaContainerFixture {
        JKRHeap::Handle heaps = smgpc::test::create_native_root_heap(16U << 20);
        JKRHeap::Handle domain = smgpc::test::create_native_solid_heap(heaps, 8U << 20);
        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding scheduler_binding{scheduler};
        smgpc::test::OriginalSceneControllerFixture original{heaps};
        smgpc::test::SceneExecutionFixture execution{scheduler, domain,
                                                   &original.scene};
    };

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
            const auto retail_source = directory / "decomp/src/Game/AreaObj/CubeCamera.cpp";
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

    void test_area_obj_source_boundaries_are_exact() {
        const auto pc_port_root = find_pc_port_root();
        require(pc_port_root.has_value(), "the source-boundary test must locate the pc-port source root");
        const auto decomp_root = *pc_port_root / "decomp";
        constexpr auto source_pairs = std::array{
            std::pair{"include/Game/AreaObj/SwitchArea.hpp", "src/Game/AreaObj/SwitchArea.hpp"},
            std::pair{"src/Game/AreaObj/SwitchArea.cpp", "src/Game/AreaObj/SwitchArea.cpp"},
            std::pair{"include/Game/AreaObj/CubeCamera.hpp", "src/Game/AreaObj/CubeCamera.hpp"},
            std::pair{"src/Game/AreaObj/CubeCamera.cpp", "src/Game/AreaObj/CubeCamera.cpp"},
            std::pair{"include/Game/AreaObj/MessageArea.hpp", "src/Game/AreaObj/MessageArea.hpp"},
            std::pair{"src/Game/AreaObj/MessageArea.cpp", "src/Game/AreaObj/MessageArea.cpp"},
            std::pair{"include/Game/AreaObj/LightArea.hpp", "src/Game/AreaObj/LightArea.hpp"},
            std::pair{"src/Game/AreaObj/LightArea.cpp", "src/Game/AreaObj/LightArea.cpp"},
        };
        for (const auto &[retail_source, host_source] : source_pairs) {
            require(smgpc::test::source_matches_with_cp932(read_file(decomp_root / retail_source), read_file(*pc_port_root / host_source)),
                    std::string("completed PC AreaObj sources must remain identical to the decompiled source except explicit CP932 encoding: ") + host_source);
        }
    }

    void verify_installed_original_managers(AreaObjContainer &container) {
        auto managers = std::size_t{};
        auto base_managers = std::size_t{};
        for (auto *object : NameObj::snapshotNativeObjects()) {
            auto *manager = dynamic_cast<AreaObjMgr *>(object);
            if (manager == nullptr) continue;
            ++managers;
            if (typeid(*manager) == typeid(AreaObjMgr)) ++base_managers;
            require(MR::getSceneObjHolder()->ownsNativeObject(manager),
                    "the scene transaction must own every manager created by the original container");
            require(container.getManager(manager->mName) == manager,
                    "every manager must be reachable through the original container");
            const auto variant = std::string(manager->mName) + "PlacementVariant";
            require(container.getManager(variant.c_str()) == manager,
                    "original manager lookup must accept a matching name prefix");
        }
        require(managers == 67U && base_managers == 61U,
                "the complete original table must construct 61 base and six specialized managers");
    }

    void test_manager_readiness_does_not_fabricate_placement_support() {
        auto fixture = AreaContainerFixture{};
        auto& holder = fixture.execution.holder();
        auto& binding = fixture.execution;
        auto *container = static_cast<AreaObjContainer *>(smgpc::test::create_area_container(holder));
        auto *manager = container->getManager("ChangeBgmCube");
        require(typeid(*manager) == typeid(AreaObjMgr) && manager->_18 == 0x20,
                "ChangeBgmCube's original base manager exists independently of its specialized actor");
        require(MR::getAreaObj("ChangeBgmCube", TVec3f{}) == nullptr,
                "a real manager with no placed actor returns a real empty-volume miss");
        auto *warp = dynamic_cast<WarpCubeMgr *>(container->getManager("WarpCube"));
        require(warp != nullptr && warp->_18 == 0x40 && warp->mWarpCube == nullptr &&
                    warp->find_in(TVec3f{}) == nullptr,
                "the original empty WarpCubeMgr must retain its null active cube and real query behavior");
        auto *glaring = dynamic_cast<GlaringLightAreaMgr *>(container->getManager("GlaringLightArea"));
        require(glaring != nullptr && glaring->_18 == 0x40 && glaring->find_in(TVec3f{}) == nullptr,
                "the original GlaringLightArea manager must exist even before actor placement support");
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
        auto fixture = AreaContainerFixture{};
        auto& holder = fixture.execution.holder();
        auto& binding = fixture.execution;
        auto *container = dynamic_cast<AreaObjContainer *>(smgpc::test::create_area_container(holder));
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
        require(manager->getAreaObj(0) == &high && manager->getAreaObj(1) == &low,
                "the general NameObj postpass must not substitute for the camera creator's load phase");
        // GameCameraCreator invokes this original phase after camera creation.
        manager->initAfterLoad();
        require(manager->getAreaObj(0) == &low && manager->getAreaObj(1) == &middle &&
                    manager->getAreaObj(2) == &high,
                "the original CubeCameraMgr::initAfterLoad must sort camera priority");
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
        auto fixture = AreaContainerFixture{};
        auto& holder = fixture.execution.holder();
        auto& binding = fixture.execution;
        auto *container = dynamic_cast<AreaObjContainer *>(smgpc::test::create_area_container(holder));
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


    }

    void test_scene_holder_owns_real_container_and_managers() {
        const auto registry_baseline =
            NameObj::snapshotNativeObjects().size();
        {
            auto fixture = AreaContainerFixture{};
            auto& holder = fixture.execution.holder();
            auto& binding = fixture.execution;
            auto *object = smgpc::test::create_area_container(holder);
            auto *container = dynamic_cast<AreaObjContainer *>(object);

            require(container != nullptr && holder.isExist(SceneObj_AreaObjContainer) &&
                        MR::getAreaObjContainer() == container,
                    "a bound scene must install the real AreaObjContainer SceneObj");
            require(smgpc::test::create_area_container(holder) == container,
                    "SceneObj creation must retain one container per scene");
            verify_installed_original_managers(*container);
            require(MR::getSceneObjHolder()->ownsNativeObject(container->getManager("SwitchArea")),
                    "AreaObj managers must use the general scene transaction owner");
            require_throws<std::logic_error>(
                [&] { (void)container->getManager("__SMGPC_missing_area_manager__"); },
                "No complete retail AreaObj manager");
            require_throws<std::invalid_argument>([&] { (void)container->getManager(nullptr); },
                                                  "non-null retail name");

            binding.init_after_placement();
            binding.init_after_placement();
        }


        {
            auto fixture = AreaContainerFixture{};
            auto& second_holder = fixture.execution.holder();
            auto *second_container = dynamic_cast<AreaObjContainer *>(
                smgpc::test::create_area_container(second_holder));
            require(second_container != nullptr &&
                        dynamic_cast<LightAreaHolder *>(
                            second_container->getManager("LightArea")) !=
                            nullptr &&
                        MR::getSceneObjHolder()->ownsNativeObject(second_container->getManager("LightArea")),
                    "destroying a scene binding must release container and manager ownership for the next scene");
        }
        require(NameObj::snapshotNativeObjects().size() ==
                    registry_baseline,
                "two AreaObj scene generations must restore the NameObj registry baseline");
    }

    JMapInfo make_generic_effect_area_row(const char *name, const std::array<s32, 8> &args) {
        constexpr auto fields = std::array{
            "name", "pos_x", "pos_y", "pos_z", "dir_x", "dir_y", "dir_z",
            "scale_x", "scale_y", "scale_z", "Obj_arg0", "Obj_arg1", "Obj_arg2",
            "Obj_arg3", "Obj_arg4", "Obj_arg5", "Obj_arg6", "Obj_arg7",
        };
        constexpr auto data_offset = 16U + fields.size() * 12U;
        constexpr auto row_size = fields.size() * 4U;
        auto bytes = std::vector<u8>(data_offset + row_size);
        const auto put32 = [&](std::size_t offset, u32 value) {
            for (std::size_t byte = 0; byte < 4; ++byte)
                bytes.at(offset + byte) = static_cast<u8>(value >> (24U - byte * 8U));
        };
        put32(0, 1);
        put32(4, fields.size());
        put32(8, data_offset);
        put32(12, row_size);
        constexpr auto placement = std::array{25.0F, -50.0F, 75.0F, 0.0F, 0.0F, 0.0F, 0.5F, 2.0F, 0.25F};
        for (std::size_t field = 0; field < fields.size(); ++field) {
            const auto header = 16U + field * 12U;
            put32(header, smgpc::resource::jmap_hash(fields[field]));
            put32(header + 4U, 0xffffffffU);
            bytes[header + 8U] = static_cast<u8>((field * 4U) >> 8U);
            bytes[header + 9U] = static_cast<u8>(field * 4U);
            const auto type = field == 0U ? smgpc::resource::BcsvFieldType::StringOffset :
                              field < 10U ? smgpc::resource::BcsvFieldType::Float :
                                            smgpc::resource::BcsvFieldType::Int32;
            bytes[header + 11U] = static_cast<u8>(type);
            if (field > 0U && field < 10U)
                put32(data_offset + field * 4U, std::bit_cast<u32>(placement[field - 1U]));
            else if (field >= 10U)
                put32(data_offset + field * 4U, static_cast<u32>(args[field - 10U]));
        }
        const auto text = std::string_view(name);
        bytes.insert(bytes.end(), text.begin(), text.end());
        bytes.push_back(0);
        auto info = JMapInfo::from_bcsv(bytes);
        info.setPlacedZoneId(0);
        return info;
    }

    void test_generic_effect_areas_use_original_init_and_queries() {
        const auto baseline = NameObj::snapshotNativeObjects().size();
        {
            auto heaps = smgpc::test::create_native_root_heap(16U << 20);
            auto domain = smgpc::test::create_native_solid_heap(heaps, 8U << 20);
            auto scheduler = smgpc::runtime::SceneScheduler{};
            auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding(scheduler);
            auto original = smgpc::test::OriginalSceneControllerFixture(heaps);
        auto execution = smgpc::test::SceneExecutionFixture(scheduler, domain,
                                                          &original.scene);
            auto &holder = execution.holder();
            auto &binding = execution;
            {
                const const JKRHeap::CurrentHeapScope game(*(domain));
                const aurora::allocation::ClientAllocationScope gameRouting({true, true});
                for (const auto id : {SceneObj_StageSwitchContainer, SceneObj_SwitchWatcherHolder,
                                      SceneObj_SleepControllerHolder, SceneObj_AreaObjContainer})
                    require((id == SceneObj_AreaObjContainer ? smgpc::test::create_area_container(holder) : holder.create(id)) != nullptr, "generic area init requires original scene services");
            }

            constexpr auto cases = std::array{
                std::tuple{"EffectCylinder", "EffectCylinder", 6, 0x40, AreaForm::Type_Cylinder,
                           TVec3f{274.0F, 949.0F, 75.0F}, TVec3f{275.0F, 50.0F, 75.0F},
                           TVec3f{25.0F, 950.01F, 75.0F}},
                std::tuple{"SmokeEffectColorAreaCube", "SmokeEffectColorArea", 43, 0x10, AreaForm::Type_Cube2,
                           TVec3f{274.0F, 1949.0F, 199.0F}, TVec3f{25.0F, 50.0F, 200.0F},
                           TVec3f{25.0F, 1950.0F, 75.0F}},
            };
            constexpr auto args = std::array<s32, 8>{17, 31, 63, 127, 191, 223, -1, -7};
            auto rows = std::vector<JMapInfo>{};
            rows.reserve(cases.size());
            auto objects = std::vector<std::unique_ptr<NameObj>>{};
            for (const auto &[name, manager_name, order, capacity, form, inside, outside_side, outside_top] : cases) {
                auto *manager = MR::getAreaObjContainer()->getManager(manager_name);
                require(typeid(*manager) == typeid(AreaObjMgr) && manager->_18 == capacity,
                        "generic effects must use the original base AreaObjMgr");
                require(MR::getAreaObj(manager_name, inside) == nullptr,
                        "an installed empty effect manager must return a real miss");
                rows.push_back(make_generic_effect_area_row(name, args));
                auto object = std::unique_ptr<NameObj>{};
                {
                    const const JKRHeap::CurrentHeapScope game(*(domain));
                const aurora::allocation::ClientAllocationScope gameRouting({true, true});
                    object = std::make_unique<AreaObj>(form, name);
                }
                auto *area = dynamic_cast<AreaObj *>(object.get());
                require(area != nullptr && typeid(*area) == typeid(AreaObj) && area->mFormType == form,
                        "effect areas use the original generic class and authored form");
                {
                    const const JKRHeap::CurrentHeapScope game(*(domain));
                const aurora::allocation::ClientAllocationScope gameRouting({true, true});
                    object->init(JMapInfoIter(&rows.back(), 0));
                }
                require(MR::getAreaObj(manager_name, inside) == area && manager->mArray.size() == 1U,
                        "original init must register the real parsed volume with its manager");
                require(MR::getAreaObj(manager_name, outside_side) == nullptr &&
                            MR::getAreaObj(manager_name, outside_top) == nullptr &&
                            MR::getAreaObj(manager_name, TVec3f{25.0F, -50.01F, 75.0F}) == nullptr,
                        "effect volume queries must preserve scaled side and height bounds");
                require(MR::getAreaObj(manager_name, TVec3f{25.0F, -50.0F, 75.0F}) == area,
                        "both original base-origin forms include their lower face");
                for (s32 arg = 0; arg < static_cast<s32>(args.size()); ++arg)
                    require(MR::getAreaObjArg(area, arg) == args[arg],
                            "Mario effect selectors and all color channels must retain authored arguments");
                area->invalidate();
                require(MR::getAreaObj(manager_name, inside) == nullptr,
                        "disabled effect volumes must cease matching");
                area->validate();
                require(MR::getAreaObj(manager_name, inside) == area,
                        "revalidated effect volumes must restore the same original area identity");
                objects.push_back(std::move(object));
            }
            binding.init_after_placement();
        }
        require(NameObj::snapshotNativeObjects().size() == baseline,
                "generic effect areas and their scene managers must fully retire");
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
        require_throws<std::logic_error>([&] { (void)MR::isInWater(position); }, "active scene-owned");

        require_throws<std::logic_error>(
            [] { MR::getDivideMercatorRailPosition(nullptr, nullptr, 0, 10.0F, 10); },
            "retail transformation routine");

        auto result = RecordingDivideInfo{};
        require_throws<std::logic_error>(
            [&] { MR::getDivideMercatorRailPosition(&result, nullptr, 8, 10.0F, 10); },
            "retail transformation routine");
        require(result.writes == 0, "unavailable Mercator placement must not emit invented rail positions");
    }

    void test_area_movement_uses_actual_sphere_and_arguments() {
        TVec3f velocity(1, 2, 3);
        require_throws<std::logic_error>([&] { MR::calcAreaMoveVelocity(&velocity, TVec3f(0,0,0)); }, "active scene-owned");
        auto fixture = AreaContainerFixture{};
        auto& holder = fixture.execution.holder();
        auto& binding = fixture.execution;
        auto* container = static_cast<AreaObjContainer*>(smgpc::test::create_area_container(holder));
        auto* manager = container->getManager("AreaMoveSphere");
        require(manager != nullptr && manager->_18 == 0x10, "Area movement requires the actual retail manager");
        require(!MR::calcAreaMoveVelocity(&velocity, TVec3f(0,0,0)) && velocity.squared() == 0,
                "An empty real manager returns the original zero/false result");
        auto area = AreaObj(AreaForm::Type_Sphere, "AreaMoveSphere");
        auto* sphere = static_cast<AreaFormSphere*>(area.mForm);
        sphere->mTranslation.set(0,0,0);
        sphere->mRadius = 100;
        sphere->mUp.set(0,1,0);
        manager->entry(&area);
        require(MR::calcAreaMoveVelocity(&velocity, TVec3f(10,0,0)) && velocity.epsilonEquals(TVec3f(0,10,0), 1e-5F),
                "Missing authored speed defaults to ten along the sphere's tangent up vector");
        area.mObjArg0 = -4;
        require(MR::calcAreaMoveVelocity(&velocity, TVec3f(10,0,0)) && velocity.epsilonEquals(TVec3f(0,-4,0), 1e-5F),
                "Explicit negative authored speed must be preserved");
        require(MR::calcAreaMoveVelocity(&velocity, TVec3f(0,10,0)) && velocity.squared() == 0,
                "An axial point has no tangent up component");
        require(MR::calcAreaMoveVelocity(&velocity, TVec3f(0,0,0)) && velocity.epsilonEquals(TVec3f(0,-4,0), 1e-5F),
                "Original normalize-or-zero keeps the up vector at the exact center");
        require(!MR::calcAreaMoveVelocity(&velocity, TVec3f(100,0,0)) && velocity.squared() == 0,
                "Original sphere membership excludes its exact radius");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main(int argc, char **argv) {
    constexpr auto tests = std::array{
        TestCase{"AreaObj source boundaries are exact", test_area_obj_source_boundaries_are_exact},
        TestCase{"area queries reject missing scene owner", test_area_queries_reject_missing_scene_owner},
        TestCase{"scene holder owns real container and managers", test_scene_holder_owns_real_container_and_managers},
        TestCase{"manager readiness does not fabricate placement support", test_manager_readiness_does_not_fabricate_placement_support},
        TestCase{"CubeCamera manager finalizes priority and reverse query", test_cube_camera_manager_finalizes_priority_and_reverse_query},
        TestCase{"LightArea priority and stable zone identity", test_light_area_priority_and_stable_zone_identity},
        TestCase{"generic effect areas use original init and queries", test_generic_effect_areas_use_original_init_and_queries},
        TestCase{"area movement uses actual sphere and arguments", test_area_movement_uses_actual_sphere_and_arguments},
        TestCase{"water and Mercator do not fabricate results", test_water_and_mercator_do_not_fabricate_results},
    };

    auto failures = 0;
    auto executed = std::size_t{};
    for (const auto &test : tests) {
        if (argc > 1 && std::string_view(test.name).find(argv[1]) == std::string_view::npos) continue;
        ++executed;
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (executed == 0U) {
        std::cerr << "No AreaObj runtime tests matched the supplied name filter\n";
        return 1;
    }
    if (failures != 0) {
        std::cerr << failures << " AreaObj runtime test(s) failed\n";
        return 1;
    }

    std::cout << executed << " AreaObj runtime test(s) passed\n";
    return 0;
}
