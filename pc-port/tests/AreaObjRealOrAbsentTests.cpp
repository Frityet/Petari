#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/MercatorTransformCube.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <iostream>
#include <memory>
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

    private:
        int _id;
        std::vector<int> &_events;
        int &_destroyed;
    };

    void test_manager_lifecycle_is_owned_ordered_and_once() {
        auto events = std::vector<int>{};
        auto destroyed = 0;
        {
            auto runtime = smgpc::scene::AreaObjRuntime{};
            (void)runtime.adopt_manager(std::make_unique<LifecycleManager>(1, events, destroyed));
            (void)runtime.adopt_manager(std::make_unique<LifecycleManager>(2, events, destroyed));
            runtime.init_after_placement();
            runtime.init_after_placement();
            require(events == std::vector<int>({1, 2}),
                    "scene-owned AreaObj managers must receive post-placement in creation order exactly once");
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

        auto second_holder = SceneObjHolder{};
        const auto second_binding = smgpc::scene::SceneObjHolderBinding(second_holder);
        require(second_holder.create(SceneObj_AreaObjContainer) != nullptr,
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
            std::tuple{"PullBackCylinder", "PullBackCylinder", 17, 0x40, AreaForm::Type_Cylinder},
            std::tuple{"ViewGroupCtrlCube", "ViewGroupCtrlCube", 32, 0x40, AreaForm::Type_Cube2},
            std::tuple{"LensFlareArea", "LensFlareArea", 33, 0x40, AreaForm::Type_Cube2},
            std::tuple{"BlueStarGuidanceCube", "BlueStarGuidanceCube", 40, 0x10, AreaForm::Type_Cube2},
        };
        require(descriptors.size() == expected_descriptors.size(),
                "only the four completed passive Gateway AreaObj closures should be registered");
        for (auto index = std::size_t{}; index < expected_descriptors.size(); ++index) {
            const auto &[object_name, manager_name, retail_order, capacity, form_type] =
                expected_descriptors[index];
            const auto &descriptor = descriptors[index];
            require(descriptor.object_name == object_name && descriptor.manager_name == manager_name &&
                        descriptor.retail_manager_order == retail_order &&
                        descriptor.manager_capacity == capacity,
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
        TestCase{"area queries reject missing scene owner", test_area_queries_reject_missing_scene_owner},
        TestCase{"scene holder owns real container and managers", test_scene_holder_owns_real_container_and_managers},
        TestCase{"retail prefix collision uses first manager", test_retail_prefix_collision_uses_first_manager},
        TestCase{"manager lifecycle owned ordered and once", test_manager_lifecycle_is_owned_ordered_and_once},
        TestCase{"descriptor registry and strict area preflight", test_descriptor_registry_and_strict_area_preflight},
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
