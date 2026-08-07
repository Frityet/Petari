#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "compat/GameGravityCompat.hpp"
#include "resource/BcsvTable.hpp"
#include "scene/StageGravityService.hpp"
#include "scene/StagePlacementResolver.hpp"

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

    void test_absent_service_is_explicit() {
        auto actor = LiveActor("gravity-absence-probe");
        auto destination = TVec3f{3.0F, 4.0F, 5.0F};
        require_throws<std::logic_error>(
            [&] { (void)MR::calcGravityVector(&actor, &destination, nullptr, 0U); },
            "gravity queries must reject a missing scene-owned service");
        require(destination.epsilonEquals(TVec3f{3.0F, 4.0F, 5.0F}, 0.0F),
                "an unavailable gravity query must not fabricate a zero vector");

        auto gravity = ConstantGravity(TVec3f{0.0F, -1.0F, 0.0F}, 100.0F);
        require_throws<std::logic_error>([&] { MR::registerGravity(&gravity); },
                                         "registration must reject a missing scene owner");
        require_throws<std::invalid_argument>(
            [&] { (void)MR::calcGravityVector(static_cast<const LiveActor*>(nullptr), &destination, nullptr, 0U); },
            "a null actor must not be treated as zero gravity");
    }

    void test_real_manager_rules_and_info() {
        auto service = smgpc::scene::StageGravityService{};
        service.activate();

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
        service.clear();
        service.deactivate();
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

    void test_placement_support_or_explicit_rejection() {
        auto point = smgpc::scene::StagePlacementObject{};
        point.object_name = "GlobalPointGravity";
        point.translation = {0.0F, 0.0F, 0.0F};
        point.scale = {1.0F, 1.0F, 1.0F};
        point.object_args.fill(-1);
        point.jmap_info = make_fieldless_jmap();
        point.jmap_entry_index = 0;

        auto service = smgpc::scene::StageGravityService{};
        const auto placements = std::array{point};
        const auto stats = service.load(placements);
        auto destination = TVec3f{};
        require(stats.gravity_count == 1U && stats.unsupported_count == 0U &&
                    service.query(TVec3f{0.0F, 600.0F, 0.0F}, &destination) &&
                    destination.epsilonEquals(TVec3f{0.0F, -1.0F, 0.0F}, 0.0001F),
                "supported point placement data should create a real PlanetGravity field");

        auto switched_point = point;
        switched_point.switch_a_id = 3;
        const auto dynamic = std::array{switched_point};
        require_throws<std::runtime_error>([&] { (void)service.load(dynamic); },
                                           "dynamic gravity must require the real switch/follower actor lifecycle");
        require(service.empty() && service.stats().unsupported_count == 1U,
                "a rejected dynamic placement must not retain a static substitute field");

        auto cube = point;
        cube.object_name = "GlobalCubeGravity";
        const auto unsupported = std::array{cube};
        require_throws<std::runtime_error>([&] { (void)service.load(unsupported); },
                                           "unimplemented gravity classes must be explicitly unavailable");
        require(service.empty() && service.stats().unsupported_count == 1U,
                "a rejected gravity load must not retain a partial stand-in field");

        auto other = smgpc::scene::StageGravityService{};
        service.activate();
        require_throws<std::logic_error>([&] { other.activate(); },
                                         "two scenes must not silently replace gravity ownership");
        service.deactivate();
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    const auto tests = std::array{
        TestCase{"absent service is explicit", test_absent_service_is_explicit},
        TestCase{"real manager rules and info", test_real_manager_rules_and_info},
        TestCase{"JMap parameters are real", test_jmap_parameters_are_real},
        TestCase{"placement support or explicit rejection", test_placement_support_or_explicit_rejection},
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
