#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/MercatorTransformCube.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template <typename Function>
    void require_unavailable(Function&& function, std::string_view message_fragment) {
        try {
            function();
        } catch (const std::logic_error& error) {
            require(std::string_view(error.what()).find(message_fragment) != std::string_view::npos,
                    "the unavailable operation must explain its missing real dependency");
            return;
        }
        throw std::runtime_error("the unavailable operation silently returned instead of rejecting the query");
    }

    void test_area_queries_reject_missing_scene_data() {
        constexpr auto position = TVec3f{10.0F, 20.0F, 30.0F};

        require_unavailable([] { (void)MR::getAreaObjContainer(); }, "stage placement data");
        require_unavailable([] { (void)MR::getAreaObjManager("DeathArea"); }, "stage placement data");
        require_unavailable([&] { (void)MR::getAreaObj("DeathArea", position); }, "stage placement data");
        require_unavailable([&] { (void)MR::isInAreaObj("DeathArea", position); }, "stage placement data");
        require_unavailable([&] { (void)MR::isInDeath(position); }, "stage placement data");
        require_unavailable([&] { (void)MR::isInDarkMatter(position); }, "stage placement data");
    }

    void test_scene_holder_does_not_construct_a_partial_container() {
        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);

        require_unavailable([&] { (void)holder.create(SceneObj_AreaObjContainer); }, "parsed stage placement");
        require(!holder.isExist(SceneObj_AreaObjContainer),
                "rejected AreaObj construction must not install a partial scene object");

        constexpr auto position = TVec3f{};
        require_unavailable([&] { (void)MR::isInAreaObj("SwitchArea", position); }, "stage placement data");
    }

    class RecordingDivideInfo final : public DivideMercatorRailPosInfo {
    public:
        void setPosition(s32, const TVec3f&) override {
            ++writes;
        }

        int writes = 0;
    };

    void test_water_and_mercator_do_not_fabricate_results() {
        constexpr auto position = TVec3f{};
        require_unavailable([&] { (void)MR::isInWater(position); }, "WaterArea");

        require_unavailable(
            [] { MR::getDivideMercatorRailPosition(nullptr, nullptr, 0, 10.0F, 10); }, "retail transformation routine");

        auto result = RecordingDivideInfo{};
        require_unavailable(
            [&] { MR::getDivideMercatorRailPosition(&result, nullptr, 8, 10.0F, 10); }, "retail transformation routine");
        require(result.writes == 0, "unavailable Mercator placement must not emit invented rail positions");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"area queries reject missing scene data", test_area_queries_reject_missing_scene_data},
        TestCase{"scene holder rejects partial container", test_scene_holder_does_not_construct_a_partial_container},
        TestCase{"water and Mercator do not fabricate results", test_water_and_mercator_do_not_fabricate_results},
    };

    auto failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " AreaObj real-or-absent test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " AreaObj real-or-absent test(s) passed\n";
    return 0;
}
