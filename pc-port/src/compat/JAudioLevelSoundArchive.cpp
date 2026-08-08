#include "compat/JAudioLevelSoundArchive.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

namespace smgpc::compat {
    namespace {
        constexpr std::uint32_t fourcc(char a, char b, char c, char d) {
            return static_cast<std::uint32_t>(static_cast<unsigned char>(a)) << 24U |
                   static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 16U |
                   static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 8U |
                   static_cast<std::uint32_t>(static_cast<unsigned char>(d));
        }

        [[noreturn]] void malformed(std::string_view detail) {
            throw std::runtime_error("Malformed JAudio level-sound resource: " +
                                     std::string(detail));
        }

        class Reader final {
        public:
            explicit Reader(std::span<const std::uint8_t> bytes)
                : Reader(bytes, 0U, bytes.size()) {}

            Reader(std::span<const std::uint8_t> bytes, std::size_t begin,
                   std::size_t length)
                : _bytes(bytes), _begin(begin), _end(begin) {
                if (begin > bytes.size() || length > bytes.size() - begin) {
                    malformed("resource segment extends outside its containing archive");
                }
                _end = begin + length;
            }

            [[nodiscard]] std::size_t size() const { return _end - _begin; }

            void require(std::size_t offset, std::size_t length,
                         std::string_view detail) const {
                if (offset < _begin || offset > _end || length > _end - offset) {
                    malformed(detail);
                }
            }

            [[nodiscard]] std::uint8_t u8(std::size_t offset,
                                          std::string_view detail) const {
                require(offset, 1U, detail);
                return _bytes[offset];
            }

            [[nodiscard]] std::uint16_t u16(std::size_t offset,
                                            std::string_view detail) const {
                require(offset, 2U, detail);
                return static_cast<std::uint16_t>(_bytes[offset]) << 8U |
                       static_cast<std::uint16_t>(_bytes[offset + 1U]);
            }

            [[nodiscard]] std::int16_t s16(std::size_t offset,
                                           std::string_view detail) const {
                return std::bit_cast<std::int16_t>(u16(offset, detail));
            }

            [[nodiscard]] std::uint32_t u24(std::size_t offset,
                                            std::string_view detail) const {
                require(offset, 3U, detail);
                return static_cast<std::uint32_t>(_bytes[offset]) << 16U |
                       static_cast<std::uint32_t>(_bytes[offset + 1U]) << 8U |
                       static_cast<std::uint32_t>(_bytes[offset + 2U]);
            }

            [[nodiscard]] std::uint32_t u32(std::size_t offset,
                                            std::string_view detail) const {
                require(offset, 4U, detail);
                return static_cast<std::uint32_t>(_bytes[offset]) << 24U |
                       static_cast<std::uint32_t>(_bytes[offset + 1U]) << 16U |
                       static_cast<std::uint32_t>(_bytes[offset + 2U]) << 8U |
                       static_cast<std::uint32_t>(_bytes[offset + 3U]);
            }

            [[nodiscard]] float f32(std::size_t offset,
                                    std::string_view detail) const {
                return std::bit_cast<float>(u32(offset, detail));
            }

            [[nodiscard]] std::span<const std::uint8_t>
            slice(std::size_t offset, std::size_t length,
                  std::string_view detail) const {
                require(offset, length, detail);
                return _bytes.subspan(offset, length);
            }

            [[nodiscard]] std::string string(std::size_t offset,
                                             std::size_t maximum_length,
                                             std::string_view detail) const {
                require(offset, maximum_length, detail);
                const auto region = _bytes.subspan(offset, maximum_length);
                const auto terminator = std::ranges::find(region, std::uint8_t{0});
                if (terminator == region.end()) {
                    malformed(detail);
                }
                return std::string(reinterpret_cast<const char *>(region.data()),
                                   static_cast<std::size_t>(terminator - region.begin()));
            }

        private:
            std::span<const std::uint8_t> _bytes;
            std::size_t _begin = 0;
            std::size_t _end = 0;
        };

        struct Segment {
            std::size_t offset = 0;
            std::size_t size = 0;
        };

        struct BankRecord {
            std::uint32_t wave_bank_index = 0;
            Segment data;
        };

        struct TrackRecipe {
            std::uint8_t bank = 0;
            std::uint8_t program = 0;
            std::uint8_t note = 0;
            std::uint8_t velocity = 0;
            std::uint16_t direct_release = 0;
            double start_delay_seconds = 0.0;
        };

        struct InstrumentRecipe {
            std::uint16_t wave_id = 0;
            float volume = 1.0F;
            float pitch = 1.0F;
            double attack_seconds = 0.0;
            aurora::audio::EnvelopeCurve attack_curve =
                aurora::audio::EnvelopeCurve::Linear;
            float attack_peak = 1.0F;
        };

        struct WaveRecipe {
            std::string archive_name;
            std::uint8_t format = 0;
            std::uint8_t base_key = 60;
            float sample_rate = 0.0F;
            std::uint32_t archive_offset = 0;
            std::uint32_t archive_length = 0;
            std::uint32_t loop_start = 0;
            std::uint32_t loop_end = 0;
            std::uint32_t sample_count = 0;
            std::int16_t loop_yn1 = 0;
            std::int16_t loop_yn2 = 0;
        };

