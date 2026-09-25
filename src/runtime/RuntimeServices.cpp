#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include "Game/Screen/StarPointerTarget.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "RuntimeServices.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/System/WPadRumbleData.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/BmgMessageArchive.hpp"
#include "resource/TextEncoding.hpp"

namespace smgpc::runtime {
    namespace {

        [[nodiscard]] bool exists_regular_file(const std::filesystem::path &path) {
            std::error_code error{};
            return std::filesystem::is_regular_file(path, error);
        }

        constexpr auto SAVE_DATA_CONTAINER_NAME = std::string_view{"GameData.bin"};
        constexpr auto SAVE_DATA_VERSION = std::uint32_t{2U};
        constexpr auto SAVE_DATA_FILE_INFO_SIZE = std::size_t{16U};
        constexpr auto SAVE_DATA_FILE_NAME_SIZE = std::size_t{12U};
        constexpr auto SAVE_DATA_HEADER_SIZE = std::size_t{16U};
        constexpr auto SAVE_DATA_GAME_FILE_SIZE = std::size_t{0xF80U};
        constexpr auto SAVE_DATA_CONFIG_FILE_SIZE = std::size_t{0x60U};
        constexpr auto SAVE_DATA_SYSTEM_FILE_SIZE = std::size_t{0x80U};
        enum class SaveDataByteOrder {
            BigEndian,
            LittleEndian,
        };

