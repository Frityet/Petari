#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::game {

    class RarcArchive;

    struct BmgFormatArg {
        enum class Type {
            Number,
            String,
        };

        [[nodiscard]] static BmgFormatArg number(std::int32_t value);
        [[nodiscard]] static BmgFormatArg string(std::u16string_view value);

        Type type = Type::Number;
        std::int32_t number_value = 0;
        std::u16string string_value;
    };

    struct BmgControlTag {
        std::size_t raw_offset = 0U;
        std::uint16_t size_bytes = 0U;
        std::uint16_t type = 0U;
        std::vector<std::uint16_t> payload_words = {};
    };

    struct BmgMessageInfo {
        std::uint32_t text_offset = 0U;
        std::uint16_t camera_set_id = 0U;
        std::uint8_t unknown_06 = 0U;
        std::uint8_t camera_type = 0U;
        std::uint8_t talk_type = 0U;
        std::uint8_t balloon_type = 0U;
        std::uint8_t unknown_0a = 0U;
        std::uint8_t unknown_0b = 0U;
    };

    struct BmgMessage {
        std::string id;
        BmgMessageInfo info{};
        std::u16string raw_text;
        std::u16string display_text;
        std::vector<BmgControlTag> control_tags;
    };

    [[nodiscard]] std::vector<BmgControlTag> bmg_control_tags(std::u16string_view raw_text);
    [[nodiscard]] std::u16string format_bmg_text(std::u16string_view raw_text, std::span<const BmgFormatArg> args);

    class BmgMessageArchive final {
    public:
        [[nodiscard]] static BmgMessageArchive from_message_archive(const RarcArchive &archive);
        [[nodiscard]] static BmgMessageArchive from_bytes(std::span<const std::uint8_t> bmg_data,
                                                          std::span<const std::uint8_t> message_id_table);

        [[nodiscard]] std::span<const BmgMessage> messages() const;
        [[nodiscard]] std::size_t message_count() const;
        [[nodiscard]] const BmgMessage *find(std::string_view message_id) const;

    private:
        explicit BmgMessageArchive(std::vector<BmgMessage> messages);

        std::vector<BmgMessage> _messages;
        std::map<std::string, std::size_t, std::less<>> _index_by_id;
    };

}  // namespace smgpc::game
