#include "compat/JAudioLevelSoundArchive.hpp"
#include "compat/JAudioSoundParameterSemantics.hpp"
#include "resource/Yaz0.hpp"
#include "runtime/AtmosphereLevelSoundService.hpp"

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
    pcm16_little_endian(const aurora::audio::LoopingPcmLayer &layer) {
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
        };
    }

    [[nodiscard]] std::unique_ptr<smgpc::compat::JAudioLevelSoundArchive>
    make_archive(const RetailAudioFixture &fixture) {
        return std::make_unique<smgpc::compat::JAudioLevelSoundArchive>(
            *fixture.baa,
            [wave = fixture.wave_archive](std::string_view name) {
                if (name != "B64kawa_0.aw") {
                    throw std::runtime_error(
                        "test fixture was asked for an unaudited AW archive");
                }
                return *wave;
            });
    }

    [[nodiscard]] std::unique_ptr<smgpc::compat::JAudioLevelSoundArchive>
    make_archive(std::span<const std::uint8_t> baa,
                 const RetailAudioFixture &fixture) {
        return std::make_unique<smgpc::compat::JAudioLevelSoundArchive>(
            baa, [wave = fixture.wave_archive](std::string_view name) {
                if (name != "B64kawa_0.aw") {
                    throw std::runtime_error(
                        "test fixture was asked for an unaudited AW archive");
                }
                return *wave;
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

        const auto wind1 = archive->resolve_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1");
        const auto wind2 = archive->resolve_level_sound(
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
                [&] { (void)archive->resolve_level_sound(wind_name); },
                "invalid BST root",
                "BST references must not escape their declared segment");
        }

        const auto bsc = find_baa_table_segment(
            *fixture.baa, fourcc('b', 's', 'c', ' '));

        {
            auto corrupted = *fixture.baa;
            write_be32(corrupted, bsc.offset + 8U + 6U * 4U,
                       static_cast<std::uint32_t>(bsc.size));
            auto archive = make_archive(corrupted, fixture);
            require_throws<std::runtime_error>(
                [&] { (void)archive->resolve_level_sound(wind_name); },
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
                [&] { (void)archive->resolve_level_sound(wind_name); },
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
                [&] { (void)archive->resolve_level_sound(wind_name); },
                "invalid WSYS archive-bank offset",
                "WSYS references must not escape their declared wave bank");
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
                [&] { (void)archive->resolve_level_sound(wind_name); },
                "lacks an end command",
                "a BSC root without 0xff must fail at its instruction bound");
        }
    }

    void test_deterministic_mixer_release() {
        auto samples = std::make_shared<const std::vector<float>>(
            std::initializer_list<float>{0.25F, -0.25F, 0.5F, -0.5F});
        auto mixer = aurora::audio::LoopingAudioMixer(8U);
        const auto token = mixer.start_voice(aurora::audio::LoopingVoiceSpec{
            .layers = {aurora::audio::LoopingPcmLayer{
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
        auto mixer = aurora::audio::LoopingAudioMixer{};
        require_throws<std::runtime_error>(
            [&] { mixer.open_default_playback(); },
            "SDL audio initialization failed",
            "an unavailable SDL driver must fail instead of opening silent playback");
        require(!mixer.is_device_open(),
                "a failed SDL open must not report an attached playback device");
    }

    void test_production_rejects_dummy_sink() {
        set_audio_driver("dummy");
        auto mixer = aurora::audio::LoopingAudioMixer{};
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
        auto mixer = std::make_unique<aurora::audio::LoopingAudioMixer>(
            48000U,
            aurora::audio::PlaybackDevicePolicy::AllowExplicitTestSink);
        auto *mixer_observer = mixer.get();
        auto service = smgpc::runtime::AtmosphereLevelSoundService(
            std::move(factory), std::move(mixer));

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

        service.begin_frame(1U);
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
        service.begin_frame(2U);
        (void)service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1", 40, -1);
        service.end_frame();
        const auto dying_wind2_token = wind2->backendToken();
        service.begin_frame(3U);
        auto *same_dying_wind2 = service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_2", -1, -1);
        require(same_dying_wind2 == wind2 &&
                    wind2->backendToken() == dying_wind2_token &&
                    service.active_voice_count() == 2U,
                "a retail level-sound call during release must return the same dying handle");
        (void)service.start_level_sound(
            "SE_AT_LV_ASTRO_DOME_WIND_1", 40, -1);
        service.end_frame();

        const auto release_deadline = std::chrono::steady_clock::now() +
                                      std::chrono::seconds(3);
        auto frame = std::uint64_t{4U};
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
        constexpr auto abc = std::array<std::uint8_t, 3U>{'a', 'b', 'c'};
        require(sha256_hex(abc) ==
                    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                "test SHA-256 implementation must pass its standard oracle");
        test_afc_decoder_matches_dolphin_oracle();
        test_deterministic_mixer_release();
        test_recovered_parameter_semantics();
        test_missing_device_fails_in_fresh_process(argv[0]);

        const auto fixture = load_retail_audio_fixture();
        if (!fixture.has_value()) {
            std::cout << "[skip] retail RMGK02 atmosphere audio proof "
                         "(set SMGPC_RETAIL_FILES_ROOT or retain orig/RMGK02/files)\n";
            return 0;
        }
        test_retail_archive_metadata_and_pcm(*fixture);
        test_archive_rejects_cross_segment_references(*fixture);
        test_real_stream_handle_and_lifetime(*fixture);
        std::cout << "[ok] retail AFC metadata/PCM, SDL callback, handle, and lifetime proof\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] atmosphere level sound: " << error.what() << '\n';
        return 1;
    }
}