        [[nodiscard]] std::size_t checked_add(std::size_t a, std::size_t b,
                                              std::string_view detail) {
            if (b > std::numeric_limits<std::size_t>::max() - a) {
                malformed(detail);
            }
            return a + b;
        }

        [[nodiscard]] std::size_t checked_multiply(
            std::size_t a, std::size_t b, std::string_view detail) {
            if (a != 0U && b > std::numeric_limits<std::size_t>::max() / a) {
                malformed(detail);
            }
            return a * b;
        }

        void require_table(const Reader &reader, std::size_t offset,
                           std::size_t count, std::size_t stride,
                           std::string_view detail) {
            reader.require(offset, checked_multiply(count, stride, detail),
                           detail);
        }

        [[nodiscard]] std::size_t relative(std::size_t base, std::uint32_t offset,
                                           const Reader &reader,
                                           std::string_view detail) {
            const auto result = checked_add(base, static_cast<std::size_t>(offset), detail);
            reader.require(result, 1U, detail);
            return result;
        }

        [[nodiscard]] std::pair<std::uint32_t, std::size_t>
        read_midi_value(const Reader &reader, std::size_t cursor) {
            auto value = std::uint32_t{0};
            for (auto index = 0U; index != 4U; ++index) {
                const auto byte = reader.u8(cursor++, "truncated BSC MIDI value");
                value = value << 7U | static_cast<std::uint32_t>(byte & 0x7fU);
                if ((byte & 0x80U) == 0U) {
                    return {value, cursor};
                }
            }
            malformed("BSC MIDI value exceeds JASSeqReader's four-byte limit");
        }

        [[nodiscard]] aurora::audio::EnvelopeCurve curve_from_jaudio(
            std::int16_t curve) {
            switch (curve) {
            case 0:
                return aurora::audio::EnvelopeCurve::Linear;
            case 3:
                return aurora::audio::EnvelopeCurve::JAudioSampleCell;
            default:
                malformed("unsupported JAudio oscillator curve");
            }
        }

    } // namespace

    struct JAudioLevelSoundArchive::Impl {
        explicit Impl(std::span<const std::uint8_t> data,
                      WaveArchiveLoader loader)
            : baa(data.begin(), data.end()), wave_loader(std::move(loader)) {
            if (!wave_loader) {
                throw std::invalid_argument("JAudio wave archive loader is required");
            }
            parse_baa();
        }

        void parse_baa() {
            const auto reader = Reader{baa};
            if (reader.u32(0U, "missing BAA header") != fourcc('A', 'A', '_', '<')) {
                malformed("invalid BAA header");
            }

            auto cursor = std::size_t{4};
            while (true) {
                const auto command = reader.u32(cursor, "truncated BAA command");
                cursor += 4U;
                if (command == fourcc('>', '_', 'A', 'A')) {
                    break;
                }
                switch (command) {
                case fourcc('w', 's', ' ', ' '): {
                    const auto index = reader.u32(cursor, "truncated BAA WS command");
                    const auto offset = reader.u32(cursor + 4U, "truncated BAA WS command");
                    (void)reader.u32(cursor + 8U, "truncated BAA WS command");
                    const auto absolute = relative(0U, offset, reader, "invalid WSYS offset");
                    const auto size = reader.u32(absolute + 4U, "truncated WSYS header");
                    reader.require(absolute, size, "WSYS extends outside BAA");
                    wave_banks[index] = Segment{absolute, size};
                    cursor += 12U;
                    break;
                }
                case fourcc('b', 'n', 'k', ' '): {
                    const auto wave_bank = reader.u32(cursor, "truncated BAA BNK command");
                    const auto offset = reader.u32(cursor + 4U, "truncated BAA BNK command");
                    const auto absolute = relative(0U, offset, reader, "invalid IBNK offset");
                    const auto size = reader.u32(absolute + 4U, "truncated IBNK header");
                    reader.require(absolute, size, "IBNK extends outside BAA");
                    const auto internal_number =
                        reader.u32(absolute + 8U, "truncated IBNK bank number");
                    banks[internal_number] = BankRecord{wave_bank, {absolute, size}};
                    cursor += 8U;
                    break;
                }
                case fourcc('b', 's', 'c', ' '):
                case fourcc('b', 's', 't', ' '):
                case fourcc('b', 's', 't', 'n'): {
                    const auto begin = reader.u32(cursor, "truncated BAA table command");
                    const auto end = reader.u32(cursor + 4U, "truncated BAA table command");
                    if (end < begin) {
                        malformed("backwards BAA table range");
                    }
                    reader.require(begin, end - begin, "BAA table extends outside archive");
                    const auto segment = Segment{begin, end - begin};
                    if (command == fourcc('b', 's', 'c', ' ')) {
                        bsc = segment;
                    } else if (command == fourcc('b', 's', 't', ' ')) {
                        bst = segment;
                    } else {
                        bstn = segment;
                    }
                    cursor += 8U;
                    break;
                }
                case fourcc('b', 'l', '_', '<'):
                    reader.require(cursor, 8U, "truncated BAA bank-list command");
                    cursor += 8U;
                    break;
                case fourcc('>', '_', 'b', 'l'):
                    break;
                case fourcc('b', 'm', 's', ' '):
                    reader.require(cursor, 12U, "truncated BAA BMS command");
                    cursor += 12U;
                    break;
                case fourcc('b', 'm', 's', 'a'):
                case fourcc('d', 's', 'q', 'b'):
                case fourcc('b', 's', 'f', 't'):
                case fourcc('s', 'e', 'c', 't'):
                    reader.require(cursor, 4U, "truncated BAA command argument");
                    cursor += 4U;
                    break;
                case fourcc('v', 'b', 'n', 'k'):
                    reader.require(cursor, 8U, "truncated BAA voice-bank command");
                    cursor += 8U;
                    break;
                default:
                    malformed("unsupported BAA command");
                }
            }

            if (bst.size == 0U || bstn.size == 0U || bsc.size == 0U) {
                malformed("BAA is missing BST, BSTN, or BSC data");
            }
        }

