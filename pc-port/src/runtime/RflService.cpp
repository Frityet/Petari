#include "runtime/RflService.hpp"

#include <JSystem/JUtility/JUTTexture.hpp>

#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace smgpc::runtime {
    namespace {

        constexpr auto RFL_DB_MAGIC = std::array<std::uint8_t, 4U>{'S', 'R', 'F', 'L'};
        constexpr auto RFL_DB_VERSION = std::uint16_t {1U};
        constexpr auto RFL_DB_HEADER_SIZE = std::size_t {8U};
        constexpr auto RFL_DB_ENTRY_SIZE = std::size_t {66U};

        constexpr auto RFL_ENTRY_FLAG_AVAILABLE = std::uint8_t {1U << 0U};
        constexpr auto RFL_ENTRY_FLAG_FAVORITE = std::uint8_t {1U << 1U};

        constexpr auto RFL_BASE_WORK_SIZE = std::size_t {0x82000U};
        constexpr auto RFL_DELUXE_WORK_SIZE = std::size_t {0x9A000U};

        [[nodiscard]] bool is_supported_source(RFLDataSource source) {
            const auto value = static_cast<int>(source);
            return value >= static_cast<int>(RFLDataSource_Official) && value < static_cast<int>(RFLDataSource_Max);
        }

        [[nodiscard]] bool is_supported_expression(RFLExpression expression) {
            const auto value = static_cast<int>(expression);
            return value >= static_cast<int>(RFLExp_Normal) && value < static_cast<int>(RFLExp_Max);
        }

        [[nodiscard]] std::uint16_t effective_resolution(RFLResolution resolution) {
            const auto value = static_cast<u32>(resolution);
            if ((value & static_cast<u32>(RFLResolution_256)) != 0U) {
                return 256U;
            }
            if ((value & static_cast<u32>(RFLResolution_128)) != 0U) {
                return 128U;
            }
            return 64U;
        }

        [[nodiscard]] RFLExpression first_expression_from_flags(u32 expression_flags) {
            for (auto value = static_cast<int>(RFLExp_Normal); value < static_cast<int>(RFLExp_Max); ++value) {
                if ((expression_flags & (1U << static_cast<u32>(value))) != 0U) {
                    return static_cast<RFLExpression>(value);
                }
            }
            return RFLExp_Normal;
        }

        [[nodiscard]] std::uint16_t read_be_u16(std::span<const std::uint8_t> bytes, std::size_t offset) {
            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) | bytes[offset + 1U]);
        }

        void append_be_u16(std::vector<std::uint8_t> &bytes, std::uint16_t value) {
            bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
            bytes.push_back(static_cast<std::uint8_t>(value));
        }

        void write_be_u16(std::span<std::uint8_t> bytes, std::size_t offset, std::uint16_t value) {
            bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
            bytes[offset + 1U] = static_cast<std::uint8_t>(value);
        }

        [[nodiscard]] bool is_empty_create_id(const RFLCreateID &id) {
            return std::ranges::all_of(id.data, [](u8 value) { return value == 0U; });
        }

        [[nodiscard]] RFLCreateID create_id_from_index(s32 index) {
            auto id = RFLCreateID {};
            std::memcpy(id.data, &index, std::min(sizeof(index), sizeof(id.data)));
            return id;
        }

        [[nodiscard]] bool create_ids_equal(const RFLCreateID &lhs, const RFLCreateID &rhs) {
            return std::memcmp(lhs.data, rhs.data, sizeof(lhs.data)) == 0;
        }

        [[nodiscard]] bool mii_select_keys_equal(const RflMiiSelectIconKey &lhs, const RflMiiSelectIconKey &rhs) {
            return lhs.kind == rhs.kind && lhs.id == rhs.id && lhs.source == rhs.source;
        }

        [[nodiscard]] std::string read_utf16_text(std::span<const std::uint8_t> bytes, std::size_t offset, std::size_t character_count) {
            auto text = std::u16string {};
            text.reserve(character_count);
            for (auto index = std::size_t {}; index < character_count; ++index) {
                const auto value = read_be_u16(bytes, offset + index * sizeof(std::uint16_t));
                if (value == 0U) {
                    break;
                }
                text.push_back(static_cast<char16_t>(value));
            }
            return smgpc::resource::utf8_from_utf16_lossy(text);
        }

        void append_utf16_text(std::vector<std::uint8_t> &bytes, std::string_view text, std::size_t character_count) {
            const auto utf16 = smgpc::resource::utf16_from_utf8_lossy(text);
            for (auto index = std::size_t {}; index < character_count; ++index) {
                const auto value = index < utf16.size() ? static_cast<std::uint16_t>(utf16[index]) : std::uint16_t {};
                append_be_u16(bytes, value);
            }
        }

        [[nodiscard]] RflMiiEntry normalized_entry(RflMiiEntry entry) {
            entry.index = std::clamp(entry.index, 0, 0xffff);
            if (!is_supported_source(entry.source)) {
                entry.source = RFLDataSource_Official;
            }
            if (is_empty_create_id(entry.create_id)) {
                entry.create_id = create_id_from_index(entry.index);
            }
            entry.sex &= 1U;
            entry.bmonth &= 0xfU;
            entry.bday &= 0x1fU;
            entry.color &= 0xfU;
            entry.height = std::min(entry.height, 0x7fU);
            entry.build = std::min(entry.build, 0x7fU);
            return entry;
        }

        [[nodiscard]] bool parse_database(std::span<const std::uint8_t> bytes, std::vector<RflMiiEntry> &entries) {
            entries.clear();
            if (bytes.size() < RFL_DB_HEADER_SIZE || !std::equal(RFL_DB_MAGIC.begin(), RFL_DB_MAGIC.end(), bytes.begin())) {
                return false;
            }

            const auto version = read_be_u16(bytes, 4U);
            const auto count = read_be_u16(bytes, 6U);
            if (version != RFL_DB_VERSION || bytes.size() < RFL_DB_HEADER_SIZE + static_cast<std::size_t>(count) * RFL_DB_ENTRY_SIZE) {
                return false;
            }

            entries.reserve(count);
            for (auto entry_index = std::uint16_t {}; entry_index < count; ++entry_index) {
                const auto offset = RFL_DB_HEADER_SIZE + static_cast<std::size_t>(entry_index) * RFL_DB_ENTRY_SIZE;
                const auto source = static_cast<RFLDataSource>(bytes[offset + 2U]);
                if (!is_supported_source(source)) {
                    return false;
                }

                auto entry = RflMiiEntry {};
                entry.index = read_be_u16(bytes, offset);
                entry.source = source;
                entry.available = (bytes[offset + 3U] & RFL_ENTRY_FLAG_AVAILABLE) != 0U;
                entry.favorite = (bytes[offset + 3U] & RFL_ENTRY_FLAG_FAVORITE) != 0U;
                entry.sex = bytes[offset + 4U] & 1U;
                entry.bmonth = bytes[offset + 5U] & 0xfU;
                entry.bday = bytes[offset + 6U] & 0x1fU;
                entry.color = bytes[offset + 7U] & 0xfU;
                entry.height = bytes[offset + 8U] & 0x7fU;
                entry.build = bytes[offset + 9U] & 0x7fU;
                entry.skin_color = GXColor {
                    .r = bytes[offset + 10U],
                    .g = bytes[offset + 11U],
                    .b = bytes[offset + 12U],
                    .a = bytes[offset + 13U],
                };
                std::memcpy(entry.create_id.data, bytes.data() + offset + 14U, sizeof(entry.create_id.data));
                entry.name = read_utf16_text(bytes, offset + 22U, RFL_NAME_LEN + 1U);
                entry.creator = read_utf16_text(bytes, offset + 44U, RFL_CREATOR_LEN + 1U);
                entries.push_back(normalized_entry(std::move(entry)));
            }
            return true;
        }

        void append_entry(std::vector<std::uint8_t> &bytes, RflMiiEntry entry) {
            entry = normalized_entry(std::move(entry));
            append_be_u16(bytes, static_cast<std::uint16_t>(entry.index));
            bytes.push_back(static_cast<std::uint8_t>(entry.source));
            bytes.push_back(static_cast<std::uint8_t>((entry.available ? RFL_ENTRY_FLAG_AVAILABLE : 0U) |
                                                      (entry.favorite ? RFL_ENTRY_FLAG_FAVORITE : 0U)));
            bytes.push_back(static_cast<std::uint8_t>(entry.sex));
            bytes.push_back(static_cast<std::uint8_t>(entry.bmonth));
            bytes.push_back(static_cast<std::uint8_t>(entry.bday));
            bytes.push_back(static_cast<std::uint8_t>(entry.color));
            bytes.push_back(static_cast<std::uint8_t>(entry.height));
            bytes.push_back(static_cast<std::uint8_t>(entry.build));
            bytes.push_back(entry.skin_color.r);
            bytes.push_back(entry.skin_color.g);
            bytes.push_back(entry.skin_color.b);
            bytes.push_back(entry.skin_color.a);
            bytes.insert(bytes.end(), entry.create_id.data, entry.create_id.data + sizeof(entry.create_id.data));
            append_utf16_text(bytes, entry.name, RFL_NAME_LEN + 1U);
            append_utf16_text(bytes, entry.creator, RFL_CREATOR_LEN + 1U);
        }

        void copy_utf16_text(u16 *destination, std::size_t character_count, std::string_view text) {
            std::fill_n(destination, character_count, u16 {});
            const auto utf16 = smgpc::resource::utf16_from_utf8_lossy(text);
            const auto count = std::min(utf16.size(), character_count == 0U ? 0U : character_count - 1U);
            for (auto index = std::size_t {}; index < count; ++index) {
                destination[index] = static_cast<u16>(utf16[index]);
            }
        }

        [[nodiscard]] RFLErrcode load_error_for_status(const RflDbStatus &status) {
            if (status.last_error == RFLErrcode_DBNodata) {
                return RFLErrcode_DBNodata;
            }
            if (status.last_error == RFLErrcode_Success) {
                return RFLErrcode_Broken;
            }
            return status.last_error;
        }

        [[nodiscard]] GXColor favorite_color(u32 color) {
            constexpr auto colors = std::array<GXColor, RFLFavoriteColor_Max>{
                GXColor {220U, 48U, 52U, 255U},
                GXColor {238U, 126U, 42U, 255U},
                GXColor {245U, 206U, 73U, 255U},
                GXColor {159U, 204U, 62U, 255U},
                GXColor {73U, 174U, 74U, 255U},
                GXColor {47U, 102U, 201U, 255U},
                GXColor {64U, 180U, 220U, 255U},
                GXColor {235U, 111U, 168U, 255U},
                GXColor {134U, 86U, 185U, 255U},
                GXColor {122U, 82U, 54U, 255U},
                GXColor {238U, 238U, 238U, 255U},
                GXColor {48U, 48U, 52U, 255U},
            };
            return colors[std::min<std::size_t>(color, colors.size() - 1U)];
        }

        [[nodiscard]] std::uint16_t encode_rgb5a3(GXColor color) {
            if (color.a >= 0xE0U) {
                return static_cast<std::uint16_t>(
                    0x8000U | ((static_cast<std::uint16_t>(color.r) >> 3U) << 10U) |
                    ((static_cast<std::uint16_t>(color.g) >> 3U) << 5U) | (static_cast<std::uint16_t>(color.b) >> 3U));
            }

            return static_cast<std::uint16_t>(((static_cast<std::uint16_t>(color.a) >> 5U) << 12U) |
                                              ((static_cast<std::uint16_t>(color.r) >> 4U) << 8U) |
                                              ((static_cast<std::uint16_t>(color.g) >> 4U) << 4U) |
                                              (static_cast<std::uint16_t>(color.b) >> 4U));
        }

        [[nodiscard]] std::uint32_t texture_width_blocks(std::uint16_t width) {
            return (static_cast<std::uint32_t>(width - 1U) / 4U) + 1U;
        }

        [[nodiscard]] std::size_t rgb5a3_tiled_offset(std::uint16_t width, std::uint16_t x, std::uint16_t y) {
            const auto block_x = x / 4U;
            const auto block_y = y / 4U;
            const auto in_block_x = x % 4U;
            const auto in_block_y = y % 4U;
            return (static_cast<std::size_t>(block_y) * texture_width_blocks(width) + block_x) * 32U +
                   (static_cast<std::size_t>(in_block_y) * 4U + in_block_x) * 2U;
        }

        void write_rgb5a3_texel(std::span<std::uint8_t> bytes, std::uint16_t width, std::uint16_t x, std::uint16_t y, GXColor color) {
            const auto offset = rgb5a3_tiled_offset(width, x, y);
            if (offset + 1U >= bytes.size()) {
                return;
            }
            write_be_u16(bytes, offset, encode_rgb5a3(color));
        }

        [[nodiscard]] GXColor mix_color(GXColor a, GXColor b, std::uint8_t b_weight) {
            const auto a_weight = static_cast<std::uint16_t>(255U - b_weight);
            return GXColor {
                .r = static_cast<u8>((static_cast<std::uint16_t>(a.r) * a_weight + static_cast<std::uint16_t>(b.r) * b_weight) / 255U),
                .g = static_cast<u8>((static_cast<std::uint16_t>(a.g) * a_weight + static_cast<std::uint16_t>(b.g) * b_weight) / 255U),
                .b = static_cast<u8>((static_cast<std::uint16_t>(a.b) * a_weight + static_cast<std::uint16_t>(b.b) * b_weight) / 255U),
                .a = static_cast<u8>((static_cast<std::uint16_t>(a.a) * a_weight + static_cast<std::uint16_t>(b.a) * b_weight) / 255U),
            };
        }

        [[nodiscard]] std::vector<std::uint8_t> make_icon_rgb5a3(const RflMiiEntry &entry, std::uint16_t width, std::uint16_t height,
                                                                 GXColor background, RFLExpression expression, bool xlu_only) {
            const auto size = GXGetTexBufferSize(width, height, GX_TF_RGB5A3, GX_FALSE, 1U);
            auto bytes = std::vector<std::uint8_t>(size);
            const auto accent = favorite_color(entry.color);
            const auto skin = entry.skin_color;
            const auto hair = mix_color(GXColor {24U, 20U, 18U, 255U}, accent, static_cast<std::uint8_t>((entry.index * 17U) & 0x5fU));
            const auto eye = GXColor {24U, 24U, 28U, 255U};
            const auto mouth = expression == RFLExp_Smile ? GXColor {180U, 52U, 70U, 255U} : GXColor {112U, 36U, 48U, 255U};
            const auto alpha = xlu_only ? 176U : 255U;

            for (auto y = std::uint16_t {}; y < height; ++y) {
                for (auto x = std::uint16_t {}; x < width; ++x) {
                    auto color = background;
                    const auto nx = (static_cast<int>(x) * 1000 / std::max<std::uint16_t>(width, 1U)) - 500;
                    const auto ny = (static_cast<int>(y) * 1000 / std::max<std::uint16_t>(height, 1U)) - 500;
                    const auto face = (nx * nx * 100 / (330 * 330)) + ((ny + 20) * (ny + 20) * 100 / (395 * 395)) <= 100;
                    const auto hair_band = face && ny < -210 + static_cast<int>(entry.build);
                    const auto left_eye = face && ny > -110 && ny < -30 && nx > -185 && nx < -85;
                    const auto right_eye = face && ny > -110 && ny < -30 && nx > 85 && nx < 185;
                    const auto blink = expression == RFLExp_Blink || expression == RFLExp_Smile;
                    const auto mouth_y = expression == RFLExp_OpenMouth ? 165 : 130;
                    const auto mouth_open = face && ny > mouth_y && ny < mouth_y + (expression == RFLExp_OpenMouth ? 92 : 32) &&
                                            nx > -120 && nx < 120;
                    const auto shirt = ny > 355;

                    if (shirt) {
                        color = accent;
                    } else if (hair_band) {
                        color = hair;
                    } else if (left_eye || right_eye) {
                        color = blink ? mix_color(skin, eye, 120U) : eye;
                    } else if (mouth_open) {
                        color = mouth;
                    } else if (face) {
                        color = skin;
                    }

                    color.a = static_cast<u8>(std::min<std::uint16_t>(color.a, alpha));
                    write_rgb5a3_texel(bytes, width, x, y, color);
                }
            }

            return bytes;
        }

        void write_icon_res_timg(void *buffer, const RflIconTexture &texture) {
            auto *timg = static_cast<ResTIMG *>(buffer);
            timg->mFormat = GX_TF_RGB5A3;
            timg->mTransparency = 0U;
            timg->mWidth = texture.width;
            timg->mHeight = texture.height;
            timg->mWrapS = GX_CLAMP;
            timg->mWrapT = GX_CLAMP;
            timg->mPaletteName = GX_TLUT0;
            timg->mPaletteFormat = GX_TL_IA8;
            timg->mPaletteNum = 0U;
            timg->mPaletteDataOffset = 0U;
            timg->mMipmap = false;
            timg->mDoEdgeLod = false;
            timg->mBiasClamp = false;
            timg->mMaxAnisotropy = GX_ANISO_1;
            timg->mMinType = GX_LINEAR;
            timg->mMagType = GX_LINEAR;
            timg->mMinLod = 0U;
            timg->mMaxLod = 0U;
            timg->mImageNum = 1U;
            timg->mLodBias = 0;
            timg->mImageDataOffset = sizeof(ResTIMG);

            auto *image = reinterpret_cast<std::uint8_t *>(timg) + timg->mImageDataOffset;
            std::copy(texture.rgb5a3.begin(), texture.rgb5a3.end(), image);
            DCFlushRange(image, static_cast<u32>(texture.rgb5a3.size()));
        }

    }  // namespace

    RflService::RflService() = default;

    RflService::RflService(NandFileSystemService &nand)
        : _nand(&nand) {
    }

    void RflService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        if (!_async_pending || frame_index < _async_complete_frame) {
            return;
        }

        _async_pending = false;
        _initialized = true;
        _cache_loaded = false;
        ensure_loaded();
    }

    void RflService::set_nand(NandFileSystemService &nand) {
        _nand = &nand;
        clear_for_reload();
    }

    void RflService::set_initialized(bool initialized) {
        _initialized = initialized;
    }

    void RflService::set_error(bool error) {
        _forced_error = error;
    }

    std::size_t RflService::work_size(bool deluxe_textures) const {
        return deluxe_textures ? RFL_DELUXE_WORK_SIZE : RFL_BASE_WORK_SIZE;
    }

    RFLErrcode RflService::init_resources(void *work_buffer, const void *resource_buffer, std::size_t resource_size, bool deluxe_textures,
                                          bool async) {
        _deluxe_textures = deluxe_textures;
        _resource_initialized = false;
        _last_reason = 0;

        auto result = RFLErrcode_Success;
        if (work_buffer == nullptr) {
            result = RFLErrcode_WrongParam;
            _last_reason = NAND_RESULT_INVALID;
        } else if (resource_buffer == nullptr || resource_size == 0U) {
            result = RFLErrcode_Loadfail;
            _last_reason = NAND_RESULT_NOEXISTS;
        }

        _status.resource_initialized = result == RFLErrcode_Success && !async;
        _status.deluxe_textures = deluxe_textures;
        _status.last_error = result == RFLErrcode_Success ? _status.last_error : result;
        _status.last_reason = _last_reason;
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::InitResource,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .result = result == RFLErrcode_Success && async ? RFLErrcode_Busy : result,
            .byte_count = resource_size,
            .async_pending = async && result == RFLErrcode_Success,
        });

        if (result != RFLErrcode_Success) {
            _initialized = true;
            _async_pending = false;
            _forced_error = true;
            return result;
        }

        _forced_error = false;
        if (async) {
            _resource_initialized = true;
            request_async_load(1U);
            return RFLErrcode_Busy;
        }

        _resource_initialized = true;
        _initialized = true;
        return RFLErrcode_Success;
    }

    void RflService::exit() {
        _resource_initialized = false;
        _initialized = false;
        _async_pending = false;
        _cache_loaded = false;
        _miis.clear();
        _valid_miis.clear();
        _status.resource_initialized = false;
        _status.last_error = RFLErrcode_NotAvailable;
    }

    bool RflService::available() const {
        return _resource_initialized && is_initialized() && !has_error();
    }

    RFLErrcode RflService::async_status() const {
        if (_async_pending) {
            return RFLErrcode_Busy;
        }
        if (!_resource_initialized || !_initialized) {
            return RFLErrcode_NotAvailable;
        }
        if (has_error()) {
            return _forced_error ? RFLErrcode_Broken : load_error_for_status(_status);
        }
        return RFLErrcode_Success;
    }

    s32 RflService::last_reason() const {
        return _last_reason;
    }

    void RflService::request_async_load(std::uint64_t delay_frames) {
        _initialized = false;
        _async_pending = true;
        _async_complete_frame = _frame_index + delay_frames;
        _cache_loaded = false;
        _miis.clear();
        _valid_miis.clear();
        _status.async_pending = true;
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::LoadBegin,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .result = RFLErrcode_Busy,
            .async_pending = true,
        });
        if (delay_frames == 0U) {
            begin_frame(_frame_index);
        }
    }

    void RflService::clear_for_reload() {
        _async_pending = false;
        _initialized = true;
        _cache_loaded = false;
        _manual_override = false;
        _miis.clear();
        _valid_miis.clear();
        _status = RflDbStatus {};
        _status.resource_initialized = _resource_initialized;
        _status.deluxe_textures = _deluxe_textures;
        _status.last_reason = _last_reason;
    }

    void RflService::set_miis(std::vector<RflMiiEntry> miis) {
        auto status = RflDbStatus {
            .nand_bound = _nand != nullptr,
            .db_present = false,
            .fallback_used = false,
            .async_pending = false,
            .byte_count = 0U,
            .entry_count = miis.size(),
            .loaded_frame = _frame_index,
            .last_error = RFLErrcode_Success,
        };
        replace_cache(std::move(miis), status, true);
    }

    void RflService::add_or_replace_mii(RflMiiEntry mii) {
        ensure_loaded();
        mii = normalized_entry(std::move(mii));
        const auto found = std::find_if(_miis.begin(), _miis.end(), [&mii](const auto &entry) {
            return entry.source == mii.source && entry.index == mii.index;
        });
        if (found == _miis.end()) {
            _miis.push_back(std::move(mii));
        } else {
            *found = std::move(mii);
        }
        _manual_override = true;
        _status.entry_count = _miis.size();
        _status.last_error = RFLErrcode_Success;
        rebuild_valid_miis();
    }

    void RflService::persist_to_nand() {
        if (_nand == nullptr) {
            throw std::logic_error("Cannot persist RFL DB without a NAND service.");
        }

        ensure_loaded();
        const auto bytes = serialize_miis();
        _nand->write_file(NandFileSystemService::rfl_db_path(), bytes);
        _manual_override = false;
        _status.nand_bound = true;
        _status.db_present = true;
        _status.fallback_used = false;
        _status.async_pending = false;
        _status.resource_initialized = _resource_initialized;
        _status.deluxe_textures = _deluxe_textures;
        _status.byte_count = bytes.size();
        _status.entry_count = _miis.size();
        _status.loaded_frame = _frame_index;
        _status.last_error = RFLErrcode_Success;
        _status.last_reason = _last_reason;
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::Persist,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .result = RFLErrcode_Success,
            .byte_count = bytes.size(),
            .entry_count = _miis.size(),
            .db_present = true,
        });
    }

    std::vector<std::uint8_t> RflService::serialize_miis() const {
        ensure_loaded();
        if (_miis.size() > std::numeric_limits<std::uint16_t>::max()) {
            throw std::logic_error("RFL DB entry count exceeds the host serializer limit.");
        }

        auto bytes = std::vector<std::uint8_t>{};
        bytes.reserve(RFL_DB_HEADER_SIZE + _miis.size() * RFL_DB_ENTRY_SIZE);
        bytes.insert(bytes.end(), RFL_DB_MAGIC.begin(), RFL_DB_MAGIC.end());
        append_be_u16(bytes, RFL_DB_VERSION);
        append_be_u16(bytes, static_cast<std::uint16_t>(_miis.size()));
        for (const auto &entry : _miis) {
            append_entry(bytes, entry);
        }
        return bytes;
    }

    bool RflService::load_from_bytes(std::span<const std::uint8_t> bytes) {
        auto entries = std::vector<RflMiiEntry>{};
        if (!parse_database(bytes, entries)) {
            auto status = RflDbStatus {
                .nand_bound = _nand != nullptr,
                .db_present = true,
                .fallback_used = false,
                .async_pending = false,
                .byte_count = bytes.size(),
                .entry_count = 0U,
                .loaded_frame = _frame_index,
                .last_error = RFLErrcode_Broken,
            };
            replace_cache({}, status, true);
            return false;
        }

        auto status = RflDbStatus {
            .nand_bound = _nand != nullptr,
            .db_present = true,
            .fallback_used = false,
            .async_pending = false,
            .byte_count = bytes.size(),
            .entry_count = entries.size(),
            .loaded_frame = _frame_index,
            .last_error = RFLErrcode_Success,
        };
        replace_cache(std::move(entries), status, true);
        return true;
    }

    bool RflService::is_initialized() const {
        return _initialized && !_async_pending;
    }

    bool RflService::has_error() const {
        if (!is_initialized()) {
            return false;
        }
        ensure_loaded();
        return _forced_error || (_status.last_error != RFLErrcode_Success && _status.last_error != RFLErrcode_DBNodata);
    }

    const RflDbStatus &RflService::db_status() const {
        _status.async_pending = _async_pending;
        ensure_loaded();
        _status.async_pending = _async_pending;
        return _status;
    }

    std::span<const RflMiiEntry> RflService::valid_miis() const {
        if (!is_initialized()) {
            return {};
        }
        ensure_loaded();
        return _valid_miis;
    }

    std::span<const RflOperationTrace> RflService::trace() const {
        return _trace;
    }

    RFLErrcode RflService::additional_info(RFLAdditionalInfo &info, RFLDataSource source, const RFLMiddleDB *, u16 index) const {
        std::memset(&info, 0, sizeof(info));
        if (!is_supported_source(source)) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::AdditionalInfo,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = RFLErrcode_WrongParam,
                .async_pending = _async_pending,
            });
            return RFLErrcode_WrongParam;
        }
        if (!is_initialized()) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::AdditionalInfo,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = RFLErrcode_NotAvailable,
                .async_pending = _async_pending,
            });
            return RFLErrcode_NotAvailable;
        }
        if (has_error()) {
            const auto result = _forced_error ? RFLErrcode_Broken : load_error_for_status(_status);
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::AdditionalInfo,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = result,
                .byte_count = _status.byte_count,
                .entry_count = _status.entry_count,
                .db_present = _status.db_present,
                .fallback_used = _status.fallback_used,
                .async_pending = _async_pending,
            });
            return result;
        }

        const auto *entry = find_entry(source, index);
        if (entry == nullptr) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::AdditionalInfo,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = RFLErrcode_DBNodata,
                .byte_count = _status.byte_count,
                .entry_count = _status.entry_count,
                .db_present = _status.db_present,
                .fallback_used = _status.fallback_used,
                .async_pending = _async_pending,
            });
            return RFLErrcode_DBNodata;
        }
        if (!entry->available) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::AdditionalInfo,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = RFLErrcode_NotAvailable,
                .byte_count = _status.byte_count,
                .entry_count = _status.entry_count,
                .db_present = _status.db_present,
                .fallback_used = _status.fallback_used,
                .async_pending = _async_pending,
            });
            return RFLErrcode_NotAvailable;
        }

        copy_utf16_text(info.name, RFL_NAME_LEN + 1U, entry->name);
        copy_utf16_text(info.creator, RFL_CREATOR_LEN + 1U, entry->creator);
        info.createID = entry->create_id;
        info.sex = entry->sex;
        info.bmonth = entry->bmonth;
        info.bday = entry->bday;
        info.color = entry->color;
        info.favorite = entry->favorite ? 1U : 0U;
        info.height = entry->height;
        info.build = entry->build;
        info.skinColor = entry->skin_color;
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::AdditionalInfo,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .source = source,
            .index = static_cast<s32>(index),
            .result = RFLErrcode_Success,
            .byte_count = _status.byte_count,
            .entry_count = _status.entry_count,
            .db_present = _status.db_present,
            .fallback_used = _status.fallback_used,
            .async_pending = _async_pending,
        });
        return RFLErrcode_Success;
    }

    bool RflService::search_official_data(const RFLCreateID &id, u16 &index) const {
        if (!is_initialized()) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::SearchOfficial,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .result = RFLErrcode_NotAvailable,
                .async_pending = _async_pending,
            });
            return false;
        }
        ensure_loaded();
        if (!_initialized || has_error()) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::SearchOfficial,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .result = has_error() ? RFLErrcode_Broken : RFLErrcode_NotAvailable,
                .byte_count = _status.byte_count,
                .entry_count = _status.entry_count,
                .db_present = _status.db_present,
                .fallback_used = _status.fallback_used,
                .async_pending = _async_pending,
            });
            return false;
        }

        const auto found = std::find_if(_miis.begin(), _miis.end(), [&id](const auto &entry) {
            return entry.source == RFLDataSource_Official && entry.available && create_ids_equal(entry.create_id, id);
        });
        if (found == _miis.end()) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::SearchOfficial,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .result = RFLErrcode_DBNodata,
                .byte_count = _status.byte_count,
                .entry_count = _status.entry_count,
                .db_present = _status.db_present,
                .fallback_used = _status.fallback_used,
                .async_pending = _async_pending,
            });
            return false;
        }

        index = static_cast<u16>(found->index);
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::SearchOfficial,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .source = RFLDataSource_Official,
            .index = static_cast<s32>(index),
            .result = RFLErrcode_Success,
            .byte_count = _status.byte_count,
            .entry_count = _status.entry_count,
            .db_present = _status.db_present,
            .fallback_used = _status.fallback_used,
            .async_pending = _async_pending,
        });
        return true;
    }

    bool RflService::is_available_official_data(u16 index) const {
        if (!is_initialized()) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::CheckAvailable,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = RFLDataSource_Official,
                .index = static_cast<s32>(index),
                .result = RFLErrcode_NotAvailable,
                .async_pending = _async_pending,
            });
            return false;
        }
        const auto *entry = find_entry(RFLDataSource_Official, index);
        const auto available = entry != nullptr && entry->available && !has_error() && is_initialized();
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::CheckAvailable,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .source = RFLDataSource_Official,
            .index = static_cast<s32>(index),
            .result = available ? RFLErrcode_Success : (entry == nullptr ? RFLErrcode_DBNodata : RFLErrcode_NotAvailable),
            .byte_count = _status.byte_count,
            .entry_count = _status.entry_count,
            .db_present = _status.db_present,
            .fallback_used = _status.fallback_used,
            .async_pending = _async_pending,
        });
        return available;
    }

    std::size_t RflService::model_buffer_size(RFLResolution resolution, u32 expression_flags) const {
        const auto base = effective_resolution(resolution);
        const auto expression_mask = (1U << static_cast<u32>(RFLExp_Max)) - 1U;
        const auto expression_count = std::max(1U, static_cast<u32>(std::popcount(expression_flags & expression_mask)));
        return 0x180U + static_cast<std::size_t>(base) * static_cast<std::size_t>(base) / 8U * expression_count;
    }

    RFLErrcode RflService::init_char_model(RFLCharModel &model, RFLDataSource source, const RFLMiddleDB *db, u16 index, void *work,
                                           RFLResolution resolution, u32 expression_flags) const {
        if (!is_supported_source(source) || expression_flags == 0U) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::InitCharModel,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = RFLErrcode_WrongParam,
                .expression_flags = expression_flags,
            });
            return RFLErrcode_WrongParam;
        }
        if (!available()) {
            const auto result = _async_pending || !_initialized || !_resource_initialized ? RFLErrcode_NotAvailable : RFLErrcode_Broken;
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::InitCharModel,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = result,
                .async_pending = _async_pending,
                .expression_flags = expression_flags,
            });
            return result;
        }

        const auto *entry = find_entry(source, index);
        if (entry == nullptr || !entry->available) {
            const auto result = entry == nullptr ? RFLErrcode_DBNodata : RFLErrcode_NotAvailable;
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::InitCharModel,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = result,
                .byte_count = _status.byte_count,
                .entry_count = _status.entry_count,
                .db_present = _status.db_present,
                .fallback_used = _status.fallback_used,
                .expression_flags = expression_flags,
            });
            return result;
        }

        model.source = source;
        model.middleDB = const_cast<RFLMiddleDB *>(db);
        model.index = index;
        model.resolution = resolution;
        model.expressionFlags = expression_flags;
        model.expression = first_expression_from_flags(expression_flags);
        model.work = work;
        model.initialized = TRUE;
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::InitCharModel,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .source = source,
            .index = static_cast<s32>(index),
            .result = RFLErrcode_Success,
            .byte_count = _status.byte_count,
            .entry_count = _status.entry_count,
            .db_present = _status.db_present,
            .fallback_used = _status.fallback_used,
            .expression = model.expression,
            .expression_flags = expression_flags,
        });
        return RFLErrcode_Success;
    }

    void RflService::set_model_expression(RFLCharModel &model, RFLExpression expression) const {
        if (!is_supported_expression(expression)) {
            return;
        }
        model.expression = expression;
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::SetExpression,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .source = model.source,
            .index = static_cast<s32>(model.index),
            .result = RFLErrcode_Success,
            .expression = expression,
            .expression_flags = model.expressionFlags,
        });
    }

    void RflService::draw_model(const RFLCharModel *model) const {
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::DrawModel,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .source = model != nullptr ? model->source : RFLDataSource_Official,
            .index = model != nullptr ? static_cast<s32>(model->index) : -1,
            .result = model != nullptr && model->initialized ? RFLErrcode_Success : RFLErrcode_WrongParam,
            .expression = model != nullptr ? model->expression : RFLExp_Normal,
            .expression_flags = model != nullptr ? model->expressionFlags : 0U,
        });
    }

    RflIconTexture RflService::make_icon_texture(RFLDataSource source, const RFLMiddleDB *, u16 index, RFLExpression expression,
                                                 const RFLIconSetting &setting) const {
        auto texture = RflIconTexture {
            .source = source,
            .index = index,
            .width = setting.width,
            .height = setting.height,
            .expression = expression,
            .background = setting.bgColor,
            .texture_available = false,
            .rgb5a3 = {},
        };

        auto finish = [&](RFLErrcode result) {
            texture.result = result;
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::MakeIcon,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = result,
                .byte_count = texture.rgb5a3.size(),
                .entry_count = _status.entry_count,
                .db_present = _status.db_present,
                .fallback_used = _status.fallback_used,
                .async_pending = _async_pending,
                .texture_available = texture.texture_available,
                .width = texture.width,
                .height = texture.height,
                .expression = expression,
            });
            return texture;
        };

        if (!is_supported_source(source) || !is_supported_expression(expression) || setting.width == 0U || setting.height == 0U) {
            return finish(RFLErrcode_WrongParam);
        }
        if (!available()) {
            return finish(_async_pending || !_initialized || !_resource_initialized ? RFLErrcode_NotAvailable : RFLErrcode_Broken);
        }

        const auto *entry = find_entry(source, index);
        if (entry == nullptr) {
            return finish(RFLErrcode_DBNodata);
        }
        if (!entry->available) {
            return finish(RFLErrcode_NotAvailable);
        }

        texture.background = setting.bgType == RFLIconBG_Favorite ? favorite_color(entry->color) : setting.bgColor;
        texture.rgb5a3 = make_icon_rgb5a3(*entry, setting.width, setting.height, texture.background, expression, setting.drawXluOnly != FALSE);
        texture.texture_available = !texture.rgb5a3.empty();
        return finish(RFLErrcode_Success);
    }

    RFLErrcode RflService::make_icon(void *buffer, RFLDataSource source, const RFLMiddleDB *db, u16 index, RFLExpression expression,
                                     const RFLIconSetting &setting) const {
        if (buffer == nullptr) {
            push_trace(RflOperationTrace {
                .kind = RflOperationKind::MakeIcon,
                .frame_index = _frame_index,
                .path = NandFileSystemService::rfl_db_path(),
                .source = source,
                .index = static_cast<s32>(index),
                .result = RFLErrcode_WrongParam,
                .width = setting.width,
                .height = setting.height,
                .expression = expression,
            });
            return RFLErrcode_WrongParam;
        }

        const auto texture = make_icon_texture(source, db, index, expression, setting);
        if (texture.result == RFLErrcode_Success) {
            write_icon_res_timg(buffer, texture);
        }
        return texture.result;
    }

    RflMiiSelectPageState RflService::mii_select_page_state(std::span<const RflMiiSelectSpecialIcon> special_icons,
                                                            std::optional<RflMiiSelectIconKey> prohibited, std::size_t page_index,
                                                            std::optional<RflMiiSelectIconKey> selected) const {
        constexpr auto icons_per_page = std::size_t {8U};

        auto state = RflMiiSelectPageState {
            .icons_per_page = icons_per_page,
            .page_index = page_index,
            .page_count = 0U,
            .icon_count = 0U,
            .has_selected_icon = false,
            .has_selected_create_id = false,
            .selected_create_id = {},
            .icons = {},
        };

        auto append_icon = [&](RflMiiSelectIcon icon) {
            icon.prohibited = prohibited.has_value() && mii_select_keys_equal(icon.key, *prohibited);
            icon.selectable = icon.selectable && !icon.prohibited;
            icon.selected = selected.has_value() && mii_select_keys_equal(icon.key, *selected);
            if (icon.selected) {
                state.has_selected_icon = true;
                if (icon.has_create_id) {
                    state.has_selected_create_id = true;
                    state.selected_create_id = icon.create_id;
                }
            }
            state.icons.push_back(std::move(icon));
        };

        for (const auto &special : special_icons) {
            if (!special.valid) {
                continue;
            }

            append_icon(RflMiiSelectIcon {
                .key = RflMiiSelectIconKey {
                    .kind = RflMiiSelectIconKind::Special,
                    .id = special.id,
                    .source = RFLDataSource_Official,
                },
                .name = special.name,
                .favorite = false,
                .selectable = true,
                .prohibited = false,
                .selected = false,
                .texture_available = special.texture_available,
                .page_index = 0U,
                .slot_index = 0U,
                .has_create_id = false,
                .create_id = {},
            });
        }

        const auto rfl_available = available();
        for (const auto &entry : valid_miis()) {
            append_icon(RflMiiSelectIcon {
                .key = RflMiiSelectIconKey {
                    .kind = RflMiiSelectIconKind::Mii,
                    .id = entry.index,
                    .source = entry.source,
                },
                .name = entry.name,
                .favorite = entry.favorite,
                .selectable = true,
                .prohibited = false,
                .selected = false,
                .texture_available = rfl_available,
                .page_index = 0U,
                .slot_index = 0U,
                .has_create_id = true,
                .create_id = entry.create_id,
            });
        }

        state.icon_count = state.icons.size();
        state.page_count = state.icon_count == 0U ? 0U : ((state.icon_count - 1U) / icons_per_page) + 1U;
        if (state.page_count == 0U) {
            state.page_index = 0U;
        } else if (state.page_index >= state.page_count) {
            state.page_index = state.page_count - 1U;
        }

        for (auto index = std::size_t {}; index < state.icons.size(); ++index) {
            state.icons[index].page_index = index / icons_per_page;
            state.icons[index].slot_index = index % icons_per_page;
        }

        push_trace(RflOperationTrace {
            .kind = RflOperationKind::MiiSelectPage,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .result = rfl_available ? RFLErrcode_Success : async_status(),
            .entry_count = _status.entry_count,
            .db_present = _status.db_present,
            .fallback_used = _status.fallback_used,
            .async_pending = _async_pending,
            .texture_available = std::ranges::any_of(state.icons, [](const auto &icon) { return icon.texture_available; }),
            .page_index = state.page_index,
            .page_count = state.page_count,
            .icon_count = state.icon_count,
            .selected = state.has_selected_icon,
            .prohibited = prohibited.has_value(),
        });

        return state;
    }

    void RflService::clear_trace() {
        _trace.clear();
    }

    void RflService::ensure_loaded() const {
        if (_async_pending || !_initialized || _cache_loaded) {
            return;
        }

        auto status = RflDbStatus {
            .nand_bound = _nand != nullptr,
            .db_present = false,
            .fallback_used = false,
            .async_pending = false,
            .byte_count = 0U,
            .entry_count = 0U,
            .loaded_frame = _frame_index,
            .last_error = RFLErrcode_DBNodata,
        };

        if (_nand == nullptr) {
            status.fallback_used = true;
            replace_cache(fallback_miis(), status, false);
            return;
        }

        const auto bytes = _nand->read_file(NandFileSystemService::rfl_db_path());
        if (!bytes.has_value()) {
            status.fallback_used = true;
            replace_cache(fallback_miis(), status, false);
            return;
        }

        status.db_present = true;
        status.byte_count = bytes->size();

        auto entries = std::vector<RflMiiEntry>{};
        if (bytes->empty() || !parse_database(*bytes, entries)) {
            status.last_error = RFLErrcode_Broken;
            replace_cache({}, status, false);
            return;
        }

        status.entry_count = entries.size();
        status.last_error = RFLErrcode_Success;
        replace_cache(std::move(entries), status, false);
    }

    void RflService::rebuild_valid_miis() const {
        _valid_miis.clear();
        for (const auto &entry : _miis) {
            if (entry.source == RFLDataSource_Official && entry.available) {
                _valid_miis.push_back(entry);
            }
        }

        std::sort(_valid_miis.begin(), _valid_miis.end(), [](const auto &lhs, const auto &rhs) {
            if (lhs.favorite != rhs.favorite) {
                return lhs.favorite && !rhs.favorite;
            }
            return lhs.index < rhs.index;
        });
    }

    void RflService::replace_cache(std::vector<RflMiiEntry> miis, RflDbStatus status, bool manual_override) const {
        for (auto &entry : miis) {
            entry = normalized_entry(std::move(entry));
        }
        std::sort(miis.begin(), miis.end(), [](const auto &lhs, const auto &rhs) {
            if (lhs.source != rhs.source) {
                return static_cast<int>(lhs.source) < static_cast<int>(rhs.source);
            }
            return lhs.index < rhs.index;
        });

        status.entry_count = miis.size();
        status.resource_initialized = _resource_initialized;
        status.deluxe_textures = _deluxe_textures;
        status.last_reason = _last_reason;
        _miis = std::move(miis);
        _status = status;
        _status.async_pending = _async_pending;
        _cache_loaded = true;
        _manual_override = manual_override;
        rebuild_valid_miis();
        push_trace(RflOperationTrace {
            .kind = status.last_error == RFLErrcode_Success || status.last_error == RFLErrcode_DBNodata ? RflOperationKind::LoadComplete :
                                                                                                          RflOperationKind::LoadFailed,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .result = status.last_error,
            .byte_count = status.byte_count,
            .entry_count = _miis.size(),
            .db_present = status.db_present,
            .fallback_used = status.fallback_used,
            .async_pending = _async_pending,
        });
    }

    void RflService::push_trace(RflOperationTrace trace) const {
        _trace.push_back(std::move(trace));
    }

    const RflMiiEntry *RflService::find_entry(RFLDataSource source, u16 index) const {
        ensure_loaded();
        const auto found = std::find_if(_miis.begin(), _miis.end(), [source, index](const auto &entry) {
            return entry.source == source && entry.index == static_cast<s32>(index);
        });
        return found == _miis.end() ? nullptr : &*found;
    }

    std::vector<RflMiiEntry> RflService::fallback_miis() {
        auto mario = RflMiiEntry {
            .index = 0,
            .source = RFLDataSource_Official,
            .available = true,
            .name = "Mario",
            .creator = "SMGPC",
            .favorite = true,
            .height = 64U,
            .build = 64U,
            .skin_color = GXColor {255U, 224U, 189U, 255U},
        };
        mario.create_id = create_id_from_index(mario.index);

        auto default_mii = mario;
        default_mii.source = RFLDataSource_Default;
        return {std::move(mario), std::move(default_mii)};
    }

}  // namespace smgpc::runtime
