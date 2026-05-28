#include "BmgMessageArchive.hpp"

#include <algorithm>
#include <stdexcept>

#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"

namespace smgpc::compat {
    namespace {

        constexpr std::uint64_t MESG_MAGIC = 0x4d455347626d6731;
        constexpr std::uint32_t INF1_MAGIC = 0x494e4631;
        constexpr std::uint32_t DAT1_MAGIC = 0x44415431;
        constexpr std::uint8_t UTF16_ENCODING_SIZE = 2;
        constexpr std::size_t BMG_HEADER_SIZE = 0x20;
        constexpr std::size_t BMG_BLOCK_HEADER_SIZE = 0x08;
        constexpr std::size_t INF1_ENTRIES_OFFSET = 0x10;
        constexpr std::uint16_t BMG_CONTROL_MARKER = 0x001a;

        struct BlockRange {
            std::size_t offset = 0U;
            std::size_t size = 0U;
        };

        struct DecodedBmgText {
            std::u16string raw_text;
            std::u16string display_text;
        };

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

        [[nodiscard]] std::optional<BlockRange> find_block(std::span<const std::uint8_t> data, std::uint32_t magic) {
            if (data.size() < BMG_HEADER_SIZE || read_be64(data, 0U) != MESG_MAGIC) {
                throw std::runtime_error("BMG data is not a MESGbmg1 file");
            }

            const auto file_size = read_be32(data, 0x08U);
            if (file_size > data.size()) {
                throw std::runtime_error("BMG declared file size is outside buffer");
            }

            const auto block_count = read_be32(data, 0x0cU);
            auto cursor = BMG_HEADER_SIZE;
            for (auto i = 0U; i < block_count; ++i) {
                if (cursor + BMG_BLOCK_HEADER_SIZE > file_size) {
                    throw std::runtime_error("BMG block header is outside file");
                }

                const auto block_magic = read_be32(data, cursor);
                const auto block_size = read_be32(data, cursor + 4U);
                if (block_size < BMG_BLOCK_HEADER_SIZE || cursor + block_size > file_size) {
                    throw std::runtime_error("BMG block size is outside file");
                }

                if (block_magic == magic) {
                    return BlockRange{.offset = cursor, .size = block_size};
                }

                cursor += block_size;
            }

            return std::nullopt;
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
            auto decoded = DecodedBmgText{};
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

        [[nodiscard]] std::vector<BmgMessage> parse_bmg_messages(std::span<const std::uint8_t> bmg_data) {
            if (bmg_data.size() < BMG_HEADER_SIZE || read_be64(bmg_data, 0U) != MESG_MAGIC) {
                throw std::runtime_error("BMG data is not a MESGbmg1 file");
            }
            if (bmg_data[0x10U] != UTF16_ENCODING_SIZE) {
                throw std::runtime_error("BMG text encoding is not UTF-16BE");
            }

            const auto inf1 = find_block(bmg_data, INF1_MAGIC);
            const auto dat1 = find_block(bmg_data, DAT1_MAGIC);
            if (!inf1.has_value() || !dat1.has_value()) {
                throw std::runtime_error("BMG data is missing INF1 or DAT1 blocks");
            }

            const auto message_count = read_be16(bmg_data, inf1->offset + 0x08U);
            const auto item_size = read_be16(bmg_data, inf1->offset + 0x0aU);
            if (item_size < 4U) {
                throw std::runtime_error("BMG INF1 item size is too small");
            }
            if (inf1->offset + INF1_ENTRIES_OFFSET + static_cast<std::size_t>(message_count) * item_size > inf1->offset + inf1->size) {
                throw std::runtime_error("BMG INF1 entries are truncated");
            }

            const auto dat1_text = bmg_data.subspan(dat1->offset + BMG_BLOCK_HEADER_SIZE, dat1->size - BMG_BLOCK_HEADER_SIZE);
            auto messages = std::vector<BmgMessage>{};
            messages.reserve(message_count);
            for (auto i = 0U; i < message_count; ++i) {
                const auto entry_offset = inf1->offset + INF1_ENTRIES_OFFSET + static_cast<std::size_t>(i) * item_size;
                auto info = BmgMessageInfo{
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
                messages.push_back(BmgMessage{
                    .id = {},
                    .info = info,
                    .raw_text = std::move(decoded.raw_text),
                    .display_text = std::move(decoded.display_text),
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

    BmgMessageArchive BmgMessageArchive::from_message_archive(const RarcArchive &archive) {
        return from_bytes(archive_file_data_any(archive, {"message.bmg", "Message.bmg"}), archive_file_data_any(archive, {"messageid.tbl", "MessageId.tbl"}));
    }

    BmgMessageArchive BmgMessageArchive::from_bytes(std::span<const std::uint8_t> bmg_data, std::span<const std::uint8_t> message_id_table) {
        auto messages = parse_bmg_messages(bmg_data);
        attach_message_ids(messages, message_id_table);
        return BmgMessageArchive(std::move(messages));
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

    BmgMessageArchive::BmgMessageArchive(std::vector<BmgMessage> messages) : _messages(std::move(messages)) {
        for (auto i = std::size_t{}; i < _messages.size(); ++i) {
            _index_by_id.emplace(_messages[i].id, i);
        }
    }

}  // namespace smgpc::compat
