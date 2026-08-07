#include "runtime/RflService.hpp"

#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <utility>

namespace smgpc::runtime {
    namespace {

        constexpr auto RFL_BASE_WORK_SIZE = std::size_t {0x4CF24U};
        constexpr auto RFL_DELUXE_WORK_SIZE = std::size_t {0x65F24U};
        constexpr auto RFL_CHAR_MODEL_RESOURCE_SIZE = std::size_t {0x8260U};
        constexpr auto RFL_PPC_TEX_OBJ_SIZE = std::size_t {0x20U};
        constexpr auto RFL_RESOURCE_ARCHIVE_COUNT = std::uint16_t {18U};
        constexpr auto RFL_RESOURCE_PATH = std::string_view {"/ObjectData/MiiFaceDatabase.arc/RFL_Res.dat"};

        [[nodiscard]] bool is_supported_source(RFLDataSource source) {
            const auto value = static_cast<int>(source);
            return value >= static_cast<int>(RFLDataSource_Official) && value < static_cast<int>(RFLDataSource_Max);
        }

        [[nodiscard]] bool is_supported_expression(RFLExpression expression) {
            const auto value = static_cast<int>(expression);
            return value >= static_cast<int>(RFLExp_Normal) && value < static_cast<int>(RFLExp_Max);
        }

        [[nodiscard]] constexpr std::size_t round_up_32(std::size_t value) {
            return (value + 31U) & ~std::size_t {31U};
        }

        [[nodiscard]] std::size_t mask_buffer_size(RFLResolution resolution) {
            const auto flags = static_cast<u32>(resolution);
            constexpr std::uint32_t dimensions[] = {32U, 64U, 128U, 256U};
            auto size = std::size_t {};
            for (const auto dimension : dimensions) {
                if ((flags & dimension) != 0U) {
                    size += 2U * dimension * dimension;
                }
            }
            return size;
        }

