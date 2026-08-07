#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/FixedPosition.hpp"
#include "compat/FixedPositionCompat.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
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

    void write_be32(std::vector< std::uint8_t >& bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast< std::uint8_t >(value >> 24U);
        bytes[offset + 1U] = static_cast< std::uint8_t >(value >> 16U);
        bytes[offset + 2U] = static_cast< std::uint8_t >(value >> 8U);
        bytes[offset + 3U] = static_cast< std::uint8_t >(value);
    }

    void write_be16(std::vector< std::uint8_t >& bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast< std::uint8_t >(value >> 8U);
        bytes[offset + 1U] = static_cast< std::uint8_t >(value);
    }

    void write_be_float(std::vector< std::uint8_t >& bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast< std::uint32_t >(value));
    }

    void write_bcsv_field(std::vector< std::uint8_t >& bytes, std::size_t index, std::string_view name, std::uint16_t offset,
                          smgpc::resource::BcsvFieldType type) {
        const auto field_offset = 0x10U + index * 0x0cU;
        write_be32(bytes, field_offset, smgpc::resource::jmap_hash(name));
        write_be32(bytes, field_offset + 0x04U, 0xffffffffU);
        write_be16(bytes, field_offset + 0x08U, offset);
        bytes[field_offset + 0x0aU] = 0U;
        bytes[field_offset + 0x0bU] = static_cast< std::uint8_t >(type);
    }

    std::vector< std::uint8_t > make_fixed_position_bcsv(std::string_view joint_name, const TVec3f& translation, const TVec3f& rotation) {
        constexpr auto field_count = 7U;
        constexpr auto entry_size = 28U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        const auto string_table_offset = data_offset + entry_size;
        auto bytes = std::vector< std::uint8_t >(string_table_offset + joint_name.size() + 1U, 0U);

        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "JointName", 0U, smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 1U, "TransX", 4U, smgpc::resource::BcsvFieldType::Float);
        write_bcsv_field(bytes, 2U, "TransY", 8U, smgpc::resource::BcsvFieldType::Float);
        write_bcsv_field(bytes, 3U, "TransZ", 12U, smgpc::resource::BcsvFieldType::Float);
        write_bcsv_field(bytes, 4U, "RotateX", 16U, smgpc::resource::BcsvFieldType::Float);
        write_bcsv_field(bytes, 5U, "RotateY", 20U, smgpc::resource::BcsvFieldType::Float);
        write_bcsv_field(bytes, 6U, "RotateZ", 24U, smgpc::resource::BcsvFieldType::Float);
        write_be32(bytes, data_offset, 0U);
        write_be_float(bytes, data_offset + 4U, translation.x);
        write_be_float(bytes, data_offset + 8U, translation.y);
        write_be_float(bytes, data_offset + 12U, translation.z);
        write_be_float(bytes, data_offset + 16U, rotation.x);
        write_be_float(bytes, data_offset + 20U, rotation.y);
        write_be_float(bytes, data_offset + 24U, rotation.z);
        std::copy(joint_name.begin(), joint_name.end(), bytes.begin() + static_cast< std::ptrdiff_t >(string_table_offset));
        return bytes;
    }

    smgpc::resource::RarcArchive make_single_resource_rarc(std::string_view file_name, const std::vector< std::uint8_t >& file_data) {
        constexpr auto header_size = std::size_t{0x20U};
        constexpr auto info_offset = std::size_t{0x20U};
        constexpr auto directory_offset = std::size_t{0x40U};
        constexpr auto file_entry_offset = std::size_t{0x50U};
        constexpr auto string_table_offset = std::size_t{0x64U};
        constexpr auto file_data_offset = std::size_t{0x100U};
        require(string_table_offset + file_name.size() + 1U <= file_data_offset, "test RARC file name must fit before its data section");

        auto bytes = std::vector< std::uint8_t >(file_data_offset + file_data.size(), 0U);
        write_be32(bytes, 0x00U, 0x52415243U);
        write_be32(bytes, 0x04U, static_cast< std::uint32_t >(bytes.size()));
        write_be32(bytes, 0x08U, header_size);
        write_be32(bytes, 0x0cU, file_data_offset - header_size);
        write_be32(bytes, 0x10U, static_cast< std::uint32_t >(file_data.size()));
        write_be32(bytes, info_offset + 0x00U, 1U);
        write_be32(bytes, info_offset + 0x04U, directory_offset - info_offset);
        write_be32(bytes, info_offset + 0x08U, 1U);
        write_be32(bytes, info_offset + 0x0cU, file_entry_offset - info_offset);
        write_be32(bytes, info_offset + 0x10U, static_cast< std::uint32_t >(file_name.size() + 1U));
        write_be32(bytes, info_offset + 0x14U, string_table_offset - info_offset);
        write_be16(bytes, directory_offset + 0x0aU, 1U);
        write_be32(bytes, directory_offset + 0x0cU, 0U);
        write_be16(bytes, file_entry_offset + 0x00U, 0U);
        write_be16(bytes, file_entry_offset + 0x02U, smgpc::resource::RarcArchive::hash_name(file_name));
        bytes[file_entry_offset + 0x04U] = 1U;
        write_be32(bytes, file_entry_offset + 0x08U, 0U);
        write_be32(bytes, file_entry_offset + 0x0cU, static_cast< std::uint32_t >(file_data.size()));
        std::copy(file_name.begin(), file_name.end(), bytes.begin() + static_cast< std::ptrdiff_t >(string_table_offset));
        std::copy(file_data.begin(), file_data.end(), bytes.begin() + static_cast< std::ptrdiff_t >(file_data_offset));
        return smgpc::resource::RarcArchive::from_bytes(std::move(bytes));
    }

    void test_reads_requested_resource_data() {
        const auto translation = TVec3f{11.0F, -22.5F, 33.25F};
        const auto rotation = TVec3f{-45.0F, 90.0F, 12.5F};
        const auto archive = make_single_resource_rarc("FiXeDReSoUrCe.BcSv", make_fixed_position_bcsv("", translation, rotation));
        const auto resource = smgpc::compat::load_fixed_position_resource(archive, "FixedResource");
        require(!resource.joint_name.has_value(), "empty JointName should select the actor base matrix");
        require(resource.translation.epsilonEquals(translation, 0.0F) && resource.rotation.epsilonEquals(rotation, 0.0F),
                "requested BCSV Trans/Rotate values must not become invented zero offsets");
    }

    void test_preserves_requested_joint_name() {
        const auto archive =
            make_single_resource_rarc("JointResource.bcsv", make_fixed_position_bcsv("HandR", TVec3f{1.0F, 2.0F, 3.0F}, TVec3f{4.0F, 5.0F, 6.0F}));
        const auto resource = smgpc::compat::load_fixed_position_resource(archive, "JointResource");
        require(resource.joint_name == std::optional< std::string >{"HandR"}, "requested JointName must be preserved");
    }

    void test_missing_resource_is_absent() {
        const auto archive = make_single_resource_rarc("FixedResource.bcsv", make_fixed_position_bcsv("", TVec3f{}, TVec3f{}));
        auto rejected = false;
        try {
            static_cast< void >(smgpc::compat::load_fixed_position_resource(archive, "NotPresent"));
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        require(rejected, "missing FixedPosition BCSV must remain absent");
    }

    void test_unsupported_construction_fails_explicitly() {
        auto host = LiveActor("fixed-position-absence-host");
        host.calcAndSetBaseMtx();

        auto resource_rejected = false;
        try {
            static_cast< void >(FixedPosition(&host, "MissingResource", static_cast< const LiveActor* >(nullptr)));
        } catch (const std::runtime_error&) {
            resource_rejected = true;
        }
        require(resource_rejected, "resource construction without a real archive/runtime must fail instead of using zero offsets");

        auto joint_rejected = false;
        try {
            static_cast< void >(FixedPosition(&host, "HandR", TVec3f{}, TVec3f{}));
        } catch (const std::runtime_error&) {
            joint_rejected = true;
        }
        require(joint_rejected, "unavailable named-joint construction must fail instead of substituting the actor base matrix");
    }

    void test_supported_actor_relative_path_remains_real() {
        auto host = LiveActor("fixed-position-supported-host");
        host.mPosition.set(10.0F, 20.0F, 30.0F);
        host.calcAndSetBaseMtx();
        auto fixed = FixedPosition(&host, TVec3f{1.0F, 2.0F, 3.0F}, TVec3f{});
        fixed.calc();
        auto world = TVec3f{};
        fixed.copyTrans(&world);
        require(world.epsilonEquals(TVec3f{11.0F, 22.0F, 33.0F}, 0.00001F),
                "supported actor-relative FixedPosition must retain its actual transform behavior");
    }
}  // namespace

int main() {
    const auto tests = std::vector< std::pair< std::string_view, void (*)() > >{
        {"requested resource data", test_reads_requested_resource_data},
        {"requested joint name", test_preserves_requested_joint_name},
        {"missing resource absent", test_missing_resource_is_absent},
        {"unsupported construction explicit", test_unsupported_construction_fails_explicitly},
        {"supported actor-relative path", test_supported_actor_relative_path_remains_real},
    };

    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[ok] " << name << '\n';
        } catch (const std::exception& error) {
            std::cerr << "[fail] " << name << ": " << error.what() << '\n';
            return 1;
        }
    }

    std::cout << tests.size() << " FixedPosition real-or-absent tests passed\n";
    return 0;
}
