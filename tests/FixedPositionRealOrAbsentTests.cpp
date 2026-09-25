#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/FixedPosition.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "resource/BcsvTable.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
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

    void test_reads_requested_resource_data() {
        const auto translation = TVec3f{11.0F, -22.5F, 33.25F};
        const auto rotation = TVec3f{-45.0F, 90.0F, 12.5F};
        JMapInfo csv(smgpc::resource::BcsvTable::from_bytes(make_fixed_position_bcsv("", translation, rotation)));
        const char* joint = "initial";
        TVec3f trans(0.0F, 0.0F, 0.0F);
        TVec3f rotate(0.0F, 0.0F, 0.0F);
        MR::getCsvDataStrOrNULL(&joint, &csv, "JointName", 0);
        MR::getCsvDataVec(&trans, &csv, "Trans", 0);
        MR::getCsvDataVec(&rotate, &csv, "Rotate", 0);
        require(joint == nullptr, "the original CSV reader uses an empty JointName to select the actor base matrix");
        require(trans.epsilonEquals(translation, 0.0F) && rotate.epsilonEquals(rotation, 0.0F),
                "the original CSV readers must preserve requested Trans/Rotate offsets");
    }

    void test_preserves_requested_joint_name() {
        JMapInfo csv(smgpc::resource::BcsvTable::from_bytes(
            make_fixed_position_bcsv("HandR", TVec3f{1.0F, 2.0F, 3.0F}, TVec3f{4.0F, 5.0F, 6.0F})));
        const char* joint = nullptr;
        MR::getCsvDataStrOrNULL(&joint, &csv, "JointName", 0);
        require(joint != nullptr && std::string_view(joint) == "HandR", "the original CSV reader preserves the requested joint");
    }

    void test_rotation_from_supplied_matrix() {
        FixedPosition fixed(static_cast<MtxPtr>(nullptr), TVec3f(0, 0, 0), TVec3f(0, 0, 45));
        fixed.calc();
        TVec3f rotation;
        fixed.copyRotate(&rotation);
        require(rotation.epsilonEquals(TVec3f(0, 0, 45), 0.001F),
                "FixedPosition must extract degree rotation from its actual calculated matrix");
    }

    class MatrixActor final : public LiveActor {
    public:
        MatrixActor() : LiveActor("fixed-position-matrix-host") { mMatrix.identity(); }
        MtxPtr getBaseMtx() const override { return const_cast<MtxPtr>(mMatrix.toMtxPtr()); }
        TPos3f mMatrix;
    };

    void test_supported_actor_relative_path_remains_real() {
        auto host = MatrixActor();
        host.mMatrix.setTrans(TVec3f(10.0F, 20.0F, 30.0F));
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
        {"supplied matrix rotation", test_rotation_from_supplied_matrix},
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