        [[nodiscard]] std::uint16_t read_be_u16(std::span<const std::uint8_t> bytes, std::size_t offset) {
            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) | bytes[offset + 1U]);
        }

        [[nodiscard]] std::uint32_t read_be_u32(std::span<const std::uint8_t> bytes, std::size_t offset) {
            return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
                   static_cast<std::uint32_t>(bytes[offset + 3U]);
        }

        [[nodiscard]] bool contains_range(std::size_t size, std::size_t offset, std::size_t length) {
            return offset <= size && length <= size - offset;
        }

        [[nodiscard]] bool is_valid_resource(std::span<const std::uint8_t> bytes) {
            const auto header_size = std::size_t {4U} +
                                     static_cast<std::size_t>(RFL_RESOURCE_ARCHIVE_COUNT) * sizeof(std::uint32_t);
            if (bytes.size() < header_size || read_be_u16(bytes, 0U) != RFL_RESOURCE_ARCHIVE_COUNT ||
                read_be_u16(bytes, 2U) == 0U) {
                return false;
            }

            auto previous_end = header_size;
            for (auto archive_index = std::size_t {}; archive_index < RFL_RESOURCE_ARCHIVE_COUNT; ++archive_index) {
                const auto section_offset = static_cast<std::size_t>(read_be_u32(bytes, 4U + archive_index * sizeof(std::uint32_t)));
                if (section_offset < previous_end || !contains_range(bytes.size(), section_offset, 4U)) {
                    return false;
                }

                const auto file_count = static_cast<std::size_t>(read_be_u16(bytes, section_offset));
                const auto biggest_size = static_cast<std::size_t>(read_be_u16(bytes, section_offset + 2U));
                if (file_count == 0U) {
                    return false;
                }

                const auto table_offset = section_offset + 4U;
                const auto table_size = (file_count + 1U) * sizeof(std::uint32_t);
                if (!contains_range(bytes.size(), table_offset, table_size) || read_be_u32(bytes, table_offset) != 0U) {
                    return false;
                }

                auto previous_file_end = std::size_t {};
                auto largest_file = std::size_t {};
                for (auto file_index = std::size_t {}; file_index < file_count; ++file_index) {
                    const auto next_file_end = static_cast<std::size_t>(
                        read_be_u32(bytes, table_offset + (file_index + 1U) * sizeof(std::uint32_t)));
                    if (next_file_end < previous_file_end) {
                        return false;
                    }
                    largest_file = std::max(largest_file, next_file_end - previous_file_end);
                    previous_file_end = next_file_end;
                }

                const auto data_offset = table_offset + table_size;
                if (largest_file != biggest_size || !contains_range(bytes.size(), data_offset, previous_file_end)) {
                    return false;
                }
                previous_end = data_offset + previous_file_end;
            }

            return true;
        }

        [[nodiscard]] bool is_empty_create_id(const RFLCreateID &id) {
            return std::ranges::all_of(id.data, [](u8 value) { return value == 0U; });
        }

        [[nodiscard]] bool create_ids_equal(const RFLCreateID &lhs, const RFLCreateID &rhs) {
            return std::memcmp(lhs.data, rhs.data, sizeof(lhs.data)) == 0;
        }

        [[nodiscard]] bool mii_select_keys_equal(const RflMiiSelectIconKey &lhs, const RflMiiSelectIconKey &rhs) {
            return lhs.kind == rhs.kind && lhs.id == rhs.id && lhs.source == rhs.source;
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

    }  // namespace

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
        if (_resource_init_pending) {
            _resource_initialized = true;
            _resource_init_pending = false;
        }
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

    std::size_t RflService::work_size(bool deluxe_textures) {
        return deluxe_textures ? RFL_DELUXE_WORK_SIZE : RFL_BASE_WORK_SIZE;
    }

    RFLErrcode RflService::init_resources(void *work_buffer, const void *resource_buffer, std::size_t resource_size, bool deluxe_textures,
                                          bool async) {
        _deluxe_textures = deluxe_textures;
        _resource_initialized = false;
        _resource_init_pending = false;
        _last_reason = 0;

        auto result = RFLErrcode_Success;
        if (work_buffer == nullptr) {
            result = RFLErrcode_WrongParam;
            _last_reason = NAND_RESULT_INVALID;
        } else if (resource_buffer == nullptr || resource_size == 0U) {
            result = RFLErrcode_Fatal;
            _last_reason = NAND_RESULT_NOEXISTS;
        } else if (!is_valid_resource(std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(resource_buffer), resource_size))) {
            result = RFLErrcode_Broken;
            _last_reason = NAND_RESULT_CORRUPT;
        }

        _status.resource_initialized = result == RFLErrcode_Success && !async;
        _status.deluxe_textures = deluxe_textures;
        _status.last_error = result == RFLErrcode_Success ? _status.last_error : result;
        _status.last_reason = _last_reason;
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::InitResource,
            .frame_index = _frame_index,
            .path = std::string {RFL_RESOURCE_PATH},
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
            _resource_init_pending = true;
            request_async_load(1U);
            return RFLErrcode_Busy;
        }

        _resource_initialized = true;
        _initialized = true;
        return RFLErrcode_Success;
    }

    void RflService::exit() {
        _resource_initialized = false;
        _resource_init_pending = false;
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
        if (_forced_error) {
            return _status.last_error != RFLErrcode_Success && _status.last_error != RFLErrcode_DBNodata ?
                       _status.last_error :
                       RFLErrcode_Broken;
        }
        if (!_resource_initialized || !_initialized) {
            return RFLErrcode_NotAvailable;
        }
        if (has_error()) {
            return load_error_for_status(_status);
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
        _miis.clear();
        _valid_miis.clear();
        _status = RflDbStatus {};
        _status.resource_initialized = _resource_initialized;
        _status.deluxe_textures = _deluxe_textures;
        _status.last_reason = _last_reason;
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
                .async_pending = _async_pending,
            });
            return false;
        }

        const auto found = std::find_if(_miis.begin(), _miis.end(), [&id](const auto &entry) {
            return !is_empty_create_id(id) && !is_empty_create_id(entry.create_id) &&
                   entry.source == RFLDataSource_Official && entry.available && create_ids_equal(entry.create_id, id);
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
            .async_pending = _async_pending,
        });
        return available;
    }

    std::size_t RflService::model_buffer_size(RFLResolution resolution, u32 expression_flags) {
        const auto expression_mask = (1U << static_cast<u32>(RFLExp_Max)) - 1U;
        const auto expression_count = static_cast<std::size_t>(std::popcount(expression_flags & expression_mask));
        return round_up_32(expression_count * RFL_PPC_TEX_OBJ_SIZE) + round_up_32(RFL_CHAR_MODEL_RESOURCE_SIZE) +
               round_up_32(mask_buffer_size(resolution) * expression_count);
    }

    RFLErrcode RflService::init_char_model(RFLCharModel &model, RFLDataSource source, const RFLMiddleDB *db, u16 index, void *work,
                                           RFLResolution resolution, u32 expression_flags) const {
        (void)model;
        (void)db;
        (void)work;
        (void)resolution;
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
            const auto result = async_status();
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
                .expression_flags = expression_flags,
            });
            return result;
        }

        push_trace(RflOperationTrace {
            .kind = RflOperationKind::InitCharModel,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .source = source,
            .index = static_cast<s32>(index),
            .result = RFLErrcode_NotAvailable,
            .byte_count = _status.byte_count,
            .entry_count = _status.entry_count,
            .db_present = _status.db_present,
            .expression_flags = expression_flags,
        });
        return RFLErrcode_NotAvailable;
    }

    void RflService::set_model_expression(RFLCharModel &model, RFLExpression expression) const {
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::SetExpression,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .source = model.source,
            .index = static_cast<s32>(model.index),
            .result = !is_supported_expression(expression) || model.initialized == FALSE ? RFLErrcode_WrongParam : RFLErrcode_NotAvailable,
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
            .result = model != nullptr && model->initialized ? RFLErrcode_NotAvailable : RFLErrcode_WrongParam,
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
            return finish(async_status());
        }

        const auto *entry = find_entry(source, index);
        if (entry == nullptr) {
            return finish(RFLErrcode_DBNodata);
        }
        if (!entry->available) {
            return finish(RFLErrcode_NotAvailable);
        }

        return finish(RFLErrcode_NotAvailable);
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

        return make_icon_texture(source, db, index, expression, setting).result;
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
                .has_create_id = !is_empty_create_id(entry.create_id),
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

        const auto page_result = rfl_available && _status.last_error != RFLErrcode_DBNodata ?
                                     RFLErrcode_Success :
                                     (rfl_available ? RFLErrcode_DBNodata : async_status());
        push_trace(RflOperationTrace {
            .kind = RflOperationKind::MiiSelectPage,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .result = page_result,
            .entry_count = _status.entry_count,
            .db_present = _status.db_present,
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
            .async_pending = false,
            .byte_count = 0U,
            .entry_count = 0U,
            .loaded_frame = _frame_index,
            .last_error = RFLErrcode_DBNodata,
        };

        if (_nand == nullptr) {
            replace_cache(status);
            return;
        }

        const auto bytes = _nand->read_file(NandFileSystemService::rfl_db_path());
        if (!bytes.has_value()) {
            replace_cache(status);
            return;
        }

        status.db_present = true;
        status.byte_count = bytes->size();
        status.last_error = RFLErrcode_NotAvailable;
        replace_cache(status);
    }

    void RflService::replace_cache(RflDbStatus status) const {
        status.entry_count = 0U;
        status.resource_initialized = _resource_initialized;
        status.deluxe_textures = _deluxe_textures;
        status.last_reason = _last_reason;
        _miis.clear();
        _valid_miis.clear();
        _status = status;
        _status.async_pending = _async_pending;
        _cache_loaded = true;
        push_trace(RflOperationTrace {
            .kind = status.last_error == RFLErrcode_Success || status.last_error == RFLErrcode_DBNodata ? RflOperationKind::LoadComplete :
                                                                                                          RflOperationKind::LoadFailed,
            .frame_index = _frame_index,
            .path = NandFileSystemService::rfl_db_path(),
            .result = status.last_error,
            .byte_count = status.byte_count,
            .entry_count = _miis.size(),
            .db_present = status.db_present,
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

}  // namespace smgpc::runtime
