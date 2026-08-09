#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/GraviryFollower.hpp"
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
#include "Game/Util/JMapLinkInfo.hpp"
#include "compat/GameGravityCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/GlobalGravityOwnership.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageHostScene.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
#include <utility>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

#ifndef NDEBUG
    class ScopedEnvironmentVariable final {
    public:
        ScopedEnvironmentVariable(const char* name, const std::string& value)
            : _name(name) {
            if (const auto* previous = std::getenv(_name.c_str()); previous != nullptr) {
                _previous = previous;
            }
            if (::setenv(_name.c_str(), value.c_str(), 1) != 0) {
                throw std::runtime_error("failed to configure test environment variable: " + _name);
            }
        }

        ~ScopedEnvironmentVariable() {
            if (_previous.has_value()) {
                (void)::setenv(_name.c_str(), _previous->c_str(), 1);
            } else {
                (void)::unsetenv(_name.c_str());
            }
        }

        ScopedEnvironmentVariable(const ScopedEnvironmentVariable&) = delete;
        ScopedEnvironmentVariable& operator=(const ScopedEnvironmentVariable&) = delete;

    private:
        std::string _name;
        std::optional<std::string> _previous;
    };
#endif

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
        constexpr auto field_count = 9U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        constexpr auto entry_size = 36U;
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
        write_field(bytes, 8U, "FollowId", 32U, smgpc::resource::BcsvFieldType::Int32);
        write_be32(bytes, data_offset, 42U);
        write_be32(bytes, data_offset + 4U, 42U);
        write_be_float(bytes, data_offset + 8U, 10.0F);
        write_be_float(bytes, data_offset + 12U, 20.0F);
        write_be_float(bytes, data_offset + 16U, 30.0F);
        write_be32(bytes, data_offset + 32U, 7U);

        auto info = JMapInfo::from_bcsv(bytes);
        info.setName("objinfo");
        info.setPlacedZoneId(3);
        return info;
    }

    JMapInfo make_wire_gravity_jmap() {
        auto placement = make_fieldless_jmap();

        constexpr auto path_field_count = 1U;
        constexpr auto path_data_offset = 0x10U + path_field_count * 0x0cU;
        constexpr auto path_entry_size = 16U;
        auto path_bytes = std::vector<std::uint8_t>(
            path_data_offset + path_entry_size, 0U);
        write_be32(path_bytes, 0U, 1U);
        write_be32(path_bytes, 4U, path_field_count);
        write_be32(path_bytes, 8U, path_data_offset);
        write_be32(path_bytes, 12U, path_entry_size);
        write_field(path_bytes, 0U, "closed", 0U,
                    smgpc::resource::BcsvFieldType::InlineString);
        auto path_info = JMapInfo::from_bcsv(path_bytes);

        constexpr auto point_field_count = 9U;
        constexpr auto point_data_offset =
            0x10U + point_field_count * 0x0cU;
        constexpr auto point_entry_size = 36U;
        auto point_bytes = std::vector<std::uint8_t>(
            point_data_offset + 2U * point_entry_size, 0U);
        write_be32(point_bytes, 0U, 2U);
        write_be32(point_bytes, 4U, point_field_count);
        write_be32(point_bytes, 8U, point_data_offset);
        write_be32(point_bytes, 12U, point_entry_size);
        constexpr auto point_fields = std::array{
            "pnt0_x", "pnt0_y", "pnt0_z", "pnt1_x", "pnt1_y",
            "pnt1_z", "pnt2_x", "pnt2_y", "pnt2_z",
        };
        for (auto index = std::size_t{}; index < point_fields.size(); ++index) {
            write_field(point_bytes, index, point_fields[index],
                        static_cast<std::uint16_t>(index * sizeof(float)),
                        smgpc::resource::BcsvFieldType::Float);
        }
        for (const auto field_index : {1U, 4U, 7U}) {
            write_be_float(point_bytes,
                           point_data_offset + point_entry_size +
                               field_index * sizeof(float),
                           1000.0F);
        }
        auto point_info = JMapInfo::from_bcsv(point_bytes);
        placement.setRailInfo(0, std::move(path_info), std::move(point_info), 0);
        return placement;
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
            : dvd("/"), placements(), demo(dvd, placements), holder(), binding(holder),
              manager(static_cast<PlanetGravityManager*>(
                  MR::createSceneObj(SceneObj_PlanetGravityManager))) {
            require(manager != nullptr,
                    "a bound stage scene must create the exact PlanetGravityManager SceneObj");
        }

        smgpc::runtime::DvdFileSystemService dvd;
        std::array<smgpc::scene::StagePlacementObject, 0U> placements;
        smgpc::compat::DemoSceneRuntime demo;
        SceneObjHolder holder;
        smgpc::scene::SceneObjHolderBinding binding;
        PlanetGravityManager* manager;
    };

    void init_captured_gravity(NameObj& object, const JMapInfoIter& iter) {
        smgpc::compat::prepare_global_gravity_init(object);
        try {
            object.init(iter);
        } catch (...) {
            smgpc::compat::capture_failed_global_gravity_children(object);
            throw;
        }
        smgpc::compat::capture_global_gravity_children(object);
    }

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

    void test_factory_owned_creator_field_and_wire_graph_reclamation() {
        constexpr auto creator_names = std::array<std::string_view, 10U>{
            "GlobalCubeGravity",
            "GlobalConeGravity",
            "GlobalDiskGravity",
            "GlobalDiskTorusGravity",
            "GlobalPlaneGravity",
            "GlobalPlaneGravityInBox",
            "GlobalPlaneGravityInCylinder",
            "GlobalPointGravity",
            "GlobalSegmentGravity",
            "GlobalWireGravity",
        };
        const auto totals_before =
            smgpc::compat::global_gravity_ownership_totals();
        {
            auto dvd = smgpc::runtime::DvdFileSystemService("/");
            auto scene = GravityScene{};
            auto wrappers = std::vector<std::unique_ptr<NameObj>>{};
            wrappers.reserve(creator_names.size());
            auto fieldless = make_fieldless_jmap();
            auto wire = make_wire_gravity_jmap();

            for (const auto creator_name : creator_names) {
                auto object = smgpc::scene::nameobj::create_name_obj(
                    dvd, creator_name, creator_name.data());
                auto* actor = dynamic_cast<GlobalGravityObj*>(object.get());
                require(actor != nullptr && actor->mGravityCreator != nullptr,
                        "the generic factory hook must retain each exact gravity wrapper and creator");
                const auto& info = creator_name == "GlobalWireGravity" ? wire : fieldless;
                init_captured_gravity(*actor, JMapInfoIter(&info, 0));
                require(actor->getGravity() != nullptr &&
                            actor->getGravity()->mIsRegistered,
                        "each exact creator field must remain registered until scene teardown");
                if (creator_name == "GlobalWireGravity") {
                    auto* creator = dynamic_cast<WireGravityCreator*>(
                        actor->mGravityCreator);
                    require(creator != nullptr && creator->mRailRider != nullptr &&
                                creator->mRailRider->mBezierRail != nullptr &&
                                creator->mRailRider->mBezierRail->mNumRailParts == 1 &&
                                creator->mGravityInstance != nullptr &&
                                creator->mGravityInstance->mPoints.size() == 21,
                            "WireGravity must retain its complete retail rail and sampled-point graph before teardown");
                }
                wrappers.push_back(std::move(object));
            }
            wrappers.clear();
        }
        const auto totals_after =
            smgpc::compat::global_gravity_ownership_totals();
        require(totals_after.adopted_creators ==
                        totals_before.adopted_creators + creator_names.size() &&
                    totals_after.reclaimed_creators ==
                        totals_before.reclaimed_creators + creator_names.size() &&
                    totals_after.reclaimed_fields ==
                        totals_before.reclaimed_fields + creator_names.size() &&
                    totals_after.reclaimed_wire_rail_riders ==
                        totals_before.reclaimed_wire_rail_riders + 1U &&
                    totals_after.reclaimed_wire_bezier_rails ==
                        totals_before.reclaimed_wire_bezier_rails + 1U &&
                    totals_after.reclaimed_wire_rail_parts ==
                        totals_before.reclaimed_wire_rail_parts + 1U,
                "scene teardown must type-delete all ten creator/field variants and the complete Wire rail graph");
    }

    void test_transactional_rejection_and_duplicate_rules() {
        auto dvd = smgpc::runtime::DvdFileSystemService("/");
        const auto totals_before =
            smgpc::compat::global_gravity_ownership_totals();
        require_throws<std::logic_error>(
            [&] {
                (void)smgpc::scene::nameobj::create_name_obj(
                    dvd, "GlobalPointGravity", "missing-owner");
            },
            "gravity factory construction without an active scene owner must be rejected transactionally");

        {
            auto scene = GravityScene{};
            auto first = smgpc::scene::nameobj::create_name_obj(
                dvd, "GlobalPointGravity", "first-open-gravity");
            require_throws<std::logic_error>(
                [&] {
                    (void)smgpc::scene::nameobj::create_name_obj(
                        dvd, "GlobalPointGravity", "overlapping-open-gravity");
                },
                "overlapping uninitialized gravity actors must be rejected and reclaimed");

            auto* actor = dynamic_cast<GlobalGravityObj*>(first.get());
            require(actor != nullptr, "the first adopted gravity wrapper must remain alive");
            require_throws<std::logic_error>(
                [&] { smgpc::compat::adopt_global_gravity_children(*actor); },
                "the same gravity creator must not be adopted twice");
            require(actor->mGravityCreator != nullptr,
                    "duplicate rejection must not reclaim the already-owned creator");
            auto info = make_fieldless_jmap();
            init_captured_gravity(*actor, JMapInfoIter(&info, 0));
            first.reset();
        }

        const auto totals_after =
            smgpc::compat::global_gravity_ownership_totals();
        require(totals_after.rejected_adoptions ==
                        totals_before.rejected_adoptions + 3U &&
                    totals_after.transactional_creator_reclaims ==
                        totals_before.transactional_creator_reclaims + 2U,
                "missing-owner and overlap failures must reclaim new raw creators while duplicate adoption preserves the owned one");
    }

    void test_partial_follower_capture_preserves_original_failure() {
        auto dvd = smgpc::runtime::DvdFileSystemService("/");
        const auto totals_before =
            smgpc::compat::global_gravity_ownership_totals();
        {
            auto scene = GravityScene{};
            auto object = smgpc::scene::nameobj::create_name_obj(
                dvd, "GlobalPointGravity", "partial-follow-gravity");
            auto* actor = dynamic_cast<GlobalGravityObj*>(object.get());
            require(actor != nullptr,
                    "the partial-init probe must create its exact gravity wrapper");
            auto linked = make_linked_gravity_jmap();
            const auto iter = JMapInfoIter(&linked, 0);
            smgpc::compat::prepare_global_gravity_init(*actor);
            auto* field = actor->mGravityCreator->createFromJMap(iter);
            auto* holder = static_cast<BaseMatrixFollowTargetHolder*>(
                MR::createSceneObj(SceneObj_BaseMatrixFollowTargetHolder));
            require(field != nullptr && holder != nullptr,
                    "the partial-init probe requires its registered field and follower holder");
            auto* partial_follower = new GraviryFollower(actor, iter);
            holder->mFollowers.push_back(partial_follower);

            auto observed_error = std::string{};
            try {
                try {
                    throw std::runtime_error("retail-init-sentinel");
                } catch (...) {
                    smgpc::compat::capture_failed_global_gravity_children(*actor);
                    throw;
                }
            } catch (const std::runtime_error& error) {
                observed_error = error.what();
            }
            require(observed_error == "retail-init-sentinel" &&
                        partial_follower->mGravity == field &&
                        partial_follower->mLinkInfo != nullptr &&
                        partial_follower->mFollowTarget == nullptr,
                    "failure capture must retain the partial follower/link without replacing the original init error");
            smgpc::compat::capture_failed_global_gravity_children(*actor);
            object.reset();
        }
        const auto totals_after =
            smgpc::compat::global_gravity_ownership_totals();
        require(totals_after.reclaimed_creators ==
                        totals_before.reclaimed_creators + 1U &&
                    totals_after.reclaimed_fields ==
                        totals_before.reclaimed_fields + 1U &&
                    totals_after.reclaimed_followers ==
                        totals_before.reclaimed_followers + 1U &&
                    totals_after.reclaimed_link_infos ==
                        totals_before.reclaimed_link_infos + 1U &&
                    totals_after.reclaimed_follow_targets ==
                        totals_before.reclaimed_follow_targets,
                "partial init teardown must reclaim its creator, field, follower, and link exactly once without inventing a target");
    }

    void test_deferred_field_follower_and_same_holder_recreation() {
        auto dvd = smgpc::runtime::DvdFileSystemService("/");
        auto placements =
            std::array<smgpc::scene::StagePlacementObject, 0U>{};
        auto demo = smgpc::compat::DemoSceneRuntime(dvd, placements);
        auto holder = SceneObjHolder{};
        const auto totals_before =
            smgpc::compat::global_gravity_ownership_totals();
        auto* external_target = static_cast<BaseMatrixFollowTarget*>(nullptr);
        auto* external_link = static_cast<JMapLinkInfo*>(nullptr);
        {
            auto binding = smgpc::scene::SceneObjHolderBinding(holder);
            auto* manager = static_cast<PlanetGravityManager*>(
                MR::createSceneObj(SceneObj_PlanetGravityManager));
            require(manager != nullptr, "the first ownership generation requires its exact manager");

            auto linked = make_linked_gravity_jmap();
            {
                auto stale_owner = NameObj("expired-external-follower-owner");
                auto stale_follower = FollowBindingProbe(
                    &stale_owner, JMapInfoIter(&linked, 0));
                MR::addBaseMatrixFollower(&stale_follower);
                external_target = stale_follower.mFollowTarget;
                external_link = stale_follower.mLinkInfo;
            }

            auto object = smgpc::scene::nameobj::create_name_obj(
                dvd, "GlobalPointGravity", "owned-follow-gravity");
            auto* actor = dynamic_cast<GlobalGravityObj*>(object.get());
            require(actor != nullptr, "the followed point gravity must use the exact wrapper");
            init_captured_gravity(*actor, JMapInfoIter(&linked, 0));
            auto* field = actor->getGravity();
            auto* followers = static_cast<BaseMatrixFollowTargetHolder*>(
                holder.getObj(SceneObj_BaseMatrixFollowTargetHolder));
            auto* owned_follower = followers != nullptr && followers->mFollowers.size() == 2 ?
                                       dynamic_cast<GraviryFollower*>(followers->mFollowers[1]) :
                                       nullptr;
            require(owned_follower != nullptr && owned_follower->mFollowID == 7 &&
                        owned_follower->mGravity == field,
                    "capture must skip the expired prefix and retain the exact FollowId gravity suffix");

            object.reset();
            auto caller = NameObj("post-wrapper-gravity-query");
            auto destination = TVec3f{};
            require(manager->calcTotalGravityVector(
                        &destination, nullptr, TVec3f{10.0F, 620.0F, 30.0F},
                        GRAVITY_TYPE_NORMAL, 0U) &&
                        destination.epsilonEquals(
                            TVec3f{0.0F, -1.0F, 0.0F}, 0.0001F),
                    "the manager must keep its registered field alive after the actor wrapper retires");
        }
        require(!holder.isExist(SceneObj_PlanetGravityManager) &&
                    !holder.isExist(SceneObj_BaseMatrixFollowTargetHolder),
                "reverse SceneObj teardown must reconstruct the external holder with empty slots");
        const auto first_generation_totals =
            smgpc::compat::global_gravity_ownership_totals();
        require(first_generation_totals.reclaimed_follow_targets ==
                    totals_before.reclaimed_follow_targets,
                "a gravity follower that reused the external prefix target must leave that borrowed target unreclaimed");
        require(external_target != nullptr && external_link != nullptr,
                "the expired prefix must retain its independently owned target/link fixture");
        delete external_target;
        delete external_link;

        {
            auto second_binding = smgpc::scene::SceneObjHolderBinding(holder);
            auto* second_manager = static_cast<PlanetGravityManager*>(
                MR::createSceneObj(SceneObj_PlanetGravityManager));
            require(second_manager != nullptr,
                    "the same external holder must create a fresh second-generation manager");
            auto object = smgpc::scene::nameobj::create_name_obj(
                dvd, "GlobalPointGravity", "second-generation-gravity");
            auto* actor = dynamic_cast<GlobalGravityObj*>(object.get());
            require(actor != nullptr,
                    "the second generation must create its exact gravity wrapper");
            auto info = make_linked_gravity_jmap();
            init_captured_gravity(*actor, JMapInfoIter(&info, 0));
            object.reset();
        }
        require(!holder.isExist(SceneObj_PlanetGravityManager),
                "the recreated holder must also finish with no stale SceneObj slots");

        const auto totals_after =
            smgpc::compat::global_gravity_ownership_totals();
        require(totals_after.reclaimed_creators ==
                        totals_before.reclaimed_creators + 2U &&
                    totals_after.reclaimed_fields ==
                        totals_before.reclaimed_fields + 2U &&
                    totals_after.reclaimed_followers ==
                        totals_before.reclaimed_followers + 2U &&
                    totals_after.reclaimed_link_infos ==
                        totals_before.reclaimed_link_infos + 2U &&
                    totals_after.reclaimed_follow_targets ==
                        totals_before.reclaimed_follow_targets + 1U,
                "two ownership generations must reclaim fields, the proven follower/link/target graph, and recreate cleanly");
    }

    void test_explicit_blocked_preflight_has_no_side_effects() {
        auto placements = std::array<smgpc::scene::StagePlacementObject, 2U>{};
        placements[0].object_name = "GlobalPointGravity";
        placements[0].creator_identifier = "GlobalPointGravity";
        placements[0].factory_supported = true;
        placements[0].table_path = "jmp/placement/common/planetobjinfo";
        placements[1].object_name = "RestartCube";
        placements[1].creator_identifier = "RestartCube";
        placements[1].table_path = "jmp/placement/common/areaobjinfo";

        auto scene = GravityScene{};
        auto creator = PointGravityCreator{};
        const auto jmap = make_fieldless_jmap();
        auto construction_reached = false;
        auto rejected = false;
#ifndef NDEBUG
        const auto report_path = std::filesystem::temp_directory_path() /
                                 ("smgpc-blocked-preflight-" +
                                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                                  ".md");
        const auto report_environment = ScopedEnvironmentVariable(
            "SMGPC_STAGE_PLACEMENT_REPORT_PATH", report_path.string());
#endif
        try {
            smgpc::scene::preflight_stage_placements_or_throw(
                "PreflightProbeGalaxy", 7, placements, &placements[0]);
            construction_reached = true;
            (void)creator.createFromJMap(JMapInfoIter(&jmap, 0));
        } catch (const std::runtime_error&) {
            rejected = true;
        }
#ifndef NDEBUG
        auto report_stream = std::ifstream(report_path);
        const auto report = std::string(
            std::istreambuf_iterator<char>(report_stream), std::istreambuf_iterator<char>());
        std::filesystem::remove(report_path);
        require(report.find("stage: PreflightProbeGalaxy\n") != std::string::npos &&
                    report.find("scenario: 7\n") != std::string::npos &&
                    report.find("phase: preflight\n") != std::string::npos &&
                    report.find("total_objects: 2\n") != std::string::npos &&
                    report.find("complete_objects: 1\n") != std::string::npos &&
                    report.find("blocked_objects: 1\n") != std::string::npos &&
                    report.find("intentionally_ignored_objects: 0\n") != std::string::npos &&
                    report.find("- status: complete\n  object: GlobalPointGravity\n") != std::string::npos &&
                    report.find("object: RestartCube\n") != std::string::npos &&
                    report.find("archive: \n") == std::string::npos &&
                    report.find("created_objects:") == std::string::npos &&
                    report.find("- status: created\n") == std::string::npos,
                "strict preflight must write the complete placement report before rejecting construction");
#endif

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
        TestCase{"factory-owned creator field and wire graph reclamation", test_factory_owned_creator_field_and_wire_graph_reclamation},
        TestCase{"transactional rejection and duplicate rules", test_transactional_rejection_and_duplicate_rules},
        TestCase{"partial follower capture preserves original failure", test_partial_follower_capture_preserves_original_failure},
        TestCase{"deferred field follower and same-holder recreation", test_deferred_field_follower_and_same_holder_recreation},
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
