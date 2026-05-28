#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <revolution.h>

namespace smgpc::runtime {

    enum class SysConfigValueKind {
        Byte,
        Long,
        SmallArray,
        BigArray,
    };

    enum class SysConfigAccessKind {
        ResetDefaults,
        Read,
        Write,
        DecodeSaveData,
        EncodeSaveData,
    };

    struct SysConfigEntry {
        SysConfigValueKind kind = SysConfigValueKind::Byte;
        std::vector<std::uint8_t> bytes;
    };

    struct SysConfigAccessTrace {
        SysConfigAccessKind kind = SysConfigAccessKind::Read;
        std::string key;
        std::size_t byte_count = 0U;
        u32 value = 0U;
    };

    class SysConfigService final {
    public:
        SysConfigService();

        void reset_defaults();
        [[nodiscard]] bool has(std::string_view key) const;
        [[nodiscard]] std::optional<std::vector<std::uint8_t>> read_bytes(std::string_view key) const;
        [[nodiscard]] u32 read_u32(std::string_view key, u32 fallback = 0U) const;
        [[nodiscard]] bool read_bool(std::string_view key, bool fallback = false) const;
        void write_bytes(std::string_view key, SysConfigValueKind kind, std::span<const std::uint8_t> bytes);
        void write_u32(std::string_view key, u32 value);
        void write_bool(std::string_view key, bool value);

        void set_time_announced(OSTime time);
        void set_time_sent(OSTime time);
        void set_sent_bytes(u32 bytes);
        [[nodiscard]] OSTime time_announced() const;
        [[nodiscard]] OSTime time_sent() const;
        [[nodiscard]] u32 sent_bytes() const;

        void decode_save_data_binary(std::span<const std::uint8_t> bytes);
        [[nodiscard]] std::vector<std::uint8_t> encode_save_data_binary(std::size_t size) const;
        [[nodiscard]] std::span<const SysConfigAccessTrace> access_trace() const;
        void clear_trace();

    private:
        void trace(SysConfigAccessKind kind, std::string_view key, std::size_t byte_count, u32 value) const;

        std::map<std::string, SysConfigEntry, std::less<>> _entries;
        OSTime _time_announced = 0;
        OSTime _time_sent = 0;
        u32 _sent_bytes = 0U;
        mutable std::vector<SysConfigAccessTrace> _access_trace;
    };

}  // namespace smgpc::runtime
