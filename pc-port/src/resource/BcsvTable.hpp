#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::resource {

    enum class BcsvFieldType : std::uint8_t {
        Int32 = 0U,
        InlineString = 1U,
        Float = 2U,
        UInt32 = 3U,
        Int16 = 4U,
        Int8 = 5U,
        StringOffset = 6U,
    };

    struct BcsvField {
        std::uint32_t hash = 0U;
        std::uint32_t mask = 0xffffffffU;
        std::uint16_t offset = 0U;
        std::uint8_t shift = 0U;
        BcsvFieldType type = BcsvFieldType::Int32;
    };

    class BcsvTable final {
    public:
        static BcsvTable from_bytes(std::span<const std::uint8_t> data);

        [[nodiscard]] std::uint32_t entry_count() const;
        [[nodiscard]] std::uint32_t entry_size() const;
        [[nodiscard]] std::span<const BcsvField> fields() const;

        [[nodiscard]] std::optional<std::size_t> field_index(std::uint32_t hash) const;
        [[nodiscard]] std::optional<std::size_t> field_index(std::string_view name) const;
        [[nodiscard]] std::optional<std::span<const std::uint8_t>> raw_value(std::size_t entry_index, std::uint32_t hash) const;
        [[nodiscard]] std::optional<std::span<const std::uint8_t>> raw_value(std::size_t entry_index, std::string_view name) const;

        [[nodiscard]] std::optional<std::int32_t> get_s32(std::size_t entry_index, std::uint32_t hash) const;
        [[nodiscard]] std::optional<std::int32_t> get_s32(std::size_t entry_index, std::string_view name) const;
        [[nodiscard]] std::optional<std::uint32_t> get_u32(std::size_t entry_index, std::uint32_t hash) const;
        [[nodiscard]] std::optional<std::uint32_t> get_u32(std::size_t entry_index, std::string_view name) const;
        [[nodiscard]] std::optional<float> get_float(std::size_t entry_index, std::uint32_t hash) const;
        [[nodiscard]] std::optional<float> get_float(std::size_t entry_index, std::string_view name) const;
        [[nodiscard]] std::optional<std::string> get_string(std::size_t entry_index, std::uint32_t hash) const;
        [[nodiscard]] std::optional<std::string> get_string(std::size_t entry_index, std::string_view name) const;
        [[nodiscard]] std::optional<std::array<float, 3U>> get_vec3(std::size_t entry_index, std::string_view base_name) const;

        [[nodiscard]] std::string value_string(std::size_t entry_index, std::size_t field_index) const;

    private:
        explicit BcsvTable(std::vector<std::uint8_t> data);

        void parse();
        [[nodiscard]] std::size_t entry_offset(std::size_t entry_index) const;
        [[nodiscard]] std::size_t value_offset(std::size_t entry_index, const BcsvField &field) const;
        [[nodiscard]] std::string read_c_string(std::size_t offset) const;

        std::vector<std::uint8_t> _data;
        std::vector<BcsvField> _fields;
        std::vector<std::size_t> _field_indices_by_hash;
        std::uint32_t _entry_count = 0U;
        std::uint32_t _data_offset = 0U;
        std::uint32_t _entry_size = 0U;
        std::uint32_t _string_table_offset = 0U;
    };

    [[nodiscard]] std::uint32_t jmap_hash(std::string_view text);
    [[nodiscard]] std::string bcsv_field_type_name(BcsvFieldType type);

}  // namespace smgpc::resource
