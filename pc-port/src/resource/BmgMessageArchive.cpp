#include "BmgMessageArchive.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <stdexcept>
#include <string>

#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"

namespace smgpc::resource {
    namespace {

        constexpr std::uint64_t MESG_MAGIC = 0x4d455347626d6731;
        constexpr std::uint8_t UTF16_ENCODING_SIZE = 2;
        constexpr std::size_t BMG_HEADER_SIZE = 0x20;
        constexpr std::size_t BMG_BLOCK_HEADER_SIZE = 0x08;
        constexpr std::size_t INF1_ENTRIES_OFFSET = 0x10;
        constexpr std::uint16_t BMG_CONTROL_MARKER = 0x001a;
        constexpr std::uint16_t BMG_STRING_TAG_TYPE_A = 0x0005;
        constexpr std::uint16_t BMG_NUMERIC_TAG_TYPE = 0x0006;
        constexpr std::uint16_t BMG_STRING_TAG_TYPE_B = 0x0007;
        constexpr std::uint16_t BMG_ZERO_PAD_TWO_DIGITS = 0x0005;

        struct BmgHeader {
            std::uint32_t declared_file_size = 0U;
            std::uint32_t block_count = 0U;
            std::uint8_t encoding = 0U;
        };

        struct DecodedBmgText {
            std::u16string raw_text;
            std::u16string display_text;
        };

        [[nodiscard]] std::string magic_string(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("BMG magic read past end of buffer");
            }

            return std::string(reinterpret_cast<const char *>(data.data() + offset), 4U);
        }

        [[nodiscard]] std::uint16_t tag_size(char16_t packed_size_type) {
            return static_cast<std::uint16_t>((static_cast<std::uint32_t>(packed_size_type) >> 8U) & 0xffU);
        }

        [[nodiscard]] std::uint16_t tag_type(char16_t packed_size_type) {
            return static_cast<std::uint16_t>(static_cast<std::uint32_t>(packed_size_type) & 0xffU);
        }

        [[nodiscard]] std::size_t tag_word_count(char16_t packed_size_type) {
            const auto size = tag_size(packed_size_type);
            return size >= 2U ? static_cast<std::size_t>((size - 2U) / 2U) : 0U;
        }

