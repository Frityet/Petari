#include <aurora/j_audio_sound_archive.hpp>
#include <aurora/j_audio_stream.hpp>
#include "Game/AudioLib/AudBgm.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/AudioFacadeCompat.hpp"
#include "compat/JAudioSoundParameterSemantics.hpp"
#include "resource/Yaz0.hpp"
#include "runtime/JAudioPlaybackService.hpp"
#include "runtime/RuntimeContext.hpp"

#include <aurora/audio.hpp>
#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, float tolerance,
                      std::string_view message) {
        require(std::abs(actual - expected) <= tolerance, message);
    }

    template <typename Exception, typename Function>
    void require_throws(Function &&function, std::string_view expected_text,
                        std::string_view message) {
        try {
            function();
        } catch (const Exception &error) {
            require(std::string_view(error.what()).find(expected_text) !=
                        std::string_view::npos,
                    message);
            return;
        }
        throw std::runtime_error(std::string(message));
    }

    [[nodiscard]] std::uint32_t rotate_right(std::uint32_t value,
                                              unsigned amount) {
        return std::rotr(value, static_cast<int>(amount));
    }

    [[nodiscard]] std::array<std::uint8_t, 32U>
    sha256(std::span<const std::uint8_t> bytes) {
        constexpr auto round_constants = std::array<std::uint32_t, 64U>{
            0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
            0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
            0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
            0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
            0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
            0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
            0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
            0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
            0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
            0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
            0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
            0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
            0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
            0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
            0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
            0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
        };
        auto hash = std::array<std::uint32_t, 8U>{
            0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
            0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
        };

        auto padded = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
        const auto bit_length = static_cast<std::uint64_t>(bytes.size()) * 8U;
        padded.push_back(0x80U);
        while ((padded.size() + 8U) % 64U != 0U) {
            padded.push_back(0U);
        }
        for (auto shift = 56; shift >= 0; shift -= 8) {
            padded.push_back(
                static_cast<std::uint8_t>(bit_length >> shift));
        }

        for (auto block_offset = std::size_t{0};
             block_offset < padded.size(); block_offset += 64U) {
            auto schedule = std::array<std::uint32_t, 64U>{};
            for (auto index = std::size_t{0}; index < 16U; ++index) {
                const auto offset = block_offset + index * 4U;
                schedule[index] =
                    static_cast<std::uint32_t>(padded[offset]) << 24U |
                    static_cast<std::uint32_t>(padded[offset + 1U]) << 16U |
                    static_cast<std::uint32_t>(padded[offset + 2U]) << 8U |
                    static_cast<std::uint32_t>(padded[offset + 3U]);
            }
            for (auto index = std::size_t{16U}; index < schedule.size();
                 ++index) {
                const auto s0 = rotate_right(schedule[index - 15U], 7U) ^
                                rotate_right(schedule[index - 15U], 18U) ^
                                (schedule[index - 15U] >> 3U);
                const auto s1 = rotate_right(schedule[index - 2U], 17U) ^
                                rotate_right(schedule[index - 2U], 19U) ^
                                (schedule[index - 2U] >> 10U);
                schedule[index] = schedule[index - 16U] + s0 +
                                  schedule[index - 7U] + s1;
            }

            auto a = hash[0];
            auto b = hash[1];
            auto c = hash[2];
            auto d = hash[3];
            auto e = hash[4];
            auto f = hash[5];
            auto g = hash[6];
            auto h = hash[7];
            for (auto index = std::size_t{0}; index < schedule.size();
                 ++index) {
                const auto sigma1 = rotate_right(e, 6U) ^ rotate_right(e, 11U) ^
                                    rotate_right(e, 25U);
                const auto choose = (e & f) ^ (~e & g);
                const auto temp1 = h + sigma1 + choose +
                                   round_constants[index] + schedule[index];
                const auto sigma0 = rotate_right(a, 2U) ^ rotate_right(a, 13U) ^
                                    rotate_right(a, 22U);
                const auto majority = (a & b) ^ (a & c) ^ (b & c);
                const auto temp2 = sigma0 + majority;
                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }
            hash[0] += a;
            hash[1] += b;
            hash[2] += c;
            hash[3] += d;
            hash[4] += e;
            hash[5] += f;
            hash[6] += g;
            hash[7] += h;
        }

        auto digest = std::array<std::uint8_t, 32U>{};
        for (auto index = std::size_t{0}; index < hash.size(); ++index) {
            digest[index * 4U] = static_cast<std::uint8_t>(hash[index] >> 24U);
            digest[index * 4U + 1U] =
                static_cast<std::uint8_t>(hash[index] >> 16U);
            digest[index * 4U + 2U] =
                static_cast<std::uint8_t>(hash[index] >> 8U);
            digest[index * 4U + 3U] =
                static_cast<std::uint8_t>(hash[index]);
        }
        return digest;
    }

    [[nodiscard]] std::string sha256_hex(std::span<const std::uint8_t> bytes) {
        const auto digest = sha256(bytes);
        auto output = std::ostringstream{};
        output << std::hex << std::setfill('0');
        for (const auto byte : digest) {
            output << std::setw(2) << static_cast<unsigned>(byte);
        }
        return output.str();
    }

    [[nodiscard]] std::vector<std::uint8_t>
    read_file(const std::filesystem::path &path) {
        auto stream = std::ifstream(path, std::ios::binary | std::ios::ate);
        if (!stream) {
            throw std::runtime_error("Cannot open retail fixture " + path.string());
        }
        const auto size = stream.tellg();
        if (size < 0) {
            throw std::runtime_error("Cannot size retail fixture " + path.string());
        }
        auto bytes = std::vector<std::uint8_t>(static_cast<std::size_t>(size));
        stream.seekg(0);
        stream.read(reinterpret_cast<char *>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
        if (!stream) {
            throw std::runtime_error("Cannot read retail fixture " + path.string());
        }
        return bytes;
    }

    [[nodiscard]] std::optional<std::filesystem::path>
    find_retail_audio_root() {
        if (const auto *configured = std::getenv("SMGPC_RETAIL_FILES_ROOT");
            configured != nullptr && configured[0] != '\0') {
            for (const auto &candidate : {
                     std::filesystem::path(configured) / "KrKorean" / "AudioRes",
                     std::filesystem::path(configured) / "AudioRes",
                 }) {
                if (std::filesystem::is_regular_file(candidate / "SMR.szs")) {
                    return candidate;
                }
            }
        }

        auto directory = std::filesystem::current_path();
        while (true) {
            const auto candidate = directory / "orig" / "RMGK02" / "files" /
                                   "KrKorean" / "AudioRes";
            if (std::filesystem::is_regular_file(candidate / "SMR.szs")) {
                return candidate;
            }
            const auto parent = directory.parent_path();
            if (parent.empty() || parent == directory) {
                break;
            }
            directory = parent;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<std::uint8_t>
    pcm16_little_endian(const aurora::audio::PcmLayer &layer) {
        auto bytes = std::vector<std::uint8_t>{};
        bytes.reserve(layer.samples->size() * 2U);
        for (const auto sample : *layer.samples) {
            const auto signed_value = static_cast<std::int16_t>(
                std::lround(sample * 32768.0F));
            const auto value = static_cast<std::uint16_t>(signed_value);
            bytes.push_back(static_cast<std::uint8_t>(value));
            bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
        }
        return bytes;
    }

    struct RetailAudioFixture {
        std::shared_ptr<const std::vector<std::uint8_t>> baa;
        std::shared_ptr<const std::vector<std::uint8_t>> wave_archive;
        std::filesystem::path localized_audio_root;
    };

    constexpr std::uint32_t fourcc(char a, char b, char c, char d) {
        return static_cast<std::uint32_t>(static_cast<unsigned char>(a)) << 24U |
               static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 16U |
               static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 8U |
               static_cast<std::uint32_t>(static_cast<unsigned char>(d));
    }

    [[nodiscard]] std::uint32_t read_be32(
        std::span<const std::uint8_t> bytes, std::size_t offset) {
        require(offset <= bytes.size() && bytes.size() - offset >= 4U,
                "test fixture big-endian read must stay in range");
        return static_cast<std::uint32_t>(bytes[offset]) << 24U |
               static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U |
               static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U |
               static_cast<std::uint32_t>(bytes[offset + 3U]);
    }

    void write_be32(std::span<std::uint8_t> bytes, std::size_t offset,
                    std::uint32_t value) {
        require(offset <= bytes.size() && bytes.size() - offset >= 4U,
                "test fixture big-endian write must stay in range");
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    struct TestSegment {
        std::size_t offset = 0;
        std::size_t size = 0;
    };

    template <typename Visitor>
    void visit_baa_commands(std::span<const std::uint8_t> baa,
                            Visitor &&visitor) {
        require(read_be32(baa, 0U) == fourcc('A', 'A', '_', '<'),
                "retail fixture must begin with a BAA header");
        auto cursor = std::size_t{4U};
        while (true) {
            const auto command = read_be32(baa, cursor);
            cursor += 4U;
            if (command == fourcc('>', '_', 'A', 'A')) {
                return;
            }
            visitor(command, cursor);
            if (command == fourcc('w', 's', ' ', ' ') ||
                command == fourcc('b', 'm', 's', ' ')) {
                cursor += 12U;
            } else if (command == fourcc('b', 'n', 'k', ' ') ||
                       command == fourcc('b', 's', 'c', ' ') ||
                       command == fourcc('b', 's', 't', ' ') ||
                       command == fourcc('b', 's', 't', 'n') ||
                       command == fourcc('b', 'l', '_', '<') ||
                       command == fourcc('v', 'b', 'n', 'k')) {
                cursor += 8U;
            } else if (command == fourcc('>', '_', 'b', 'l')) {
                continue;
            } else if (command == fourcc('b', 'm', 's', 'a') ||
                       command == fourcc('d', 's', 'q', 'b') ||
                       command == fourcc('b', 's', 'f', 't') ||
                       command == fourcc('s', 'e', 'c', 't')) {
                cursor += 4U;
            } else {
                throw std::runtime_error(
                    "test BAA command walker found an unknown retail command");
            }
        }
    }

    [[nodiscard]] TestSegment find_baa_table_segment(
        std::span<const std::uint8_t> baa, std::uint32_t wanted) {
        auto result = TestSegment{};
        visit_baa_commands(baa, [&](std::uint32_t command, std::size_t args) {
            if (command == wanted) {
                const auto begin = read_be32(baa, args);
                const auto end = read_be32(baa, args + 4U);
                require(end >= begin, "retail BAA table segment must not be backwards");
                result = TestSegment{begin, end - begin};
            }
        });
        require(result.size != 0U, "retail BAA table segment must be present");
        return result;
    }

    struct TestBankSegment {
        TestSegment data;
        std::uint32_t wave_bank_index = 0;
    };

    [[nodiscard]] TestBankSegment find_ibnk_segment(
        std::span<const std::uint8_t> baa, std::uint32_t bank_number) {
        auto result = TestBankSegment{};
        visit_baa_commands(baa, [&](std::uint32_t command, std::size_t args) {
            if (command != fourcc('b', 'n', 'k', ' ')) {
                return;
            }
            const auto base = static_cast<std::size_t>(read_be32(baa, args + 4U));
            if (read_be32(baa, base + 8U) == bank_number) {
                result = TestBankSegment{
                    .data = TestSegment{base, read_be32(baa, base + 4U)},
                    .wave_bank_index = read_be32(baa, args),
                };
            }
        });
        require(result.data.size != 0U, "retail IBNK bank must be present");
        return result;
    }

    [[nodiscard]] TestSegment find_wsys_segment(
        std::span<const std::uint8_t> baa, std::uint32_t wanted_index) {
        auto result = TestSegment{};
        visit_baa_commands(baa, [&](std::uint32_t command, std::size_t args) {
            if (command == fourcc('w', 's', ' ', ' ') &&
                read_be32(baa, args) == wanted_index) {
                const auto base =
                    static_cast<std::size_t>(read_be32(baa, args + 4U));
                result = TestSegment{base, read_be32(baa, base + 4U)};
            }
        });
        require(result.size != 0U, "retail WSYS bank must be present");
        return result;
    }

    [[nodiscard]] std::optional<RetailAudioFixture>
    load_retail_audio_fixture() {
        const auto root = find_retail_audio_root();
        if (!root.has_value()) {
            return std::nullopt;
        }
        const auto smr = read_file(*root / "SMR.szs");
        const auto wave = read_file(*root / "Waves" / "B64kawa_0.aw");
        require(smr.size() == 196608U &&
                    sha256_hex(smr) ==
                        "a9f7d2f7828052098a1828b1cd172f301f2b2091526dd207dde154b59f9f49aa",
                "RMGK02 SMR.szs bytes must match the audited retail fixture");
        require(wave.size() == 4770848U &&
                    sha256_hex(wave) ==
                        "088f96c7ac62ace546a5f3234cfcdd4d2a2b478b8a2629e3195a1f608efcbbc8",
                "RMGK02 B64kawa_0.aw bytes must match the audited retail fixture");
        auto baa = smgpc::resource::decompress_yaz0(smr);
        require(baa.size() == 596320U &&
                    sha256_hex(baa) ==
                        "57b8e3e2af9d48a9a3c3e86a56b9f0502cd3de98ab990ad7cc5fe9826a43a575",
                "decompressed SMR BAA must match the audited retail bytes");
        return RetailAudioFixture{
            .baa = std::make_shared<const std::vector<std::uint8_t>>(
                std::move(baa)),
            .wave_archive =
                std::make_shared<const std::vector<std::uint8_t>>(wave),
            .localized_audio_root = *root,
        };
    }

    [[nodiscard]] std::vector<std::uint8_t> load_fixture_wave(
        const RetailAudioFixture &fixture, std::string_view name) {
        if (name == "B64kawa_0.aw") {
            return *fixture.wave_archive;
        }
        const auto filename = std::filesystem::path(name);
        require(!filename.empty() && !filename.is_absolute() &&
                    filename.filename() == filename,
                "retail fixture AW name must be a plain filename");
        const auto disc_root =
            fixture.localized_audio_root.parent_path().parent_path();
        for (const auto &candidate : {
                 fixture.localized_audio_root / "Waves" / filename,
                 disc_root / "AudioRes" / "Waves" / filename,
             }) {
            if (std::filesystem::is_regular_file(candidate)) {
                return read_file(candidate);
            }
        }
        throw std::runtime_error(
            "retail fixture is missing referenced AW archive " +
            std::string(name));
    }

    [[nodiscard]] std::unique_ptr<aurora::audio::JAudioSoundArchive>
    make_archive(const RetailAudioFixture &fixture) {
        return std::make_unique<aurora::audio::JAudioSoundArchive>(
            *fixture.baa,
            [fixture](std::string_view name) {
                return load_fixture_wave(fixture, name);
            });
    }

    [[nodiscard]] std::unique_ptr<aurora::audio::JAudioSoundArchive>
    make_archive(std::span<const std::uint8_t> baa,
                 const RetailAudioFixture &fixture) {
        return std::make_unique<aurora::audio::JAudioSoundArchive>(
            baa, [fixture](std::string_view name) {
                return load_fixture_wave(fixture, name);
            });
    }

    [[nodiscard]] TestSegment find_ibnk_chunk(
        std::span<const std::uint8_t> baa, TestSegment bank,
        std::uint32_t wanted_chunk) {
        auto cursor = bank.offset + 0x20U;
        const auto end = bank.offset + bank.size;
        while (cursor + 8U <= end) {
            const auto id = read_be32(baa, cursor);
            const auto payload_size =
                static_cast<std::size_t>(read_be32(baa, cursor + 4U));
            require(payload_size <= end - (cursor + 8U),
                    "retail IBNK chunk must stay inside its bank");
            if (id == wanted_chunk) {
                return TestSegment{cursor + 8U, payload_size};
            }
            cursor = (cursor + 11U + payload_size) & ~std::size_t{3U};
        }
        throw std::runtime_error("retail IBNK chunk must be present");
    }

    void test_afc_decoder_matches_dolphin_oracle() {
        constexpr auto encoded = std::array<std::uint8_t, 9U>{
            0x54U, 0x17U, 0x8fU, 0x2eU, 0xd3U,
            0x40U, 0xb5U, 0x6aU, 0x9cU,
        };
        constexpr auto expected = std::array<std::int16_t, 16U>{
            3067,  5124,  6925,  8694,  10527, 12296, 13969, 15738,
            17635, 19532, 21269, 23166, 25255, 27152, 28825, 30370,
        };
        const auto decoded = aurora::audio::decode_jaudio_afc_hq(
            encoded, expected.size(),
            aurora::audio::AfcState{
                .previous_sample = 1234,
                .older_sample = -567,
            });
        require(std::ranges::equal(decoded.samples, expected) &&
                    decoded.final_state.previous_sample == 30370 &&
                    decoded.final_state.older_sample == 28825,
                "AFC HQ predictor output must match Dolphin's DecodeAFC recurrence");
        require_throws<std::invalid_argument>(
            [] {
                constexpr auto short_block = std::array<std::uint8_t, 8U>{};
                (void)aurora::audio::decode_jaudio_afc_hq(short_block, 16U);
            },
            "shorter", "AFC HQ must reject an eight-byte DSP-style frame");

        constexpr auto exponent_15 = std::array<std::uint8_t, 9U>{
            0xf0U, 0x1fU, 0x1fU, 0x1fU, 0x1fU,
            0x1fU, 0x1fU, 0x1fU, 0x1fU,
        };
        const auto wrapped = aurora::audio::decode_jaudio_afc_hq(
            exponent_15, 16U);
        for (auto index = std::size_t{0}; index < wrapped.samples.size(); ++index) {
            require(wrapped.samples[index] ==
                        ((index & 1U) == 0U ? -32768 : 32767),
                    "AFC exponent 15 must use Dolphin's signed-s16 delta");
        }
    }

    void test_retail_archive_metadata_and_pcm(const RetailAudioFixture &fixture) {
        auto archive = make_archive(fixture);
        require(archive->find_sound_id("SE_AT_LV_ASTRO_DOME_WIND_1") ==
                    0x0006001aU &&
                    archive->find_sound_id("SE_AT_LV_ASTRO_DOME_WIND_2") ==
                    0x0006001bU &&
                    !archive->find_sound_id("SE_AT_LV_NOT_A_RETAIL_SOUND").has_value(),
                "BSTN must dynamically resolve the two SphereSelector level sounds");

        const auto wind1 = archive->resolve_persistent_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1");
        const auto wind2 = archive->resolve_persistent_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_2");
        require(wind1.has_value() && wind2.has_value() &&
                    wind1->priority == 128U && wind1->table_volume == 80U &&
                    wind2->priority == 128U && wind2->table_volume == 40U &&
                    wind1->layers.size() == 2U && wind2->layers.size() == 2U,
                "BST/BSC must resolve the exact two-layer retail recipes");

        const auto &w1a = wind1->layers[0];
        const auto &w1b = wind1->layers[1];
        const auto &w2a = wind2->layers[0];
        const auto &w2b = wind2->layers[1];
        require(w1a.bank == 64U && w1a.program == 134U && w1a.note == 48U &&
                    w1a.velocity == 110U && w1a.wave_id == 140U &&
                    w1a.wave_archive_offset == 0x246e00U &&
                    w1a.encoded_length == 0x49efU &&
                    w1a.decoded_loop_start == 17U &&
                    w1a.decoded_loop_end == 33639U &&
                    w1a.source_sample_rate == 16000U &&
                    w1a.direct_release_ticks == 600U &&
                    w1a.loop_history_yn1 == 12288 &&
                    w1a.loop_history_yn2 == 12288,
                "wind-1 primary layer must match retail BSC/IBNK/WSYS metadata");
        require(w1b.bank == 64U && w1b.program == 80U && w1b.note == 60U &&
                    w1b.velocity == 60U && w1b.wave_id == 43U &&
                    w1b.wave_archive_offset == 0x0c3f00U &&
                    w1b.encoded_length == 0x8121U &&
                    w1b.decoded_loop_start == 214U &&
                    w1b.decoded_loop_end == 58761U &&
                    w1b.source_sample_rate == 16000U &&
                    w1b.direct_release_ticks == 600U &&
                    w1b.loop_history_yn1 == 7059 &&
                    w1b.loop_history_yn2 == 3987,
                "wind-1 delayed layer must match retail BSC/IBNK/WSYS metadata");
        require(w2a.wave_id == 140U && w2a.note == 77U &&
                    w2a.decoded_loop_end == 33639U &&
                    w2b.wave_id == 41U && w2b.note == 77U &&
                    w2b.wave_archive_offset == 0x0b0da0U &&
                    w2b.encoded_length == 0xb007U &&
                    w2b.decoded_loop_start == 142U &&
                    w2b.decoded_loop_end == 80103U &&
                    w2b.source_sample_rate == 22050U &&
                    w2b.loop_history_yn1 == -8986 &&
                    w2b.loop_history_yn2 == -8856,
                "wind-2 layers must match retail BSC/IBNK/WSYS metadata");

        const auto &w1_voice_a = wind1->voice.layers[0];
        const auto &w1_voice_b = wind1->voice.layers[1];
        const auto &w2_voice_a = wind2->voice.layers[0];
        const auto &w2_voice_b = wind2->voice.layers[1];
        require_near(w1_voice_a.pitch_ratio, 0.5F, 0.000001F,
                     "wind-1 primary bank pitch must be exact");
        require_near(w1_voice_b.pitch_ratio, 1.0F, 0.000001F,
                     "wind-1 delayed bank pitch must be exact");
        require_near(w2_voice_a.pitch_ratio, 2.6696796F, 0.00001F,
                     "wind-2 primary bank pitch must be exact");
        require_near(w2_voice_b.pitch_ratio, 0.6674199F, 0.00001F,
                     "wind-2 delayed bank pitch must be exact");
        require_near(static_cast<float>(w1_voice_a.attack_seconds),
                     235.0F / 600.0F, 0.000001F,
                     "program 134 attack must come from its IBNK oscillator");
        require_near(static_cast<float>(w1_voice_b.attack_seconds),
                     123.0F / 600.0F, 0.000001F,
                     "program 80 attack must come from its IBNK oscillator");
        require_near(static_cast<float>(w1_voice_b.start_delay_seconds),
                     1.0F / 6.0F, 0.000001F,
                     "child-track wait must use the retail tempo/timebase");
        require_near(static_cast<float>(w1_voice_a.release_seconds), 1.0F,
                     0.000001F,
                     "direct release 600 must produce a one-second envelope");

        require(sha256_hex(pcm16_little_endian(w1_voice_a)) ==
                    "fcea2c737cc5d2a9d6bd5180ce0276df967718b66bc07111ad7a1f87eacaa2f1" &&
                    sha256_hex(pcm16_little_endian(w1_voice_b)) ==
                    "97c1ca349eea02ff337b6edc203add1f9fbde183b04d6dad53cec171974cfbad" &&
                    sha256_hex(pcm16_little_endian(w2_voice_a)) ==
                    "fcea2c737cc5d2a9d6bd5180ce0276df967718b66bc07111ad7a1f87eacaa2f1" &&
                    sha256_hex(pcm16_little_endian(w2_voice_b)) ==
                    "d8bb87c35e7d6b6a5a0960c525b583dd8ab12c84ab1ca6ec037d22a3ea5a9a1d",
                "decoded PCM16 must match independent retail AFC SHA-256 oracles");
    }

    void test_retail_title_and_picturebook_recipes(
        const RetailAudioFixture &fixture) {
        auto archive = make_archive(fixture);
        for (const auto &expected : {
                 std::tuple{"STM_TITLE", 0x02000001U,
                            std::string_view{"SMG_title_strm.ast"}},
                 std::tuple{"STM_PROLOGUE_01", 0x0200001fU,
                            std::string_view{"SMG_ev_prolo01_strm.ast"}},
                 std::tuple{"STM_PROLOGUE_01_B", 0x02000046U,
                            std::string_view{"SMG_ev_prolo01_b_strm.ast"}},
                 std::tuple{"STM_PROLOGUE_02", 0x02000020U,
                            std::string_view{"SMG_ev_prolo02_strm.ast"}},
             }) {
            const auto metadata = archive->resolve_sound(std::get<0>(expected));
            if (!metadata.has_value() ||
                metadata->sound_id != std::get<1>(expected) ||
                metadata->kind != aurora::audio::JAudioSoundKind::Stream ||
                !metadata->stream_path.ends_with(std::get<2>(expected))) {
                throw std::runtime_error(
                    std::string("unexpected retail stream metadata for ") +
                    std::get<0>(expected) +
                    (metadata.has_value()
                         ? " id=" + std::to_string(metadata->sound_id) +
                               " path=" + metadata->stream_path
                         : " (name absent)"));
            }
        }

        const auto disc_root =
            fixture.localized_audio_root.parent_path().parent_path();
        auto impossible_stream = read_file(
            disc_root / "AudioRes" / "Stream" / "SMG_title_strm.ast");
        write_be32(impossible_stream, 0x14U,
                   std::numeric_limits<std::uint32_t>::max());
        require_throws<std::runtime_error>(
            [&] {
                (void)aurora::audio::decode_jaudio_stream(
                    impossible_stream, 0U);
            },
            "cannot fit in the block payload",
            "a tiny malformed STRM header must reject an impossible sample count before allocation");

        constexpr auto finite_names = std::array{
            "SE_SY_GAME_START",
            "SE_SY_TALK_FOCUS_ITEM",
            "SE_SY_PICTUREBOOK_NEXT_ST",
            "SE_SY_PICBOOK_CONTENTS_CUR",
            "SE_SY_TALK_OK",
            "SE_SY_PICTUREBOOK_NEXT_F_ST",
            "SE_SY_PICTUREBOOK_NEXT_F_ED",
            "SE_SY_PICTUREBOOK_NEXT_ED",
            "SE_SY_PICTUREBOOK_END",
            "SE_SY_GALAXY_DECIDE_CANCEL",
            "SE_SY_GALAXY_SELECTED",
            "SE_SY_BUTTON_CURSOR_ON",
            "SE_SY_LETTER_APPEAR",
            "SE_SV_PEACH_OPENING_LETTER",
            "SE_DM_ARRIVE_CASTLE_STAR",
            "SE_DM_ASTRO_HANDLE_GRAB",
        };
        for (const auto *name : finite_names) {
            try {
                const auto recipe = archive->resolve_sound_effect(name);
                require(recipe.has_value() && !recipe->voice.layers.empty() &&
                            recipe->voice.layers.size() == recipe->layers.size(),
                        "reachable system SE must resolve a concrete finite PCM recipe");
                for (std::size_t index = 0; index < recipe->voice.layers.size();
                     ++index) {
                    const auto &layer = recipe->voice.layers[index];
                    require(layer.samples != nullptr && !layer.samples->empty() &&
                                layer.sample_rate != 0U,
                            "finite JAudio layer must own decoded retail PCM");
                    require(layer.loop_end == 0U ||
                                (layer.gate_seconds > 0.0 &&
                                 layer.release_seconds >= 0.0),
                            "a looping finite layer must have a scheduled retail gate");
                }
            } catch (const std::exception &error) {
                throw std::runtime_error(std::string(name) + ": " + error.what());
            }
        }

        const auto game_start =
            archive->resolve_sound_effect("SE_SY_GAME_START");
        require(game_start.has_value() && game_start->sound_id == 0x20U &&
                    game_start->layers.size() == 21U &&
                    std::ranges::any_of(game_start->layers, [](const auto &layer) {
                        return layer.bank == 65U;
                    }),
                "Title start must include every retail bank-64/bank-65 note");
        const auto focus =
            archive->resolve_sound_effect("SE_SY_TALK_FOCUS_ITEM");
        require(focus.has_value() && focus->sound_id == 0x36U &&
                    focus->layers.size() == 4U,
                "PictureBook focus must include its four-note retail recipe");
        const auto peach =
            archive->resolve_sound_effect("SE_SV_PEACH_OPENING_LETTER");
        require(peach.has_value() && peach->layers.size() == 2U &&
                    peach->layers[0].waits_for_sample_completion &&
                    peach->layers[1].waits_for_sample_completion &&
                    std::abs(peach->layers[1].start_delay_seconds -
                             2.0752333333333333) < 0.00001,
                "Peach letter note two must wait for note one's natural sample lifetime");
        const auto arrive =
            archive->resolve_sound_effect("SE_DM_ARRIVE_CASTLE_STAR");
        const auto grab =
            archive->resolve_sound_effect("SE_DM_ASTRO_HANDLE_GRAB");
        const auto selected =
            archive->resolve_sound_effect("SE_SY_GALAXY_SELECTED");
        if (!(arrive.has_value() && arrive->sound_id == 0x00070016U &&
              arrive->layers.size() == 1U && grab.has_value() &&
              grab->sound_id == 0x00070005U && grab->layers.size() == 2U &&
              std::ranges::all_of(grab->layers, [](const auto &layer) {
                  return layer.bank == 88U;
              }) && selected.has_value() && selected->sound_id == 0x5eU &&
              selected->layers.size() == 17U)) {
            const auto shape = [](const auto &recipe) {
                if (!recipe.has_value()) {
                    return std::string{"absent"};
                }
                return std::to_string(recipe->sound_id) + "/" +
                       std::to_string(recipe->layers.size());
            };
            throw std::runtime_error(
                "unexpected audited finite recipe shape: arrive=" +
                shape(arrive) + ", grab=" + shape(grab) +
                ", selected=" + shape(selected));
        }
    }

    void test_archive_rejects_cross_segment_references(
        const RetailAudioFixture &fixture) {
        constexpr auto wind_name = "SE_AT_LV_ASTRO_DOME_WIND_1";
        const auto bstn = find_baa_table_segment(
            *fixture.baa, fourcc('b', 's', 't', 'n'));
        {
            auto corrupted = *fixture.baa;
            write_be32(corrupted, bstn.offset + 12U,
                       static_cast<std::uint32_t>(bstn.size));
            auto archive = make_archive(corrupted, fixture);
            require_throws<std::runtime_error>(
                [&] { (void)archive->find_sound_id(wind_name); },
                "invalid BSTN root",
                "BSTN references must not escape their declared segment");
        }

        const auto bst = find_baa_table_segment(
            *fixture.baa, fourcc('b', 's', 't', ' '));
        {
            auto corrupted = *fixture.baa;
            write_be32(corrupted, bst.offset + 12U,
                       static_cast<std::uint32_t>(bst.size));
            auto archive = make_archive(corrupted, fixture);
            require_throws<std::runtime_error>(
                [&] { (void)archive->resolve_persistent_sound(wind_name); },
                "invalid BST root",
                "BST references must not escape their declared segment");
        }

        const auto bsc = find_baa_table_segment(
            *fixture.baa, fourcc('b', 's', 'c', ' '));
        const auto wind_sequence_entry = [&](const auto &bytes) {
            const auto group = bsc.offset +
                               read_be32(bytes,
                                         bsc.offset + 8U + 6U * 4U);
            return bsc.offset +
                   read_be32(bytes, group + 4U + 0x1aU * 4U);
        };

        {
            auto corrupted = *fixture.baa;
            write_be32(corrupted, bsc.offset + 8U + 6U * 4U,
                       static_cast<std::uint32_t>(bsc.size));
            auto archive = make_archive(corrupted, fixture);
            require_throws<std::runtime_error>(
                [&] { (void)archive->resolve_persistent_sound(wind_name); },
                "invalid BSC group offset",
                "BSC references must not escape their declared segment");
        }

        const auto bank = find_ibnk_segment(*fixture.baa, 64U);
        {
            auto corrupted = *fixture.baa;
            const auto list = find_ibnk_chunk(
                corrupted, bank.data, fourcc('L', 'I', 'S', 'T'));
            write_be32(corrupted, list.offset + 4U + 134U * 4U,
                       static_cast<std::uint32_t>(bank.data.size));
            auto archive = make_archive(corrupted, fixture);
            require_throws<std::runtime_error>(
                [&] { (void)archive->resolve_persistent_sound(wind_name); },
                "invalid IBNK instrument offset",
                "IBNK references must not escape their declared bank");
        }

        {
            auto corrupted = *fixture.baa;
            const auto wsys =
                find_wsys_segment(corrupted, bank.wave_bank_index);
            write_be32(corrupted, wsys.offset + 16U,
                       static_cast<std::uint32_t>(wsys.size));
            auto archive = make_archive(corrupted, fixture);
            require_throws<std::runtime_error>(
                [&] { (void)archive->resolve_persistent_sound(wind_name); },
                "invalid WSYS archive-bank offset",
                "WSYS references must not escape their declared wave bank");
        }

        {
            auto corrupted = *fixture.baa;
            const auto cursor = wind_sequence_entry(corrupted);
            require(cursor <= bsc.offset + bsc.size &&
                        bsc.offset + bsc.size - cursor >= 6U,
                    "shift-corruption fixture must stay inside its BSC segment");
            corrupted[cursor] = 0xdaU;
            corrupted[cursor + 1U] = 9U;
            corrupted[cursor + 2U] = 0U;
            corrupted[cursor + 3U] = 0U;
            corrupted[cursor + 4U] = 16U;
            corrupted[cursor + 5U] = 0xffU;
            auto archive = make_archive(corrupted, fixture);
            require_throws<std::runtime_error>(
                [&] { (void)archive->resolve_sound_effect(wind_name); },
                "shift operand exceeds",
                "a BSC register shift must reject operands outside its 16-bit register width");
        }

        {
            auto corrupted = *fixture.baa;
            const auto cursor = wind_sequence_entry(corrupted);
            require(cursor <= bsc.offset + bsc.size &&
                        bsc.offset + bsc.size - cursor >= 11U,
                    "gate-overflow fixture must stay inside its BSC segment");
            corrupted[cursor] = 0xd8U;
            corrupted[cursor + 1U] = 0x65U;
            corrupted[cursor + 2U] = 0xffU;
            corrupted[cursor + 3U] = 0xffU;
            corrupted[cursor + 4U] = 0x3cU;
            corrupted[cursor + 5U] = 0U;
            corrupted[cursor + 6U] = 0x7fU;
            corrupted[cursor + 7U] = 0x84U;
            corrupted[cursor + 8U] = 0x80U;
            corrupted[cursor + 9U] = 0x02U;
            corrupted[cursor + 10U] = 0xffU;
            auto archive = make_archive(corrupted, fixture);
            require_throws<std::runtime_error>(
                [&] { (void)archive->resolve_sound_effect(wind_name); },
                "gate duration overflows",
                "a BSC note gate must reject duration/rate products outside the JAS tick range");
        }

        {
            auto corrupted = *fixture.baa;
            const auto group = bsc.offset +
                               read_be32(corrupted,
                                         bsc.offset + 8U + 6U * 4U);
            auto cursor = bsc.offset +
                          read_be32(corrupted,
                                    group + 4U + 0x1aU * 4U);
            auto prior_instructions = std::size_t{0U};
            while (true) {
                const auto command = corrupted[cursor];
                if (command == 0xffU) {
                    break;
                }
                require(command == 0xc1U || command == 0xc3U,
                        "retail BSC root must contain only audited commands");
                cursor += command == 0xc1U ? 5U : 4U;
                ++prior_instructions;
            }
            const auto replacement_count = 32U - prior_instructions;
            require(cursor <= bsc.offset + bsc.size &&
                        replacement_count * 4U <=
                            bsc.offset + bsc.size - cursor,
                    "test BSC root replacement must stay inside its segment");
            for (auto index = std::size_t{0}; index < replacement_count;
                 ++index) {
                corrupted[cursor++] = 0xc3U;
                corrupted[cursor++] = 0U;
                corrupted[cursor++] = 0U;
                corrupted[cursor++] = 0U;
            }
            auto archive = make_archive(corrupted, fixture);
            require_throws<std::runtime_error>(
                [&] { (void)archive->resolve_persistent_sound(wind_name); },
                "lacks an end command",
                "a BSC root without 0xff must fail at its instruction bound");
        }
    }

    void test_deterministic_mixer_release() {
        auto samples = std::make_shared<const std::vector<float>>(
            std::initializer_list<float>{0.25F, -0.25F, 0.5F, -0.5F});
        auto mixer = aurora::audio::PcmAudioMixer(8U);
        const auto token = mixer.start_voice(aurora::audio::PcmVoiceSpec{
            .layers = {aurora::audio::PcmLayer{
                .samples = samples,
                .sample_rate = 4U,
                .loop_start = 0U,
                .loop_end = samples->size(),
                .gain = 1.0F,
                .pitch_ratio = 1.0F,
                .pan = 0.5F,
                .release_seconds = 0.25,
            }},
            .pitch_multiplier = 1.0F,
        });
        auto output = std::array<float, 16U>{};
        mixer.render_interleaved(output);
        require(token && mixer.is_voice_active(token) &&
                    std::ranges::any_of(output, [](float value) {
                        return std::abs(value) > 0.00001F;
                    }),
                "offline mixer path must render a concrete nonzero looping voice");
        require(mixer.try_update_voice(token, 0.75F, 1.5F) &&
                    mixer.voice_gain_multiplier(token) == 0.75F &&
                    mixer.voice_pitch_multiplier(token) == 1.5F,
                "one atomic update must mutate both live voice parameters");
        mixer.release_voice(token);
        mixer.render_interleaved(output);
        require(!mixer.is_voice_active(token) &&
                    !mixer.voice_pitch_multiplier(token).has_value() &&
                    !mixer.try_update_voice(token, 1.0F, 1.0F),
                "release completion must be reclaimed off-callback and reject stale updates");
    }

    void test_recovered_parameter_semantics() {
        const auto low =
            smgpc::compat::resolve_jaudio_sound_parameter_adjustment(
                0x0006001aU, -20, 99);
        const auto high =
            smgpc::compat::resolve_jaudio_sound_parameter_adjustment(
                0x0006001aU, 120, -99);
        const auto identity =
            smgpc::compat::resolve_jaudio_sound_parameter_adjustment(
                0x0006001bU, 12345, -12345);
        require(low.has_value() && low->gain_multiplier == 1.0F &&
                    low->pitch_multiplier == 1.0F && high.has_value() &&
                    high->gain_multiplier == 1.0F &&
                    high->pitch_multiplier == 1.5F,
                "Kawamura pitch policy must clamp outside parameter 1 to 0..100");
        require(identity.has_value() && identity->gain_multiplier == 1.0F &&
                    identity->pitch_multiplier == 1.0F &&
                    !smgpc::compat::resolve_jaudio_sound_parameter_adjustment(
                         0x0006001cU, 0, 0)
                         .has_value(),
                "audited identity must ignore both parameters and unknown IDs must stay absent");
    }

    void set_audio_driver(std::string_view name) {
#if defined(_WIN32)
        if (_putenv_s("SDL_AUDIODRIVER", std::string(name).c_str()) != 0) {
            throw std::runtime_error("Could not select SDL audio driver");
        }
#else
        if (setenv("SDL_AUDIODRIVER", std::string(name).c_str(), 1) != 0) {
            throw std::runtime_error("Could not select SDL audio driver");
        }
#endif
    }

    void test_missing_device_fails_explicitly() {
        set_audio_driver("smgpc-intentionally-missing-audio-driver");
        auto mixer = aurora::audio::PcmAudioMixer{};
        require_throws<std::runtime_error>(
            [&] { mixer.open_default_playback(); },
            "SDL audio initialization failed",
            "an unavailable SDL driver must fail instead of opening silent playback");
        require(!mixer.is_device_open(),
                "a failed SDL open must not report an attached playback device");
    }

    void test_production_rejects_dummy_sink() {
        set_audio_driver("dummy");
        auto mixer = aurora::audio::PcmAudioMixer{};
        require_throws<std::runtime_error>(
            [&] { mixer.open_default_playback(); }, "not an audible",
            "production playback must reject SDL's non-audible dummy sink");
        require(!mixer.is_device_open(),
                "a rejected dummy sink must not report an open playback device");
    }

    void test_missing_device_fails_in_fresh_process(
        const std::filesystem::path &executable) {
        const auto command = '"' + executable.string() +
                             "\" --missing-device-probe";
        require(std::system(command.c_str()) == 0,
                "fresh-process missing-device probe must fail explicitly");
        const auto dummy_command = '"' + executable.string() +
                                   "\" --dummy-production-probe";
        require(std::system(dummy_command.c_str()) == 0,
                "fresh-process production dummy-sink probe must fail explicitly");
    }

    void test_real_stream_handle_and_lifetime(const RetailAudioFixture &fixture) {
        set_audio_driver("dummy");
        auto factory = [fixture] { return make_archive(fixture); };
        auto mixer = std::make_unique<aurora::audio::PcmAudioMixer>(
            48000U,
            aurora::audio::PlaybackDevicePolicy::AllowExplicitTestSink);
        auto *mixer_observer = mixer.get();
        const auto disc_root =
            fixture.localized_audio_root.parent_path().parent_path();
        auto service = smgpc::runtime::JAudioPlaybackService(
            std::move(factory),
            [disc_root](std::string_view path) {
                auto relative = std::filesystem::path(path);
                if (relative.is_absolute()) {
                    relative = relative.relative_path();
                }
                return read_file(disc_root / relative);
            },
            std::move(mixer));

        auto *title = service.start_stage_bgm("STM_TITLE", true);
        const auto prepared_stop_token = aurora::audio::VoiceToken{
            service.stage_bgm_backend_token()};
        require(title != nullptr && title->isSoundAttached() &&
                    title->backendOwner() == &service &&
                    prepared_stop_token &&
                    service.stage_bgm_id() == 0x02000001U &&
                    service.is_stage_bgm_prepared() &&
                    service.is_stage_bgm_paused() &&
                    service.active_voice_count() == 1U,
                "prepared Title BGM must own a paused concrete retail stream token");
        const auto paused_callback_deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (service.playback_stats().device_callbacks == 0U &&
               std::chrono::steady_clock::now() < paused_callback_deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        require(service.playback_stats().device_callbacks != 0U &&
                    mixer_observer->voice_rendered_frames(prepared_stop_token) ==
                        0U,
                "prepared stream callbacks must not advance the paused backend token");
        service.pause_stage_bgm(true);
        service.pause_stage_bgm(false);
        require(service.is_stage_bgm_prepared() &&
                    service.is_stage_bgm_paused() &&
                    mixer_observer->voice_rendered_frames(prepared_stop_token) ==
                        0U,
                "clearing host pause must not bypass a prepared stream's unlock gate");

        constexpr auto prepared_stop_fade_frames = 6U;
        service.stop_stage_bgm(prepared_stop_fade_frames);
        require(service.is_stage_bgm_stopping() &&
                    !service.is_stage_bgm_prepared() &&
                    !service.is_stage_bgm_paused() &&
                    title->isSoundAttached(),
                "stopping a prepared stream must release the host pause and retain its handle through the fade");
        const auto prepared_stop_deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (mixer_observer->is_voice_active(prepared_stop_token) &&
               std::chrono::steady_clock::now() < prepared_stop_deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        require(!mixer_observer->is_voice_active(prepared_stop_token),
                "a prepared stream stopped before unlock must retire its concrete token");
        service.begin_frame(0U);
        service.end_frame();
        require(!title->isSoundAttached() &&
                    !service.has_active_stage_bgm(),
                "a prepared stream stopped before unlock must detach after retirement");

        title = service.start_stage_bgm("STM_TITLE", true);
        const auto title_token = aurora::audio::VoiceToken{
            service.stage_bgm_backend_token()};
        require(title != nullptr && title->isSoundAttached() && title_token &&
                    title_token != prepared_stop_token &&
                    service.is_stage_bgm_prepared() &&
                    service.is_stage_bgm_paused(),
                "a new prepared stream must own a fresh paused backend token");
        service.pause_stage_bgm(true);
        service.unlock_stage_bgm();
        require(!service.is_stage_bgm_prepared() &&
                    service.is_stage_bgm_paused() &&
                    mixer_observer->voice_rendered_frames(title_token) == 0U,
                "Title unlock must preserve an independent host pause");
        service.pause_stage_bgm(false);
        require(!service.is_stage_bgm_paused(),
                "clearing host pause after unlock must resume the concrete stream");
        const auto unlocked_deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (mixer_observer->voice_rendered_frames(title_token) == 0U &&
               std::chrono::steady_clock::now() < unlocked_deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        require(mixer_observer->voice_rendered_frames(title_token).value_or(0U) !=
                    0U,
                "unlock must advance the same prepared stream token");

        constexpr auto title_fade_frames = 12U;
        service.pause_stage_bgm(true);
        require(service.is_stage_bgm_paused(),
                "explicit stage pause must freeze the active stream before stop");
        service.stop_stage_bgm(title_fade_frames);
        require(service.is_stage_bgm_stopping() && title->isSoundAttached() &&
                    service.has_active_stage_bgm() &&
                    !service.is_stage_bgm_paused(),
                "a nonzero Title fade must release an explicit host pause and remain attached while stopping");
        const auto fade_deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (mixer_observer->is_voice_active(title_token) &&
               std::chrono::steady_clock::now() < fade_deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        require(!mixer_observer->is_voice_active(title_token),
                "a nonzero stage-BGM fade must retire its mixer token");
        service.begin_frame(1U);
        service.end_frame();
        require(!title->isSoundAttached() &&
                    !service.has_active_stage_bgm() &&
                    service.active_voice_count() == 0U,
                "completed stage-BGM fade must detach its concrete stream handle");

        for (const auto &prologue_stream : {
                 std::pair{"STM_PROLOGUE_01", 0x0200001fU},
                 std::pair{"STM_PROLOGUE_02", 0x02000020U},
             }) {
            auto *prologue =
                service.start_stage_bgm(prologue_stream.first, false);
            const auto prologue_token = aurora::audio::VoiceToken{
                service.stage_bgm_backend_token()};
            require(prologue != nullptr && prologue->isSoundAttached() &&
                        prologue->backendOwner() == &service &&
                        prologue_token &&
                        service.stage_bgm_id() == prologue_stream.second &&
                        !service.is_stage_bgm_prepared() &&
                        !service.is_stage_bgm_paused(),
                    std::string("Prologue route must decode and start its concrete retail stream: ") +
                        prologue_stream.first);
            const auto prologue_deadline =
                std::chrono::steady_clock::now() + std::chrono::seconds(3);
            while (mixer_observer->voice_rendered_frames(prologue_token) ==
                       0U &&
                   std::chrono::steady_clock::now() < prologue_deadline) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            require(mixer_observer->voice_rendered_frames(prologue_token)
                            .value_or(0U) != 0U,
                    std::string("Prologue route stream must advance on the concrete mixer: ") +
                        prologue_stream.first);
            service.stop_stage_bgm(0U);
            require(!prologue->isSoundAttached() &&
                        !service.has_active_stage_bgm(),
                    std::string("Prologue route stream must detach on immediate teardown: ") +
                        prologue_stream.first);
        }

        constexpr auto prologue_effects = std::array{
            "SE_SY_LETTER_APPEAR",
            "SE_SV_PEACH_OPENING_LETTER",
            "SE_SY_TALK_OK",
            "SE_SY_TALK_FOCUS_ITEM",
            "SE_DM_ARRIVE_CASTLE_STAR",
        };
        for (const auto *name : prologue_effects) {
            auto *effect = service.start_sound_effect(name, -1, -1);
            require(effect != nullptr && effect->isSoundAttached() &&
                        effect->backendOwner() == &service &&
                        effect->backendToken() != 0U &&
                        service.active_voice_count() == 1U,
                    std::string("Prologue finite SE must start a concrete mixer voice: ") +
                        name);
            service.stop_sound_effect(name, 0U);
            require(!effect->isSoundAttached() &&
                        service.active_voice_count() == 0U,
                    std::string("Prologue finite SE must detach on teardown: ") +
                        name);
        }

        auto *game_start = service.start_sound_effect(
            "SE_SY_GAME_START", -1, -1);
        require(game_start != nullptr && game_start->isSoundAttached() &&
                    game_start->backendOwner() == &service &&
                    game_start->backendToken() != 0U &&
                    service.active_voice_count() == 1U,
                "Title confirm must return a handle attached to real finite PCM");
        service.stop_sound_effect("SE_SY_GAME_START", 0U);
        require(!game_start->isSoundAttached() &&
                    service.active_voice_count() == 0U,
                "system-SE stop-by-name must retire every matching concrete token");
        require_throws<std::logic_error>(
            [&] {
                (void)service.start_sound_effect(
                    "SE_SY_GAME_START", 10, -1);
            },
            "Parameterized",
            "unproven one-shot parameters must fail instead of changing fake state");

        service.set_level_sound_permitted(false);
        require(!service.is_level_sound_permitted() &&
                    service.start_level_sound(
                        "SE_AT_LV_ASTRO_DOME_WIND_1", 0, -1) == nullptr &&
                    service.active_voice_count() == 0U,
                "submitted level sounds must suppress allocation without a logical-only event");
        service.set_level_sound_permitted(true);
        require(service.is_level_sound_permitted(),
                "permitting level sounds must restore concrete allocation eligibility");

        require_throws<std::invalid_argument>(
            [&] {
                (void)service.start_level_sound(
                    "SE_AT_LV_NOT_A_RETAIL_SOUND", 0, -1);
            },
            "absent", "an unknown JAudio sound name must fail explicitly");
        require_throws<std::logic_error>(
            [&] {
                (void)service.start_level_sound(
                    "SE_AT_LV_ASTRO_PATH_APPEAR", -1, -1);
            },
            "semantics have not been proven",
            "a known but unaudited sound ID must fail before playback");

        service.begin_frame(2U);
        auto *wind1 = service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1", 100, -1);
        require(wind1 != nullptr && wind1->isSoundAttached() &&
                    wind1->backendOwner() == &service &&
                    wind1->backendToken() != 0U &&
                    service.active_voice_count() == 1U &&
                    service.is_device_open(),
                "a level call must return a handle attached to a resumed SDL voice");
        const auto first_wind1_token = wind1->backendToken();
        require(mixer_observer->voice_pitch_multiplier(
                    aurora::audio::VoiceToken{first_wind1_token}) == 1.5F,
                "RMGK02 wind-1 parameter 100 must apply the retail 1.5 pitch");

        auto *same_wind1 = service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1", 0, -1);
        require(same_wind1 == wind1 &&
                    same_wind1->backendToken() == first_wind1_token &&
                    service.active_voice_count() == 1U &&
                    mixer_observer->voice_pitch_multiplier(
                        aurora::audio::VoiceToken{first_wind1_token}) == 1.0F,
                "same-ID refresh must reuse one token and update its live pitch");

        auto *wind2 = service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_2", -1, -1);
        require(wind2 != nullptr && wind2 != wind1 &&
                    wind2->backendToken() != first_wind1_token &&
                    service.active_voice_count() == 2U,
                "the second retail level sound must own a distinct backend voice");
        service.end_frame();

        const auto callback_deadline = std::chrono::steady_clock::now() +
                                       std::chrono::seconds(3);
        while (std::chrono::steady_clock::now() < callback_deadline) {
            const auto stats = service.playback_stats();
            if (stats.device_callbacks != 0U && stats.nonzero_samples != 0U) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        const auto callback_stats = service.playback_stats();
        require(callback_stats.device_callbacks != 0U &&
                    callback_stats.nonzero_samples != 0U &&
                    callback_stats.mixed_frames != 0U,
                "SDL dummy device must invoke the real callback and consume nonzero retail PCM");
        std::cout << "[info] SDL default(dummy) callbacks="
                  << callback_stats.device_callbacks
                  << ";mixed_frames=" << callback_stats.mixed_frames
                  << ";nonzero_samples=" << callback_stats.nonzero_samples
                  << '\n';

        // Refresh wind 1, but omit wind 2. Its one-second direct release must
        // run to completion and detach only after the backend token is gone.
        const auto dying_wind2_token = wind2->backendToken();
        const auto attack_deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (mixer_observer->voice_rendered_frames(
                   aurora::audio::VoiceToken{dying_wind2_token})
                   .value_or(0U) < 24000U &&
               std::chrono::steady_clock::now() < attack_deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        require(mixer_observer->voice_rendered_frames(
                    aurora::audio::VoiceToken{dying_wind2_token})
                    .value_or(0U) >= 24000U,
                "release test must advance past the retail attack before synchronizing the callback");
        mixer_observer->set_voice_paused(
            aurora::audio::VoiceToken{dying_wind2_token}, true);
        service.begin_frame(3U);
        (void)service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1", 40, -1);
        service.end_frame();
        service.begin_frame(4U);
        auto *same_dying_wind2 = service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_2", -1, -1);
        if (!(same_dying_wind2 == wind2 &&
              wind2->backendToken() == dying_wind2_token &&
              service.active_voice_count() == 2U)) {
            throw std::runtime_error(
                "a retail level-sound call during release must return the same dying handle: same=" +
                std::to_string(same_dying_wind2 == wind2) +
                ";old-token=" + std::to_string(dying_wind2_token) +
                ";new-token=" + std::to_string(wind2->backendToken()) +
                ";active=" + std::to_string(service.active_voice_count()));
        }
        mixer_observer->set_voice_paused(
            aurora::audio::VoiceToken{dying_wind2_token}, false);
        (void)service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1", 40, -1);
        service.end_frame();

        const auto release_deadline = std::chrono::steady_clock::now() +
                                      std::chrono::seconds(3);
        auto frame = std::uint64_t{5U};
        while (wind2->isSoundAttached() &&
               std::chrono::steady_clock::now() < release_deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            service.begin_frame(frame++);
            (void)service.start_level_sound(
                "SE_AT_LV_ASTRO_DOME_WIND_1", 40, -1);
            service.end_frame();
        }
        require(!wind2->isSoundAttached() && wind1->isSoundAttached() &&
                    service.active_voice_count() == 1U,
                "missed refresh must finish retail release despite later calls to the dying handle");

        service.begin_frame(frame++);
        auto *restarted_wind2 = service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_2", -1, -1);
        (void)service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1", 40, -1);
        require(restarted_wind2 == wind2 &&
                    restarted_wind2->backendToken() != 0U &&
                    restarted_wind2->backendToken() != dying_wind2_token &&
                    service.active_voice_count() == 2U,
                "the stable handle must attach to a new token only after retail release ends");
        const auto before_reset_wind1_token = wind1->backendToken();
        const auto before_reset_wind2_token = wind2->backendToken();

        // Stage teardown can occur inside RuntimeContext::begin_frame. It must
        // clear voices without breaking the matching end_frame transaction.
        service.reset_scene();
        require(!wind1->isSoundAttached() && !wind2->isSoundAttached() &&
                    service.active_voice_count() == 0U,
                "scene teardown must stop voices and detach every real handle");
        service.end_frame();

        service.begin_frame(frame++);
        auto *reentered_wind1 = service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1", 40, -1);
        auto *reentered_wind2 = service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_2", -1, -1);
        require(reentered_wind1 == wind1 && reentered_wind2 == wind2 &&
                    wind1->backendToken() != 0U &&
                    wind1->backendToken() != before_reset_wind1_token &&
                    wind2->backendToken() != 0U &&
                    wind2->backendToken() != before_reset_wind2_token &&
                    service.active_voice_count() == 2U,
                "scene re-entry must reuse stable handles with fresh backend tokens");
        service.end_frame();

        auto removed = SDL_Event{};
        removed.type = SDL_EVENT_AUDIO_DEVICE_REMOVED;
        removed.adevice.which = 0x1234U;
        removed.adevice.recording = false;
        require(SDL_PushEvent(&removed) && !service.is_device_open(),
                "a playback-device removal must make the output terminally unhealthy");
        require_throws<std::runtime_error>(
            [&] {
                (void)service.start_level_sound(
                    "SE_AT_LV_ASTRO_DOME_WIND_1", 40, -1);
            },
            "stopped accepting mixed audio",
            "device loss must fail a refresh instead of feeding SDL's silent zombie");
        service.reset_scene();
    }

    void test_runtime_sound_util_backend_binding(
        const RetailAudioFixture &fixture) {
        set_audio_driver("dummy");
        auto factory = [fixture] { return make_archive(fixture); };
        const auto disc_root =
            fixture.localized_audio_root.parent_path().parent_path();
        auto playback = std::make_unique<smgpc::runtime::JAudioPlaybackService>(
            std::move(factory),
            [disc_root](std::string_view path) {
                auto relative = std::filesystem::path(path);
                if (relative.is_absolute()) {
                    relative = relative.relative_path();
                }
                return read_file(disc_root / relative);
            },
            std::make_unique<aurora::audio::PcmAudioMixer>(
                48000U,
                aurora::audio::PlaybackDevicePolicy::AllowExplicitTestSink));
        auto *playback_observer = playback.get();
        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 320,
            .height = 228,
            .title = "SMG PC JAudio runtime binding proof",
        });
        auto runtime = smgpc::runtime::RuntimeContext(
            *logger, window, std::move(playback));

        const auto system_event_count = runtime.audio().events().size();
        auto *game_start = MR::startSystemSE("SE_SY_GAME_START", -1, -1);
        require(game_start != nullptr && game_start->isSoundAttached() &&
                    game_start->backendOwner() == playback_observer &&
                    game_start->backendToken() != 0U &&
                    runtime.audio().events().size() == system_event_count + 1U &&
                    runtime.audio().events().back().kind ==
                        smgpc::runtime::AudioEventKind::SystemSoundStart,
                "SoundUtil system-SE must return a concrete RuntimeContext backend handle before recording telemetry");
        MR::stopSystemSE("SE_SY_GAME_START", 0U);
        require(!game_start->isSoundAttached(),
                "SoundUtil stop-by-name must detach its concrete system-SE handle");

        MR::submitLevelSE();
        const auto submitted_event_count = runtime.audio().events().size();
        require(MR::startSystemLevelSE(
                    "SE_AT_LV_ASTRO_DOME_WIND_1", 0, -1) == nullptr &&
                    runtime.audio().events().size() == submitted_event_count,
                "SoundUtil submit must suppress a real level allocation without emitting a fake start event");
        MR::permitLevelSE();
        auto *wind = MR::startSystemLevelSE(
            "SE_AT_LV_ASTRO_DOME_WIND_1", 0, -1);
        require(wind != nullptr && wind->isSoundAttached() &&
                    wind->backendOwner() == playback_observer &&
                    runtime.audio().events().back().kind ==
                        smgpc::runtime::AudioEventKind::SystemLevelSoundStart,
                "SoundUtil permit must restore a concrete level-sound allocation and only then record the event");

        auto *title = MR::startStageBGM("STM_TITLE", true);
        require(title != nullptr && title->isSoundAttached() &&
                    title->backendOwner() == playback_observer &&
                    runtime.audio().has_active_stage_bgm() &&
                    playback_observer->has_active_stage_bgm() &&
                    runtime.audio().current_stage_bgm_id() ==
                        playback_observer->stage_bgm_id(),
                "SoundUtil stage start must keep logical identity and concrete backend token consistent");
        smgpc::compat::synchronize_audio_facade_state();
        auto *stage_bgm = AudWrap::getStageBgm();
        auto *facade_handle = stage_bgm != nullptr
                                  ? stage_bgm->getHandle()
                                  : nullptr;
        require(facade_handle != nullptr && facade_handle->isSoundAttached() &&
                    facade_handle->backendOwner() == playback_observer &&
                    facade_handle->backendToken() == title->backendToken(),
                "AudWrap must expose the same concrete RuntimeContext stage token");
        require_throws<std::logic_error>(
            [&] { stage_bgm->mTrackController[0].mute(); },
            "active JAudio track muting",
            "active track mutation must fail until it controls concrete backend layers");
        require_throws<std::logic_error>(
            [] { AudWrap::setNextIdStageBgm(0x0200001fU); },
            "next-BGM scheduler",
            "next-BGM queueing must fail until the concrete retail scheduler exists");

        MR::unlockStageBGM();
        runtime.begin_frame(smgpc::render::FrameContext{
            .frame_index = 1U,
            .frame_time_seconds = 1.0 / 60.0,
            .frame_delta_seconds = 1.0 / 60.0,
            .framebuffer = {.width = 320U, .height = 228U},
        });
        require(playback_observer->has_active_stage_bgm() &&
                    facade_handle->isSoundAttached(),
                "a normal RuntimeContext frame must preserve an active default track controller and stage token");
        constexpr auto fade_frames = 6U;
        stage_bgm->stop(fade_frames);
        require(!runtime.audio().has_active_stage_bgm() &&
                    playback_observer->has_active_stage_bgm() &&
                    playback_observer->is_stage_bgm_stopping(),
                "logical stop and concrete fade state must describe the same in-progress teardown");
        const auto fade_deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (playback_observer->has_active_stage_bgm() &&
               std::chrono::steady_clock::now() < fade_deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        require(!playback_observer->has_active_stage_bgm() &&
                    !playback_observer->stage_bgm_id().has_value(),
                "RuntimeContext stage fade must reach backend-token retirement");
        auto *manager = AudWrap::getBgmMgr();
        runtime.begin_frame(smgpc::render::FrameContext{
            .frame_index = 2U,
            .frame_time_seconds = 2.0 / 60.0,
            .frame_delta_seconds = 1.0 / 60.0,
            .framebuffer = {.width = 320U, .height = 228U},
        });
        require(manager->mBgm[AudBgmMgr::BgmType_Stage] == nullptr &&
                    manager->mKeeper.mSingleBgmActiveFlags == 0U,
                "post-fade manager movement must release the detached BGM keeper object");
    }

    void test_runtime_sound_util_backend_binding_in_fresh_process(
        const std::filesystem::path &executable) {
        const auto command = "xvfb-run -a \"" + executable.string() +
                             "\" --runtime-audio-binding-probe";
        require(std::system(command.c_str()) == 0,
                "fresh-process RuntimeContext/SoundUtil binding proof must pass under Xvfb");
    }

    void test_logical_actor_and_sub_bgm_requests() {
        auto audio = smgpc::runtime::AudioEventService{};
        const auto binding =
            smgpc::compat::ScopedAudioEventServiceOverride{audio};
        auto actor = LiveActor{"LogicalAudioProbe"};
        audio.begin_frame(50U);

        require(MR::startSound(&actor, "SE_SM_RABBIT_JUMP", 12, 34) ==
                    nullptr,
                "logical actor sound must not manufacture a concrete positional handle");
        const auto &actor_sound = audio.events().back();
        require(actor_sound.kind ==
                        smgpc::runtime::AudioEventKind::ActorSoundStart &&
                    actor_sound.name == "SE_SM_RABBIT_JUMP" &&
                    actor_sound.source_identity == &actor &&
                    actor_sound.source_name == "LogicalAudioProbe" &&
                    actor_sound.parameter_1 == 12 &&
                    actor_sound.parameter_2 == 34 &&
                    actor_sound.parameter_3 == -1 &&
                    actor_sound.frame_index == 50U,
                "logical actor sound must retain its source, retail name, parameters, and frame");

        require(MR::startLevelSound(
                    &actor, "SE_SM_LV_TICO_FLOAT", 56, 78, 90) == nullptr,
                "logical actor level sound must not manufacture a concrete positional handle");
        const auto &actor_level_sound = audio.events().back();
        require(actor_level_sound.kind ==
                        smgpc::runtime::AudioEventKind::ActorLevelSoundStart &&
                    actor_level_sound.name == "SE_SM_LV_TICO_FLOAT" &&
                    actor_level_sound.source_identity == &actor &&
                    actor_level_sound.source_name == "LogicalAudioProbe" &&
                    actor_level_sound.parameter_1 == 56 &&
                    actor_level_sound.parameter_2 == 78 &&
                    actor_level_sound.parameter_3 == 90,
                "logical actor level sound must preserve all three retail request parameters");
        const auto first_level_event_count = audio.events().size();
        require(MR::startLevelSound(
                    &actor, "SE_SM_LV_TICO_FLOAT", 23, 45, 67) == nullptr &&
                    audio.events().size() == first_level_event_count &&
                    audio.events().back().parameter_1 == 23 &&
                    audio.events().back().parameter_2 == 45 &&
                    audio.events().back().parameter_3 == 67,
                "same-frame actor level refreshes must coalesce by actor and retail sound identity");
        audio.begin_frame(51U);
        require(MR::startLevelSound(
                    &actor, "SE_SM_LV_TICO_FLOAT", 23, 45, 67) == nullptr &&
                    audio.events().size() == first_level_event_count + 1U &&
                    audio.events().back().frame_index == 51U,
                "a new frame must retain a new logical actor level refresh");

        MR::limitedSound("SE_SM_LV_TICO_OOP_WAIT", 1);
        const auto &limited_sound = audio.events().back();
        require(limited_sound.kind ==
                        smgpc::runtime::AudioEventKind::LimitedSoundRegister &&
                    limited_sound.name == "SE_SM_LV_TICO_OOP_WAIT" &&
                    limited_sound.parameter_1 == 1,
                "limitedSound must remain a non-throwing logical registration with retail identity");

        require(!audio.has_active_stage_bgm(),
                "the logical audio probe must begin without stage BGM state");
        audio.begin_frame(60U);
        require(MR::startSubBGM("BGM_MEET_TICO_ZOOM_OUT", false) == nullptr &&
                    audio.has_active_sub_bgm() &&
                    !audio.is_sub_bgm_stopping() &&
                    !audio.is_sub_bgm_prepared() &&
                    audio.current_sub_bgm_name() ==
                        "BGM_MEET_TICO_ZOOM_OUT" &&
                    audio.sub_bgm_fade_frames_remaining() == 0U &&
                    !audio.has_active_stage_bgm(),
                "logical sub-BGM must occupy a separate non-SDL slot without changing stage BGM");
        const auto &sub_start = audio.events().back();
        require(sub_start.kind ==
                        smgpc::runtime::AudioEventKind::SubBgmStart &&
                    sub_start.name == "BGM_MEET_TICO_ZOOM_OUT" &&
                    !sub_start.prepared,
                "logical sub-BGM start evidence must retain name and preparation mode");

        MR::stopSubBGM(4U);
        const auto &sub_stop = audio.events().back();
        require(audio.has_active_sub_bgm() && audio.is_sub_bgm_stopping() &&
                    audio.current_sub_bgm_name() ==
                        "BGM_MEET_TICO_ZOOM_OUT" &&
                    audio.sub_bgm_fade_frames_remaining() == 4U &&
                    sub_stop.kind ==
                        smgpc::runtime::AudioEventKind::SubBgmStop &&
                    sub_stop.name == "BGM_MEET_TICO_ZOOM_OUT" &&
                    sub_stop.fade_frames == 4,
                "logical sub-BGM stop must expose its retained identity and fade state");
        audio.begin_frame(63U);
        require(audio.has_active_sub_bgm() && audio.is_sub_bgm_stopping() &&
                    audio.sub_bgm_fade_frames_remaining() == 1U,
                "logical sub-BGM fade must advance on the scene frame clock");
        audio.begin_frame(64U);
        require(!audio.has_active_sub_bgm() &&
                    !audio.is_sub_bgm_stopping() &&
                    audio.current_sub_bgm_name().empty() &&
                    audio.sub_bgm_fade_frames_remaining() == 0U,
                "logical sub-BGM fade must retire at its exact frame boundary");

        require(MR::startSubBGM("BGM_MUTEKI_A", true) == nullptr &&
                    audio.is_sub_bgm_prepared(),
                "a replacement logical sub-BGM must retain its preparation parameter");
        MR::stopSubBGM(0U);
        require(!audio.has_active_sub_bgm() &&
                    !audio.is_sub_bgm_stopping() &&
                    !audio.is_sub_bgm_prepared(),
                "a zero-frame logical sub-BGM stop must retire immediately");

        const auto retained_before = audio.events().size();
        const auto dropped_before = audio.dropped_event_count();
        const auto expected_trim =
            smgpc::runtime::AudioEventService::cEventRetentionLimit / 2U;
        require(retained_before < expected_trim,
                "logical audio retention probe setup must fit inside one trim window");
        for (auto index = std::size_t{};
             index <= smgpc::runtime::AudioEventService::cEventRetentionLimit;
             ++index) {
            audio.start_actor_sound(
                &actor, actor.getName(), "SE_SM_RABBIT_JUMP",
                static_cast<s32>(index), -1);
        }
        require(audio.events().size() ==
                        retained_before +
                            smgpc::runtime::AudioEventService::cEventRetentionLimit +
                            1U - expected_trim &&
                    audio.dropped_event_count() ==
                        dropped_before + expected_trim &&
                    audio.events().front().kind ==
                        smgpc::runtime::AudioEventKind::ActorSoundStart &&
                    audio.events().front().parameter_1 ==
                        static_cast<s32>(expected_trim - retained_before) &&
                    audio.events().back().parameter_1 == static_cast<s32>(
                        smgpc::runtime::AudioEventService::cEventRetentionLimit),
                "logical audio history must trim the oldest half in order while retaining the newest request and cumulative drop evidence");
    }

} // namespace

int main(int argc, char **argv) {
    try {
        if (argc == 2 &&
            std::string_view(argv[1]) == "--missing-device-probe") {
            test_missing_device_fails_explicitly();
            return 0;
        }
        if (argc == 2 &&
            std::string_view(argv[1]) == "--dummy-production-probe") {
            test_production_rejects_dummy_sink();
            return 0;
        }
        if (argc == 2 &&
            std::string_view(argv[1]) ==
                "--runtime-audio-binding-probe") {
            const auto fixture = load_retail_audio_fixture();
            require(fixture.has_value(),
                    "RuntimeContext audio binding probe requires the retail fixture");
            test_runtime_sound_util_backend_binding(*fixture);
            return 0;
        }
        constexpr auto abc = std::array<std::uint8_t, 3U>{'a', 'b', 'c'};
        require(sha256_hex(abc) ==
                    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                "test SHA-256 implementation must pass its standard oracle");
        test_afc_decoder_matches_dolphin_oracle();
        test_deterministic_mixer_release();
        test_recovered_parameter_semantics();
        test_missing_device_fails_in_fresh_process(argv[0]);
        test_logical_actor_and_sub_bgm_requests();

        const auto fixture = load_retail_audio_fixture();
        if (!fixture.has_value()) {
            std::cout << "[skip] retail RMGK02 atmosphere audio proof "
                         "(set SMGPC_RETAIL_FILES_ROOT or retain orig/RMGK02/files)\n";
            return 0;
        }
        test_retail_archive_metadata_and_pcm(*fixture);
        test_retail_title_and_picturebook_recipes(*fixture);
        test_archive_rejects_cross_segment_references(*fixture);
        test_real_stream_handle_and_lifetime(*fixture);
        test_runtime_sound_util_backend_binding_in_fresh_process(argv[0]);
        std::cout << "[ok] retail AFC metadata/PCM, SDL callback, handle, and lifetime proof\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] JAudio playback: " << error.what() << '\n';
        return 1;
    }
}