        [[nodiscard]] std::optional<std::uint32_t>
        find_sound_id(std::string_view wanted) const {
            const auto reader = Reader{baa, bstn.offset, bstn.size};
            const auto base = bstn.offset;
            if (reader.u32(base, "truncated BSTN") != fourcc('B', 'S', 'T', 'N')) {
                malformed("invalid BSTN header");
            }
            const auto root = relative(base, reader.u32(base + 12U, "truncated BSTN root"),
                                       reader, "invalid BSTN root");
            const auto section_count = reader.u32(root, "truncated BSTN section count");
            require_table(reader, root + 4U, section_count, 4U,
                          "truncated BSTN section table");
            for (auto section = std::uint32_t{0}; section < section_count; ++section) {
                const auto section_offset =
                    reader.u32(root + 4U + static_cast<std::size_t>(section) * 4U,
                               "truncated BSTN section table");
                if (section_offset == 0U) {
                    continue;
                }
                const auto section_data = relative(base, section_offset, reader,
                                                   "invalid BSTN section offset");
                const auto group_count =
                    reader.u32(section_data, "truncated BSTN group count");
                require_table(reader, section_data + 8U, group_count, 4U,
                              "truncated BSTN group table");
                for (auto group = std::uint32_t{0}; group < group_count; ++group) {
                    const auto group_offset = reader.u32(
                        section_data + 8U + static_cast<std::size_t>(group) * 4U,
                        "truncated BSTN group table");
                    if (group_offset == 0U) {
                        continue;
                    }
                    const auto group_data =
                        relative(base, group_offset, reader, "invalid BSTN group offset");
                    const auto item_count =
                        reader.u32(group_data, "truncated BSTN item count");
                    require_table(reader, group_data + 8U, item_count, 4U,
                                  "truncated BSTN item table");
                    for (auto item = std::uint32_t{0}; item < item_count; ++item) {
                        const auto name_offset = reader.u32(
                            group_data + 8U + static_cast<std::size_t>(item) * 4U,
                            "truncated BSTN item table");
                        if (name_offset == 0U) {
                            continue;
                        }
                        const auto name_data = relative(base, name_offset, reader,
                                                       "invalid BSTN name offset");
                        const auto name = reader.string(
                            name_data, bstn.offset + bstn.size - name_data,
                            "unterminated BSTN sound name");
                        if (name == wanted) {
                            if (section > 0xffU || group > 0xffU || item > 0xffffU) {
                                malformed("BSTN identity exceeds JAISoundID fields");
                            }
                            return section << 24U | group << 16U | item;
                        }
                    }
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] std::pair<std::uint8_t, std::uint8_t>
        sound_table_properties(std::uint32_t sound_id) const {
            const auto reader = Reader{baa, bst.offset, bst.size};
            const auto base = bst.offset;
            if (reader.u32(base, "truncated BST") != fourcc('B', 'S', 'T', ' ')) {
                malformed("invalid BST header");
            }
            const auto root = relative(base, reader.u32(base + 12U, "truncated BST root"),
                                       reader, "invalid BST root");
            const auto section_count =
                reader.u32(root, "truncated BST section count");
            require_table(reader, root + 4U, section_count, 4U,
                          "truncated BST section table");
            const auto section = sound_id >> 24U;
            const auto group = sound_id >> 16U & 0xffU;
            const auto item = sound_id & 0xffffU;
            if (section >= section_count) {
                malformed("sound ID section is outside BST");
            }
            const auto section_data = relative(
                base,
                reader.u32(root + 4U +
                               static_cast<std::size_t>(section) * 4U,
                           "truncated BST section table"),
                reader, "invalid BST section offset");
            const auto group_count =
                reader.u32(section_data, "truncated BST group count");
            require_table(reader, section_data + 4U, group_count, 4U,
                          "truncated BST group table");
            if (group >= group_count) {
                malformed("sound ID group is outside BST");
            }
            const auto group_data = relative(
                base, reader.u32(section_data + 4U +
                                     static_cast<std::size_t>(group) * 4U,
                                 "truncated BST group table"),
                reader, "invalid BST group offset");
            const auto item_count =
                reader.u32(group_data, "truncated BST item count");
            require_table(reader, group_data + 8U, item_count, 4U,
                          "truncated BST item table");
            if (item >= item_count) {
                malformed("sound ID item is outside BST");
            }
            const auto item_entry =
                reader.u32(group_data + 8U +
                               static_cast<std::size_t>(item) * 4U,
                           "truncated BST item table");
            if ((item_entry >> 24U & 0xf0U) != 0x50U) {
                malformed("requested level sound is not a JAudio SE sequence");
            }
            const auto item_data = relative(base, item_entry & 0x00ffffffU, reader,
                                            "invalid BST item offset");
            return {reader.u8(item_data, "truncated BST SE item"),
                    reader.u8(item_data + 1U, "truncated BST SE item")};
        }

        [[nodiscard]] Reader bsc_reader() const {
            const auto segment_reader = Reader{baa, bsc.offset, bsc.size};
            const auto file_size = segment_reader.u32(
                bsc.offset + 4U, "truncated BSC size");
            if (file_size < 8U || file_size > bsc.size) {
                malformed("BSC declared size exceeds its BAA segment");
            }
            return Reader{baa, bsc.offset, file_size};
        }

        [[nodiscard]] std::size_t sequence_offset(std::uint32_t sound_id) const {
            const auto reader = bsc_reader();
            const auto base = bsc.offset;
            if (reader.u16(base, "truncated BSC") != 0x5343U) {
                malformed("invalid BSC header");
            }
            const auto group = sound_id >> 16U & 0xffU;
            const auto item = sound_id & 0xffffU;
            const auto group_count = reader.u16(base + 2U, "truncated BSC group count");
            const auto file_size = reader.u32(base + 4U, "truncated BSC size");
            if (file_size > bsc.size) {
                malformed("BSC declared size exceeds its BAA segment");
            }
            if (group >= group_count) {
                malformed("sound ID group is outside BSC");
            }
            require_table(reader, base + 8U, group_count, 4U,
                          "truncated BSC group table");
            const auto group_data = relative(
                base,
                reader.u32(base + 8U + static_cast<std::size_t>(group) * 4U,
                           "truncated BSC group table"),
                reader, "invalid BSC group offset");
            const auto item_count =
                reader.u32(group_data, "truncated BSC item count");
            require_table(reader, group_data + 4U, item_count, 4U,
                          "truncated BSC sequence table");
            if (item >= item_count) {
                malformed("sound ID item is outside BSC");
            }
            return relative(base,
                            reader.u32(group_data + 4U +
                                           static_cast<std::size_t>(item) * 4U,
                                       "truncated BSC sequence table"),
                            reader, "invalid BSC sequence offset");
        }

        void execute_track_setup(std::size_t &cursor, std::vector<std::size_t> &calls,
                                 TrackRecipe &track, double &wait_ticks,
                                 bool stop_at_note) const {
            const auto reader = bsc_reader();
            for (auto instructions = std::size_t{0}; instructions != 512U; ++instructions) {
                const auto command_offset = cursor;
                const auto command = reader.u8(cursor++, "truncated BSC track");
                if (command < 0x80U) {
                    const auto control = reader.u8(cursor++, "truncated BSC note control");
                    const auto velocity = reader.u8(cursor++, "truncated BSC note velocity");
                    if ((control & 0x07U) != 0U || (control & 0x40U) == 0U) {
                        malformed("level-sound track does not use JAudio's persistent gate channel");
                    }
                    const auto [duration, after_duration] = read_midi_value(reader, cursor);
                    (void)duration;
                    cursor = after_duration;
                    track.note = command;
                    track.velocity = velocity;
                    if (!stop_at_note) {
                        malformed("a BSC setup subroutine unexpectedly starts a note");
                    }
                    if (reader.u8(cursor++, "truncated BSC level-sound loop") != 0xc7U ||
                        reader.u24(cursor, "truncated BSC level-sound jump") !=
                            command_offset - bsc.offset) {
                        malformed("level-sound note is not a persistent self-loop");
                    }
                    cursor += 3U;
                    return;
                }

                switch (command) {
                case 0xc3U:
                    if (calls.size() == 8U) {
                        malformed("BSC call stack exceeds JASSeqReader limit");
                    }
                    calls.push_back(cursor + 3U);
                    cursor = relative(bsc.offset,
                                      reader.u24(cursor, "truncated BSC call"), reader,
                                      "invalid BSC call target");
                    break;
                case 0xc5U:
                    if (calls.empty()) {
                        if (stop_at_note) {
                            malformed("level-sound track returns before starting a note");
                        }
                        return;
                    }
                    cursor = calls.back();
                    calls.pop_back();
                    break;
                case 0xd8U: {
                    const auto reg = reader.u8(cursor++, "truncated BSC register load");
                    const auto value = reader.u16(cursor, "truncated BSC register load");
                    cursor += 2U;
                    if (reg == 0x67U) {
                        track.bank = static_cast<std::uint8_t>(value);
                    } else if (reg == 0x68U) {
                        track.program = static_cast<std::uint8_t>(value);
                    } else if (reg == 0x6dU) {
                        track.direct_release = value;
                    }
                    break;
                }
                case 0xe1U: {
                    const auto value = reader.u16(cursor, "truncated BSC bank/program command");
                    cursor += 2U;
                    track.bank = static_cast<std::uint8_t>(value >> 8U);
                    track.program = static_cast<std::uint8_t>(value);
                    break;
                }
                case 0xeaU:
                    reader.require(cursor, 3U, "truncated BSC bus-connect command");
                    cursor += 3U;
                    break;
                case 0xf0U:
                    wait_ticks += reader.u8(cursor++, "truncated BSC byte wait");
                    break;
                default:
                    malformed("unsupported command in JAudio level-sound track");
                }
            }
            malformed("BSC level-sound track exceeds instruction limit");
        }

        [[nodiscard]] std::vector<TrackRecipe>
        sequence_tracks(std::uint32_t sound_id) const {
            const auto reader = bsc_reader();
            auto cursor = sequence_offset(sound_id);
            auto children = std::vector<std::size_t>{};
            auto terminated = false;
            for (auto instructions = std::size_t{0}; instructions != 32U; ++instructions) {
                const auto command = reader.u8(cursor++, "truncated BSC root track");
                if (command == 0xc1U) {
                    (void)reader.u8(cursor++, "truncated BSC open-track index");
                    children.push_back(relative(
                        bsc.offset, reader.u24(cursor, "truncated BSC open-track target"),
                        reader, "invalid BSC child-track target"));
                    cursor += 3U;
                } else if (command == 0xc3U) {
                    // The retail root enters its keep-alive helper only after
                    // opening every child. It cannot change inherited child state.
                    if (children.empty()) {
                        malformed("BSC root calls a helper before opening level tracks");
                    }
                    (void)relative(
                        bsc.offset,
                        reader.u24(cursor, "truncated BSC root call"), reader,
                        "invalid BSC root call target");
                    cursor += 3U;
                } else if (command == 0xffU) {
                    terminated = true;
                    break;
                } else {
                    malformed("unsupported JAudio level-sound root sequence");
                }
            }
            if (!terminated) {
                malformed("BSC level-sound root lacks an end command");
            }
            if (children.empty()) {
                malformed("JAudio level sound has no child tracks");
            }

            auto result = std::vector<TrackRecipe>{};
            result.reserve(children.size());
            for (auto child : children) {
                auto track = TrackRecipe{};
                auto calls = std::vector<std::size_t>{};
                auto wait_ticks = 0.0;
                execute_track_setup(child, calls, track, wait_ticks, true);
                track.start_delay_seconds = wait_ticks * 60.0 / (120.0 * 48.0);
                if (track.direct_release == 0U) {
                    malformed("level-sound track lacks a concrete release duration");
                }
                result.push_back(track);
            }
            return result;
        }

        [[nodiscard]] InstrumentRecipe instrument(const BankRecord &bank,
                                                  const TrackRecipe &track) const {
            const auto reader = Reader{baa, bank.data.offset, bank.data.size};
            const auto base = bank.data.offset;
            if (reader.u32(base, "truncated IBNK") != fourcc('I', 'B', 'N', 'K') ||
                reader.u32(base + 12U, "truncated IBNK version") != 1U) {
                malformed("level sound requires an IBNK version-1 bank");
            }

            auto envt = Segment{};
            auto osct = Segment{};
            auto list = Segment{};
            auto chunk = base + 0x20U;
            const auto end = base + bank.data.size;
            while (chunk + 8U <= end) {
                const auto id = reader.u32(chunk, "truncated IBNK chunk");
                const auto payload_size = reader.u32(chunk + 4U, "truncated IBNK chunk");
                reader.require(chunk + 8U, payload_size, "IBNK chunk extends outside bank");
                const auto segment = Segment{chunk + 8U, payload_size};
                if (id == fourcc('E', 'N', 'V', 'T')) {
                    envt = segment;
                } else if (id == fourcc('O', 'S', 'C', 'T')) {
                    osct = segment;
                } else if (id == fourcc('L', 'I', 'S', 'T')) {
                    list = segment;
                }
                chunk = (chunk + 11U + payload_size) & ~std::size_t{3};
            }
            if (envt.size == 0U || osct.size == 0U || list.size == 0U) {
                malformed("IBNK is missing ENVT, OSCT, or LIST");
            }

            const auto list_reader = Reader{baa, list.offset, list.size};
            const auto program_count =
                list_reader.u32(list.offset, "truncated IBNK LIST");
            require_table(list_reader, list.offset + 4U, program_count, 4U,
                          "truncated IBNK program table");
            if (track.program >= program_count) {
                malformed("BSC program is outside IBNK LIST");
            }
            const auto instrument_offset =
                list_reader.u32(list.offset + 4U + track.program * 4U,
                                "truncated IBNK program table");
            if (instrument_offset == 0U) {
                malformed("BSC program has no IBNK instrument");
            }
            auto cursor = relative(base, instrument_offset, reader,
                                   "invalid IBNK instrument offset");
            if (reader.u32(cursor, "truncated IBNK instrument") !=
                fourcc('I', 'n', 's', 't')) {
                malformed("BSC program does not select a melodic IBNK instrument");
            }
            cursor += 4U;
            const auto oscillator_count =
                reader.u32(cursor, "truncated IBNK oscillator count");
            cursor += 4U;
            require_table(reader, cursor, oscillator_count, 4U,
                          "truncated IBNK oscillator table");
            auto oscillator_indices = std::vector<std::uint32_t>{};
            oscillator_indices.reserve(oscillator_count);
            for (auto index = std::uint32_t{0}; index < oscillator_count; ++index) {
                oscillator_indices.push_back(
                    reader.u32(cursor, "truncated IBNK oscillator table"));
                cursor += 4U;
            }
            const auto effect_count = reader.u32(cursor, "truncated IBNK effect count");
            cursor += 4U;
            if (effect_count != 0U) {
                malformed("IBNK instrument effects are not implemented for level playback");
            }
            const auto key_count = reader.u32(cursor, "truncated IBNK key count");
            cursor += 4U;
            require_table(reader, cursor, key_count, 8U,
                          "truncated IBNK key map");

            auto recipe = InstrumentRecipe{};
            auto selected = false;
            for (auto key = std::uint32_t{0}; key < key_count; ++key) {
                const auto high_key = reader.u8(cursor, "truncated IBNK key map");
                const auto velocity_count =
                    reader.u32(cursor + 4U, "truncated IBNK velocity count");
                cursor += 8U;
                require_table(reader, cursor, velocity_count, 16U,
                              "truncated IBNK velocity map");
                for (auto velocity = std::uint32_t{0}; velocity < velocity_count;
                     ++velocity) {
                    const auto high_velocity =
                        reader.u8(cursor, "truncated IBNK velocity map");
                    const auto wave_id = static_cast<std::uint16_t>(
                        reader.u32(cursor + 4U, "truncated IBNK wave ID"));
                    const auto volume = reader.f32(cursor + 8U, "truncated IBNK volume");
                    const auto pitch = reader.f32(cursor + 12U, "truncated IBNK pitch");
                    if (!selected && track.note <= high_key &&
                        track.velocity <= high_velocity) {
                        recipe.wave_id = wave_id;
                        recipe.volume = volume;
                        recipe.pitch = pitch;
                        selected = true;
                    }
                    cursor += 16U;
                }
            }
            const auto instrument_volume = reader.f32(cursor, "truncated IBNK volume");
            const auto instrument_pitch = reader.f32(cursor + 4U, "truncated IBNK pitch");
            if (!selected) {
                malformed("no IBNK key/velocity region matches the level-sound note");
            }
            recipe.volume *= instrument_volume;
            recipe.pitch *= instrument_pitch;

            const auto osct_reader = Reader{baa, osct.offset, osct.size};
            const auto envt_reader = Reader{baa, envt.offset, envt.size};
            const auto osct_count =
                osct_reader.u32(osct.offset, "truncated IBNK OSCT");
            require_table(osct_reader, osct.offset + 4U, osct_count, 28U,
                          "truncated IBNK oscillator table");
            for (const auto oscillator_index : oscillator_indices) {
                if (oscillator_index >= osct_count) {
                    malformed("IBNK oscillator index is outside OSCT");
                }
                const auto oscillator =
                    osct.offset + 4U +
                    static_cast<std::size_t>(oscillator_index) * 28U;
                osct_reader.require(oscillator, 28U, "truncated IBNK oscillator");
                if (osct_reader.u8(oscillator + 4U,
                                   "truncated IBNK oscillator target") != 0U) {
                    continue;
                }
                const auto counter_scale =
                    osct_reader.f32(oscillator + 8U,
                                    "truncated IBNK oscillator scale");
                const auto table_offset =
                    osct_reader.u32(oscillator + 12U,
                                    "truncated IBNK envelope offset");
                const auto value_scale =
                    osct_reader.f32(oscillator + 20U,
                                    "truncated IBNK envelope scale");
                const auto value_offset =
                    osct_reader.f32(oscillator + 24U,
                                    "truncated IBNK envelope offset");
                if (counter_scale != 1.0F || value_scale != 1.0F ||
                    value_offset != 0.0F) {
                    malformed("unsupported scaled IBNK volume oscillator");
                }
                const auto point = relative(envt.offset, table_offset, envt_reader,
                                            "invalid IBNK envelope offset");
                envt_reader.require(point, 6U, "truncated IBNK attack point");
                const auto curve =
                    envt_reader.s16(point, "truncated IBNK attack curve");
                const auto duration = envt_reader.s16(
                    point + 2U, "truncated IBNK attack duration");
                const auto target = envt_reader.s16(
                    point + 4U, "truncated IBNK attack target");
                if (duration <= 0 || target <= 0) {
                    malformed("IBNK volume oscillator lacks a concrete attack");
                }
                recipe.attack_seconds = static_cast<double>(duration) / 600.0;
                recipe.attack_curve = curve_from_jaudio(curve);
                recipe.attack_peak = static_cast<float>(target) / 32768.0F;
                break;
            }
            return recipe;
        }

        [[nodiscard]] WaveRecipe wave(const BankRecord &bank,
                                      std::uint16_t wanted_wave) const {
            const auto bank_iter = wave_banks.find(bank.wave_bank_index);
            if (bank_iter == wave_banks.end()) {
                malformed("IBNK references a missing WSYS wave bank");
            }
            const auto reader = Reader{baa, bank_iter->second.offset,
                                       bank_iter->second.size};
            const auto base = bank_iter->second.offset;
            if (reader.u32(base, "truncated WSYS") != fourcc('W', 'S', 'Y', 'S')) {
                malformed("invalid WSYS header");
            }
            const auto archive_bank = relative(
                base, reader.u32(base + 16U, "truncated WSYS archive bank"), reader,
                "invalid WSYS archive-bank offset");
            const auto control_group = relative(
                base, reader.u32(base + 20U, "truncated WSYS control group"), reader,
                "invalid WSYS control-group offset");
            const auto group_count =
                reader.u32(control_group + 8U, "truncated WSYS group count");
            require_table(reader, control_group + 12U, group_count, 4U,
                          "truncated WSYS scene table");
            require_table(reader, archive_bank + 8U, group_count, 4U,
                          "truncated WSYS archive table");
            for (auto group = std::uint32_t{0}; group < group_count; ++group) {
                const auto scene = relative(
                    base,
                    reader.u32(control_group + 12U +
                                   static_cast<std::size_t>(group) * 4U,
                                     "truncated WSYS scene table"),
                    reader, "invalid WSYS scene offset");
                const auto control = relative(
                    base, reader.u32(scene + 12U, "truncated WSYS control offset"),
                    reader, "invalid WSYS control offset");
                const auto wave_count =
                    reader.u32(control + 4U, "truncated WSYS wave count");
                const auto archive = relative(
                    base,
                    reader.u32(archive_bank + 8U +
                                   static_cast<std::size_t>(group) * 4U,
                                     "truncated WSYS archive table"),
                    reader, "invalid WSYS archive offset");
                require_table(reader, control + 8U, wave_count, 4U,
                              "truncated WSYS wave mapping table");
                reader.require(archive, 0x74U,
                               "truncated WSYS archive header");
                require_table(reader, archive + 0x74U, wave_count, 4U,
                              "truncated WSYS wave table");
                if (reader.u32(archive + 0x70U, "truncated WSYS archive wave count") <
                    wave_count) {
                    malformed("WSYS archive/control wave counts disagree");
                }
                for (auto index = std::uint32_t{0}; index < wave_count; ++index) {
                    const auto mapping = relative(
                        base,
                        reader.u32(control + 8U +
                                       static_cast<std::size_t>(index) * 4U,
                                         "truncated WSYS wave mapping"),
                        reader, "invalid WSYS wave mapping");
                    if (reader.u16(mapping + 2U, "truncated WSYS wave ID") !=
                        wanted_wave) {
                        continue;
                    }
                    const auto wave_data = relative(
                        base,
                        reader.u32(archive + 0x74U +
                                       static_cast<std::size_t>(index) * 4U,
                                         "truncated WSYS wave table"),
                        reader, "invalid WSYS wave offset");
                    auto result = WaveRecipe{};
                    result.archive_name =
                        reader.string(archive, 0x70U, "unterminated WSYS AW filename");
                    result.format = reader.u8(wave_data + 1U, "truncated WSYS wave");
                    result.base_key = reader.u8(wave_data + 2U, "truncated WSYS wave");
                    result.sample_rate = reader.f32(wave_data + 4U, "truncated WSYS wave");
                    result.archive_offset = reader.u32(wave_data + 8U, "truncated WSYS wave");
                    result.archive_length = reader.u32(wave_data + 12U, "truncated WSYS wave");
                    const auto loop_flags = reader.u32(wave_data + 16U, "truncated WSYS wave");
                    result.loop_start = reader.u32(wave_data + 20U, "truncated WSYS wave");
                    result.loop_end = reader.u32(wave_data + 24U, "truncated WSYS wave");
                    result.sample_count = reader.u32(wave_data + 28U, "truncated WSYS wave");
                    result.loop_yn1 = reader.s16(wave_data + 32U, "truncated WSYS wave");
                    result.loop_yn2 = reader.s16(wave_data + 34U, "truncated WSYS wave");
                    if (result.format != 0U || loop_flags == 0U ||
                        result.loop_start >= result.loop_end ||
                        !std::isfinite(result.sample_rate) || result.sample_rate <= 0.0F) {
                        malformed("level sound requires a looping JAudio AFC-HQ wave");
                    }
                    const auto encoded_for_loop =
                        (static_cast<std::size_t>(result.loop_end) + 15U) / 16U * 9U;
                    if (encoded_for_loop != result.archive_length) {
                        malformed("WSYS AFC-HQ length does not cover exactly its loop end");
                    }
                    return result;
                }
            }
            malformed("IBNK wave ID is absent from its WSYS bank");
        }

        [[nodiscard]] JAudioLevelSoundRecipe
        resolve(std::uint32_t sound_id) const {
            const auto [priority, table_volume] = sound_table_properties(sound_id);
            const auto tracks = sequence_tracks(sound_id);
            auto result = JAudioLevelSoundRecipe{
                .sound_id = sound_id,
                .priority = priority,
                .table_volume = table_volume,
                .voice = {},
                .layers = {},
            };
            result.voice.layers.reserve(tracks.size());
            result.layers.reserve(tracks.size());

            auto wave_cache = std::map<std::string, std::vector<std::uint8_t>>{};
            for (const auto &track : tracks) {
                const auto bank_iter = banks.find(track.bank);
                if (bank_iter == banks.end()) {
                    malformed("BSC selects an unavailable IBNK bank");
                }
                const auto inst = instrument(bank_iter->second, track);
                const auto wave_info = wave(bank_iter->second, inst.wave_id);
                auto archive_iter = wave_cache.find(wave_info.archive_name);
                if (archive_iter == wave_cache.end()) {
                    archive_iter = wave_cache.emplace(
                        wave_info.archive_name, wave_loader(wave_info.archive_name)).first;
                }
                const auto &archive = archive_iter->second;
                if (wave_info.archive_offset > archive.size() ||
                    wave_info.archive_length > archive.size() - wave_info.archive_offset) {
                    malformed("WSYS wave range extends outside its AW file");
                }
                const auto encoded = std::span<const std::uint8_t>{archive}.subspan(
                    wave_info.archive_offset, wave_info.archive_length);
                auto decoded = aurora::audio::decode_jaudio_afc_hq(
                    encoded, wave_info.loop_end);
                const auto block_start =
                    static_cast<std::size_t>(wave_info.loop_start / 16U) * 16U;
                if (block_start < 2U ||
                    decoded.samples[block_start - 1U] != wave_info.loop_yn1 ||
                    decoded.samples[block_start - 2U] != wave_info.loop_yn2) {
                    malformed("decoded AFC loop history disagrees with WSYS metadata");
                }

                auto pcm = std::make_shared<std::vector<float>>();
                pcm->reserve(decoded.samples.size());
                for (const auto sample : decoded.samples) {
                    pcm->push_back(static_cast<float>(sample) / 32768.0F);
                }
                const auto velocity = static_cast<float>(track.velocity) / 127.0F;
                const auto gain = static_cast<float>(table_volume) / 255.0F *
                                  velocity * velocity * inst.volume * inst.attack_peak;
                const auto pitch = inst.pitch * std::exp2(
                    (static_cast<float>(track.note) - static_cast<float>(wave_info.base_key)) /
                    12.0F);
                result.voice.layers.push_back(aurora::audio::LoopingPcmLayer{
                    .samples = std::move(pcm),
                    .sample_rate = static_cast<std::uint32_t>(std::lround(wave_info.sample_rate)),
                    .loop_start = wave_info.loop_start,
                    .loop_end = wave_info.loop_end,
                    .gain = gain,
                    .pitch_ratio = pitch,
                    .pan = 0.5F,
                    .start_delay_seconds = track.start_delay_seconds,
                    .attack_seconds = inst.attack_seconds,
                    .release_seconds = static_cast<double>(track.direct_release & 0x3fffU) /
                                       600.0,
                    .attack_curve = inst.attack_curve,
                    .release_curve = curve_from_jaudio(
                        static_cast<std::int16_t>(track.direct_release >> 14U & 3U)),
                });
                result.layers.push_back(JAudioLevelSoundLayerAudit{
                    .bank = track.bank,
                    .program = track.program,
                    .note = track.note,
                    .velocity = track.velocity,
                    .wave_id = inst.wave_id,
                    .wave_archive_name = wave_info.archive_name,
                    .wave_archive_offset = wave_info.archive_offset,
                    .encoded_length = wave_info.archive_length,
                    .decoded_loop_start = wave_info.loop_start,
                    .decoded_loop_end = wave_info.loop_end,
                    .source_sample_rate =
                        static_cast<std::uint32_t>(std::lround(wave_info.sample_rate)),
                    .direct_release_ticks = track.direct_release,
                    .loop_history_yn1 = wave_info.loop_yn1,
                    .loop_history_yn2 = wave_info.loop_yn2,
                });
            }
            return result;
        }

        std::vector<std::uint8_t> baa;
        WaveArchiveLoader wave_loader;
        Segment bst;
        Segment bstn;
        Segment bsc;
        std::map<std::uint32_t, Segment> wave_banks;
        std::map<std::uint32_t, BankRecord> banks;
    };

    JAudioLevelSoundArchive::JAudioLevelSoundArchive(
        std::span<const std::uint8_t> decompressed_baa,
        WaveArchiveLoader wave_archive_loader)
        : _impl(std::make_unique<Impl>(decompressed_baa,
                                      std::move(wave_archive_loader))) {
    }

    JAudioLevelSoundArchive::~JAudioLevelSoundArchive() = default;
    JAudioLevelSoundArchive::JAudioLevelSoundArchive(JAudioLevelSoundArchive &&) noexcept = default;
    JAudioLevelSoundArchive &
    JAudioLevelSoundArchive::operator=(JAudioLevelSoundArchive &&) noexcept = default;

    std::optional<std::uint32_t>
    JAudioLevelSoundArchive::find_sound_id(std::string_view name) const {
        return _impl->find_sound_id(name);
    }

    std::optional<JAudioLevelSoundRecipe>
    JAudioLevelSoundArchive::resolve_level_sound(std::string_view name) const {
        const auto sound_id = find_sound_id(name);
        if (!sound_id.has_value()) {
            return std::nullopt;
        }
        return _impl->resolve(*sound_id);
    }

} // namespace smgpc::compat