        [[nodiscard]] std::u16string number_text(std::int32_t value, bool zero_pad_two_digits) {
            auto buffer = std::array<char, 32U>{};
            const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
            auto text = std::string_view(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
            auto out = std::u16string {};
            if (zero_pad_two_digits && value >= 0 && value < 10) {
                out.push_back(u'0');
            }
            for (const auto character : text) {
                out.push_back(static_cast<char16_t>(character));
            }
            return out;
        }

        [[nodiscard]] const BmgFormatArg *arg_at(std::span<const BmgFormatArg> args, std::size_t index) {
            return index < args.size() ? &args[index] : nullptr;
        }

        [[nodiscard]] std::vector<BmgControlTag> scan_bmg_control_tags(std::u16string_view raw_text) {
            auto tags = std::vector<BmgControlTag>{};

            for (auto cursor = std::size_t {}; cursor < raw_text.size();) {
                if (raw_text[cursor] != BMG_CONTROL_MARKER || cursor + 1U >= raw_text.size()) {
                    ++cursor;
                    continue;
                }

                const auto packed_size_type = raw_text[cursor + 1U];
                const auto word_count = tag_word_count(packed_size_type);
                if (word_count == 0U || cursor + word_count >= raw_text.size()) {
                    ++cursor;
                    continue;
                }

                auto tag = BmgControlTag {
                    .raw_offset = cursor,
                    .size_bytes = tag_size(packed_size_type),
                    .type = tag_type(packed_size_type),
                    .payload_words = {},
                };
                tag.payload_words.reserve(word_count - 1U);
                for (auto word = std::size_t {2U}; word <= word_count; ++word) {
                    tag.payload_words.push_back(static_cast<std::uint16_t>(raw_text[cursor + word]));
                }
                tags.push_back(std::move(tag));

                cursor += 1U + word_count;
            }

            return tags;
        }

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 2U > data.size()) {
                throw std::runtime_error("BMG read past end of buffer");
            }

            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | data[offset + 1U]);
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("BMG read past end of buffer");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | data[offset + 3U];
        }

        [[nodiscard]] std::uint64_t read_be64(std::span<const std::uint8_t> data, std::size_t offset) {
            return (static_cast<std::uint64_t>(read_be32(data, offset)) << 32U) | read_be32(data, offset + 4U);
        }

        [[nodiscard]] BmgHeader read_bmg_header(std::span<const std::uint8_t> data) {
            if (data.size() < BMG_HEADER_SIZE || read_be64(data, 0U) != MESG_MAGIC) {
                throw std::runtime_error("BMG data is not a MESGbmg1 file");
            }

            const auto file_size = read_be32(data, 0x08U);
            if (file_size > data.size()) {
                throw std::runtime_error("BMG declared file size is outside buffer");
            }

            return BmgHeader {
                .declared_file_size = file_size,
                .block_count = read_be32(data, 0x0cU),
                .encoding = data[0x10U],
            };
        }

        [[nodiscard]] std::vector<BmgBlockInfo> parse_block_directory(std::span<const std::uint8_t> data, const BmgHeader &header) {
            auto blocks = std::vector<BmgBlockInfo>{};
            blocks.reserve(header.block_count);

            auto cursor = BMG_HEADER_SIZE;
            for (auto i = 0U; i < header.block_count; ++i) {
                if (cursor + BMG_BLOCK_HEADER_SIZE > data.size()) {
                    throw std::runtime_error("BMG block header is outside file");
                }

                const auto block_size = read_be32(data, cursor + 4U);
                if (block_size < BMG_BLOCK_HEADER_SIZE) {
                    throw std::runtime_error("BMG block size is outside file");
                }

                const auto remaining_size = data.size() - cursor;
                const auto available_size = std::min<std::size_t>(block_size, remaining_size);
                if (available_size < BMG_BLOCK_HEADER_SIZE) {
                    throw std::runtime_error("BMG block available size is outside file");
                }

                blocks.push_back(BmgBlockInfo {
                    .magic = magic_string(data, cursor),
                    .offset = cursor,
                    .declared_size = block_size,
                    .available_size = available_size,
                    .truncated = available_size != block_size,
                    .extends_past_declared_file_size = cursor >= header.declared_file_size ||
                                                       cursor + static_cast<std::size_t>(block_size) > header.declared_file_size,
                });

                if (cursor + static_cast<std::size_t>(block_size) > data.size()) {
                    cursor = data.size();
                } else {
                    cursor += block_size;
                }
            }

            return blocks;
        }

        [[nodiscard]] const BmgBlockInfo *find_block(std::span<const BmgBlockInfo> blocks, std::string_view magic) {
            const auto found = std::ranges::find_if(blocks, [magic](const auto &block) {
                return block.magic == magic;
            });

            return found == blocks.end() ? nullptr : &(*found);
        }

        [[nodiscard]] std::span<const std::uint8_t> block_data(std::span<const std::uint8_t> data, const BmgBlockInfo &block) {
            if (block.offset + block.available_size > data.size()) {
                throw std::runtime_error("BMG block data is outside file");
            }

            return data.subspan(block.offset, block.available_size);
        }

        [[nodiscard]] std::optional<BmgFlowData> parse_flow_data(std::span<const std::uint8_t> bmg_data,
                                                                 std::span<const BmgBlockInfo> blocks) {
            const auto *flw_info = find_block(blocks, "FLW1");
            if (flw_info == nullptr) {
                return std::nullopt;
            }

            const auto flw = block_data(bmg_data, *flw_info);
            if (flw.size() < 0x10U) {
                throw std::runtime_error("BMG FLW1 block is truncated");
            }

            auto flow = BmgFlowData {
                .node_count = read_be16(flw, 0x08U),
                .branch_count = read_be16(flw, 0x0aU),
                .unknown_0c = read_be32(flw, 0x0cU),
                .nodes = {},
                .branch_node_indices = {},
                .event_data = {},
                .index_entries = {},
            };

            const auto node_table_offset = 0x10U;
            const auto node_table_size = static_cast<std::size_t>(flow.node_count) * 8U;
            const auto branch_table_offset = node_table_offset + node_table_size;
            const auto branch_table_size = static_cast<std::size_t>(flow.branch_count) * 2U;
            if (branch_table_offset + branch_table_size > flw.size()) {
                throw std::runtime_error("BMG FLW1 node or branch table is truncated");
            }

            flow.nodes.reserve(flow.node_count);
            for (auto i = 0U; i < flow.node_count; ++i) {
                const auto offset = node_table_offset + static_cast<std::size_t>(i) * 8U;
                flow.nodes.push_back(BmgFlowNode {
                    .node_type = flw[offset],
                    .group_id = flw[offset + 1U],
                    .index = read_be16(flw, offset + 2U),
                    .next_index = read_be16(flw, offset + 4U),
                    .next_group = read_be16(flw, offset + 6U),
                    .raw_next = read_be32(flw, offset + 4U),
                });
            }

            flow.branch_node_indices.reserve(flow.branch_count);
            for (auto i = 0U; i < flow.branch_count; ++i) {
                flow.branch_node_indices.push_back(read_be16(flw, branch_table_offset + static_cast<std::size_t>(i) * 2U));
            }

            const auto event_offset = branch_table_offset + branch_table_size;
            flow.event_data.assign(flw.begin() + static_cast<std::ptrdiff_t>(event_offset), flw.end());

            if (const auto *fli_info = find_block(blocks, "FLI1"); fli_info != nullptr) {
                const auto fli = block_data(bmg_data, *fli_info);
                if (fli.size() < 0x10U) {
                    throw std::runtime_error("BMG FLI1 block is truncated");
                }

                const auto entry_count = read_be16(fli, 0x08U);
                const auto entry_size = fli[0x0aU];
                if (entry_size < 8U) {
                    throw std::runtime_error("BMG FLI1 entry size is too small");
                }
                if (0x10U + static_cast<std::size_t>(entry_count) * entry_size > fli.size()) {
                    throw std::runtime_error("BMG FLI1 entries are truncated");
                }

                flow.index_entries.reserve(entry_count);
                for (auto i = 0U; i < entry_count; ++i) {
                    const auto offset = 0x10U + static_cast<std::size_t>(i) * entry_size;
                    auto entry = BmgFlowIndexEntry {
                        .message_index = read_be16(fli, offset),
                        .node_index = read_be16(fli, offset + 4U),
                        .raw = {},
                    };
                    for (auto raw_index = 0U; raw_index < entry.raw.size(); ++raw_index) {
                        entry.raw[raw_index] = fli[offset + raw_index];
                    }
                    flow.index_entries.push_back(entry);
                }
            }

            return flow;
        }

        [[nodiscard]] std::span<const std::uint8_t> archive_file_data_any(const RarcArchive &archive, std::initializer_list<std::string_view> names) {
            for (const auto name : names) {
                if (const auto *entry = archive.find(name)) {
                    return archive.file_data(*entry);
                }
            }

            throw std::runtime_error("Message archive is missing required BMG resource");
        }

        [[nodiscard]] DecodedBmgText decode_utf16be_bmg_text(std::span<const std::uint8_t> data, std::size_t offset) {
            auto decoded = DecodedBmgText {};
            if (offset >= data.size()) {
                throw std::runtime_error("BMG text offset is outside DAT1 data");
            }

            auto cursor = offset;
            while (cursor + 2U <= data.size()) {
                const auto code = read_be16(data, cursor);
                cursor += 2U;
                if (code == 0U) {
                    return decoded;
                }

                decoded.raw_text.push_back(static_cast<char16_t>(code));
                if (code == BMG_CONTROL_MARKER) {
                    if (cursor >= data.size()) {
                        throw std::runtime_error("BMG control sequence is truncated");
                    }

                    const auto control_size = data[cursor];
                    if (control_size < 2U || cursor + static_cast<std::size_t>(control_size - 2U) > data.size()) {
                        throw std::runtime_error("BMG control sequence size is invalid");
                    }

                    const auto control_end = cursor + static_cast<std::size_t>(control_size - 2U);
                    while (cursor + 1U < control_end) {
                        decoded.raw_text.push_back(static_cast<char16_t>(read_be16(data, cursor)));
                        cursor += 2U;
                    }
                    cursor = control_end;
                    continue;
                }

                decoded.display_text.push_back(static_cast<char16_t>(code));
            }

            throw std::runtime_error("BMG text is not null terminated");
        }

        [[nodiscard]] std::vector<BmgMessage> parse_bmg_messages(std::span<const std::uint8_t> bmg_data,
                                                                 std::span<const BmgBlockInfo> blocks, std::uint8_t encoding) {
            if (encoding != UTF16_ENCODING_SIZE) {
                throw std::runtime_error("BMG text encoding is not UTF-16BE");
            }

            const auto *inf1 = find_block(blocks, "INF1");
            const auto *dat1 = find_block(blocks, "DAT1");
            if (inf1 == nullptr || dat1 == nullptr) {
                throw std::runtime_error("BMG data is missing INF1 or DAT1 blocks");
            }

            const auto message_count = read_be16(bmg_data, inf1->offset + 0x08U);
            const auto item_size = read_be16(bmg_data, inf1->offset + 0x0aU);
            if (item_size < 4U) {
                throw std::runtime_error("BMG INF1 item size is too small");
            }
            if (inf1->offset + INF1_ENTRIES_OFFSET + static_cast<std::size_t>(message_count) * item_size >
                inf1->offset + inf1->available_size) {
                throw std::runtime_error("BMG INF1 entries are truncated");
            }

            const auto dat1_text = bmg_data.subspan(dat1->offset + BMG_BLOCK_HEADER_SIZE, dat1->available_size - BMG_BLOCK_HEADER_SIZE);
            auto messages = std::vector<BmgMessage>{};
            messages.reserve(message_count);
            for (auto i = 0U; i < message_count; ++i) {
                const auto entry_offset = inf1->offset + INF1_ENTRIES_OFFSET + static_cast<std::size_t>(i) * item_size;
                auto info = BmgMessageInfo {
                    .text_offset = read_be32(bmg_data, entry_offset),
                };

                if (item_size >= 12U) {
                    info.camera_set_id = read_be16(bmg_data, entry_offset + 4U);
                    info.unknown_06 = bmg_data[entry_offset + 6U];
                    info.camera_type = bmg_data[entry_offset + 7U];
                    info.talk_type = bmg_data[entry_offset + 8U];
                    info.balloon_type = bmg_data[entry_offset + 9U];
                    info.unknown_0a = bmg_data[entry_offset + 0x0aU];
                    info.unknown_0b = bmg_data[entry_offset + 0x0bU];
                }

                auto decoded = decode_utf16be_bmg_text(dat1_text, info.text_offset);
                auto control_tags = scan_bmg_control_tags(decoded.raw_text);
                messages.push_back(BmgMessage {
                    .id = {},
                    .info = info,
                    .raw_text = std::move(decoded.raw_text),
                    .display_text = std::move(decoded.display_text),
                    .control_tags = std::move(control_tags),
                });
            }

            return messages;
        }

        void attach_message_ids(std::vector<BmgMessage> &messages, std::span<const std::uint8_t> message_id_table) {
            const auto table = BcsvTable::from_bytes(message_id_table);
            for (auto entry = 0U; entry < table.entry_count(); ++entry) {
                const auto id = table.get_string(entry, "MessageId");
                const auto index = table.get_s32(entry, "Index");
                if (!id.has_value() || !index.has_value()) {
                    throw std::runtime_error("MessageId.tbl is missing MessageId or Index fields");
                }
                if (*index < 0 || static_cast<std::size_t>(*index) >= messages.size()) {
                    throw std::runtime_error("MessageId.tbl index is outside BMG message table");
                }

                messages[static_cast<std::size_t>(*index)].id = *id;
            }

            const auto missing_id = std::ranges::find_if(messages, [](const auto &message) {
                return message.id.empty();
            });
            if (missing_id != messages.end()) {
                throw std::runtime_error("MessageId.tbl did not name every BMG message");
            }
        }

    }  // namespace

    BmgFormatArg BmgFormatArg::number(std::int32_t value) {
        return BmgFormatArg {
            .type = Type::Number,
            .number_value = value,
            .string_value = {},
        };
    }

    BmgFormatArg BmgFormatArg::string(std::u16string_view value) {
        return BmgFormatArg {
            .type = Type::String,
            .number_value = 0,
            .string_value = std::u16string(value),
        };
    }

    std::vector<BmgControlTag> bmg_control_tags(std::u16string_view raw_text) {
        return scan_bmg_control_tags(raw_text);
    }

    std::u16string format_bmg_text(std::u16string_view raw_text, std::span<const BmgFormatArg> args) {
        auto formatted = std::u16string {};

        for (auto cursor = std::size_t {}; cursor < raw_text.size();) {
            const auto code = raw_text[cursor];
            if (code != BMG_CONTROL_MARKER || cursor + 1U >= raw_text.size()) {
                formatted.push_back(code);
                ++cursor;
                continue;
            }

            const auto packed_size_type = raw_text[cursor + 1U];
            const auto word_count = tag_word_count(packed_size_type);
            if (word_count == 0U || cursor + word_count >= raw_text.size()) {
                ++cursor;
                continue;
            }

            const auto type = tag_type(packed_size_type);
            if (type == BMG_NUMERIC_TAG_TYPE && word_count >= 6U) {
                const auto format = static_cast<std::uint16_t>(raw_text[cursor + 2U]);
                const auto arg_index = static_cast<std::size_t>(raw_text[cursor + 6U]);
                if (const auto *arg = arg_at(args, arg_index); arg != nullptr) {
                    if (arg->type == BmgFormatArg::Type::Number) {
                        formatted.append(number_text(arg->number_value, format == BMG_ZERO_PAD_TWO_DIGITS));
                    } else {
                        formatted.append(arg->string_value);
                    }
                }
            } else if ((type == BMG_STRING_TAG_TYPE_A || type == BMG_STRING_TAG_TYPE_B) && word_count >= 2U) {
                const auto arg_index = static_cast<std::size_t>(raw_text[cursor + word_count]);
                if (const auto *arg = arg_at(args, arg_index); arg != nullptr) {
                    if (arg->type == BmgFormatArg::Type::String) {
                        formatted.append(arg->string_value);
                    } else {
                        formatted.append(number_text(arg->number_value, false));
                    }
                }
            }

            cursor += 1U + word_count;
        }

        return formatted;
    }

    BmgMessageArchive BmgMessageArchive::from_message_archive(const RarcArchive &archive) {
        return from_bytes(archive_file_data_any(archive, {"message.bmg", "Message.bmg"}), archive_file_data_any(archive, {"messageid.tbl", "MessageId.tbl"}));
    }

    BmgMessageArchive BmgMessageArchive::from_bytes(std::span<const std::uint8_t> bmg_data, std::span<const std::uint8_t> message_id_table) {
        const auto header = read_bmg_header(bmg_data);
        auto blocks = parse_block_directory(bmg_data, header);
        auto flow = parse_flow_data(bmg_data, blocks);
        auto messages = parse_bmg_messages(bmg_data, blocks, header.encoding);
        attach_message_ids(messages, message_id_table);
        return BmgMessageArchive(std::move(messages), std::move(blocks), header.declared_file_size, header.encoding, std::move(flow));
    }

    std::span<const BmgMessage> BmgMessageArchive::messages() const {
        return _messages;
    }

    std::size_t BmgMessageArchive::message_count() const {
        return _messages.size();
    }

    const BmgMessage *BmgMessageArchive::find(std::string_view message_id) const {
        const auto found = _index_by_id.find(message_id);
        return found == _index_by_id.end() ? nullptr : &_messages[found->second];
    }

    std::uint32_t BmgMessageArchive::declared_file_size() const {
        return _declared_file_size;
    }

    std::uint8_t BmgMessageArchive::encoding() const {
        return _encoding;
    }

    std::span<const BmgBlockInfo> BmgMessageArchive::blocks() const {
        return _blocks;
    }

    const BmgBlockInfo *BmgMessageArchive::block(std::string_view magic) const {
        return find_block(_blocks, magic);
    }

    const std::optional<BmgFlowData> &BmgMessageArchive::flow() const {
        return _flow;
    }

    BmgMessageArchive::BmgMessageArchive(std::vector<BmgMessage> messages, std::vector<BmgBlockInfo> blocks,
                                         std::uint32_t declared_file_size, std::uint8_t encoding, std::optional<BmgFlowData> flow)
        : _messages(std::move(messages)), _blocks(std::move(blocks)), _declared_file_size(declared_file_size), _encoding(encoding),
          _flow(std::move(flow)) {
        for (auto i = std::size_t {}; i < _messages.size(); ++i) {
            _index_by_id.emplace(_messages[i].id, i);
        }
    }

}  // namespace smgpc::resource