        [[nodiscard]] std::uint16_t read_save_u16(std::span<const std::uint8_t> bytes, std::size_t offset, SaveDataByteOrder byte_order) {
            if (offset + sizeof(std::uint16_t) > bytes.size()) {
                return 0U;
            }

            if (byte_order == SaveDataByteOrder::BigEndian) {
                return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) | bytes[offset + 1U]);
            }
            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset + 1U]) << 8U) | bytes[offset]);
        }

        [[nodiscard]] std::uint32_t read_save_u32(std::span<const std::uint8_t> bytes, std::size_t offset, SaveDataByteOrder byte_order) {
            if (offset + sizeof(std::uint32_t) > bytes.size()) {
                return 0U;
            }

            if (byte_order == SaveDataByteOrder::BigEndian) {
                return (static_cast<std::uint32_t>(bytes[offset]) << 24U) | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
                       (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) | bytes[offset + 3U];
            }
            return (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U) | (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) | bytes[offset];
        }

        void write_save_u32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value,
                            SaveDataByteOrder byte_order = SaveDataByteOrder::BigEndian) {
            if (offset + sizeof(value) > bytes.size()) {
                return;
            }

            if (byte_order == SaveDataByteOrder::BigEndian) {
                bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
                bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
                bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
                bytes[offset + 3U] = static_cast<std::uint8_t>(value);
            } else {
                bytes[offset] = static_cast<std::uint8_t>(value);
                bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 8U);
                bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 16U);
                bytes[offset + 3U] = static_cast<std::uint8_t>(value >> 24U);
            }
        }

        [[nodiscard]] std::uint32_t save_check_sum(std::span<const std::uint8_t> bytes, SaveDataByteOrder byte_order) {
            auto sum = std::uint16_t{};
            auto inv_sum = std::uint16_t{};
            const auto word_count = bytes.size() / sizeof(std::uint16_t);
            for (auto index = std::size_t{}; index < word_count; ++index) {
                const auto word = read_save_u16(bytes, index * sizeof(std::uint16_t), byte_order);
                sum = static_cast<std::uint16_t>(sum + word);
                inv_sum = static_cast<std::uint16_t>(inv_sum + static_cast<std::uint16_t>(~word));
            }
            return (static_cast<std::uint32_t>(sum) << 16U) | inv_sum;
        }

        [[nodiscard]] std::uint32_t align_save_data_size(std::uint32_t size) {
            return (size + 0x1FU) & ~0x1FU;
        }

        [[nodiscard]] bool is_valid_save_data_container_shape(std::uint32_t version, std::uint32_t file_count,
                                                              std::uint32_t data_size, std::size_t byte_count) {
            return version == SAVE_DATA_VERSION && file_count > 0U && file_count < 24U &&
                   data_size >= SAVE_DATA_HEADER_SIZE + file_count * SAVE_DATA_FILE_INFO_SIZE &&
                   align_save_data_size(data_size) == byte_count;
        }

        [[nodiscard]] bool has_valid_save_data_checksum(std::span<const std::uint8_t> bytes,
                                                        SaveDataByteOrder byte_order) {
            if (bytes.size() < SAVE_DATA_HEADER_SIZE) {
                return false;
            }
            const auto version = read_save_u32(bytes, 4U, byte_order);
            const auto file_count = read_save_u32(bytes, 8U, byte_order);
            const auto data_size = read_save_u32(bytes, 12U, byte_order);
            if (!is_valid_save_data_container_shape(version, file_count, data_size, bytes.size())) {
                return false;
            }
            const auto expected = read_save_u32(bytes, 0U, byte_order);
            const auto actual = save_check_sum(bytes.subspan(sizeof(std::uint32_t), data_size - sizeof(std::uint32_t)),
                                               byte_order);
            return expected == actual;
        }

        [[nodiscard]] std::vector<std::uint8_t> convert_save_data_container_byte_order(std::span<const std::uint8_t> bytes,
                                                                                       SaveDataByteOrder source_byte_order,
                                                                                       SaveDataByteOrder destination_byte_order) {
            auto converted = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
            const auto version = read_save_u32(bytes, 4U, source_byte_order);
            const auto file_count = read_save_u32(bytes, 8U, source_byte_order);
            const auto data_size = read_save_u32(bytes, 12U, source_byte_order);
            if (!has_valid_save_data_checksum(bytes, source_byte_order)) {
                aurora::throw_host_exception<std::invalid_argument>("Save-data byte-order conversion requires a valid source container");
            }

            write_save_u32(converted, 4U, version, destination_byte_order);
            write_save_u32(converted, 8U, file_count, destination_byte_order);
            write_save_u32(converted, 12U, data_size, destination_byte_order);
            for (auto file_index = std::uint32_t{}; file_index < file_count; ++file_index) {
                const auto info_offset = SAVE_DATA_HEADER_SIZE + static_cast<std::size_t>(file_index) * SAVE_DATA_FILE_INFO_SIZE;
                write_save_u32(converted, info_offset + SAVE_DATA_FILE_NAME_SIZE,
                               read_save_u32(bytes, info_offset + SAVE_DATA_FILE_NAME_SIZE, source_byte_order), destination_byte_order);
            }

            write_save_u32(converted, 0U,
                           save_check_sum(std::span<const std::uint8_t>(converted).subspan(sizeof(std::uint32_t), data_size - sizeof(std::uint32_t)),
                                          destination_byte_order),
                           destination_byte_order);
            return converted;
        }

        [[nodiscard]] std::vector<std::uint8_t> retail_save_data_container_for_host(
            std::span<const std::uint8_t> retail_bytes) {
            if (!has_valid_save_data_checksum(retail_bytes, SaveDataByteOrder::BigEndian)) {
                aurora::throw_host_exception<std::invalid_argument>("Persisted GameData.bin is not a valid retail big-endian container");
            }
            return convert_save_data_container_byte_order(retail_bytes, SaveDataByteOrder::BigEndian,
                                                          SaveDataByteOrder::LittleEndian);
        }

        [[nodiscard]] std::vector<std::uint8_t> host_save_data_container_for_retail(
            std::span<const std::uint8_t> host_bytes) {
            if (!has_valid_save_data_checksum(host_bytes, SaveDataByteOrder::LittleEndian)) {
                aurora::throw_host_exception<std::invalid_argument>("Host save buffer is not a valid translated retail container");
            }
            return convert_save_data_container_byte_order(host_bytes, SaveDataByteOrder::LittleEndian,
                                                          SaveDataByteOrder::BigEndian);
        }

        [[nodiscard]] std::vector<std::uint8_t> convert_nand_banner_byte_order(
            std::span<const std::uint8_t> bytes, SaveDataByteOrder source_byte_order,
            SaveDataByteOrder destination_byte_order) {
            constexpr std::size_t texture_offset = 0xa0;
            constexpr std::size_t banner_texture_size = 192 * 64 * 2;
            constexpr std::size_t icon_texture_size = 48 * 48 * 2;
            constexpr std::size_t base_size = texture_offset + banner_texture_size;
            if (bytes.size() < base_size || bytes.size() > base_size + 8 * icon_texture_size ||
                (bytes.size() - base_size) % icon_texture_size != 0 ||
                read_save_u32(bytes, 0, source_byte_order) != 0x5749424eU) {
                aurora::throw_host_exception<std::invalid_argument>("NAND banner requires a valid WIBN header and complete texture records");
            }
            auto converted = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
            write_save_u32(converted, 0, read_save_u32(bytes, 0, source_byte_order), destination_byte_order);
            write_save_u32(converted, 4, read_save_u32(bytes, 4, source_byte_order), destination_byte_order);
            if (source_byte_order != destination_byte_order) {
                std::swap(converted[8], converted[9]);
                for (std::size_t offset = 0x20; offset < texture_offset; offset += 2) {
                    std::swap(converted[offset], converted[offset + 1]);
                }
            }
            // GX texture data is already encoded by the original BTI resource.
            return converted;
        }

        [[nodiscard]] std::optional<std::size_t> save_data_file_size(std::string_view name) {
            if (name.starts_with("mario") || name.starts_with("luigi")) {
                return SAVE_DATA_GAME_FILE_SIZE;
            }
            if (name.starts_with("config")) {
                return SAVE_DATA_CONFIG_FILE_SIZE;
            }
            if (name == "sysconf") {
                return SAVE_DATA_SYSTEM_FILE_SIZE;
            }
            return std::nullopt;
        }

        struct StarPointerProjection {
            float x = 0.0F;
            float y = 0.0F;
            float radius = 0.0F;
        };

        [[nodiscard]] std::optional<StarPointerProjection> project_star_pointer_target(const StarPointerTargetState &target, const smgpc::camera::CameraPose &pose, bool check_z) {
            if (target.actor == nullptr) {
                return std::nullopt;
            }

            constexpr auto PI = 3.14159265358979323846F;
            TVec3f position;
            target.actor->mStarPointerTarget->calcPosition(&position);
            const auto world = smgpc::camera::CameraParamVec3{
                .x = position.x,
                .y = position.y,
                .z = position.z,
            };
            const auto camera = smgpc::camera::transform_world_to_camera(pose, world);
            if (check_z && camera.z <= pose.near_clip) {
                return std::nullopt;
            }

            const auto depth = std::abs(camera.z);
            if (depth <= 0.0001F) {
                return std::nullopt;
            }

            const auto fovy = pose.fovy_degrees * PI / 180.0F;
            const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
            const auto focal_x = focal_y / pose.aspect_ratio;
            const auto half_width = static_cast<float>(render::core::kWiiLogicalFramebufferWidth) * 0.5F;
            const auto half_height = static_cast<float>(render::core::kWiiLogicalFramebufferHeight) * 0.5F;
            const auto ndc_x = (camera.x / camera.z) * focal_x + pose.projection_offset_x;
            const auto ndc_y = (camera.y / camera.z) * focal_y + pose.projection_offset_y;
            return StarPointerProjection{
                .x = (ndc_x * half_width) + half_width,
                .y = (ndc_y * half_height) + half_height,
                .radius = std::max((target.actor->mStarPointerTarget->mRadius3d / depth) * focal_y * half_height, 1.0F),
            };
        }

        [[nodiscard]] std::filesystem::path weakly_canonical_or_normal(const std::filesystem::path &path) {
            std::error_code error{};
            auto canonical = std::filesystem::weakly_canonical(path, error);
            if (!error) {
                return canonical;
            }

            return path.lexically_normal();
        }

        [[nodiscard]] std::vector<std::uint8_t> read_binary_file(const std::filesystem::path &path) {
            auto file = std::ifstream(path, std::ios::binary);
            if (!file) {
                aurora::throw_host_exception<std::runtime_error>("Cannot open save file " + path.string());
            }

            file.seekg(0, std::ios::end);
            const auto size = file.tellg();
            if (size < 0) {
                aurora::throw_host_exception<std::runtime_error>("Cannot determine save file size " + path.string());
            }

            auto bytes = std::vector<std::uint8_t>(static_cast<std::size_t>(size));
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!file) {
                aurora::throw_host_exception<std::runtime_error>("Cannot read save file " + path.string());
            }

            return bytes;
        }

        void write_binary_file(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
            std::error_code error{};
            std::filesystem::create_directories(path.parent_path(), error);
            if (error) {
                aurora::throw_host_exception<std::runtime_error>("Cannot create save directory " + path.parent_path().string());
            }

            auto file = std::ofstream(path, std::ios::binary | std::ios::trunc);
            if (!file) {
                aurora::throw_host_exception<std::runtime_error>("Cannot open save file for writing " + path.string());
            }

            file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!file) {
                aurora::throw_host_exception<std::runtime_error>("Cannot write save file " + path.string());
            }
        }

    }  // namespace

    DvdFileSystemService::DvdFileSystemService(std::filesystem::path root) : _root(std::move(root)) {
    }

    void DvdFileSystemService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        complete_ready_async_reads();
    }

    const std::filesystem::path &DvdFileSystemService::root() const {
        return _root;
    }

    std::string DvdFileSystemService::normalize_disc_path_string(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path(disc_path);
        const auto key = entry_key(normalized);
        return key.empty() ? std::string("/") : "/" + key;
    }

    std::filesystem::path DvdFileSystemService::resolve(std::string_view disc_path) const {
        auto key = normalize_disc_path_string(disc_path);
        // DVDConvertPathToEntrynum compares ASCII letters without case. Native
        // archive and resource caches must use that same identity, including
        // fixed memory mounts which do not have an entry in the disc FST.
        for (char& value : key)
            if (value >= 'A' && value <= 'Z') value += 'a' - 'A';
        return std::filesystem::path(std::move(key));
    }

    bool DvdFileSystemService::exists(std::string_view disc_path) const {
        return entry_metadata(disc_path).has_value();
    }

    s32 DvdFileSystemService::entry_num(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path_string(disc_path);
        return DVDConvertPathToEntrynum(normalized.c_str());
    }

    std::optional<DvdEntryMetadata> DvdFileSystemService::entry_metadata(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path_string(disc_path);
        const auto entry = DVDConvertPathToEntrynum(normalized.c_str());
        if (entry < 0) {
            return std::nullopt;
        }

        auto file_info = DVDFileInfo{};
        if (DVDOpen(normalized.c_str(), &file_info)) {
            const auto length = file_info.length;
            (void)DVDClose(&file_info);
            return DvdEntryMetadata{
                .entry_num = entry,
                .disc_path = normalized,
                .resolved_path = normalized,
                .is_directory = false,
                .length = length,
            };
        }

        auto dir = DVDDir{};
        if (DVDOpenDir(normalized.c_str(), &dir)) {
            (void)DVDCloseDir(&dir);
            return DvdEntryMetadata{
                .entry_num = entry,
                .disc_path = normalized,
                .resolved_path = normalized,
                .is_directory = true,
                .length = 0U,
            };
        }

        return std::nullopt;
    }

    std::optional<DvdEntryMetadata> DvdFileSystemService::entry_metadata(s32 entry_num) const {
        if (entry_num < 0) {
            return std::nullopt;
        }

        auto file_info = DVDFileInfo{};
        if (DVDFastOpen(entry_num, &file_info)) {
            const auto length = file_info.length;
            (void)DVDClose(&file_info);
            return DvdEntryMetadata{
                .entry_num = entry_num,
                .disc_path = std::to_string(entry_num),
                .resolved_path = std::to_string(entry_num),
                .is_directory = false,
                .length = length,
            };
        }

        auto dir = DVDDir{};
        if (DVDFastOpenDir(entry_num, &dir)) {
            (void)DVDCloseDir(&dir);
            return DvdEntryMetadata{
                .entry_num = entry_num,
                .disc_path = std::to_string(entry_num),
                .resolved_path = std::to_string(entry_num),
                .is_directory = true,
                .length = 0U,
            };
        }

        return std::nullopt;
    }

    std::vector<DvdDirectoryEntry> DvdFileSystemService::directory_entries(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path_string(disc_path);
        auto dir = DVDDir{};
        if (!DVDOpenDir(normalized.c_str(), &dir)) {
            return {};
        }

        auto entries = std::vector<DvdDirectoryEntry>{};
        auto dir_entry = DVDDirEntry{};
        while (DVDReadDir(&dir, &dir_entry)) {
            entries.push_back(DvdDirectoryEntry{
                .entry_num = static_cast<s32>(dir_entry.entryNum),
                .disc_path = (std::filesystem::path(normalized) / (dir_entry.name != nullptr ? dir_entry.name : "")).generic_string(),
                .name = dir_entry.name != nullptr ? dir_entry.name : "",
                .is_directory = dir_entry.isDir != FALSE,
            });
        }
        (void)DVDCloseDir(&dir);

        return entries;
    }

    std::optional<std::filesystem::path> DvdFileSystemService::find_first(std::initializer_list<std::filesystem::path> candidates) const {
        for (const auto &candidate : candidates) {
            const auto path = normalize_disc_path_string(candidate.generic_string());
            if (exists(path)) {
                return std::filesystem::path(path);
            }
        }

        return std::nullopt;
    }

    std::optional<std::filesystem::path> DvdFileSystemService::find_layout_archive(std::string_view layout_name) const {
        const auto archive_name = std::string(layout_name) + ".arc";
        return find_first({
            std::filesystem::path("KrKorean") / "LayoutData" / archive_name,
            std::filesystem::path("LayoutData") / archive_name,
        });
    }

    std::optional<std::filesystem::path> DvdFileSystemService::find_object_archive(std::string_view object_name) const {
        const auto archive_name = std::string(object_name) + ".arc";
        return find_first({
            std::filesystem::path("ObjectData") / archive_name,
        });
    }

    std::vector<std::uint8_t> DvdFileSystemService::read_file(std::string_view disc_path) const {
        return read_file_range(disc_path, 0U, std::numeric_limits<std::size_t>::max(), 0);
    }

    std::vector<std::uint8_t> DvdFileSystemService::read_file_range(std::string_view disc_path, std::size_t offset, std::size_t length,
                                                                    s32 priority) const {
        const auto entry = entry_metadata(disc_path);
        if (!entry.has_value() || entry->is_directory) {
            aurora::throw_host_exception<std::runtime_error>("Cannot open DVD file " + std::string(disc_path));
        }
        if (offset > entry->length) {
            aurora::throw_host_exception<std::runtime_error>("DVD read offset is outside file " + std::string(disc_path));
        }

        const auto read_size = std::min(length, entry->length - offset);
        auto bytes = std::vector<std::uint8_t>(read_size);
        auto file_info = DVDFileInfo{};
        if (!DVDOpen(entry->disc_path.c_str(), &file_info)) {
            aurora::throw_host_exception<std::runtime_error>("Cannot open DVD file " + entry->disc_path);
        }
        const auto result = DVDReadPrio(&file_info, bytes.data(), static_cast<s32>(bytes.size()), static_cast<s32>(offset), priority);
        (void)DVDClose(&file_info);
        if (result < 0) {
            aurora::throw_host_exception<std::runtime_error>("Cannot read DVD file " + entry->disc_path);
        }
        bytes.resize(static_cast<std::size_t>(result));

        _file_read_trace.push_back(DvdFileReadTrace{
            .requested_path = std::string(disc_path),
            .disc_path = entry->disc_path,
            .resolved_path = entry->resolved_path,
            .entry_num = entry->entry_num,
            .byte_count = bytes.size(),
            .offset = offset,
            .priority = priority,
        });

        return bytes;
    }

    std::uint64_t DvdFileSystemService::submit_async_read(std::string_view disc_path, DVDFileInfo *file_info, void *destination,
                                                          std::size_t length, std::size_t offset, s32 priority, DVDCallback callback,
                                                          std::uint64_t delay_frames) {
        const auto entry = entry_metadata(disc_path);
        if (!entry.has_value() || entry->is_directory) {
            aurora::throw_host_exception<std::runtime_error>("Cannot queue DVD read for " + std::string(disc_path));
        }

        auto request = DvdAsyncReadRequest{};
        request.id = _next_async_read_id++;
        request.disc_path = entry->disc_path;
        request.entry_num = entry->entry_num;
        request.file_info = file_info;
        request.destination = destination;
        request.length = length;
        request.offset = offset;
        request.priority = priority;
        request.submitted_frame = _frame_index;
        request.completion_frame = _frame_index + delay_frames;
        request.callback = callback;
        _async_read_trace.push_back(request);
        complete_ready_async_reads();
        return request.id;
    }

    smgpc::resource::RarcArchive &DvdFileSystemService::archive(std::string_view disc_path) {
        return archive_for_path_with_request(std::filesystem::path(normalize_disc_path_string(disc_path)), disc_path);
    }

    smgpc::resource::RarcArchive &DvdFileSystemService::archive_for_path(const std::filesystem::path &path) {
        return archive_for_path_with_request(path, path.generic_string());
    }

    std::shared_ptr<const smgpc::resource::RarcArchive> DvdFileSystemService::retain_archive_for_path(const std::filesystem::path &path) {
        (void)archive_for_path(path);
        return _archives.at(archive_cache_key_for_path(path));
    }

    smgpc::resource::RarcArchive &DvdFileSystemService::archive_for_path_with_request(const std::filesystem::path &path, std::string_view requested_path) {
        smgpc::compat::JkrHostAllocationScope host;
        const auto key = archive_cache_key_for_path(path);
        if (auto it = _archives.find(key); it != _archives.end()) {
            _archive_load_trace.push_back(DvdArchiveLoadTrace{
                .requested_path = std::string(requested_path),
                .resolved_path = key,
                .cache_hit = true,
                .load_count = archive_load_count_for_path(path),
                .cached_archive_count = _archives.size(),
                .resource_count = it->second->entries().size(),
            });
            return *it->second;
        }

        auto archive = std::make_shared<smgpc::resource::RarcArchive>(smgpc::resource::RarcArchive::from_bytes(read_file(key)));
        auto [it, inserted] = _archives.emplace(key, std::move(archive));
        if (inserted) {
            ++_archive_load_counts[key];
        }
        _archive_load_trace.push_back(DvdArchiveLoadTrace{
            .requested_path = std::string(requested_path),
            .resolved_path = key,
            .cache_hit = false,
            .load_count = archive_load_count_for_path(path),
            .cached_archive_count = _archives.size(),
            .resource_count = it->second->entries().size(),
        });

        return *it->second;
    }

    std::size_t DvdFileSystemService::archive_load_count(std::string_view disc_path) const {
        return archive_load_count_for_path(resolve(disc_path));
    }

    std::size_t DvdFileSystemService::archive_load_count_for_path(const std::filesystem::path &path) const {
        const auto key = archive_cache_key_for_path(path);
        if (auto it = _archive_load_counts.find(key); it != _archive_load_counts.end()) {
            return it->second;
        }

        return 0U;
    }

    std::size_t DvdFileSystemService::cached_archive_count() const {
        return _archives.size();
    }

    std::span<const DvdFileReadTrace> DvdFileSystemService::file_read_trace() const {
        return _file_read_trace;
    }

    std::span<const DvdAsyncReadRequest> DvdFileSystemService::async_read_trace() const {
        return _async_read_trace;
    }

    std::span<const DvdArchiveLoadTrace> DvdFileSystemService::archive_load_trace() const {
        return _archive_load_trace;
    }

    void DvdFileSystemService::clear_trace() {
        _file_read_trace.clear();
        _async_read_trace.clear();
        _archive_load_trace.clear();
    }

    std::filesystem::path DvdFileSystemService::normalize_disc_path(std::string_view disc_path) const {
        auto text = std::string(disc_path);
        std::ranges::replace(text, '\\', '/');

        while (!text.empty() && text.front() == '/') {
            text.erase(text.begin());
        }
        while (text.starts_with("./")) {
            text.erase(0U, 2U);
        }
        if (text.starts_with("files/")) {
            text.erase(0U, 6U);
        }

        auto parts = std::vector<std::filesystem::path>{};
        for (const auto &component : std::filesystem::path(text)) {
            const auto part = component.generic_string();
            if (part.empty() || part == ".") {
                continue;
            }
            if (part == "..") {
                if (parts.empty()) {
                    aurora::throw_host_exception<std::runtime_error>("DVD path escapes disc root: " + std::string(disc_path));
                }
                parts.pop_back();
                continue;
            }

            parts.push_back(component);
        }

        auto normalized = std::filesystem::path();
        for (const auto &part : parts) {
            normalized /= part;
        }

        return normalized;
    }

    void DvdFileSystemService::complete_ready_async_reads() {
        for (auto &request : _async_read_trace) {
            if (request.completed || request.completion_frame > _frame_index) {
                continue;
            }

            auto result = s32{-1};
            try {
                const auto bytes = read_file_range(request.disc_path, request.offset, request.length, request.priority);
                if (!bytes.empty() && request.destination != nullptr) {
                    std::memcpy(request.destination, bytes.data(), bytes.size());
                }
                result = static_cast<s32>(bytes.size());
                if (request.file_info != nullptr) {
                    request.file_info->cb.state = DVD_STATE_END;
                    request.file_info->cb.transferredSize += static_cast<u32>(bytes.size());
                }
            } catch (const std::exception &) {
                result = -1;
                if (request.file_info != nullptr) {
                    request.file_info->cb.state = DVD_STATE_FATAL_ERROR;
                }
            }

            request.result = result;
            request.completed = true;
            if (request.callback != nullptr) {
                request.callback(result, request.file_info);
            }
        }
    }

    void DvdFileSystemService::ensure_entry_table() const {
        if (_entry_table_initialized) {
            return;
        }

        _entry_table.clear();
        _entry_num_by_disc_path.clear();

        const auto add_entry = [this](std::filesystem::path disc_path, const std::filesystem::path &resolved_path,
                                      bool is_directory) -> s32 {
            const auto entry_num = static_cast<s32>(_entry_table.size());
            auto length = std::uintmax_t{};
            if (!is_directory) {
                std::error_code error{};
                length = std::filesystem::file_size(resolved_path, error);
                if (error) {
                    length = 0U;
                }
            }

            const auto key = entry_key(disc_path);
            _entry_num_by_disc_path[key] = entry_num;
            _entry_table.push_back(DvdEntryMetadata{
                .entry_num = entry_num,
                .disc_path = key.empty() ? std::string("/") : "/" + key,
                .resolved_path = weakly_canonical_or_normal(resolved_path).generic_string(),
                .is_directory = is_directory,
                .length = static_cast<std::size_t>(std::min<std::uintmax_t>(length, std::numeric_limits<std::size_t>::max())),
            });
            return entry_num;
        };

        add_entry({}, _root, true);

        const auto visit_directory = [&](const auto &self, const std::filesystem::path &relative_path) -> void {
            const auto absolute_path = relative_path.empty() ? _root : _root / relative_path;
            auto children = std::vector<std::filesystem::directory_entry>{};
            std::error_code error{};
            auto iter = std::filesystem::directory_iterator(absolute_path, std::filesystem::directory_options::skip_permission_denied, error);
            if (error) {
                return;
            }
            for (const auto &child : iter) {
                std::error_code status_error{};
                if (child.is_directory(status_error) || child.is_regular_file(status_error)) {
                    children.push_back(child);
                }
            }
            std::ranges::sort(children, [](const auto &lhs, const auto &rhs) {
                return lhs.path().filename().generic_string() < rhs.path().filename().generic_string();
            });

            for (const auto &child : children) {
                std::error_code status_error{};
                const auto is_directory = child.is_directory(status_error);
                const auto child_relative_path = relative_path / child.path().filename();
                add_entry(child_relative_path, child.path(), is_directory);
                if (is_directory) {
                    self(self, child_relative_path);
                }
            }
        };
        visit_directory(visit_directory, {});
        _entry_table_initialized = true;
    }

    std::string DvdFileSystemService::entry_key(const std::filesystem::path &disc_path) {
        auto key = disc_path.lexically_normal().generic_string();
        while (!key.empty() && key.front() == '/') {
            key.erase(key.begin());
        }
        if (key == ".") {
            key.clear();
        }
        return key;
    }

    std::string DvdFileSystemService::archive_cache_key_for_path(const std::filesystem::path &path) const {
        return resolve(path.generic_string()).generic_string();
    }

    std::string DvdFileSystemService::archive_cache_key(std::string_view disc_path) const {
        return archive_cache_key_for_path(resolve(disc_path));
    }

    void WipeService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        if ((_state != WipeState::Opening && _state != WipeState::Closing) || _remaining_frames <= 0) {
            return;
        }

        --_remaining_frames;
        if (_remaining_frames <= 0) {
            _state = _state == WipeState::Opening ? WipeState::Open : WipeState::Closed;
        }
    }

    void WipeService::open(std::string_view name, s32 frame_count) {
        start_transition(WipeEventKind::Open, WipeState::Opening, name, frame_count);
    }

    void WipeService::close(std::string_view name, s32 frame_count) {
        start_transition(WipeEventKind::Close, WipeState::Closing, name, frame_count);
    }

    void WipeService::force_open(std::string_view name) {
        const smgpc::compat::JkrHostAllocationScope host;
        _current_name = name;
        _state = WipeState::Open;
        _remaining_frames = 0;
        _duration_frames = 0;
        push_event(WipeEventKind::ForceOpen, name, 0);
    }

    void WipeService::force_close(std::string_view name) {
        const smgpc::compat::JkrHostAllocationScope host;
        _current_name = name;
        _state = WipeState::Closed;
        _remaining_frames = 0;
        _duration_frames = 0;
        push_event(WipeEventKind::ForceClose, name, 0);
    }

    bool WipeService::is_active() const {
        return _state == WipeState::Opening || _state == WipeState::Closing;
    }

    bool WipeService::is_blank() const {
        return _state == WipeState::Closed;
    }

    bool WipeService::is_open() const {
        return _state == WipeState::Open;
    }

    WipeState WipeService::state() const {
        return _state;
    }

    std::string_view WipeService::current_name() const {
        return _current_name;
    }

    s32 WipeService::remaining_frames() const {
        return _remaining_frames;
    }

    s32 WipeService::duration_frames() const {
        return _duration_frames;
    }

    std::span<const WipeEvent> WipeService::events() const {
        return _events;
    }

    void WipeService::start_transition(WipeEventKind kind, WipeState state, std::string_view name, s32 frame_count) {
        const smgpc::compat::JkrHostAllocationScope host;
        _current_name = name;
        _duration_frames = normalized_frame_count(frame_count);
        _remaining_frames = _duration_frames;
        _state = _remaining_frames <= 0 ? (state == WipeState::Opening ? WipeState::Open : WipeState::Closed) : state;
        push_event(kind, name, frame_count);
    }

    void WipeService::push_event(WipeEventKind kind, std::string_view name, s32 frame_count) {
        const smgpc::compat::JkrHostAllocationScope host;
        _events.push_back(WipeEvent{
            .kind = kind,
            .name = std::string(name),
            .frame_count = frame_count,
            .frame_index = _frame_index,
        });
    }

    s32 WipeService::normalized_frame_count(s32 frame_count) {
        return frame_count < 0 ? 30 : frame_count;
    }

    void StarPointerService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void StarPointerService::unregister_target(const LiveActor &actor) {
        _targets.erase(&actor);
    }

    void StarPointerService::start_mode(StarPointerMode mode) {
        _base_mode = mode;
        _mode_requests.clear();
        update_mode_from_requests();
    }

    void StarPointerService::push_mode(const void *requester, StarPointerMode mode) {
        if (requester == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("A star-pointer mode request requires a real requester.");
        }
        if (mode == StarPointerMode::None) {
            aurora::throw_host_exception<std::invalid_argument>("A requester cannot push the absent star-pointer mode.");
        }
        constexpr auto cRetailRequestCapacity = std::size_t{16U};
        if (_mode_requests.size() >= cRetailRequestCapacity) {
            aurora::throw_host_exception<std::overflow_error>("The retail star-pointer mode request table is full.");
        }

        _mode_requests.push_back(ModeRequest{.requester = requester, .mode = mode});
        update_mode_from_requests();
    }

    void StarPointerService::pop_mode(const void *requester) {
        if (requester == nullptr) {
            return;
        }

        const auto request = std::find_if(_mode_requests.rbegin(), _mode_requests.rend(),
                                          [requester](const auto &candidate) {
                                              return candidate.requester == requester;
                                          });
        if (request == _mode_requests.rend()) {
            return;
        }
        _mode_requests.erase(std::next(request).base());
        update_mode_from_requests();
    }

    void StarPointerService::clear_mode_requests(const void *requester) {
        if (requester == nullptr) {
            return;
        }

        const auto old_size = _mode_requests.size();
        std::erase_if(_mode_requests, [requester](const auto &request) {
            return request.requester == requester;
        });
        if (_mode_requests.size() != old_size) {
            update_mode_from_requests();
        }
    }

    void StarPointerService::update_mode_from_requests() {
        const auto retail_priority = [](StarPointerMode mode) {
            switch (mode) {
            case StarPointerMode::SystemModal:
                return 2;
            case StarPointerMode::ScreenMenu:
                return 4;
            case StarPointerMode::TargetSelection:
                return 5;
            case StarPointerMode::DocumentViewer:
                return 9;
            case StarPointerMode::SphereSelectorReaction:
                return 16;
            case StarPointerMode::SphereSelectorFinger:
                return 17;
            case StarPointerMode::None:
                return 26;
            }
            return 26;
        };

        auto selected = _base_mode;
        auto selected_priority = retail_priority(selected);
        for (const auto &request : _mode_requests) {
            const auto priority = retail_priority(request.mode);
            if (priority < selected_priority) {
                selected = request.mode;
                selected_priority = priority;
            }
        }
        if (_mode == selected) {
            return;
        }

        _mode = selected;
        _mode_events.push_back(StarPointerModeEvent{
            .mode = selected,
            .frame_index = _frame_index,
        });
    }

    void StarPointerService::set_guidance_active(bool active) {
        _guidance_active = active;
    }

    void StarPointerService::request_guidance(StarPointerGuidanceRequest request) {
        if (request == StarPointerGuidanceRequest::None) {
            return;
        }

        if (std::find(_guidance_requests.begin(), _guidance_requests.end(), request) == _guidance_requests.end()) {
            _guidance_requests.push_back(request);
        }
    }

    StarPointerMode StarPointerService::mode() const {
        return _mode;
    }

    bool StarPointerService::has_target(const LiveActor &actor) const {
        return actor.mStarPointerTarget != nullptr;
    }

    bool StarPointerService::is_pointing(const LiveActor &actor, const WpadService &wpad, const std::optional<smgpc::camera::CameraPose> &camera_pose, bool check_z) {
        smgpc::compat::JkrHostAllocationScope host;
        if (actor.mStarPointerTarget == nullptr) {
            return false;
        }
        auto [iter, inserted] = _targets.try_emplace(&actor, StarPointerTargetState{.actor = &actor});
        auto &target = iter->second;
        auto pointer = wpad.pointer(WPAD_CHAN0);
        auto projection = std::optional<StarPointerProjection>{};
        auto pointing = false;

        if (!actor.mFlag.mIsDead && camera_pose.has_value() && wpad.is_connected(WPAD_CHAN0) && pointer.valid) {
            projection = project_star_pointer_target(target, *camera_pose, check_z);
            if (projection.has_value()) {
                const auto dx = pointer.x - projection->x;
                const auto dy = pointer.y - projection->y;
                pointing = (dx * dx) + (dy * dy) <= projection->radius * projection->radius;
            }
        }

        if (pointing) {
            actor.mStarPointerTarget->mLastPointedChannel = WPAD_CHAN0;
        }
#ifndef NDEBUG
        record_target_pointing_sample(target, pointing, pointer, projection.has_value(), projection.has_value() ? projection->x : 0.0F,
                                      projection.has_value() ? projection->y : 0.0F, projection.has_value() ? projection->radius : 0.0F,
                                      check_z, wpad.is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A));
