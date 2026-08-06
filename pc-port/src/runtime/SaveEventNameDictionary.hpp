#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace smgpc::runtime::save {

    // Host metadata carried alongside the retail-compatible FLG1/VLE1 hashed
    // records. Values remain authoritative in those original records; this
    // dictionary only makes their names reversible on the PC host.
    struct EventNameDictionary {
        std::vector<std::string> flag_names;
        std::vector<std::string> value_names;
    };

    constexpr auto EVENT_NAME_DICTIONARY_SIGNATURE = std::uint32_t{0x45564e4dU};  // EVNM

    [[nodiscard]] std::uint32_t event_name_dictionary_hash();

    [[nodiscard]] std::optional<std::vector<std::uint8_t>>
    encode_event_name_dictionary(const std::map<std::string, bool> &flags,
                                 const std::map<std::string, std::uint16_t> &values,
                                 std::size_t max_data_size = std::numeric_limits<std::size_t>::max());

    [[nodiscard]] std::optional<EventNameDictionary>
    decode_event_name_dictionary(std::span<const std::uint8_t> bytes);

}  // namespace smgpc::runtime::save
