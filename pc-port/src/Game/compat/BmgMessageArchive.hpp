#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::game {

    class RarcArchive;

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
    };

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