#endif
        return pointing;
    }

    bool StarPointerService::is_guidance_active() const {
        return _guidance_active;
    }

    bool StarPointerService::is_guidance_requested(StarPointerGuidanceRequest request) const {
        return std::find(_guidance_requests.begin(), _guidance_requests.end(), request) != _guidance_requests.end();
    }

    std::span<const StarPointerGuidanceRequest> StarPointerService::guidance_requests() const {
        return _guidance_requests;
    }

    std::span<const StarPointerModeEvent> StarPointerService::mode_events() const {
        return _mode_events;
    }

    std::size_t StarPointerService::mode_request_count(const void *requester) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            _mode_requests, [requester](const auto &request) {
                return request.requester == requester;
            }));
    }

#ifndef NDEBUG
    std::span<const StarPointerTargetEvent> StarPointerService::target_events() const {
        return _target_events;
    }

    void StarPointerService::record_target_pointing_sample(StarPointerTargetState &target, bool pointing, const WpadPointerState &pointer,
                                                           bool has_projection, float target_x, float target_y, float projected_radius,
                                                           bool check_z, bool select_triggered) {
        const auto push_event = [&](StarPointerTargetEventKind kind) {
            _target_events.push_back(StarPointerTargetEvent{
                .kind = kind,
                .actor_name = target.actor != nullptr ? target.actor->getName() : "",
                .frame_index = _frame_index,
                .channel = WPAD_CHAN0,
                .pointer_x = pointer.x,
                .pointer_y = pointer.y,
                .target_x = has_projection ? target_x : 0.0F,
                .target_y = has_projection ? target_y : 0.0F,
                .projected_radius = has_projection ? projected_radius : 0.0F,
                .check_z = check_z,
            });
        };

        if (pointing != target.was_pointing) {
            push_event(pointing ? StarPointerTargetEventKind::Enter : StarPointerTargetEventKind::Leave);
            target.was_pointing = pointing;
        }

        if (pointing && select_triggered && target.last_select_frame_index != _frame_index) {
            push_event(StarPointerTargetEventKind::Select);
            target.last_select_frame_index = _frame_index;
        }
    }
