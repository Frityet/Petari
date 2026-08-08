#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/GravityCreator.hpp"
#include "Game/Gravity/GlobalGravityObj.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "compat/GameGravityCompat.hpp"
#include "resource/BcsvTable.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageHostScene.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template <typename Exception, typename Function>
    void require_throws(Function&& function, std::string_view message) {
        auto rejected = false;
        try {
            function();
        } catch (const Exception&) {
            rejected = true;
        }
        require(rejected, message);
    }

    void write_be32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be_float(std::vector<std::uint8_t>& bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    void write_field(std::vector<std::uint8_t>& bytes, std::size_t index, std::string_view name,
                     std::uint16_t offset, smgpc::resource::BcsvFieldType type) {
        const auto field_offset = 0x10U + index * 0x0cU;
        write_be32(bytes, field_offset, smgpc::resource::jmap_hash(name));
        write_be32(bytes, field_offset + 4U, 0xffffffffU);
        write_be16(bytes, field_offset + 8U, offset);
        bytes[field_offset + 10U] = 0U;
        bytes[field_offset + 11U] = static_cast<std::uint8_t>(type);
    }

    JMapInfo make_fieldless_jmap() {
        auto bytes = std::vector<std::uint8_t>(0x10U, 0U);
        write_be32(bytes, 0U, 1U);
        write_be32(bytes, 8U, 0x10U);
        return JMapInfo::from_bcsv(bytes);
    }

    JMapInfo make_linked_gravity_jmap() {
        constexpr auto field_count = 8U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        constexpr auto entry_size = 32U;
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size, 0U);
        write_be32(bytes, 0U, 1U);
        write_be32(bytes, 4U, field_count);
        write_be32(bytes, 8U, data_offset);
        write_be32(bytes, 12U, entry_size);
        write_field(bytes, 0U, "Obj_ID", 0U, smgpc::resource::BcsvFieldType::Int32);
        write_field(bytes, 1U, "l_id", 4U, smgpc::resource::BcsvFieldType::Int32);
        write_field(bytes, 2U, "pos_x", 8U, smgpc::resource::BcsvFieldType::Float);
        write_field(bytes, 3U, "pos_y", 12U, smgpc::resource::BcsvFieldType::Float);
        write_field(bytes, 4U, "pos_z", 16U, smgpc::resource::BcsvFieldType::Float);
        write_field(bytes, 5U, "dir_x", 20U, smgpc::resource::BcsvFieldType::Float);
        write_field(bytes, 6U, "dir_y", 24U, smgpc::resource::BcsvFieldType::Float);
        write_field(bytes, 7U, "dir_z", 28U, smgpc::resource::BcsvFieldType::Float);
        write_be32(bytes, data_offset, 42U);
        write_be32(bytes, data_offset + 4U, 42U);
        write_be_float(bytes, data_offset + 8U, 10.0F);
        write_be_float(bytes, data_offset + 12U, 20.0F);
        write_be_float(bytes, data_offset + 16U, 30.0F);

        auto info = JMapInfo::from_bcsv(bytes);
        info.setName("objinfo");
        info.setPlacedZoneId(3);
        return info;
    }

    JMapInfo make_complete_gravity_jmap() {
        constexpr auto field_count = 7U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        constexpr auto entry_size = 80U;
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size, 0U);
        write_be32(bytes, 0U, 1U);
        write_be32(bytes, 4U, field_count);
        write_be32(bytes, 8U, data_offset);
        write_be32(bytes, 12U, entry_size);
        write_field(bytes, 0U, "Range", 0U, smgpc::resource::BcsvFieldType::Float);
        write_field(bytes, 1U, "Distant", 4U, smgpc::resource::BcsvFieldType::Float);
        write_field(bytes, 2U, "Priority", 8U, smgpc::resource::BcsvFieldType::Int32);
        write_field(bytes, 3U, "Gravity_id", 12U, smgpc::resource::BcsvFieldType::Int32);
        write_field(bytes, 4U, "Gravity_type", 16U, smgpc::resource::BcsvFieldType::InlineString);
        write_field(bytes, 5U, "Power", 48U, smgpc::resource::BcsvFieldType::InlineString);
        write_field(bytes, 6U, "Inverse", 76U, smgpc::resource::BcsvFieldType::Int32);
        write_be_float(bytes, data_offset, 250.0F);
        write_be_float(bytes, data_offset + 4U, 10.0F);
        write_be32(bytes, data_offset + 8U, 7U);
        write_be32(bytes, data_offset + 12U, 42U);
        const auto gravity_type = std::string_view{"Shadow"};
        const auto power = std::string_view{"Light"};
        std::ranges::copy(gravity_type, bytes.begin() + data_offset + 16U);
        std::ranges::copy(power, bytes.begin() + data_offset + 48U);
        write_be32(bytes, data_offset + 76U, 1U);
        return JMapInfo::from_bcsv(bytes);
    }

    class ConstantGravity final : public PlanetGravity {
    public:
        ConstantGravity(TVec3f direction, float distance) : _direction(direction), _distance(distance) {
        }

        bool calcOwnGravityVector(TVec3f* destination, f32* scalar,
                                  const TVec3f&) const override {
            if (destination != nullptr) {
                destination->set(_direction);
            }
            if (scalar != nullptr) {
                *scalar = _distance;
            }
            return true;
        }

    private:
        TVec3f _direction;
        float _distance;
    };

    class FollowBindingProbe final : public BaseMatrixFollower {
    public:
        FollowBindingProbe(NameObj* owner, const JMapInfoIter& iter)
            : BaseMatrixFollower(owner, iter) {
        }

        void setGravityFollowHost(const NameObj* host) override {
            bound_host = host;
        }

        const NameObj* bound_host = nullptr;
    };

    class GravityScene final {
    public:
        GravityScene()
            : holder(), binding(holder),
              manager(static_cast<PlanetGravityManager*>(
                  MR::createSceneObj(SceneObj_PlanetGravityManager))) {
            require(manager != nullptr,
                    "a bound stage scene must create the exact PlanetGravityManager SceneObj");
        }

        SceneObjHolder holder;
        smgpc::scene::SceneObjHolderBinding binding;
        PlanetGravityManager* manager;
    };

    void test_absent_manager_is_explicit() {
        auto actor = LiveActor("gravity-absence-probe");
        auto destination = TVec3f{3.0F, 4.0F, 5.0F};
        require_throws<std::logic_error>(
            [&] { (void)MR::calcGravityVector(&actor, &destination, nullptr, 0U); },
            "gravity queries must reject a missing scene-owned manager");
        require(destination.epsilonEquals(TVec3f{3.0F, 4.0F, 5.0F}, 0.0F),
                "an unavailable gravity query must not fabricate a zero vector");

        auto gravity = ConstantGravity(TVec3f{0.0F, -1.0F, 0.0F}, 100.0F);
        require_throws<std::logic_error>([&] { MR::registerGravity(&gravity); },
                                         "registration must reject a missing scene owner");
        require_throws<std::invalid_argument>([&] { MR::registerGravity(nullptr); },
                                              "null registration must be rejected before scene lookup");
        require_throws<std::invalid_argument>(
            [&] { (void)MR::calcGravityVector(static_cast<const LiveActor*>(nullptr), &destination, nullptr, 0U); },
            "a null actor must not be treated as zero gravity");

        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        destination.set(3.0F, 4.0F, 5.0F);
        require_throws<std::logic_error>(
            [&] { (void)MR::calcGravityVector(&actor, &destination, nullptr, 0U); },
            "a bound holder must not lazily fabricate a missing gravity manager");
        require(destination.epsilonEquals(TVec3f{3.0F, 4.0F, 5.0F}, 0.0F),
                "a missing manager in an active scene must leave the query destination untouched");
    }

    void test_real_manager_rules_and_info() {
        auto scene = GravityScene{};

        auto caller = NameObj("gravity-caller");
        auto destination = TVec3f{9.0F, 9.0F, 9.0F};
        require(!MR::calcGravityVector(&caller, TVec3f{}, &destination, nullptr, 0U) &&
                    destination.epsilonEquals(TVec3f{}, 0.0F),
                "a real empty manager should retain retail false-and-zero behavior");

        auto low = ConstantGravity(TVec3f{1.0F, 0.0F, 0.0F}, 100.0F);
        low.mPriority = 1;
        auto high = ConstantGravity(TVec3f{0.0F, 1.0F, 0.0F}, 100.0F);
        high.mPriority = 5;
        high.mGravityPower = GRAVITY_POWER_HEAVY;
        auto strongest = ConstantGravity(TVec3f{0.0F, 0.0F, 1.0F}, 50.0F);
        strongest.mPriority = 5;
        strongest.mGravityPower = GRAVITY_POWER_LIGHT;
        MR::registerGravity(&low);
        MR::registerGravity(&high);
        MR::registerGravity(&strongest);

        auto info = GravityInfo{};
        require(MR::calcGravityVector(&caller, TVec3f{}, &destination, &info, 0U),
                "registered normal gravities should be queryable through MR");
        require(destination.epsilonEquals(TVec3f{0.0F, 0.24253562F, 0.97014248F}, 0.0001F),
                "only equal highest-priority vectors should combine before normalization");
        require(info.mGravityInstance == &strongest && info.mLargestPriority == 5 &&
                    MR::isLightGravity(info),
                "GravityInfo should identify the strongest winning field and expose its real power");

        strongest.mHost = &caller;
        info.init();
        require(MR::calcGravityVector(&caller, TVec3f{}, &destination, &info, 0U) &&
                    destination.epsilonEquals(TVec3f{0.0F, 1.0F, 0.0F}, 0.0001F) &&
                    info.mGravityInstance == &high && !MR::isLightGravity(info),
                "the requester's own gravity host should be excluded using retail host identity rules");

        auto shadow = ConstantGravity(TVec3f{-1.0F, 0.0F, 0.0F}, 25.0F);
        shadow.mPriority = 9;
        shadow.mGravityType = GRAVITY_TYPE_SHADOW;
        MR::registerGravity(&shadow);
        require(MR::calcDropShadowVector(&caller, TVec3f{}, &destination, nullptr, 0U) &&
                    destination.epsilonEquals(TVec3f{-1.0F, 0.0F, 0.0F}, 0.0001F),
                "gravity type masks must select real shadow-only fields");

        require_throws<std::logic_error>([&] { MR::registerGravity(&high); },
                                         "duplicate registration must not corrupt manager ordering");
        require_throws<std::invalid_argument>([&] { MR::registerGravity(nullptr); },
                                               "null registration must be explicitly rejected");
    }

    void test_jmap_parameters_are_real() {
        auto gravity = PlanetGravity{};
        const auto jmap = make_complete_gravity_jmap();
        MR::settingGravityParamFromJMap(&gravity, JMapInfoIter(&jmap, 0));
        require(std::abs(gravity.mRange - 250.0F) < 0.0001F &&
                    std::abs(gravity.mDistant - 10.0F) < 0.0001F && gravity.mPriority == 7 &&
                    gravity.mGravityId == 42 && gravity.mGravityType == GRAVITY_TYPE_SHADOW &&
                    gravity.mGravityPower == GRAVITY_POWER_LIGHT && gravity.mIsInverse,
                "all retail gravity JMap parameters must be applied to PlanetGravity state");

        auto info = GravityInfo{};
        info.mGravityInstance = &gravity;
        require(MR::isLightGravity(info),
                "isLightGravity must inspect the selected PlanetGravity instead of returning a constant");
        require_throws<std::invalid_argument>(
            [&] { MR::settingGravityParamFromJMap(nullptr, JMapInfoIter(&jmap, 0)); },
            "JMap setters must reject a missing PlanetGravity instead of silently doing nothing");
    }

    void test_exact_creator_registration_and_no_placement_synthesis() {
        auto scene = GravityScene{};
        auto destination = TVec3f{9.0F, 9.0F, 9.0F};
        require(!scene.manager->calcTotalGravityVector(
                    &destination, nullptr, TVec3f{}, GRAVITY_TYPE_NORMAL, 0U) &&
                    destination.epsilonEquals(TVec3f{}, 0.0F),
                "the exact scene manager must start empty without synthesized gravity");

        auto creator = PointGravityCreator{};
        const auto jmap = make_fieldless_jmap();
        auto* instance = creator.createFromJMap(JMapInfoIter(&jmap, 0));
        require(instance == creator.getGravity() &&
                    scene.manager->calcTotalGravityVector(
                        &destination, nullptr, TVec3f{0.0F, 600.0F, 0.0F},
                        GRAVITY_TYPE_NORMAL, 0U) &&
                    destination.epsilonEquals(TVec3f{0.0F, -1.0F, 0.0F}, 0.0001F),
                "the exact PointGravityCreator must own construction and register its retail field");
        require(NameObjFactory::getCreator("GlobalPointGravity") == MR::createGlobalPointGravityObj &&
                    NameObjFactory::getCreator("GlobalCubeGravity") == MR::createGlobalCubeGravityObj &&
                    NameObjFactory::getCreator("GlobalConeGravity") == MR::createGlobalConeGravityObj &&
                    NameObjFactory::getCreator("GlobalDiskGravity") == MR::createGlobalDiskGravityObj &&
                    NameObjFactory::getCreator("GlobalDiskTorusGravity") == MR::createGlobalDiskTorusGravityObj &&
                    NameObjFactory::getCreator("GlobalPlaneGravity") == MR::createGlobalPlaneGravityObj &&
                    NameObjFactory::getCreator("GlobalPlaneGravityInBox") == MR::createGlobalPlaneInBoxGravityObj &&
                    NameObjFactory::getCreator("GlobalPlaneGravityInCylinder") == MR::createGlobalPlaneInCylinderGravityObj &&
                    NameObjFactory::getCreator("GlobalSegmentGravity") == MR::createGlobalSegmentGravityObj &&
                    NameObjFactory::getCreator("GlobalWireGravity") == MR::createGlobalWireGravityObj,
                "the host factory must expose the exact retail gravity actor creators");

        auto other_holder = SceneObjHolder{};
        require_throws<std::logic_error>(
            [&] {
                const auto other_binding =
                    smgpc::scene::SceneObjHolderBinding(other_holder);
                (void)other_binding;
            },
            "two scenes must not silently replace gravity ownership");
        delete creator.mGravityInstance;
        creator.mGravityInstance = nullptr;
    }

    void test_explicit_blocked_preflight_has_no_side_effects() {
        auto placements = std::array<smgpc::scene::StagePlacementObject, 2U>{};
        placements[0].object_name = "GlobalPointGravity";
        placements[0].factory_supported = true;
        placements[0].table_path = "jmp/placement/common/planetobjinfo";
        placements[1].object_name = "RestartCube";
        placements[1].table_path = "jmp/placement/common/areaobjinfo";

        auto scene = GravityScene{};
        auto creator = PointGravityCreator{};
        const auto jmap = make_fieldless_jmap();
        auto construction_reached = false;
        auto rejected = false;
        try {
            smgpc::scene::preflight_stage_placements_or_throw(
                "PreflightProbeGalaxy", placements, &placements[0]);
            construction_reached = true;
            (void)creator.createFromJMap(JMapInfoIter(&jmap, 0));
        } catch (const std::runtime_error&) {
            rejected = true;
        }

        auto caller = NameObj("preflight-query-caller");
        auto destination = TVec3f{9.0F, 9.0F, 9.0F};
        const auto has_gravity =
            MR::calcGravityVector(&caller, TVec3f{}, &destination, nullptr, 0U);
        require(rejected && !construction_reached && creator.mGravityInstance == nullptr &&
                    !has_gravity && destination.epsilonEquals(TVec3f{}, 0.0F),
                "an explicit root request must reject all blockers before constructing or registering gravity");
    }

    void test_generic_scene_obj_post_placement_binds_followers() {
        auto holder = SceneObjHolder{};
        auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        auto link_info = make_linked_gravity_jmap();
        const auto iter = JMapInfoIter(&link_info, 0);
        auto follower_owner = NameObj("follow-binding-probe");
        auto follower = FollowBindingProbe(&follower_owner, iter);
        auto target = LiveActor("follow-target");
        auto explicit_host_mtx = TPos3f{};
        explicit_host_mtx.identity();
        explicit_host_mtx.setTrans(16.0F, 27.0F, 41.0F);

        MR::addBaseMatrixFollower(&follower);
        MR::addBaseMatrixFollowTarget(&target, iter, &explicit_host_mtx, nullptr);
        require(follower.bound_host == nullptr,
                "followers must remain unbound until the scene post-placement phase");
        binding.init_after_placement();
        require(follower.bound_host == &target && follower.getFollowTargetActor() == &target,
                "the generic SceneObjHolder pass must run the exact BaseMatrix follower binding");

        auto follow_mtx = TPos3f{};
        follower.calcFollowMatrix(&follow_mtx);
        auto follow_translation = TVec3f{};
        follow_mtx.getTrans(follow_translation);
        require(follow_translation.epsilonEquals(TVec3f{6.0F, 7.0F, 11.0F}, 0.0001F),
                "the exact follower must compute host * inverse(placement) with in-place matrix inversion");
        require(target.getBaseMtx() == nullptr,
                "a model-less LiveActor must retain the retail absent base-matrix result");
        auto modeled_target = LiveActor("modeled-follow-target");
        modeled_target.initModelManagerWithAnm("FollowTargetProbe", nullptr, false);
        require(modeled_target.getBaseMtx() != nullptr,
                "a LiveActor with a real host model must expose its base matrix");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    const auto tests = std::array{
        TestCase{"absent manager is explicit", test_absent_manager_is_explicit},
        TestCase{"real manager rules and info", test_real_manager_rules_and_info},
        TestCase{"JMap parameters are real", test_jmap_parameters_are_real},
        TestCase{"exact creator registration without placement synthesis", test_exact_creator_registration_and_no_placement_synthesis},
        TestCase{"explicit blocked preflight has no side effects", test_explicit_blocked_preflight_has_no_side_effects},
        TestCase{"generic scene object post-placement follower binding", test_generic_scene_obj_post_placement_binds_followers},
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
        std::cerr << failures << " gravity real-or-absent test(s) failed\n";
        return 1;
    }
    std::cout << tests.size() << " gravity real-or-absent test(s) passed\n";
    return 0;
}
