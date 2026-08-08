#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/Util/MtxUtil.hpp"

#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    [[nodiscard]] std::string read_file(std::string_view path) {
        auto stream = std::ifstream(std::string(path), std::ios::binary);
        require(stream.is_open(), std::string("could not open AreaObj source-boundary file: ") + std::string(path));
        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }

    void test_game_source_boundary_is_exact() {
        struct SourcePair {
            std::string_view decomp;
            std::string_view port;
        };

        constexpr auto source_pairs = std::array{
            SourcePair{"../include/Game/AreaObj/AreaForm.hpp", "src/Game/AreaObj/AreaForm.hpp"},
            SourcePair{"../src/Game/AreaObj/AreaForm.cpp", "src/Game/AreaObj/AreaForm.cpp"},
            SourcePair{"../include/Game/AreaObj/AreaObj.hpp", "src/Game/AreaObj/AreaObj.hpp"},
            SourcePair{"../src/Game/AreaObj/AreaObj.cpp", "src/Game/AreaObj/AreaObj.cpp"},
            SourcePair{"../include/Game/AreaObj/AreaObjFollower.hpp", "src/Game/AreaObj/AreaObjFollower.hpp"},
            SourcePair{"../src/Game/AreaObj/AreaObjFollower.cpp", "src/Game/AreaObj/AreaObjFollower.cpp"},
            SourcePair{"../include/Game/Util/AreaObjUtil.hpp", "src/Game/Util/AreaObjUtil.hpp"},
            SourcePair{"../src/Game/Util/AreaObjUtil.cpp", "src/Game/Util/AreaObjUtil.cpp"},
            SourcePair{"../include/Game/Map/SleepControllerHolder.hpp", "src/Game/Map/SleepControllerHolder.hpp"},
            SourcePair{"../src/Game/Map/SleepControllerHolder.cpp", "src/Game/Map/SleepControllerHolder.cpp"},
        };

        for (const auto &pair : source_pairs) {
            require(read_file(pair.decomp) == read_file(pair.port),
                    std::string("pc-port Game file must remain byte-identical to the decomp: ") +
                        std::string(pair.port));
        }
    }

    void configure_cube(AreaFormCube &cube, const TVec3f &translation, const TVec3f &rotation,
                        const TVec3f &scale) {
        cube.mTranslation = translation;
        cube.mRotation = rotation;
        cube.mScale = scale;
        cube.updateBoxParam();
    }

    TVec3f transform_position(const TPos3f &matrix, const TVec3f &position) {
        auto result = TVec3f{};
        matrix.mult(position, result);
        return result;
    }

    void test_rotated_scaled_cube_volume() {
        auto cube = AreaFormCube{0};
        configure_cube(cube, TVec3f{30.0F, -70.0F, 110.0F}, TVec3f{15.0F, 65.0F, -20.0F},
                       TVec3f{2.0F, 1.0F, 0.5F});

        auto follow = TPos3f{};
        MR::makeMtxRotate(follow, TVec3f{-25.0F, 10.0F, 35.0F});
        follow.setTrans(TVec3f{400.0F, 250.0F, -800.0F});
        cube._4 = &follow;

        auto world = TPos3f{};
        cube.calcWorldMtx(&world);

        const auto inside = transform_position(world, TVec3f{999.0F, 499.0F, 249.0F});
        const auto outside_x = transform_position(world, TVec3f{1001.0F, 0.0F, 0.0F});
        const auto outside_z = transform_position(world, TVec3f{0.0F, 0.0F, -251.0F});
        require(cube.isInVolume(inside), "rotated and followed cube must accept a point within its scaled local bounds");
        require(!cube.isInVolume(outside_x), "rotated cube must reject points beyond its wide scaled axis");
        require(!cube.isInVolume(outside_z), "rotated cube must reject points outside its narrow scaled axis");

        auto recovered = TVec3f{};
        cube.calcLocalPos(&recovered, inside);
        require(recovered.epsilonEquals(TVec3f{999.0F, 499.0F, 249.0F}, 0.001F),
                "cube local-position recovery must invert the combined placement and follow rotation");
    }

    void test_cube2_base_origin() {
        auto cube = AreaFormCube{1};
        configure_cube(cube, TVec3f{}, TVec3f{}, TVec3f{1.0F, 2.0F, 1.0F});

        require(cube.isInVolume(TVec3f{0.0F, 0.0F, 0.0F}), "Cube2 must place its lower face at the placement origin");
        require(cube.isInVolume(TVec3f{0.0F, 1999.0F, 0.0F}), "Cube2 must scale its origin-based vertical extent");
        require(!cube.isInVolume(TVec3f{0.0F, -0.01F, 0.0F}), "Cube2 must reject positions below its placement origin");
        require(!cube.isInVolume(TVec3f{0.0F, 2000.0F, 0.0F}), "Cube2 upper face must remain exclusive");
    }

    void test_followed_scaled_cylinder_volume() {
        auto cylinder = AreaFormCylinder{};
        cylinder.mTranslation.set(20.0F, -40.0F, 10.0F);
        cylinder.mRotation.set(0.0F, 1.0F, 0.0F);
        cylinder._20 = 250.0F;
        cylinder._24 = 800.0F;

        auto follow = TPos3f{};
        MR::makeMtxRotate(follow, TVec3f{0.0F, 0.0F, 90.0F});
        follow.setTrans(TVec3f{-300.0F, 500.0F, 70.0F});
        cylinder._4 = &follow;

        auto base = TVec3f{};
        auto axis = TVec3f{};
        cylinder.calcPos(&base);
        cylinder.calcUpVec(&axis);

        const auto inside = base + (axis * 400.0F) + TVec3f{0.0F, 0.0F, 249.0F};
        const auto outside_radius = base + (axis * 400.0F) + TVec3f{0.0F, 0.0F, 250.0F};
        const auto below_base = base - axis;
        require(cylinder.isInVolume(inside), "follow rotation must transform a cylinder's scaled height axis");
        require(!cylinder.isInVolume(outside_radius), "cylinder radius must retain the retail exclusive edge");
        require(!cylinder.isInVolume(below_base), "cylinder must reject points below its transformed base");
    }

    void test_retail_area_math_helpers() {
        auto cylinder = AreaFormCylinder{};
        cylinder.calcDir(TVec3f{90.0F, 0.0F, 0.0F});
        require(cylinder.mRotation.epsilonEquals(TVec3f{0.0F, 0.0F, -1.0F}, 0.0001F),
                "retail temporary X-degree matrix must rotate the area axis with the recovered sign convention");
        cylinder.calcDir(TVec3f{0.0F, 0.0F, 90.0F});
        require(cylinder.mRotation.epsilonEquals(TVec3f{1.0F, 0.0F, 0.0F}, 0.0001F),
                "retail temporary Z-degree matrix must rotate the area axis with the recovered sign convention");
    }

    void test_manager_reverse_priority() {
        auto first = AreaObj{AreaForm::Type_Cube2, "PriorityArea"};
        auto second = AreaObj{AreaForm::Type_Cube2, "PriorityArea"};
        configure_cube(*static_cast<AreaFormCube *>(first.mForm), TVec3f{}, TVec3f{}, TVec3f{1.0F, 1.0F, 1.0F});
        configure_cube(*static_cast<AreaFormCube *>(second.mForm), TVec3f{}, TVec3f{}, TVec3f{1.0F, 1.0F, 1.0F});

        auto manager = AreaObjMgr{2, "PriorityArea"};
        manager.entry(&first);
        manager.entry(&second);

        constexpr auto overlap = TVec3f{0.0F, 100.0F, 0.0F};
        require(manager.find_in(overlap) == &second, "the most recently entered overlapping area must have priority");

        second.invalidate();
        require(manager.find_in(overlap) == &first, "reverse lookup must continue to an earlier valid overlapping area");

        first.sleep();
        require(manager.find_in(overlap) == nullptr, "manager lookup must return no area when every overlap is inactive");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"Game source boundary is exact", test_game_source_boundary_is_exact},
        TestCase{"rotated scaled cube volume", test_rotated_scaled_cube_volume},
        TestCase{"Cube2 base origin", test_cube2_base_origin},
        TestCase{"followed scaled cylinder volume", test_followed_scaled_cylinder_volume},
        TestCase{"retail area math helpers", test_retail_area_math_helpers},
        TestCase{"manager reverse priority", test_manager_reverse_priority},
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
        std::cerr << failures << " AreaObj core test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " AreaObj core test(s) passed\n";
    return 0;
}
