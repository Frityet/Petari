#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::resource {

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

    struct BmgBlockInfo {
        std::string magic;
        std::size_t offset = 0U;
        std::size_t declared_size = 0U;
        std::size_t available_size = 0U;
        bool truncated = false;
        bool extends_past_declared_file_size = false;
    };

    struct BmgFlowNode {
        std::uint8_t node_type = 0U;
        std::uint8_t group_id = 0U;
        std::uint16_t index = 0U;
        std::uint16_t next_index = 0U;
        std::uint16_t next_group = 0U;
        std::uint32_t raw_next = 0U;
    };

    struct BmgFlowIndexEntry {
        std::uint16_t message_index = 0U;
        std::uint16_t node_index = 0U;
        std::array<std::uint8_t, 8U> raw{};
    };

    struct BmgFlowData {
        std::uint16_t node_count = 0U;
        std::uint16_t branch_count = 0U;
        std::uint32_t unknown_0c = 0U;
        std::vector<BmgFlowNode> nodes;
        std::vector<std::uint16_t> branch_node_indices;
        std::vector<std::uint8_t> event_data;
        std::vector<BmgFlowIndexEntry> index_entries;
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
        [[nodiscard]] std::uint32_t declared_file_size() const;
        [[nodiscard]] std::uint8_t encoding() const;
        [[nodiscard]] std::span<const BmgBlockInfo> blocks() const;
        [[nodiscard]] const BmgBlockInfo *block(std::string_view magic) const;
        [[nodiscard]] const std::optional<BmgFlowData> &flow() const;

    private:
        BmgMessageArchive(std::vector<BmgMessage> messages, std::vector<BmgBlockInfo> blocks, std::uint32_t declared_file_size,
                          std::uint8_t encoding, std::optional<BmgFlowData> flow);

        std::vector<BmgMessage> _messages;
        std::map<std::string, std::size_t, std::less<>> _index_by_id;
        std::vector<BmgBlockInfo> _blocks;
        std::uint32_t _declared_file_size = 0U;
        std::uint8_t _encoding = 0U;
        std::optional<BmgFlowData> _flow;
    };

}  // namespace smgpc::resource
