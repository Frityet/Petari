#include "BcsvTable.hpp"

#include <algorithm>
#include <bit>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>

namespace smgpc::resource {
    namespace {

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 2U > data.size()) {
                throw std::runtime_error("BCSV read past end of buffer");
            }

            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("BCSV read past end of buffer");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | data[offset + 3U];
        }

        [[nodiscard]] float read_be_float(std::span<const std::uint8_t> data, std::size_t offset) {
            return std::bit_cast<float>(read_be32(data, offset));
        }

        [[nodiscard]] std::uint32_t masked_u32(std::span<const std::uint8_t> data, std::size_t offset, const BcsvField &field) {
            return (read_be32(data, offset) & field.mask) >> field.shift;
        }

        [[nodiscard]] std::int32_t sign_extend(std::uint32_t value, std::uint8_t bits) {
            if (bits == 0U || bits >= 32U) {
                return static_cast<std::int32_t>(value);
            }

            const auto sign_bit = 1U << (bits - 1U);
            if ((value & sign_bit) == 0U) {
                return static_cast<std::int32_t>(value);
            }

            const auto extend_mask = ~((1U << bits) - 1U);
            return static_cast<std::int32_t>(value | extend_mask);
        }

        [[nodiscard]] std::uint8_t masked_bit_width(std::uint32_t mask, std::uint8_t shift, std::uint8_t storage_bits) {
            const auto shifted_mask = mask >> shift;
            auto width = std::uint8_t {};
            for (auto bit = 0U; bit < storage_bits; ++bit) {
                if ((shifted_mask & (1U << bit)) != 0U) {
                    width = static_cast<std::uint8_t>(bit + 1U);
                }
            }

            return width == 0U ? storage_bits : width;
        }

    }  // namespace

    std::uint32_t jmap_hash(std::string_view text) {
        auto hash = std::uint32_t {};
        for (const auto character : text) {
            hash = static_cast<std::uint8_t>(character) + hash * 31U;
        }
        return hash;
    }

    std::string bcsv_field_type_name(BcsvFieldType type) {
        switch (type) {
        case BcsvFieldType::Int32:
            return "s32";
        case BcsvFieldType::InlineString:
            return "inline-string";
        case BcsvFieldType::Float:
            return "float";
        case BcsvFieldType::UInt32:
            return "u32";
        case BcsvFieldType::Int16:
            return "s16";
        case BcsvFieldType::Int8:
            return "s8";
        case BcsvFieldType::StringOffset:
            return "string-offset";
        default:
            return "unknown";
        }
    }

    BcsvTable BcsvTable::from_bytes(std::span<const std::uint8_t> data) {
        return BcsvTable(std::vector<std::uint8_t>(data.begin(), data.end()));
    }

    std::uint32_t BcsvTable::entry_count() const {
        return _entry_count;
    }

    std::uint32_t BcsvTable::entry_size() const {
        return _entry_size;
    }

    std::span<const BcsvField> BcsvTable::fields() const {
        return _fields;
    }

    std::optional<std::size_t> BcsvTable::field_index(std::uint32_t hash) const {
        const auto found = std::ranges::lower_bound(_field_indices_by_hash, hash, {}, [this](std::size_t index) {
            return _fields[index].hash;
        });
        if (found == _field_indices_by_hash.end() || _fields[*found].hash != hash) {
            return std::nullopt;
        }

        return *found;
    }

    std::optional<std::size_t> BcsvTable::field_index(std::string_view name) const {
        return field_index(jmap_hash(name));
    }

    std::optional<std::span<const std::uint8_t>> BcsvTable::raw_value(std::size_t entry_index, std::uint32_t hash) const {
        const auto index = field_index(hash);
        if (!index.has_value()) {
            return std::nullopt;
        }

        const auto &field = _fields[*index];
        const auto offset = value_offset(entry_index, field);
        auto width = std::size_t {4U};
        switch (field.type) {
        case BcsvFieldType::Int8:
            width = 1U;
            break;
        case BcsvFieldType::Int16:
            width = 2U;
            break;
        case BcsvFieldType::InlineString: {
            const auto entry_end = entry_offset(entry_index) + _entry_size;
            auto end = offset;
            while (end < entry_end && _data[end] != 0U) {
                ++end;
            }
            width = std::min(entry_end - offset, (end - offset) + (end < entry_end ? 1U : 0U));
            break;
        }
        default:
            width = 4U;
            break;
        }

        if (offset + width > _data.size()) {
            throw std::runtime_error("BCSV raw field value is outside table");
        }
        return std::span<const std::uint8_t>(_data).subspan(offset, width);
    }

    std::optional<std::span<const std::uint8_t>> BcsvTable::raw_value(std::size_t entry_index, std::string_view name) const {
        return raw_value(entry_index, jmap_hash(name));
    }

    std::optional<std::int32_t> BcsvTable::get_s32(std::size_t entry_index, std::uint32_t hash) const {
        const auto index = field_index(hash);
        if (!index.has_value()) {
            return std::nullopt;
        }

        const auto &field = _fields[*index];
        const auto offset = value_offset(entry_index, field);
        switch (field.type) {
        case BcsvFieldType::Int32:
            return sign_extend(masked_u32(_data, offset, field), masked_bit_width(field.mask, field.shift, 32U));
        case BcsvFieldType::UInt32:
            return static_cast<std::int32_t>(masked_u32(_data, offset, field));
        case BcsvFieldType::Int16:
            return sign_extend((read_be16(_data, offset) & field.mask) >> field.shift, masked_bit_width(field.mask, field.shift, 16U));
        case BcsvFieldType::Int8:
            if (offset >= _data.size()) {
                throw std::runtime_error("BCSV byte value is outside table");
            }
            return sign_extend((_data[offset] & field.mask) >> field.shift, masked_bit_width(field.mask, field.shift, 8U));
        default:
            return std::nullopt;
        }
    }

    std::optional<std::int32_t> BcsvTable::get_s32(std::size_t entry_index, std::string_view name) const {
        return get_s32(entry_index, jmap_hash(name));
    }

    std::optional<std::uint32_t> BcsvTable::get_u32(std::size_t entry_index, std::uint32_t hash) const {
        const auto index = field_index(hash);
        if (!index.has_value()) {
            return std::nullopt;
        }

        const auto &field = _fields[*index];
        const auto offset = value_offset(entry_index, field);
        switch (field.type) {
        case BcsvFieldType::Int32:
        case BcsvFieldType::UInt32:
            return masked_u32(_data, offset, field);
        case BcsvFieldType::Int16:
            return (read_be16(_data, offset) & field.mask) >> field.shift;
        case BcsvFieldType::Int8:
            if (offset >= _data.size()) {
                throw std::runtime_error("BCSV byte value is outside table");
            }
            return (_data[offset] & field.mask) >> field.shift;
        default:
            return std::nullopt;
        }
    }

    std::optional<std::uint32_t> BcsvTable::get_u32(std::size_t entry_index, std::string_view name) const {
        return get_u32(entry_index, jmap_hash(name));
    }

    std::optional<float> BcsvTable::get_float(std::size_t entry_index, std::uint32_t hash) const {
        const auto index = field_index(hash);
        if (!index.has_value()) {
            return std::nullopt;
        }

        const auto &field = _fields[*index];
        if (field.type != BcsvFieldType::Float) {
            return std::nullopt;
        }

        return read_be_float(_data, value_offset(entry_index, field));
    }

    std::optional<float> BcsvTable::get_float(std::size_t entry_index, std::string_view name) const {
        return get_float(entry_index, jmap_hash(name));
    }

    std::optional<std::string> BcsvTable::get_string(std::size_t entry_index, std::uint32_t hash) const {
        const auto index = field_index(hash);
        if (!index.has_value()) {
            return std::nullopt;
        }

        const auto &field = _fields[*index];
        const auto offset = value_offset(entry_index, field);
        switch (field.type) {
        case BcsvFieldType::InlineString:
            return read_c_string(offset);
        case BcsvFieldType::StringOffset:
            return read_c_string(_string_table_offset + read_be32(_data, offset));
        default:
            return std::nullopt;
        }
    }

    std::optional<std::string> BcsvTable::get_string(std::size_t entry_index, std::string_view name) const {
        return get_string(entry_index, jmap_hash(name));
    }

    std::optional<std::array<float, 3U>> BcsvTable::get_vec3(std::size_t entry_index, std::string_view base_name) const {
        const auto x = get_float(entry_index, std::string(base_name) + ".X");
        const auto y = get_float(entry_index, std::string(base_name) + ".Y");
        const auto z = get_float(entry_index, std::string(base_name) + ".Z");
        if (!x.has_value() || !y.has_value() || !z.has_value()) {
            return std::nullopt;
        }

        return std::array<float, 3U>{*x, *y, *z};
    }

    std::string BcsvTable::value_string(std::size_t entry_index, std::size_t field_index) const {
        if (field_index >= _fields.size()) {
            throw std::runtime_error("BCSV field index is outside table");
        }

        const auto &field = _fields[field_index];
        switch (field.type) {
        case BcsvFieldType::Int32:
        case BcsvFieldType::Int16:
        case BcsvFieldType::Int8:
            if (const auto value = get_s32(entry_index, field.hash); value.has_value()) {
                return std::to_string(*value);
            }
            break;
        case BcsvFieldType::UInt32:
            if (const auto value = get_u32(entry_index, field.hash); value.has_value()) {
                return std::to_string(*value);
            }
            break;
        case BcsvFieldType::Float:
            if (const auto value = get_float(entry_index, field.hash); value.has_value()) {
                return std::to_string(*value);
            }
            break;
        case BcsvFieldType::InlineString:
        case BcsvFieldType::StringOffset:
            if (const auto value = get_string(entry_index, field.hash); value.has_value()) {
                return *value;
            }
            break;
        default:
            break;
        }

        return {};
    }

    BcsvTable::BcsvTable(std::vector<std::uint8_t> data) : _data(std::move(data)) {
        parse();
    }

    void BcsvTable::parse() {
        const auto bytes = std::span<const std::uint8_t>(_data);
        if (bytes.size() < 0x10U) {
            throw std::runtime_error("BCSV data is too short");
        }

        _entry_count = read_be32(bytes, 0x00U);
        const auto field_count = read_be32(bytes, 0x04U);
        _data_offset = read_be32(bytes, 0x08U);
        _entry_size = read_be32(bytes, 0x0cU);

        constexpr auto fields_offset = std::size_t {0x10U};
        if (fields_offset + static_cast<std::size_t>(field_count) * 0x0cU > bytes.size()) {
            throw std::runtime_error("BCSV field table is truncated");
        }
        if (_data_offset + static_cast<std::size_t>(_entry_count) * _entry_size > bytes.size()) {
            throw std::runtime_error("BCSV entry data is truncated");
        }

        _fields.clear();
        _fields.reserve(field_count);
        _field_indices_by_hash.clear();
        for (auto i = 0U; i < field_count; ++i) {
            const auto offset = fields_offset + static_cast<std::size_t>(i) * 0x0cU;
            _fields.push_back(BcsvField {
                .hash = read_be32(bytes, offset),
                .mask = read_be32(bytes, offset + 0x04U),
                .offset = read_be16(bytes, offset + 0x08U),
                .shift = bytes[offset + 0x0aU],
                .type = static_cast<BcsvFieldType>(bytes[offset + 0x0bU]),
            });
        }
        _field_indices_by_hash.resize(_fields.size());
        std::iota(_field_indices_by_hash.begin(), _field_indices_by_hash.end(), 0U);
        std::ranges::sort(_field_indices_by_hash, [this](std::size_t lhs, std::size_t rhs) {
            if (_fields[lhs].hash == _fields[rhs].hash) {
                return lhs < rhs;
            }
            return _fields[lhs].hash < _fields[rhs].hash;
        });

        for (const auto &field : _fields) {
            const auto minimum_width = field.type == BcsvFieldType::InlineString || field.type == BcsvFieldType::Int8 ? 1U :
                                       field.type == BcsvFieldType::Int16                                             ? 2U :
                                                                                                                        4U;
            if (static_cast<std::uint32_t>(field.offset) + minimum_width > _entry_size) {
                throw std::runtime_error("BCSV field offset is outside entry");
            }
        }

        _string_table_offset = _data_offset + _entry_count * _entry_size;
    }

    std::size_t BcsvTable::entry_offset(std::size_t entry_index) const {
        if (entry_index >= _entry_count) {
            throw std::runtime_error("BCSV entry index is outside table");
        }

        return _data_offset + entry_index * _entry_size;
    }

    std::size_t BcsvTable::value_offset(std::size_t entry_index, const BcsvField &field) const {
        return entry_offset(entry_index) + field.offset;
    }

    std::string BcsvTable::read_c_string(std::size_t offset) const {
        if (offset >= _data.size()) {
            throw std::runtime_error("BCSV string offset is outside table");
        }

        auto end = offset;
        while (end < _data.size() && _data[end] != 0U) {
            ++end;
        }
        if (end == _data.size()) {
            throw std::runtime_error("BCSV string is not null terminated");
        }

        return std::string(reinterpret_cast<const char *>(_data.data() + offset), end - offset);
    }

}  // namespace smgpc::resource
