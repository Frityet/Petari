#include "runtime/SysConfigService.hpp"

#include <algorithm>
#include <array>

#include "common/BinaryChunkFile.hpp"

namespace smgpc::runtime {
    namespace {
        [[nodiscard]] u32 read_big_endian_u32(std::span<const std::uint8_t> bytes) {
            if (bytes.empty()) {
                return 0U;
            }
            if (bytes.size() < sizeof(u32)) {
                return bytes[0U];
            }
            return (static_cast<u32>(bytes[0U]) << 24U) | (static_cast<u32>(bytes[1U]) << 16U) |
                   (static_cast<u32>(bytes[2U]) << 8U) | static_cast<u32>(bytes[3U]);
        }

        [[nodiscard]] std::vector<std::uint8_t> big_endian_u32_bytes(u32 value) {
            return {
                static_cast<std::uint8_t>((value >> 24U) & 0xffU),
                static_cast<std::uint8_t>((value >> 16U) & 0xffU),
                static_cast<std::uint8_t>((value >> 8U) & 0xffU),
                static_cast<std::uint8_t>(value & 0xffU),
            };
        }

        void append_attribute(std::vector<std::uint8_t> &data, const char *name, std::uint16_t offset) {
            common::append_be16(data, static_cast<std::uint16_t>(common::hash_code_31(name)));
            common::append_be16(data, offset);
        }

        [[nodiscard]] std::vector<std::uint8_t> make_sys_config_chunk_data(OSTime time_announced, OSTime time_sent, u32 sent_bytes) {
            auto data = std::vector<std::uint8_t>{};
            common::append_be16(data, 3U);
            common::append_be16(data, 20U);
            append_attribute(data, "mTimeAnnounced", 0U);
            append_attribute(data, "mTimeSent", 8U);
            append_attribute(data, "mSentBytes", 16U);
            common::append_be64(data, static_cast<std::uint64_t>(time_announced));
            common::append_be64(data, static_cast<std::uint64_t>(time_sent));
            common::append_be32(data, sent_bytes);
            return data;
        }

        [[nodiscard]] std::optional<std::size_t> find_attribute_offset(std::span<const std::uint8_t> data, const char *name, std::size_t value_size) {
            if (data.size() < 4U) {
                return std::nullopt;
            }

            const auto attribute_count = common::read_be16(data, 0U);
            const auto header_size = 4U + static_cast<std::size_t>(attribute_count) * 4U;
            if (header_size > data.size()) {
                return std::nullopt;
            }

            const auto expected_hash = static_cast<std::uint16_t>(common::hash_code_31(name));
            for (auto index = std::uint16_t{}; index < attribute_count; ++index) {
                const auto entry_offset = 4U + static_cast<std::size_t>(index) * 4U;
                if (common::read_be16(data, entry_offset) == expected_hash) {
                    const auto value_offset = header_size + common::read_be16(data, entry_offset + 2U);
                    if (value_offset + value_size <= data.size()) {
                        return value_offset;
                    }
                }
            }
            return std::nullopt;
        }
    }  // namespace

    SysConfigService::SysConfigService() {
        reset_defaults();
    }

    void SysConfigService::reset_defaults() {
        _entries.clear();
        write_bytes("IPL.LNG", SysConfigValueKind::Byte, std::array<std::uint8_t, 1U>{1U});
        write_bytes("IPL.SADR", SysConfigValueKind::BigArray, std::array<std::uint8_t, 1U>{0x6cU});
        write_bool("IPL.AR", true);
        write_bool("IPL.PGS", false);
        write_bool("IPL.E60", true);
        write_u32("BT.SENS", 3U);
        write_bytes("BT.BAR", SysConfigValueKind::Byte, std::array<std::uint8_t, 1U>{1U});
        write_bytes("BT.SPKV", SysConfigValueKind::Byte, std::array<std::uint8_t, 1U>{0x58U});
        write_bool("BT.MOT", true);
        trace(SysConfigAccessKind::ResetDefaults, {}, _entries.size(), 0U);
    }

    bool SysConfigService::has(std::string_view key) const {
        return _entries.contains(key);
    }

    std::optional<std::vector<std::uint8_t>> SysConfigService::read_bytes(std::string_view key) const {
        const auto it = _entries.find(key);
        if (it == _entries.end()) {
            trace(SysConfigAccessKind::Read, key, 0U, 0U);
            return std::nullopt;
        }

        trace(SysConfigAccessKind::Read, key, it->second.bytes.size(), read_big_endian_u32(it->second.bytes));
        return it->second.bytes;
    }

    u32 SysConfigService::read_u32(std::string_view key, u32 fallback) const {
        const auto bytes = read_bytes(key);
        return bytes.has_value() ? read_big_endian_u32(*bytes) : fallback;
    }

