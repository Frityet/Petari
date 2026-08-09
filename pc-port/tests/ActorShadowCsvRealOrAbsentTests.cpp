#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/ActorShadowCsvCompat.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    class ProbeActor final : public LiveActor {
    public:
        ProbeActor() : LiveActor("ActorShadowCsvProbe") {
        }
    };

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, std::string_view message) {
        require(std::abs(actual - expected) < 0.0001F, message);
    }

    void write_be16(std::vector< std::uint8_t >& bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast< std::uint8_t >(value >> 8U);
        bytes[offset + 1U] = static_cast< std::uint8_t >(value);
    }

    void write_be32(std::vector< std::uint8_t >& bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast< std::uint8_t >(value >> 24U);
        bytes[offset + 1U] = static_cast< std::uint8_t >(value >> 16U);
        bytes[offset + 2U] = static_cast< std::uint8_t >(value >> 8U);
        bytes[offset + 3U] = static_cast< std::uint8_t >(value);
    }

    void write_be_float(std::vector< std::uint8_t >& bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast< std::uint32_t >(value));
    }

    void write_bcsv_field(std::vector< std::uint8_t >& bytes, std::size_t index, std::string_view name, std::uint16_t offset,
                          smgpc::resource::BcsvFieldType type) {
        const auto descriptor = 0x10U + index * 0x0cU;
        write_be32(bytes, descriptor, smgpc::resource::jmap_hash(name));
        write_be32(bytes, descriptor + 0x04U, 0xffffffffU);
        write_be16(bytes, descriptor + 0x08U, offset);
        bytes[descriptor + 0x0aU] = 0U;
        bytes[descriptor + 0x0bU] = static_cast< std::uint8_t >(type);
    }

    [[nodiscard]] std::string raw_bytes(std::initializer_list< std::uint8_t > values) {
        auto result = std::string{};
        for (const auto value : values) {
            result.push_back(static_cast< char >(value));
        }
        return result;
    }

    struct ShadowRow {
        std::string type;
        std::string name;
        std::string group_name;
        std::string joint;
        std::string model;
        std::string line_start;
        std::string line_end;
        float drop_length = 1000.0F;
        float drop_start = 0.0F;
        TVec3f drop_offset{};
        float radius = 100.0F;
        TVec3f size{100.0F, 100.0F, 100.0F};
        float volume_start = 100.0F;
        float volume_end = 100.0F;
        float line_start_radius = 100.0F;
        float line_end_radius = 100.0F;
        std::int32_t collision = 0;
        std::int32_t gravity = 0;
        std::int32_t follow_scale = 1;
        std::int32_t sync_show = 1;
        std::int32_t volume_cut = 0;
    };

    [[nodiscard]] std::vector< std::uint8_t > make_shadow_bcsv(std::span< const ShadowRow > rows) {
        constexpr auto field_count = 26U;
        constexpr auto entry_size = 104U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        auto string_table = std::vector< std::uint8_t >{};
        const auto add_string = [&string_table](std::string_view value) {
            const auto offset = static_cast< std::uint32_t >(string_table.size());
            string_table.insert(string_table.end(), value.begin(), value.end());
            string_table.push_back(0U);
            return offset;
        };
        auto strings = std::vector< std::array< std::uint32_t, 7U > >{};
        strings.reserve(rows.size());
        for (const auto& row : rows) {
            strings.push_back({add_string(row.name), add_string(row.group_name), add_string(row.joint), add_string(row.type), add_string(row.model),
                               add_string(row.line_start), add_string(row.line_end)});
        }

        auto bytes = std::vector< std::uint8_t >(data_offset + rows.size() * entry_size + string_table.size(), 0U);
        write_be32(bytes, 0x00U, static_cast< std::uint32_t >(rows.size()));
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        const auto string_field = smgpc::resource::BcsvFieldType::StringOffset;
        const auto float_field = smgpc::resource::BcsvFieldType::Float;
        const auto int_field = smgpc::resource::BcsvFieldType::Int32;
        write_bcsv_field(bytes, 0U, "Name", 0U, string_field);
        write_bcsv_field(bytes, 1U, "GroupName", 4U, string_field);
        write_bcsv_field(bytes, 2U, "Joint", 8U, string_field);
        write_bcsv_field(bytes, 3U, "Type", 12U, string_field);
        write_bcsv_field(bytes, 4U, "Model", 16U, string_field);
        write_bcsv_field(bytes, 5U, "LineStart", 20U, string_field);
        write_bcsv_field(bytes, 6U, "LineEnd", 24U, string_field);
        write_bcsv_field(bytes, 7U, "DropLength", 28U, float_field);
        write_bcsv_field(bytes, 8U, "DropStart", 32U, float_field);
        write_bcsv_field(bytes, 9U, "DropOffsetX", 36U, float_field);
        write_bcsv_field(bytes, 10U, "DropOffsetY", 40U, float_field);
        write_bcsv_field(bytes, 11U, "DropOffsetZ", 44U, float_field);
        write_bcsv_field(bytes, 12U, "Radius", 48U, float_field);
        write_bcsv_field(bytes, 13U, "SizeX", 52U, float_field);
        write_bcsv_field(bytes, 14U, "SizeY", 56U, float_field);
        write_bcsv_field(bytes, 15U, "SizeZ", 60U, float_field);
        write_bcsv_field(bytes, 16U, "VolumeStart", 64U, float_field);
        write_bcsv_field(bytes, 17U, "VolumeEnd", 68U, float_field);
        write_bcsv_field(bytes, 18U, "LineStartRadius", 72U, float_field);
        write_bcsv_field(bytes, 19U, "LineEndRadius", 76U, float_field);
        write_bcsv_field(bytes, 20U, "Collision", 80U, int_field);
        write_bcsv_field(bytes, 21U, "Gravity", 84U, int_field);
        write_bcsv_field(bytes, 22U, "FollowScale", 88U, int_field);
        write_bcsv_field(bytes, 23U, "SyncShow", 92U, int_field);
        write_bcsv_field(bytes, 24U, "VolumeCut", 96U, int_field);
        write_bcsv_field(bytes, 25U, "Unused", 100U, int_field);

        for (auto row_index = std::size_t{}; row_index < rows.size(); ++row_index) {
            const auto entry = data_offset + row_index * entry_size;
            for (auto string_index = std::size_t{}; string_index < 7U; ++string_index) {
                write_be32(bytes, entry + string_index * 4U, strings[row_index][string_index]);
            }
            const auto& row = rows[row_index];
            write_be_float(bytes, entry + 28U, row.drop_length);
            write_be_float(bytes, entry + 32U, row.drop_start);
            write_be_float(bytes, entry + 36U, row.drop_offset.x);
            write_be_float(bytes, entry + 40U, row.drop_offset.y);
            write_be_float(bytes, entry + 44U, row.drop_offset.z);
            write_be_float(bytes, entry + 48U, row.radius);
            write_be_float(bytes, entry + 52U, row.size.x);
            write_be_float(bytes, entry + 56U, row.size.y);
            write_be_float(bytes, entry + 60U, row.size.z);
            write_be_float(bytes, entry + 64U, row.volume_start);
            write_be_float(bytes, entry + 68U, row.volume_end);
            write_be_float(bytes, entry + 72U, row.line_start_radius);
            write_be_float(bytes, entry + 76U, row.line_end_radius);
            write_be32(bytes, entry + 80U, static_cast< std::uint32_t >(row.collision));
            write_be32(bytes, entry + 84U, static_cast< std::uint32_t >(row.gravity));
            write_be32(bytes, entry + 88U, static_cast< std::uint32_t >(row.follow_scale));
            write_be32(bytes, entry + 92U, static_cast< std::uint32_t >(row.sync_show));
            write_be32(bytes, entry + 96U, static_cast< std::uint32_t >(row.volume_cut));
        }
        std::copy(string_table.begin(), string_table.end(), bytes.begin() + data_offset + rows.size() * entry_size);
        return bytes;
    }

    [[nodiscard]] std::vector< std::uint8_t > make_type_only_bcsv(std::span< const std::string_view > types) {
        constexpr auto field_count = 1U;
        constexpr auto entry_size = 4U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        auto string_table = std::vector< std::uint8_t >{};
        auto offsets = std::vector< std::uint32_t >{};
        for (const auto type : types) {
            offsets.push_back(static_cast< std::uint32_t >(string_table.size()));
            string_table.insert(string_table.end(), type.begin(), type.end());
            string_table.push_back(0U);
        }
        auto bytes = std::vector< std::uint8_t >(data_offset + types.size() * entry_size + string_table.size(), 0U);
        write_be32(bytes, 0x00U, static_cast< std::uint32_t >(types.size()));
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "Type", 0U, smgpc::resource::BcsvFieldType::StringOffset);
        for (auto index = std::size_t{}; index < offsets.size(); ++index) {
            write_be32(bytes, data_offset + index * entry_size, offsets[index]);
        }
        std::copy(string_table.begin(), string_table.end(), bytes.begin() + data_offset + types.size() * entry_size);
        return bytes;
    }

    [[nodiscard]] std::vector< std::uint8_t > make_fieldless_bcsv(std::uint32_t entry_count) {
        auto bytes = std::vector< std::uint8_t >(0x10U, 0U);
        write_be32(bytes, 0x00U, entry_count);
        write_be32(bytes, 0x04U, 0U);
        write_be32(bytes, 0x08U, 0x10U);
        write_be32(bytes, 0x0cU, 0U);
        return bytes;
    }

    [[nodiscard]] smgpc::resource::RarcArchive make_single_file_rarc(std::string_view file_name, const std::vector< std::uint8_t >& file_data) {
        constexpr auto header_size = std::size_t{0x20U};
        constexpr auto info_offset = std::size_t{0x20U};
        constexpr auto directory_offset = std::size_t{0x40U};
        constexpr auto file_entry_offset = std::size_t{0x50U};
        constexpr auto string_table_offset = std::size_t{0x64U};
        constexpr auto file_data_offset = std::size_t{0x100U};
        require(string_table_offset + file_name.size() + 1U <= file_data_offset, "test RARC filename must fit before data");
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
        std::copy(file_name.begin(), file_name.end(), bytes.begin() + string_table_offset);
        std::copy(file_data.begin(), file_data.end(), bytes.begin() + file_data_offset);
        return smgpc::resource::RarcArchive::from_bytes(std::move(bytes));
    }

    [[nodiscard]] std::uint32_t rotate_right(std::uint32_t value, unsigned amount) {
        return std::rotr(value, static_cast< int >(amount));
    }

    [[nodiscard]] std::string sha256_hex(std::span< const std::uint8_t > source) {
        constexpr auto constants = std::array< std::uint32_t, 64U >{
            0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U,
            0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
            0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
            0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
            0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U, 0x1e376c08U,
            0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
            0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
        };
        auto hash = std::array< std::uint32_t, 8U >{
            0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU, 0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
        };
        auto padded = std::vector< std::uint8_t >(source.begin(), source.end());
        const auto bit_length = static_cast< std::uint64_t >(source.size()) * 8U;
        padded.push_back(0x80U);
        while ((padded.size() + 8U) % 64U != 0U) {
            padded.push_back(0U);
        }
        for (auto shift = 56; shift >= 0; shift -= 8) {
            padded.push_back(static_cast< std::uint8_t >(bit_length >> shift));
        }
        for (auto block = std::size_t{}; block < padded.size(); block += 64U) {
            auto schedule = std::array< std::uint32_t, 64U >{};
            for (auto index = std::size_t{}; index < 16U; ++index) {
                const auto offset = block + index * 4U;
                schedule[index] = static_cast< std::uint32_t >(padded[offset]) << 24U | static_cast< std::uint32_t >(padded[offset + 1U]) << 16U |
                                  static_cast< std::uint32_t >(padded[offset + 2U]) << 8U | static_cast< std::uint32_t >(padded[offset + 3U]);
            }
            for (auto index = std::size_t{16U}; index < schedule.size(); ++index) {
                const auto s0 = rotate_right(schedule[index - 15U], 7U) ^ rotate_right(schedule[index - 15U], 18U) ^ (schedule[index - 15U] >> 3U);
                const auto s1 = rotate_right(schedule[index - 2U], 17U) ^ rotate_right(schedule[index - 2U], 19U) ^ (schedule[index - 2U] >> 10U);
                schedule[index] = schedule[index - 16U] + s0 + schedule[index - 7U] + s1;
            }
            auto a = hash[0];
            auto b = hash[1];
            auto c = hash[2];
            auto d = hash[3];
            auto e = hash[4];
            auto f = hash[5];
            auto g = hash[6];
            auto h = hash[7];
            for (auto index = std::size_t{}; index < schedule.size(); ++index) {
                const auto sigma1 = rotate_right(e, 6U) ^ rotate_right(e, 11U) ^ rotate_right(e, 25U);
                const auto choose = (e & f) ^ (~e & g);
                const auto temp1 = h + sigma1 + choose + constants[index] + schedule[index];
                const auto sigma0 = rotate_right(a, 2U) ^ rotate_right(a, 13U) ^ rotate_right(a, 22U);
                const auto majority = (a & b) ^ (a & c) ^ (b & c);
                const auto temp2 = sigma0 + majority;
                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }
            hash[0] += a;
            hash[1] += b;
            hash[2] += c;
            hash[3] += d;
            hash[4] += e;
            hash[5] += f;
            hash[6] += g;
            hash[7] += h;
        }
        auto output = std::ostringstream{};
        output << std::hex << std::setfill('0');
        for (const auto word : hash) {
            output << std::setw(8) << word;
        }
        return output.str();
    }

    void test_missing_csv_and_strong_replacement() {
        auto actor = ProbeActor{};
        MR::initShadowVolumeSphere(&actor, 44.0F);
        const auto unrelated = make_single_file_rarc("Other.bcsv", make_type_only_bcsv(std::array< std::string_view, 1U >{"VolumeSphere"}));
        smgpc::compat::initialize_actor_shadow_from_archive(&actor, unrelated, "Shadow");
        auto* state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(state != nullptr && state->capacity == 1U && state->controllers.empty(), "missing CSV must install the retail one-slot empty list");

        MR::initShadowVolumeSphere(&actor, 44.0F);
        const auto malformed = make_single_file_rarc("Shadow.bcsv", std::vector< std::uint8_t >{0x00U, 0x01U});
        auto rejected = false;
        try {
            smgpc::compat::initialize_actor_shadow_from_archive(&actor, malformed, "Shadow");
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(rejected && state != nullptr && state->controllers.size() == 1U && state->controllers.front().radius == 44.0F,
                "malformed replacement must preserve the complete old state");

        const auto invalid_name_rows = std::array< ShadowRow, 1U >{ShadowRow{
            .type = "SurfaceCircle",
            .name = raw_bytes({0x81U}),
        }};
        const auto invalid_name = make_single_file_rarc("Shadow.bcsv", make_shadow_bcsv(invalid_name_rows));
        rejected = false;
        try {
            smgpc::compat::initialize_actor_shadow_from_archive(&actor, invalid_name, "Shadow");
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(rejected && state != nullptr && state->controllers.size() == 1U && state->controllers.front().radius == 44.0F,
                "recognized rows with invalid CP932 must preserve the complete old state");
    }

    void test_all_types_and_exact_missing_defaults() {
        constexpr auto type_names = std::array< std::string_view, 12U >{"SurfaceCircle",   "SurfaceOval",    "SurfaceBox",        "VolumeSphere",
                                                                        "VolumeOval",      "VolumeOvalPole", "VolumeCylinder",    "VolumeBox",
                                                                        "VolumeFlatModel", "VolumeLine",     "UnknownShadowType", ""};
        constexpr auto kinds = std::array{
            smgpc::compat::ActorShadowControllerKind::SurfaceCircle,   smgpc::compat::ActorShadowControllerKind::SurfaceOval,
            smgpc::compat::ActorShadowControllerKind::SurfaceBox,      smgpc::compat::ActorShadowControllerKind::VolumeSphere,
            smgpc::compat::ActorShadowControllerKind::VolumeOval,      smgpc::compat::ActorShadowControllerKind::VolumeOvalPole,
            smgpc::compat::ActorShadowControllerKind::VolumeCylinder,  smgpc::compat::ActorShadowControllerKind::VolumeBox,
            smgpc::compat::ActorShadowControllerKind::VolumeFlatModel, smgpc::compat::ActorShadowControllerKind::VolumeLine,
        };
        const auto archive = make_single_file_rarc("sHaDoW.BcSv", make_type_only_bcsv(type_names));
        auto actor = ProbeActor{};
        smgpc::compat::initialize_actor_shadow_from_archive(&actor, archive, "Shadow");
        const auto* state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(state != nullptr && state->capacity == 12U && state->controllers.size() == 10U,
                "row capacity must survive unknown and empty Type skips");
        for (auto index = std::size_t{}; index < kinds.size(); ++index) {
            require(state->controllers[index].kind == kinds[index], "the table-driven shadow type order must remain exact");
        }
        const auto& first = state->controllers.front();
        require(first.name.empty() && first.group_name.empty() && first.joint_name.empty() &&
                    first.position_binding == smgpc::compat::ActorShadowPositionBinding::ActorTranslation &&
                    first.drop_position == &actor.mPosition && first.drop_direction == &actor.mGravity,
                "missing strings and Joint must retain actor-relative defaults");
        require_near(first.drop_length, 1000.0F, "missing DropLength must default to 1000");
        require_near(first.drop_start_offset, 0.0F, "CSV setup must replace the ctor DropStart with zero");
        require(first.follow_host_scale && first.visible_sync_host && first.calculation_mode == smgpc::compat::ActorShadowCalculationMode::Disabled &&
                    first.gravity_mode == smgpc::compat::ActorShadowGravityMode::HostDirection,
                "missing flags must use exact CSV defaults");
        const auto& line = state->controllers[9U];
        require(!line.line_start_controller_index.has_value() && !line.line_end_controller_index.has_value(),
                "host safety must keep missing multi-controller line names unresolved instead of reproducing retail's null strcmp fault");
        for (const auto index : {0U, 3U, 6U}) {
            require_near(state->controllers[index].radius, 100.0F, "missing radius must default to 100");
        }
        for (const auto index : {1U, 2U, 4U, 5U, 7U}) {
            require(state->controllers[index].size.epsilonEquals(TVec3f{100.0F, 100.0F, 100.0F}, 0.0F), "missing size must default to (100,100,100)");
        }
        for (auto index = std::size_t{3U}; index <= 9U; ++index) {
            require_near(state->controllers[index].volume_start_offset, 100.0F, "missing volume start must default to 100");
            require_near(state->controllers[index].volume_end_offset, 100.0F, "missing volume end must default to 100");
            require(!state->controllers[index].volume_cut_drop_length, "missing volume cut must default off");
        }
        require(!state->controllers[8U].model_name.has_value(), "missing FlatModel name must remain null");
        require_near(line.line_start_radius, 100.0F, "missing line start width must default to 100");
        require_near(line.line_end_radius, 100.0F, "missing line end width must default to 100");

        const auto empty_archive = make_single_file_rarc("Shadow.bcsv", make_fieldless_bcsv(0U));
        smgpc::compat::initialize_actor_shadow_from_archive(&actor, empty_archive, "Shadow");
        state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(state != nullptr && state->capacity == 0U && state->controllers.empty(), "a present empty table must retain zero capacity");
        const auto missing_type_archive = make_single_file_rarc("Shadow.bcsv", make_fieldless_bcsv(1U));
        smgpc::compat::initialize_actor_shadow_from_archive(&actor, missing_type_archive, "Shadow");
        state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(state != nullptr && state->capacity == 1U && state->controllers.empty(),
                "an absent Type row must count toward capacity and skip construction");
    }

    void test_authored_bindings_modes_and_line_order() {
        auto rows = std::vector< ShadowRow >{};
        rows.push_back(ShadowRow{
            .type = "SurfaceCircle",
            .name = raw_bytes({0x91U, 0xccU}),
            .group_name = raw_bytes({0x83U, 0x60U, 0x83U, 0x52U}),
            .joint = "::ACTOR_TRANS",
            .model = raw_bytes({0x81U}),
            .radius = 12.0F,
        });
        rows.push_back(ShadowRow{
            .type = "VolumeOval",
            .name = "base",
            .joint = "::BASE_MATRIX",
            .drop_length = 222.0F,
            .drop_start = 7.0F,
            .drop_offset = TVec3f{1.0F, 2.0F, 3.0F},
            .size = TVec3f{4.0F, 5.0F, 6.0F},
            .volume_start = 8.0F,
            .volume_end = 9.0F,
            .collision = 1,
            .gravity = 1,
            .follow_scale = 0,
            .sync_show = 0,
            .volume_cut = 1,
        });
        rows.push_back(ShadowRow{
            .type = "VolumeBox",
            .name = "fixed",
            .joint = "::FIX_POSITION",
            .collision = 2,
            .gravity = 2,
        });
        rows.push_back(ShadowRow{
            .type = "VolumeFlatModel",
            .name = "otherTrans",
            .joint = "::OTHER_TRANS",
            .model = "FlatFixture",
            .collision = 99,
            .gravity = 3,
        });
        rows.push_back(ShadowRow{
            .type = "VolumeCylinder",
            .name = "otherMtx",
            .joint = "::OTHER_MATRIX",
            .radius = 14.0F,
            .gravity = 4,
        });
        rows.push_back(ShadowRow{
            .type = "VolumeSphere",
            .name = "joint",
            .joint = "Body",
            .radius = 15.0F,
            .gravity = 5,
        });
        rows.push_back(ShadowRow{
            .type = "VolumeLine",
            .name = "line",
            .line_start = "base",
            .line_end = "forward",
            .line_start_radius = 16.0F,
            .line_end_radius = 17.0F,
        });
        rows.push_back(ShadowRow{
            .type = "VolumeSphere",
            .name = "forward",
            .gravity = 99,
        });
        rows.push_back(ShadowRow{
            .type = "SurfaceCircle",
            .name = raw_bytes({0x87U, 0x54U}),
        });
        rows.push_back(ShadowRow{
            .type = "VolumeLine",
            .name = "rawLine",
            .line_start = raw_bytes({0xfaU, 0x4aU}),
            .line_end = raw_bytes({0x87U, 0x54U}),
        });
        rows.push_back(ShadowRow{
            .type = "Unknown",
            .name = raw_bytes({0x81U}),
        });
        const auto archive = make_single_file_rarc("Shadow.bcsv", make_shadow_bcsv(rows));
        auto actor = ProbeActor{};
        actor.mPosition.set(20.0F, 30.0F, 40.0F);
        smgpc::compat::initialize_actor_model(&actor, "FixtureModel", "");
        smgpc::compat::initialize_actor_shadow_from_archive(&actor, archive, "Shadow");
        const auto* state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(state != nullptr && state->capacity == 11U && state->controllers.size() == 10U,
                "unknown row must skip before malformed irrelevant strings decode");
        require(state->controllers[0U].name == "体" && state->controllers[0U].group_name == "チコ" && state->controllers[0U].radius == 12.0F &&
                    state->controllers[0U].drop_direction == &actor.mGravity &&
                    state->controllers[0U].fixed_drop_direction.epsilonEquals(TVec3f{0.0F, -1.0F, 0.0F}, 0.0F),
                "authored CP932 strings and surface radius must be owned as UTF-8");
        const auto& base = state->controllers[1U];
        require(base.position_binding == smgpc::compat::ActorShadowPositionBinding::BaseMatrix && base.drop_position_matrix == actor.getBaseMtx() &&
                    base.drop_offset.epsilonEquals(TVec3f{1.0F, 2.0F, 3.0F}, 0.0F) && base.size.epsilonEquals(TVec3f{4.0F, 5.0F, 6.0F}, 0.0F),
                "base-matrix binding must retain authored offset and oval size");
        require(base.drop_direction == nullptr && base.fixed_drop_direction.epsilonEquals(TVec3f{0.0F, 1.0F, 0.0F}, 0.0F) &&
                    base.calculation_mode == smgpc::compat::ActorShadowCalculationMode::Continuous &&
                    base.gravity_mode == smgpc::compat::ActorShadowGravityMode::HostContinuous && !base.follow_host_scale &&
                    !base.visible_sync_host && base.volume_cut_drop_length,
                "authored collision/gravity/visibility/volume flags must remain exact");
        const auto& fixed = state->controllers[2U];
        require(fixed.position_binding == smgpc::compat::ActorShadowPositionBinding::FixedPosition &&
                    fixed.fixed_drop_position.epsilonEquals(actor.mPosition, 0.0F) &&
                    fixed.calculation_mode == smgpc::compat::ActorShadowCalculationMode::OneTime &&
                    fixed.gravity_mode == smgpc::compat::ActorShadowGravityMode::HostOneTime && fixed.drop_direction == nullptr &&
                    fixed.fixed_drop_direction.epsilonEquals(TVec3f{0.0F, 1.0F, 0.0F}, 0.0F),
                "fixed binding and one-time modes must be represented exactly");
        const auto& other_trans = state->controllers[3U];
        require(other_trans.position_binding == smgpc::compat::ActorShadowPositionBinding::OtherTranslation &&
                    other_trans.drop_position == &actor.mPosition && other_trans.drop_direction == &actor.mGravity &&
                    other_trans.calculation_mode == smgpc::compat::ActorShadowCalculationMode::Continuous &&
                    other_trans.gravity_mode == smgpc::compat::ActorShadowGravityMode::PrivateDisabled &&
                    other_trans.fixed_drop_direction.epsilonEquals(TVec3f{0.0F, -1.0F, 0.0F}, 0.0F) &&
                    other_trans.model_name == std::optional< std::string >{"FlatFixture"},
                "invalid collision and private-disabled gravity must retain ctor pointers");
        require(state->controllers[4U].position_binding == smgpc::compat::ActorShadowPositionBinding::OtherMatrix &&
                    state->controllers[4U].drop_position_matrix == actor.getBaseMtx() &&
                    state->controllers[4U].gravity_mode == smgpc::compat::ActorShadowGravityMode::PrivateContinuous &&
                    state->controllers[4U].drop_direction == nullptr &&
                    state->controllers[4U].fixed_drop_direction.epsilonEquals(TVec3f{0.0F, 1.0F, 0.0F}, 0.0F),
                "other-matrix binding and private continuous gravity must be exact");
        require(state->controllers[5U].position_binding == smgpc::compat::ActorShadowPositionBinding::JointMatrix &&
                    state->controllers[5U].joint_name == "Body" &&
                    state->controllers[5U].gravity_mode == smgpc::compat::ActorShadowGravityMode::PrivateOneTime &&
                    state->controllers[5U].drop_direction == nullptr &&
                    state->controllers[5U].fixed_drop_direction.epsilonEquals(TVec3f{0.0F, 1.0F, 0.0F}, 0.0F),
                "named joint binding and private one-time gravity must be retained");
        const auto& line = state->controllers[6U];
        require(line.line_start_controller_index == std::optional< std::size_t >{1U} && !line.line_end_controller_index.has_value() &&
                    line.line_start_radius == 16.0F && line.line_end_radius == 17.0F,
                "VolumeLine must resolve only controllers visible in row order");
        const auto& invalid_gravity = state->controllers[7U];
        require(invalid_gravity.gravity_mode == smgpc::compat::ActorShadowGravityMode::HostDirection &&
                    invalid_gravity.drop_direction == &actor.mGravity &&
                    invalid_gravity.fixed_drop_direction.epsilonEquals(TVec3f{0.0F, -1.0F, 0.0F}, 0.0F),
                "invalid gravity must retain the ctor host pointer and fixed -Y vector");
        const auto& duplicate_cp932_name = state->controllers[8U];
        const auto& raw_line = state->controllers[9U];
        require(duplicate_cp932_name.name == *raw_line.line_start_name && duplicate_cp932_name.name == *raw_line.line_end_name &&
                    !raw_line.line_start_controller_index.has_value() && raw_line.line_end_controller_index == std::optional< std::size_t >{8U},
                "line endpoints must compare raw CP932 bytes even when distinct encodings decode to the same UTF-8 name");
        require(state->calculation_enabled && state->private_gravity, "aggregate observability must follow active collision/private gravity");
    }

    void test_single_line_self_resolution() {
        const auto rows = std::array< ShadowRow, 1U >{ShadowRow{
            .type = "VolumeLine",
            .line_start = "missing-a",
            .line_end = "missing-b",
        }};
        const auto archive = make_single_file_rarc("Shadow.bcsv", make_shadow_bcsv(rows));
        auto actor = ProbeActor{};
        smgpc::compat::initialize_actor_shadow_from_archive(&actor, archive, "Shadow");
        const auto& line = smgpc::compat::actor_shadow_runtime_state(&actor)->controllers.front();
        require(line.line_start_controller_index == std::optional< std::size_t >{0U} &&
                    line.line_end_controller_index == std::optional< std::size_t >{0U},
                "single-controller retail lookup must resolve both line ends to self");
    }

    void test_generic_ctor_and_model_binding_lifetime() {
        auto actor = ProbeActor{};
        MR::initShadowVolumeSphere(&actor, 20.0F);
        auto* state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(state->calculation_enabled && state->controllers.front().calculation_mode == smgpc::compat::ActorShadowCalculationMode::Continuous &&
                    state->controllers.front().drop_start_offset == 50.0F &&
                    state->controllers.front().fixed_drop_direction.epsilonEquals(TVec3f{0.0F, -1.0F, 0.0F}, 0.0F),
                "generic ShadowController construction must use retail ctor defaults");
        MR::initShadowVolumeCylinder(&actor, 20.0F);
        state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(!state->calculation_enabled && state->controllers.front().calculation_mode == smgpc::compat::ActorShadowCalculationMode::Disabled,
                "generic cylinder convenience must apply its retail collision override");

        auto candidate = smgpc::compat::ActorShadowRuntimeState{
            .valid = true,
            .calculation_enabled = true,
            .private_gravity = false,
            .capacity = 3U,
            .controllers = {},
        };
        candidate.controllers.reserve(3U);
        Mtx joint_matrix{};
        Mtx base_matrix{};
        Mtx other_matrix{};
        for (const auto binding : {smgpc::compat::ActorShadowPositionBinding::JointMatrix, smgpc::compat::ActorShadowPositionBinding::BaseMatrix,
                                   smgpc::compat::ActorShadowPositionBinding::OtherMatrix}) {
            auto controller = smgpc::compat::make_actor_shadow_controller_runtime_state(&actor, "binding",
                                                                                        smgpc::compat::ActorShadowControllerKind::VolumeSphere, 1.0F);
            controller.position_binding = binding;
            controller.drop_position_matrix = binding == smgpc::compat::ActorShadowPositionBinding::JointMatrix ? joint_matrix :
                                              binding == smgpc::compat::ActorShadowPositionBinding::BaseMatrix  ? base_matrix :
                                                                                                                  other_matrix;
            candidate.controllers.push_back(std::move(controller));
        }
        smgpc::compat::replace_actor_shadow_runtime_state(&actor, std::move(candidate));
        smgpc::compat::initialize_actor_model(&actor, "First", "");
        state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(state->controllers[0U].drop_position_matrix == nullptr && state->controllers[1U].drop_position_matrix == base_matrix &&
                    state->controllers[2U].drop_position_matrix == other_matrix,
                "model replacement must invalidate only model-owned joint matrices");
        state->controllers[0U].drop_position_matrix = joint_matrix;
        smgpc::compat::release_actor_model_state(&actor);
        require(state->controllers[0U].drop_position_matrix == nullptr && state->controllers[1U].drop_position_matrix == base_matrix &&
                    state->controllers[2U].drop_position_matrix == other_matrix,
                "model release must not leave a dangling named-joint binding");

        auto invalid = *state;
        invalid.controllers[0U].line_start_controller_index = 99U;
        auto rejected = false;
        try {
            smgpc::compat::replace_actor_shadow_runtime_state(&actor, std::move(invalid));
        } catch (const std::out_of_range&) {
            rejected = true;
        }
        require(rejected && state->controllers.size() == 3U, "invalid generic line indices must reject before replacing old state");
    }

    [[nodiscard]] std::optional< std::filesystem::path > find_rmgk02_object(std::string_view archive_name) {
        const auto relative = std::filesystem::path("ObjectData") / (std::string(archive_name) + ".arc");
        for (const auto& root : {std::filesystem::path("../orig/RMGK02/files"), std::filesystem::path("orig/RMGK02/files"),
                                 std::filesystem::path("container/orig/RMGK02/files")}) {
            const auto path = root / relative;
            if (std::filesystem::is_regular_file(path)) {
                return path;
            }
        }
        return std::nullopt;
    }

    void test_real_rmgk02_tico_shadow() {
        const auto tico_path = find_rmgk02_object("Tico");
        const auto baby_path = find_rmgk02_object("TicoBaby");
        if (!tico_path.has_value() || !baby_path.has_value()) {
            std::cout << "[skip] RMGK02 Tico/TicoBaby archives are absent\n";
            return;
        }
        const auto tico = smgpc::resource::RarcArchive::from_file(*tico_path);
        const auto baby = smgpc::resource::RarcArchive::from_file(*baby_path);
        const auto* tico_entry = tico.find_resource("shadow.bcsv");
        const auto* baby_entry = baby.find_resource("shadow.bcsv");
        require(tico_entry != nullptr && baby_entry != nullptr, "both retail Tico archives must contain shadow.bcsv");
        const auto tico_data = tico.file_data(*tico_entry);
        const auto baby_data = baby.file_data(*baby_entry);
        constexpr auto expected_sha = "1547591fb4d1941ac96bdebe0b4a25e2909efcf3f5bc2c801c0b579925050748";
        require(std::ranges::equal(tico_data, baby_data) && sha256_hex(tico_data) == expected_sha,
                "Tico and TicoBaby shadow.bcsv must be byte-identical retail data");

        auto actor = ProbeActor{};
        smgpc::compat::initialize_actor_shadow_from_archive(&actor, tico, "Shadow");
        const auto* state = smgpc::compat::actor_shadow_runtime_state(&actor);
        require(state != nullptr && state->capacity == 1U && state->controllers.size() == 1U, "retail Tico shadow must produce one controller");
        const auto& controller = state->controllers.front();
        require(controller.name == "体" && controller.joint_name == "Body" &&
                    controller.kind == smgpc::compat::ActorShadowControllerKind::VolumeSphere &&
                    controller.position_binding == smgpc::compat::ActorShadowPositionBinding::JointMatrix,
                "retail Tico strings/type/joint binding must decode exactly");
        require_near(controller.radius, 40.0F, "retail Tico radius must remain 40");
        require_near(controller.drop_start_offset, 50.0F, "retail Tico DropStart must remain 50");
        require_near(controller.drop_length, 1000.0F, "retail Tico DropLength must remain 1000");
        require(controller.calculation_mode == smgpc::compat::ActorShadowCalculationMode::Continuous &&
                    controller.gravity_mode == smgpc::compat::ActorShadowGravityMode::HostDirection && controller.drop_direction == &actor.mGravity &&
                    controller.follow_host_scale && controller.visible_sync_host,
                "retail Tico collision/gravity/follow/sync flags must remain exact");
    }
}  // namespace

int main() try {
    const auto tests = std::array< std::pair< std::string_view, void (*)() >, 7U >{
        std::pair{"missing CSV and transaction", test_missing_csv_and_strong_replacement},
        std::pair{"all types and defaults", test_all_types_and_exact_missing_defaults},
        std::pair{"authored bindings and modes", test_authored_bindings_modes_and_line_order},
        std::pair{"single line self lookup", test_single_line_self_resolution},
        std::pair{"model binding lifetime", test_generic_ctor_and_model_binding_lifetime},
        std::pair{"real RMGK02 Tico", test_real_rmgk02_tico_shadow},
        std::pair{"registry teardown",
                  [] {
                      const auto baseline = smgpc::compat::actor_shadow_runtime_state_count();
                      {
                          auto actor = ProbeActor{};
                          MR::initShadowVolumeSphere(&actor, 1.0F);
                      }
                      require(smgpc::compat::actor_shadow_runtime_state_count() == baseline, "actor teardown must release shadow CSV state");
                  }},
    };
    auto passed = std::size_t{};
    for (const auto& [name, test] : tests) {
        test();
        ++passed;
        std::cout << "[ok] " << name << '\n';
    }
    std::cout << "Actor shadow CSV real-or-absent tests passed: " << passed << "/" << tests.size() << '\n';
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Actor shadow CSV test failed: " << error.what() << '\n';
    return 1;
}
