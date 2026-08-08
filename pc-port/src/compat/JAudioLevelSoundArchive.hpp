#pragma once

#include <aurora/audio.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::compat {

    struct JAudioLevelSoundLayerAudit {
        std::uint8_t bank = 0;
        std::uint8_t program = 0;
        std::uint8_t note = 0;
        std::uint8_t velocity = 0;
        std::uint16_t wave_id = 0;
        std::string wave_archive_name;
        std::uint32_t wave_archive_offset = 0;
        std::uint32_t encoded_length = 0;
        std::uint32_t decoded_loop_start = 0;
        std::uint32_t decoded_loop_end = 0;
        std::uint32_t source_sample_rate = 0;
        std::uint16_t direct_release_ticks = 0;
        std::int16_t loop_history_yn1 = 0;
        std::int16_t loop_history_yn2 = 0;
    };

    struct JAudioLevelSoundRecipe {
        std::uint32_t sound_id = 0;
        std::uint8_t priority = 0;
        std::uint8_t table_volume = 0;
        aurora::audio::LoopingVoiceSpec voice;
        std::vector<JAudioLevelSoundLayerAudit> layers;
    };

    // Bounds-checked reader for JAudio's BAA/BST/BSTN/BSC/IBNK/WSYS chain.
    // Wave archive data is supplied by name so the parser is independent of
    // the host/DVD filesystem used by the compatibility layer.
    class JAudioLevelSoundArchive final {
    public:
        using WaveArchiveLoader =
            std::function<std::vector<std::uint8_t>(std::string_view)>;

        JAudioLevelSoundArchive(std::span<const std::uint8_t> decompressed_baa,
                                WaveArchiveLoader wave_archive_loader);
        ~JAudioLevelSoundArchive();

        JAudioLevelSoundArchive(const JAudioLevelSoundArchive &) = delete;
        JAudioLevelSoundArchive &operator=(const JAudioLevelSoundArchive &) = delete;
        JAudioLevelSoundArchive(JAudioLevelSoundArchive &&) noexcept;
        JAudioLevelSoundArchive &operator=(JAudioLevelSoundArchive &&) noexcept;

        [[nodiscard]] std::optional<std::uint32_t>
        find_sound_id(std::string_view name) const;
        [[nodiscard]] std::optional<JAudioLevelSoundRecipe>
        resolve_level_sound(std::string_view name) const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace smgpc::compat
