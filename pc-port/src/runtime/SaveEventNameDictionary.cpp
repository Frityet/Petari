#include "SaveEventNameDictionary.hpp"

#include <algorithm>
#include <limits>
#include <string_view>

#include "common/BinaryChunkFile.hpp"

namespace smgpc::runtime::save {
    namespace {

        constexpr auto EVENT_NAME_DICTIONARY_VERSION = std::uint8_t{1U};
        constexpr auto EVENT_NAME_DICTIONARY_HEADER_SIZE = std::size_t{6U};

        [[nodiscard]] bool is_valid_event_name(std::string_view name) {
            return !name.empty() && name.size() <= std::numeric_limits<std::uint16_t>::max() &&
                   std::ranges::find(name, '\0') == name.end();
        }

        template <typename Value>
        [[nodiscard]] bool append_names(std::vector<std::uint8_t> &bytes,
                                        const std::map<std::string, Value> &entries,
                                        std::size_t max_data_size) {
            for (const auto &[name, value] : entries) {
                static_cast<void>(value);
                if (!is_valid_event_name(name)) {
                    return false;
                }

                const auto record_size = sizeof(std::uint16_t) + name.size();
                if (bytes.size() > max_data_size || record_size > max_data_size - bytes.size()) {
                    return false;
                }

                common::append_be16(bytes, static_cast<std::uint16_t>(name.size()));
                bytes.insert(bytes.end(), name.begin(), name.end());
            }
            return true;
        }

        [[nodiscard]] bool decode_names(std::span<const std::uint8_t> bytes,
                                        std::size_t &offset,
                                        std::uint16_t count,
                                        std::vector<std::string> &names) {
            names.reserve(count);
            for (auto index = std::uint16_t{}; index < count; ++index) {
                if (offset + sizeof(std::uint16_t) > bytes.size()) {
                    return false;
                }

                const auto length = static_cast<std::size_t>(common::read_be16(bytes, offset));
                offset += sizeof(std::uint16_t);
                if (length == 0U || length > bytes.size() - offset) {
                    return false;
                }

                const auto *data = reinterpret_cast<const char *>(bytes.data() + offset);
                auto name = std::string(data, length);
                if (!is_valid_event_name(name)) {
                    return false;
                }
                names.push_back(std::move(name));
                offset += length;
            }
            return true;
        }

    }  // namespace

    std::uint32_t event_name_dictionary_hash() {
        return common::hash_code_31("smgpc/event-name-dictionary/v1");
    }

    std::optional<std::vector<std::uint8_t>>
    encode_event_name_dictionary(const std::map<std::string, bool> &flags,
                                 const std::map<std::string, std::uint16_t> &values,
                                 std::size_t max_data_size) {
        if (flags.size() > std::numeric_limits<std::uint16_t>::max() ||
            values.size() > std::numeric_limits<std::uint16_t>::max() ||
            max_data_size < EVENT_NAME_DICTIONARY_HEADER_SIZE) {
            return std::nullopt;
        }

        auto bytes = std::vector<std::uint8_t>{
            EVENT_NAME_DICTIONARY_VERSION,
            0U,
        };
        common::append_be16(bytes, static_cast<std::uint16_t>(flags.size()));
        common::append_be16(bytes, static_cast<std::uint16_t>(values.size()));
        if (!append_names(bytes, flags, max_data_size) || !append_names(bytes, values, max_data_size)) {
            return std::nullopt;
        }
        return bytes;
    }

    std::optional<EventNameDictionary>
    decode_event_name_dictionary(std::span<const std::uint8_t> bytes) {
        if (bytes.size() < EVENT_NAME_DICTIONARY_HEADER_SIZE ||
            bytes[0U] != EVENT_NAME_DICTIONARY_VERSION || bytes[1U] != 0U) {
            return std::nullopt;
        }

        const auto flag_count = common::read_be16(bytes, 2U);
        const auto value_count = common::read_be16(bytes, 4U);
        auto dictionary = EventNameDictionary{};
        auto offset = EVENT_NAME_DICTIONARY_HEADER_SIZE;
        if (!decode_names(bytes, offset, flag_count, dictionary.flag_names) ||
            !decode_names(bytes, offset, value_count, dictionary.value_names) ||
            offset != bytes.size()) {
            return std::nullopt;
        }
        return dictionary;
    }

}  // namespace smgpc::runtime::save