    bool SysConfigService::read_bool(std::string_view key, bool fallback) const {
        const auto bytes = read_bytes(key);
        return bytes.has_value() ? read_big_endian_u32(*bytes) != 0U : fallback;
    }

    void SysConfigService::write_bytes(std::string_view key, SysConfigValueKind kind, std::span<const std::uint8_t> bytes) {
        auto copied = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
        _entries[std::string(key)] = SysConfigEntry{
            .kind = kind,
            .bytes = std::move(copied),
        };
        trace(SysConfigAccessKind::Write, key, bytes.size(), read_big_endian_u32(bytes));
    }

    void SysConfigService::write_u32(std::string_view key, u32 value) {
        auto bytes = big_endian_u32_bytes(value);
        write_bytes(key, SysConfigValueKind::Long, bytes);
    }

    void SysConfigService::write_bool(std::string_view key, bool value) {
        const auto byte = std::array<std::uint8_t, 1U>{static_cast<std::uint8_t>(value ? 1U : 0U)};
        write_bytes(key, SysConfigValueKind::Byte, byte);
    }

    void SysConfigService::set_time_announced(OSTime time) {
        _time_announced = time;
        trace(SysConfigAccessKind::Write, "mTimeAnnounced", sizeof(time), static_cast<u32>(time));
    }

    void SysConfigService::set_time_sent(OSTime time) {
        _time_sent = time;
        trace(SysConfigAccessKind::Write, "mTimeSent", sizeof(time), static_cast<u32>(time));
    }

    void SysConfigService::set_sent_bytes(u32 bytes) {
        _sent_bytes = bytes;
        trace(SysConfigAccessKind::Write, "mSentBytes", sizeof(bytes), bytes);
    }

    OSTime SysConfigService::time_announced() const {
        trace(SysConfigAccessKind::Read, "mTimeAnnounced", sizeof(_time_announced), static_cast<u32>(_time_announced));
        return _time_announced;
    }

    OSTime SysConfigService::time_sent() const {
        trace(SysConfigAccessKind::Read, "mTimeSent", sizeof(_time_sent), static_cast<u32>(_time_sent));
        return _time_sent;
    }

    u32 SysConfigService::sent_bytes() const {
        trace(SysConfigAccessKind::Read, "mSentBytes", sizeof(_sent_bytes), _sent_bytes);
        return _sent_bytes;
    }

    void SysConfigService::decode_save_data_binary(std::span<const std::uint8_t> bytes) {
        const auto chunks = common::decode_binary_chunk_file(bytes);
        if (!chunks.has_value()) {
            return;
        }

        const auto found = std::ranges::find_if(*chunks, [](const auto &chunk) {
            return chunk.signature == common::fourcc('S', 'Y', 'S', 'C') && chunk.hash == 0x1U;
        });
        if (found == chunks->end()) {
            return;
        }

        if (const auto offset = find_attribute_offset(found->data, "mTimeAnnounced", sizeof(std::uint64_t))) {
            _time_announced = static_cast<OSTime>(common::read_be64(found->data, *offset));
        }
        if (const auto offset = find_attribute_offset(found->data, "mTimeSent", sizeof(std::uint64_t))) {
            _time_sent = static_cast<OSTime>(common::read_be64(found->data, *offset));
        }
        if (const auto offset = find_attribute_offset(found->data, "mSentBytes", sizeof(std::uint32_t))) {
            _sent_bytes = common::read_be32(found->data, *offset);
        }
        trace(SysConfigAccessKind::DecodeSaveData, "sysconf", found->data.size(), _sent_bytes);
    }

    std::vector<std::uint8_t> SysConfigService::encode_save_data_binary(std::size_t size) const {
        const auto chunks = std::array{
            common::BinaryChunk{
                .signature = common::fourcc('S', 'Y', 'S', 'C'),
                .hash = 0x1U,
                .data = make_sys_config_chunk_data(_time_announced, _time_sent, _sent_bytes),
            },
        };
        auto bytes = common::encode_binary_chunk_file(chunks, size);
        trace(SysConfigAccessKind::EncodeSaveData, "sysconf", bytes.size(), _sent_bytes);
        return bytes;
    }

    std::span<const SysConfigAccessTrace> SysConfigService::access_trace() const {
        return _access_trace;
    }

    void SysConfigService::clear_trace() {
        _access_trace.clear();
    }

    void SysConfigService::trace(SysConfigAccessKind kind, std::string_view key, std::size_t byte_count, u32 value) const {
        _access_trace.push_back(SysConfigAccessTrace{
            .kind = kind,
            .key = std::string(key),
            .byte_count = byte_count,
            .value = value,
        });
    }

}  // namespace smgpc::runtime