#endif

    PlayerSystemService::PlayerSystemService() = default;
    PlayerSystemService::~PlayerSystemService() = default;

    void PlayerSystemService::reset_stage_state() {
        _attached_actor = nullptr;
        _actor_bridge = {};
        _has_base_matrix = false;
        _has_forced_base_matrix = false;
        _on_ground = false;
        ++_base_matrix_revision;
        _base_matrix = {};
        _position = {};
        _velocity = {};
        _gravity = {};
    }

    void PlayerSystemService::clear_stage_state() {
        reset_stage_state();
    }

    void PlayerSystemService::attach_actor(
        LiveActor &actor, PlayerActorBridge actor_bridge) {
        if (_attached_actor != nullptr && _attached_actor != &actor) {
            detach_actor(_attached_actor);
        }
        _attached_actor = &actor;
        _actor_bridge = actor_bridge;
        copy_actor_state();
    }

    void PlayerSystemService::detach_actor(const LiveActor *actor) {
        if (actor == nullptr || _attached_actor == actor) {
            _attached_actor = nullptr;
            _actor_bridge = {};
        }
    }

    void PlayerSystemService::synchronize_attached_actor() {
        if (_attached_actor == nullptr) {
            return;
        }

        // The actor's original calcAnim phase owns matrix updates and their
        // gameplay side effects. This publication only snapshots current state.
        copy_actor_state();
    }

    void PlayerSystemService::set_base_matrix(MtxPtr matrix) {
        ++_base_matrix_revision;
        _has_base_matrix = matrix != nullptr;
        _has_forced_base_matrix = matrix != nullptr;
        if (matrix == nullptr) {
            _base_matrix = {};
            _position = {};
            return;
        }

        auto index = std::size_t{};
        for (auto row = 0U; row < 3U; ++row) {
            for (auto column = 0U; column < 4U; ++column) {
                _base_matrix[index++] = matrix[row][column];
            }
        }
        _position = {_base_matrix[3U], _base_matrix[7U], _base_matrix[11U]};

        if (_attached_actor != nullptr) {
            MR::setBaseTRMtx(_attached_actor, matrix);
            _attached_actor->mPosition.set(_position[0U], _position[1U], _position[2U]);
        }
    }

    bool PlayerSystemService::has_base_matrix() const {
        return _has_base_matrix;
    }

    bool PlayerSystemService::has_forced_base_matrix() const {
        return _has_forced_base_matrix;
    }

    std::span<const f32, 12U> PlayerSystemService::base_matrix() const {
        return _base_matrix;
    }

    std::span<const f32, 3U> PlayerSystemService::position() const {
        return _position;
    }

    std::span<const f32, 3U> PlayerSystemService::velocity() const {
        return _velocity;
    }

    std::span<const f32, 3U> PlayerSystemService::gravity() const {
        return _gravity;
    }

    bool PlayerSystemService::is_on_ground() const {
        return _on_ground;
    }

    std::optional<bool> PlayerSystemService::player_dead_state() const {
        if (_attached_actor == nullptr ||
            _actor_bridge.read_nerve_change_enabled == nullptr) {
            return std::nullopt;
        }
        return !_actor_bridge.read_nerve_change_enabled(*_attached_actor);
    }

    TVec3f *PlayerSystemService::actor_center_position() const {
        if (_attached_actor == nullptr || _actor_bridge.read_center_position == nullptr) {
            return nullptr;
        }
        return _actor_bridge.read_center_position(*_attached_actor);
    }

    std::optional<s32> PlayerSystemService::player_element_mode() const {
        if (_attached_actor == nullptr || _actor_bridge.read_element_mode == nullptr) {
            return std::nullopt;
        }
        return _actor_bridge.read_element_mode(*_attached_actor);
    }

    MtxPtr PlayerSystemService::actor_base_matrix() const {
        return _attached_actor != nullptr && _actor_bridge.read_base_matrix != nullptr
                   ? _actor_bridge.read_base_matrix(*_attached_actor) : nullptr;
    }

    bool PlayerSystemService::copy_actor_up_vector(TVec3f *out) const {
        if (_attached_actor == nullptr || _actor_bridge.read_up_vector == nullptr) {
            return false;
        }
        _actor_bridge.read_up_vector(*_attached_actor, out);
        return true;
    }

    bool PlayerSystemService::copy_actor_front_vector(TVec3f *out) const {
        if (_attached_actor == nullptr || _actor_bridge.read_front_vector == nullptr) {
            return false;
        }
        _actor_bridge.read_front_vector(*_attached_actor, out);
        return true;
    }

    bool PlayerSystemService::copy_actor_side_vector(TVec3f *out) const {
        if (_attached_actor == nullptr || _actor_bridge.read_side_vector == nullptr) {
            return false;
        }
        _actor_bridge.read_side_vector(*_attached_actor, out);
        return true;
    }

    std::uint64_t PlayerSystemService::base_matrix_revision() const {
        return _base_matrix_revision;
    }

    LiveActor *PlayerSystemService::attached_actor() const {
        return _attached_actor;
    }

    void PlayerSystemService::copy_actor_state() {
        if (_attached_actor == nullptr) {
            return;
        }

        _has_base_matrix = _attached_actor->getBaseMtx() != nullptr;
        if (_has_base_matrix) {
            _base_matrix = smgpc::render::j3d_matrix_from_mtx(_attached_actor->getBaseMtx()).m;
        }
        _position = {_attached_actor->mPosition.x, _attached_actor->mPosition.y, _attached_actor->mPosition.z};
        _velocity = {_attached_actor->mVelocity.x, _attached_actor->mVelocity.y, _attached_actor->mVelocity.z};
        _gravity = {_attached_actor->mGravity.x, _attached_actor->mGravity.y, _attached_actor->mGravity.z};
        _on_ground = MR::isBindedGround(_attached_actor);
    }

    void GameLayoutService::activate_default_game_layout() {
        _default_game_layout_active = true;
    }

    void GameLayoutService::deactivate_default_game_layout() {
        _default_game_layout_active = false;
    }

    void GameLayoutService::activate_game_scene_draw_3d() {
        _game_scene_draw_3d_active = true;
    }

    void GameLayoutService::deactivate_game_scene_draw_3d() {
        _game_scene_draw_3d_active = false;
    }

    bool GameLayoutService::is_default_game_layout_active() const {
        return _default_game_layout_active;
    }

    bool GameLayoutService::is_game_scene_draw_3d_active() const {
        return _game_scene_draw_3d_active;
    }

    RumbleService::RumbleService(RumbleActuator *actuator) : _actuator(actuator) {
    }

    RumbleService::~RumbleService() {
        stop_all();
    }

    void RumbleService::attach_actuator(RumbleActuator &actuator) {
        if (_actuator == &actuator) {
            return;
        }

        stop_all();
        _actuator = &actuator;
    }

    void RumbleService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;

        for (auto channel = std::size_t{}; channel < _active_patterns.size(); ++channel) {
            auto &patterns = _active_patterns[channel];
            const auto channel_index = static_cast<s32>(channel);
            if (_actuator == nullptr || !_actuator->is_available(channel_index)) {
                set_motor(channel_index, false);
                patterns.clear();
                continue;
            }

            auto enabled = false;
            std::erase_if(patterns, [&enabled](ActivePattern &active) {
                if (active.pattern == nullptr || active.next_frame >= static_cast<std::size_t>(active.pattern->mFrame)) {
                    return true;
                }

                enabled = enabled || active.pattern->mPattern[active.next_frame] == WPAD_MOTOR_RUMBLE;
                ++active.next_frame;
                return false;
            });
            set_motor(channel_index, enabled);
        }
    }

    bool RumbleService::try_request_pattern(const void *source, std::string_view pattern_name, s32 channel) {
        if (pattern_name.empty() || channel < 0 || channel >= static_cast<s32>(_active_patterns.size()) ||
            _actuator == nullptr || !_actuator->is_available(channel)) {
            return false;
        }

        const auto *pattern = static_cast<const RumblePattern *>(nullptr);
        for (auto index = u16{}; index < RumbleData::getTableSize(); ++index) {
            const auto *candidate = RumbleData::getData(index);
            if (candidate != nullptr && candidate->mName != nullptr && pattern_name == candidate->mName) {
                pattern = candidate;
                break;
            }
        }
        if (pattern == nullptr || pattern->mFrame <= 0) {
            return false;
        }

        auto &patterns = _active_patterns[static_cast<std::size_t>(channel)];
        if (std::ranges::any_of(patterns, [source, pattern](const ActivePattern &active) {
                return active.source == source && active.pattern == pattern;
            }) ||
            patterns.size() >= 8U) {
            return false;
        }

        patterns.push_back(ActivePattern{
            .source = source,
            .pattern = pattern,
            .next_frame = 1U,
        });

        auto enabled = false;
        for (const auto &active : patterns) {
            const auto current_frame = active.next_frame == 0U ? 0U : active.next_frame - 1U;
            enabled = enabled || (current_frame < static_cast<std::size_t>(active.pattern->mFrame) &&
                                  active.pattern->mPattern[current_frame] == WPAD_MOTOR_RUMBLE);
        }
        set_motor(channel, enabled);
        _events.push_back(RumbleRequestEvent{
            .kind = RumbleRequestKind::Named,
            .pattern_name = std::string(pattern_name),
            .channel = channel,
            .frame_index = _frame_index,
        });
        return true;
    }

    void RumbleService::stop_all() noexcept {
        for (auto channel = std::size_t{}; channel < _active_patterns.size(); ++channel) {
            set_motor(static_cast<s32>(channel), false);
            _active_patterns[channel].clear();
        }
    }

    std::span<const RumbleRequestEvent> RumbleService::events() const {
        return _events;
    }

    void RumbleService::set_motor(s32 channel, bool enabled) noexcept {
        if (channel < 0 || channel >= static_cast<s32>(_motor_enabled.size())) {
            return;
        }

        auto &current = _motor_enabled[static_cast<std::size_t>(channel)];
        if (current == enabled) {
            return;
        }
        if (_actuator != nullptr) {
            _actuator->set_motor(channel, enabled);
        }
        current = enabled;
    }

    void SequenceRequestService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void SequenceRequestService::request_change_stage_in_game_after_loading_game_data() {
        if (_change_stage_in_game_after_loading_game_data_requested) {
            return;
        }

        _change_stage_in_game_after_loading_game_data_requested = true;
        _events.push_back(SequenceRequestEvent{
            .kind = SequenceRequestKind::ChangeStageInGameAfterLoadingGameData,
            .frame_index = _frame_index,
        });
    }

    bool SequenceRequestService::consume_change_stage_in_game_after_loading_game_data_request() {
        if (!_change_stage_in_game_after_loading_game_data_requested) {
            return false;
        }

        _change_stage_in_game_after_loading_game_data_requested = false;
        return true;
    }

    bool SequenceRequestService::is_change_stage_in_game_after_loading_game_data_requested() const {
        return _change_stage_in_game_after_loading_game_data_requested;
    }

    std::span<const SequenceRequestEvent> SequenceRequestService::events() const {
        return _events;
    }

    SaveDataService::SaveDataService() : _nand("/title/00010000/524d474b/data") {
    }

    SaveDataService::~SaveDataService() {
        _nand.deactivate_sdk();
    }

    void SaveDataService::activate_nand() {
        _nand.activate_sdk({
            .context = this,
            .read = [](void* owner, std::string_view path) {
                return static_cast<SaveDataService*>(owner)->read_nand_file(path);
            },
            .commit = [](void* owner, std::string_view path, std::span<const u8> bytes, u8 permission, u8 attribute) {
                static_cast<SaveDataService*>(owner)->write_nand_file(path, bytes, permission, attribute);
            },
            .create = [](void* owner, std::string_view path, u8 permission, u8 attribute) {
                return static_cast<SaveDataService*>(owner)->create_nand_file(path, permission, attribute);
            },
            .move = [](void* owner, std::string_view from, std::string_view to) {
                return static_cast<SaveDataService*>(owner)->move_nand_file(from, to);
            },
            .erase = [](void* owner, std::string_view path) {
                return static_cast<SaveDataService*>(owner)->erase_nand_file(path);
            },
        });
    }

    void SaveDataService::write_file(std::string_view name, std::span<const std::uint8_t> bytes) {
        if (!_host_directory.has_value()) {
            aurora::throw_host_exception<std::logic_error>("Save persistence is unavailable without a configured host directory");
        }
        const auto file_name = NandFileSystemService::file_name(_nand.normalize_path(name));
        if (file_name != SAVE_DATA_CONTAINER_NAME && save_data_file_size(file_name).has_value()) {
            aurora::throw_host_exception<std::invalid_argument>("Retail save members may only be persisted inside GameData.bin");
        }
        if (file_name == SAVE_DATA_CONTAINER_NAME && !decode_game_data_container(bytes).has_value()) {
            aurora::throw_host_exception<std::invalid_argument>("GameData.bin is not a valid retail big-endian container");
        }

        const auto key = std::string(name);
        _files[key] = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
        if (file_name == SAVE_DATA_CONTAINER_NAME) {
            _has_valid_game_data_container = true;
        }
        write_host_file(name, bytes);
    }

    std::optional<std::vector<std::uint8_t>> SaveDataService::read_file(std::string_view name) const {
        if (auto it = _files.find(std::string(name)); it != _files.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    void SaveDataService::write_nand_file(std::string_view name, std::span<const std::uint8_t> bytes, u8 permission, u8 attribute) {
        if (!_host_directory.has_value()) {
            aurora::throw_host_exception<std::logic_error>("NAND save persistence is unavailable without a configured host directory");
        }
        const auto file_name = NandFileSystemService::file_name(_nand.normalize_path(name));
        std::optional<std::vector<std::uint8_t>> wii_bytes;
        if (file_name == SAVE_DATA_CONTAINER_NAME) {
            wii_bytes = host_save_data_container_for_retail(bytes);
        } else if (file_name == "banner.bin") {
            constexpr auto native_order = std::endian::native == std::endian::little ?
                SaveDataByteOrder::LittleEndian : SaveDataByteOrder::BigEndian;
            wii_bytes = convert_nand_banner_byte_order(bytes, native_order, SaveDataByteOrder::BigEndian);
        }
        const auto payload = wii_bytes ? std::span<const std::uint8_t>(*wii_bytes) : bytes;
        if (file_name == SAVE_DATA_CONTAINER_NAME && !decode_game_data_container(payload).has_value()) {
            aurora::throw_host_exception<std::invalid_argument>("Translated GameData.bin does not match the retail container layout");
        }
        _nand.write_file(name, payload, permission, attribute);
        write_file(nand_file_key(name), payload);
    }

    std::optional<std::vector<std::uint8_t>> SaveDataService::read_nand_file(std::string_view name) const {
        const auto file_name = NandFileSystemService::file_name(_nand.normalize_path(name));
        auto bytes = _nand.read_file(name);
        if (!bytes.has_value()) {
            bytes = read_file(nand_file_key(name));
            if (!bytes.has_value()) {
                return std::nullopt;
            }
        }

        if (file_name == SAVE_DATA_CONTAINER_NAME) {
            if (!decode_game_data_container(*bytes).has_value()) {
                aurora::throw_host_exception<std::runtime_error>("Persisted GameData.bin is malformed or uses a non-retail byte order");
            }
            return retail_save_data_container_for_host(*bytes);
        }
        if (file_name == "banner.bin") {
            constexpr auto native_order = std::endian::native == std::endian::little ?
                SaveDataByteOrder::LittleEndian : SaveDataByteOrder::BigEndian;
            return convert_nand_banner_byte_order(*bytes, SaveDataByteOrder::BigEndian, native_order);
        }
        return bytes;
    }

    std::string SaveDataService::nand_file_key(std::string_view name) const {
        const auto path = _nand.normalize_path(name);
        const auto title_prefix = _nand.title_data_root() + "/";
        if (path.starts_with(title_prefix)) return path.substr(title_prefix.size());
        // Preserve absolute NAND namespaces outside this title's data directory.
        return "nand/" + path.substr(1U);
    }

    s32 SaveDataService::create_nand_file(std::string_view name, u8 permission, u8 attribute) {
        if (!_host_directory) {
            aurora::throw_host_exception<std::logic_error>("NAND file creation requires a configured host directory");
        }
        const auto key = nand_file_key(name);
        if (_nand.exists(name) || _files.contains(key)) return NAND_RESULT_EXISTS;
        const auto capacity = _nand.check(0, 1);
        if (capacity.result != NAND_RESULT_OK) return capacity.result;
        write_host_file(key, {});
        _nand.write_file(name, {}, permission, attribute);
        _files[key] = {};
        return NAND_RESULT_OK;
    }

    s32 SaveDataService::move_nand_file(std::string_view source, std::string_view destination) {
        if (!_host_directory) {
            aurora::throw_host_exception<std::logic_error>("NAND file movement requires a configured host directory");
        }
        const auto source_key = nand_file_key(source);
        const auto destination_key = nand_file_key(destination);
        auto bytes = _nand.read_file(source);
        if (!bytes) bytes = read_file(source_key);
        if (!bytes) return NAND_RESULT_NOEXISTS;
        if (source_key == destination_key) return NAND_RESULT_OK;
        // ISFS_Rename replaces an existing destination file of the same type.
        const auto metadata = _nand.metadata(source);
        write_host_file(destination_key, *bytes);
        erase_host_file(source_key);
        _nand.erase(source);
        _nand.write_file(destination, *bytes, metadata ? metadata->permission : 0x3cU,
                         metadata ? metadata->attribute : 0U);
        _files.erase(source_key);
        _files[destination_key] = std::move(*bytes);
        return NAND_RESULT_OK;
    }

    bool SaveDataService::erase_nand_file(std::string_view name) {
        if (!_host_directory) {
            aurora::throw_host_exception<std::logic_error>("NAND file deletion requires a configured host directory");
        }
        const auto key = nand_file_key(name);
        const auto existed = _nand.exists(name) || _files.contains(key);
        erase_host_file(key);
        _nand.erase(name);
        _files.erase(key);
        if (NandFileSystemService::file_name(key) == SAVE_DATA_CONTAINER_NAME) _has_valid_game_data_container = false;
        return existed;
    }

    NandFileSystemService &SaveDataService::nand() {
        return _nand;
    }

    const NandFileSystemService &SaveDataService::nand() const {
        return _nand;
    }

    bool SaveDataService::exists(std::string_view name) const {
        return _files.contains(std::string(name)) || _nand.exists(name);
    }

    bool SaveDataService::erase(std::string_view name) {
        if (!_host_directory.has_value()) {
            aurora::throw_host_exception<std::logic_error>("Save persistence is unavailable without a configured host directory");
        }
        const auto erased = _files.erase(std::string(name)) != 0U;
        const auto nand_erased = _nand.erase(name);
        erase_host_file(name);
        if (NandFileSystemService::file_name(_nand.normalize_path(name)) == SAVE_DATA_CONTAINER_NAME) {
            _has_valid_game_data_container = false;
        }
        return erased || nand_erased;
    }

    std::size_t SaveDataService::file_count() const {
        return _files.size();
    }

    void SaveDataService::set_host_directory(std::filesystem::path directory) {
        _host_directory = weakly_canonical_or_normal(std::move(directory));
        load_host_files();
    }

    const std::optional<std::filesystem::path> &SaveDataService::host_directory() const {
        return _host_directory;
    }

    void SaveDataService::load_host_files() {
        if (!_host_directory.has_value()) {
            return;
        }

        std::error_code error{};
        std::filesystem::create_directories(*_host_directory, error);
        if (error) {
            aurora::throw_host_exception<std::runtime_error>("Cannot create save directory " + _host_directory->string());
        }

        _files.clear();
        _nand.erase_subtree(_nand.title_data_root());
        _has_valid_game_data_container = false;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(*_host_directory, error)) {
            if (error) {
                aurora::throw_host_exception<std::runtime_error>("Cannot scan save directory " + _host_directory->string());
            }
            if (!entry.is_regular_file(error)) {
                continue;
            }

            const auto relative = std::filesystem::relative(entry.path(), *_host_directory, error);
            if (error || relative.empty()) {
                continue;
            }
            const auto relative_name = relative.generic_string();
            const auto file_name = NandFileSystemService::file_name(_nand.normalize_path(relative_name));
            if (file_name != SAVE_DATA_CONTAINER_NAME && save_data_file_size(file_name).has_value()) {
                continue;
            }
            auto bytes = read_binary_file(entry.path());
            const auto nand_path = relative_name.starts_with("nand/") ? "/" + relative_name.substr(5U) : relative_name;
            _nand.write_file(nand_path, bytes);
            _files[relative_name] = std::move(bytes);
        }

        if (const auto container = read_file(SAVE_DATA_CONTAINER_NAME)) {
            _has_valid_game_data_container = decode_game_data_container(*container).has_value();
        }
    }

    void SaveDataService::flush_host_files() {
        if (!_host_directory.has_value()) {
            aurora::throw_host_exception<std::logic_error>("Save persistence is unavailable without a configured host directory");
        }

        for (const auto &[name, bytes] : _files) {
            write_host_file(name, bytes);
        }
    }

    bool SaveDataService::has_valid_game_data_container() const {
        return _has_valid_game_data_container;
    }

    std::filesystem::path SaveDataService::host_file_path(std::string_view name) const {
        if (!_host_directory.has_value()) {
            return {};
        }

        auto relative = std::filesystem::path(std::string(name)).lexically_normal();
        if (relative.empty() || relative.is_absolute()) {
            aurora::throw_host_exception<std::runtime_error>("Invalid save file name " + std::string(name));
        }

        for (const auto &part : relative) {
            if (part == "..") {
                aurora::throw_host_exception<std::runtime_error>("Invalid save file name " + std::string(name));
            }
        }

        return *_host_directory / relative;
    }

    void SaveDataService::write_host_file(std::string_view name, std::span<const std::uint8_t> bytes) const {
        if (!_host_directory.has_value()) {
            aurora::throw_host_exception<std::logic_error>("Save persistence is unavailable without a configured host directory");
        }

        write_binary_file(host_file_path(name), bytes);
    }

    void SaveDataService::erase_host_file(std::string_view name) const {
        if (!_host_directory.has_value()) {
            aurora::throw_host_exception<std::logic_error>("Save persistence is unavailable without a configured host directory");
        }

        std::error_code error{};
        std::filesystem::remove(host_file_path(name), error);
    }

    std::optional<std::map<std::string, std::vector<std::uint8_t>>> SaveDataService::decode_game_data_container(std::span<const std::uint8_t> bytes) const {
        if (bytes.size() < SAVE_DATA_HEADER_SIZE + SAVE_DATA_FILE_INFO_SIZE) {
            return std::nullopt;
        }

        constexpr auto byte_order = SaveDataByteOrder::BigEndian;
        const auto expected_check_sum = read_save_u32(bytes, 0U, byte_order);
        const auto version = read_save_u32(bytes, 4U, byte_order);
        const auto file_count = read_save_u32(bytes, 8U, byte_order);
        const auto data_size = read_save_u32(bytes, 12U, byte_order);
        if (version != SAVE_DATA_VERSION || file_count == 0U || file_count >= 24U ||
            data_size < SAVE_DATA_HEADER_SIZE + file_count * SAVE_DATA_FILE_INFO_SIZE) {
            return std::nullopt;
        }

        const auto aligned_size = align_save_data_size(data_size);
        if (aligned_size > bytes.size()) {
            return std::nullopt;
        }

        const auto actual_check_sum =
            save_check_sum(bytes.subspan(sizeof(std::uint32_t), data_size - sizeof(std::uint32_t)), byte_order);
        if (expected_check_sum != actual_check_sum) {
            return std::nullopt;
        }

        auto decoded = std::map<std::string, std::vector<std::uint8_t>>{};
        for (auto file_index = std::uint32_t{}; file_index < file_count; ++file_index) {
            const auto info_offset = SAVE_DATA_HEADER_SIZE + static_cast<std::size_t>(file_index) * SAVE_DATA_FILE_INFO_SIZE;
            auto name_size = std::size_t{};
            while (name_size < SAVE_DATA_FILE_NAME_SIZE && bytes[info_offset + name_size] != 0U) {
                ++name_size;
            }

            const auto name = std::string(reinterpret_cast<const char *>(bytes.data() + info_offset), name_size);
            const auto file_size = save_data_file_size(name);
            const auto data_offset = read_save_u32(bytes, info_offset + SAVE_DATA_FILE_NAME_SIZE, byte_order);
            if (name.empty() || !file_size.has_value() || data_offset > data_size || *file_size > data_size - data_offset ||
                decoded.contains(name)) {
                return std::nullopt;
            }

            decoded[name] =
                std::vector<std::uint8_t>(bytes.begin() + data_offset, bytes.begin() + data_offset + *file_size);
        }
        return decoded;
    }

    void MessageService::set_message(std::string_view tag, std::string_view text) {
        smgpc::compat::JkrHostAllocationScope host;
        set_message(tag, smgpc::resource::utf16_from_utf8_lossy(text));
    }

    void MessageService::set_message(std::string_view tag, std::u16string_view text) {
        smgpc::compat::JkrHostAllocationScope host;
        _messages[std::string(tag)] = MessageText{
            .raw_utf16 = std::u16string(text),
            .raw_wide = std::wstring(text.begin(), text.end()),
            .utf16 = std::u16string(text),
            .utf8 = smgpc::resource::utf8_from_utf16_lossy(text),
            .info = {},
            .control_tags = {},
        };
    }

    std::size_t MessageService::load_message_archive(const smgpc::resource::RarcArchive &archive) {
        smgpc::compat::JkrHostAllocationScope host;
        const auto messages = smgpc::resource::BmgMessageArchive::from_message_archive(archive);
        _message_indices.clear();
        _message_ids_by_index.clear();
        _message_ids_by_index.reserve(messages.message_count());
        auto message_index = std::uint32_t{};
        for (const auto &message : messages.messages()) {
            _messages[message.id] = MessageText{
                .raw_utf16 = message.raw_text,
                .raw_wide = std::wstring(message.raw_text.begin(), message.raw_text.end()),
                .utf16 = message.display_text,
                .utf8 = smgpc::resource::utf8_from_utf16_lossy(message.display_text),
                .info = message.info,
                .control_tags = message.control_tags,
            };
            _message_indices[message.id] = message_index++;
            _message_ids_by_index.push_back(message.id);
        }
        _flow_data = messages.flow();

        return messages.message_count();
    }

    std::size_t MessageService::message_count() const {
        return _messages.size();
    }

    const std::string *MessageService::message(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.utf8;
        }

        return nullptr;
    }

    const std::u16string *MessageService::message_utf16(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.utf16;
        }

        return nullptr;
    }

    const std::u16string *MessageService::message_raw_utf16(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.raw_utf16;
        }

        return nullptr;
    }

    const std::wstring *MessageService::message_raw_wide(std::string_view tag) const {
        smgpc::compat::JkrHostAllocationScope host;
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.raw_wide;
        }
        return nullptr;
    }

    const char *MessageService::message_id_for_wide_pointer(const wchar_t *text) const noexcept {
        if (text == nullptr) return nullptr;
        for (const auto &[id, message] : _messages) {
            if (message.raw_wide.c_str() == text) return id.c_str();
        }
        return nullptr;
    }

    const smgpc::resource::BmgMessageInfo *MessageService::message_info(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.info;
        }

        return nullptr;
    }

    const std::vector<smgpc::resource::BmgControlTag> *MessageService::message_control_tags(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.control_tags;
        }

        return nullptr;
    }

    std::u16string MessageService::format_message_utf16(std::string_view tag, std::span<const smgpc::resource::BmgFormatArg> args) const {
        const auto *raw_text = message_raw_utf16(tag);
        if (raw_text == nullptr) {
            return {};
        }

        return smgpc::resource::format_bmg_text(*raw_text, args);
    }

    std::optional<std::uint32_t> MessageService::message_index(std::string_view tag) const {
        const auto found = _message_indices.find(tag);
        return found != _message_indices.end() ? std::optional<std::uint32_t>(found->second) : std::nullopt;
    }

    const std::string *MessageService::message_id(std::uint32_t index) const {
        return index < _message_ids_by_index.size() ? &_message_ids_by_index[index] : nullptr;
    }

    const smgpc::resource::BmgFlowData *MessageService::flow_data() const {
        return _flow_data.has_value() ? &*_flow_data : nullptr;
    }

    const smgpc::resource::BmgFlowNode *MessageService::flow_node(std::uint32_t index) const {
        const auto *flow = flow_data();
        return flow != nullptr && index < flow->nodes.size() ? &flow->nodes[index] : nullptr;
    }

    std::optional<std::uint32_t> MessageService::first_flow_node_for_message(std::uint32_t message_index) const {
        const auto *flow = flow_data();
        if (flow == nullptr) {
            return std::nullopt;
        }

        for (auto index = std::size_t{}; index < flow->nodes.size(); ++index) {
            const auto &node = flow->nodes[index];
            if (node.node_type == 1U && node.index == message_index) {
                return static_cast<std::uint32_t>(index);
            }
        }

        return std::nullopt;
    }

    std::optional<std::uint16_t> MessageService::branch_flow_node(std::uint32_t branch_index) const {
        const auto *flow = flow_data();
        if (flow == nullptr || branch_index >= flow->branch_node_indices.size()) {
            return std::nullopt;
        }
        const auto node_index = flow->branch_node_indices[branch_index];
        return node_index < flow->nodes.size() ? std::optional<std::uint16_t>(node_index) : std::nullopt;
    }

    void SceneLightService::clear() {
        _lights = {};
        _actor_ambient.reset();
    }

    void SceneLightService::clear_light(std::size_t index) {
        if (index >= _lights.size()) {
            return;
        }

        _lights[index] = smgpc::render::GXLightState{};
    }

    void SceneLightService::set_light(std::size_t index, const smgpc::render::GXLightState &light) {
        if (index >= _lights.size()) {
            return;
        }

        _lights[index] = light;
        _lights[index].loaded = true;
    }

    void SceneLightService::clear_actor_ambient() {
        _actor_ambient.reset();
    }

    void SceneLightService::set_actor_ambient(smgpc::render::GXColorValue color) {
        _actor_ambient = color;
    }

    const smgpc::render::GXLightState *SceneLightService::light(std::size_t index) const {
        if (index >= _lights.size() || !_lights[index].loaded) {
            return nullptr;
        }

        return &_lights[index];
    }

    std::span<const smgpc::render::GXLightState> SceneLightService::lights() const {
        return _lights;
    }

    const std::optional<smgpc::render::GXColorValue> &SceneLightService::actor_ambient() const {
        return _actor_ambient;
    }

    std::uint8_t SceneLightService::loaded_mask() const {
        auto mask = std::uint8_t{};
        for (auto index = 0zu; index < _lights.size(); ++index) {
            if (_lights[index].loaded) {
                mask |= static_cast<std::uint8_t>(1U << index);
            }
        }
        return mask;
    }

}  // namespace smgpc::runtime
