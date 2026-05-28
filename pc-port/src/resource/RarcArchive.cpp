#include "RarcArchive.hpp"

#include "Yaz0.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

namespace smgpc::resource {
    namespace {

        constexpr auto RARC_MAGIC = std::uint32_t{0x52415243U};
        constexpr auto FILE_FLAG_FILE = std::uint8_t{1U << 0U};
        constexpr auto FILE_FLAG_FOLDER = std::uint8_t{1U << 1U};

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 2U > data.size()) {
                throw std::runtime_error("RARC read past end of buffer");
            }

            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | static_cast<std::uint16_t>(data[offset + 1U]));
        }

        [[nodiscard]] std::uint32_t read_be24(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 3U > data.size()) {
                throw std::runtime_error("RARC read past end of buffer");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 16U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 8U) | static_cast<std::uint32_t>(data[offset + 2U]);
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("RARC read past end of buffer");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) | (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
        }

        [[nodiscard]] std::vector<std::uint8_t> read_file(const std::filesystem::path &path) {
            auto file = std::ifstream(path, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Cannot open RARC archive " + path.string());
            }

            file.seekg(0, std::ios::end);
            const auto size = file.tellg();
            if (size < 0) {
                throw std::runtime_error("Cannot determine RARC archive size " + path.string());
            }

            auto bytes = std::vector<std::uint8_t>(static_cast<std::size_t>(size));
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!file) {
                throw std::runtime_error("Cannot read RARC archive " + path.string());
            }

            return bytes;
        }

        [[nodiscard]] std::string lower_copy(std::string_view value) {
            auto lower = std::string(value);
            std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            return lower;
        }

        [[nodiscard]] std::string normalized_archive_path(std::string_view path) {
            auto normalized = std::string(path);
            std::ranges::replace(normalized, '\\', '/');

            const auto first_name = normalized.find_first_not_of('/');
            if (first_name == std::string::npos) {
                return {};
            }

            normalized.erase(0U, first_name);
            return normalized;
        }

        [[nodiscard]] std::string basename_lower(std::string_view path) {
            auto normalized = normalized_archive_path(path);
            while (!normalized.empty() && normalized.back() == '/') {
                normalized.pop_back();
            }

            const auto slash = normalized.find_last_of('/');
            const auto basename = slash == std::string::npos ? std::string_view(normalized) : std::string_view(normalized).substr(slash + 1U);
            return lower_copy(basename);
        }

    }  // namespace

    RarcArchive RarcArchive::from_file(const std::filesystem::path &path) {
        return from_bytes(read_file(path));
    }

    RarcArchive RarcArchive::from_bytes(std::vector<std::uint8_t> bytes) {
        return RarcArchive(decompress_yaz0(bytes));
    }

    const std::vector<RarcEntry> &RarcArchive::entries() const {
        return _entries;
    }

    std::uint16_t RarcArchive::hash_name(std::string_view name) {
        auto hash = std::uint16_t{};
        for (const auto character : name) {
            hash = static_cast<std::uint16_t>((hash * 3U) + static_cast<std::uint8_t>(character));
        }
        return hash;
    }

    bool RarcArchive::contains(std::string_view path) const {
        return find(path) != nullptr;
    }

    bool RarcArchive::contains_normalized(std::string_view path) const {
        return find_normalized(path) != nullptr;
    }

    bool RarcArchive::contains_basename(std::string_view path) const {
        return find_by_basename(path) != nullptr;
    }

    bool RarcArchive::contains_resource(std::string_view path) const {
        return find_resource(path) != nullptr;
    }

    std::uint32_t RarcArchive::count_directory_files(std::string_view directory) const {
        auto normalized = normalized_archive_path(directory);
        while (!normalized.empty() && normalized.back() == '/') {
            normalized.pop_back();
        }

        return static_cast<std::uint32_t>(std::ranges::count_if(_entries, [&normalized](const auto &entry) {
            return entry.directory == normalized;
        }));
    }

    const RarcEntry *RarcArchive::find(std::string_view path) const {
        const auto it = std::ranges::find_if(_entries, [path](const auto &entry) {
            return entry.path == path;
        });

        return it == _entries.end() ? nullptr : &*it;
    }

    const RarcEntry *RarcArchive::find_normalized(std::string_view path) const {
        if (const auto *entry = find(path); entry != nullptr) {
            return entry;
        }

        const auto normalized = normalized_archive_path(path);
        if (normalized.empty() || std::string_view(normalized) == path) {
            return nullptr;
        }

        return find(normalized);
    }

    const RarcEntry *RarcArchive::find_by_basename(std::string_view path) const {
        const auto requested = basename_lower(path);
        if (requested.empty()) {
            return nullptr;
        }

        const auto it = std::ranges::find_if(_entries, [&requested](const auto &entry) {
            return basename_lower(entry.path) == requested;
        });

        return it == _entries.end() ? nullptr : &*it;
    }

    const RarcEntry *RarcArchive::find_resource(std::string_view path) const {
        if (const auto *entry = find_normalized(path); entry != nullptr) {
            return entry;
        }

        return find_by_basename(path);
    }

    const RarcEntry *RarcArchive::find_by_file_id(std::uint16_t file_id) const {
        const auto it = std::ranges::find_if(_entries, [file_id](const auto &entry) {
            return entry.file_id == file_id;
        });

        return it == _entries.end() ? nullptr : &*it;
    }

    std::span<const std::uint8_t> RarcArchive::file_data(const RarcEntry &entry) const {
        if (entry.data_offset + entry.data_size > _bytes.size()) {
            throw std::runtime_error("RARC file data is outside archive");
        }

        return std::span<const std::uint8_t>(_bytes).subspan(entry.data_offset, entry.data_size);
    }

    std::span<const std::uint8_t> RarcArchive::file_data(std::string_view path) const {
        const auto *entry = find(path);
        if (entry == nullptr) {
            throw std::runtime_error("RARC file does not exist: " + std::string(path));
        }

        return file_data(*entry);
    }

    std::span<const std::uint8_t> RarcArchive::file_data_normalized(std::string_view path) const {
        const auto *entry = find_normalized(path);
        if (entry == nullptr) {
            throw std::runtime_error("RARC file does not exist: " + std::string(path));
        }

        return file_data(*entry);
    }

    std::span<const std::uint8_t> RarcArchive::file_data_by_basename(std::string_view path) const {
        const auto *entry = find_by_basename(path);
        if (entry == nullptr) {
            throw std::runtime_error("RARC file does not exist: " + std::string(path));
        }

        return file_data(*entry);
    }

    std::span<const std::uint8_t> RarcArchive::resource_data(std::string_view path) const {
        const auto *entry = find_resource(path);
        if (entry == nullptr) {
            throw std::runtime_error("RARC resource does not exist: " + std::string(path));
        }

        return file_data(*entry);
    }

    RarcArchive::RarcArchive(std::vector<std::uint8_t> bytes)
        : _bytes(std::move(bytes)) {
        parse();
    }

    void RarcArchive::parse() {
        const auto bytes = std::span<const std::uint8_t>(_bytes);
        if (bytes.size() < 0x40U || read_be32(bytes, 0U) != RARC_MAGIC) {
            throw std::runtime_error("Archive is not a decompressed RARC file");
        }

        _header_size = read_be32(bytes, 0x08U);
        _file_data_start = _header_size + read_be32(bytes, 0x0CU);
        if (_header_size + 0x20U > bytes.size()) {
            throw std::runtime_error("RARC info block is outside archive");
        }

        const auto info_offset = _header_size;
        _dir_count = read_be32(bytes, info_offset + 0x00U);
        _dir_offset = info_offset + read_be32(bytes, info_offset + 0x04U);
        _file_count = read_be32(bytes, info_offset + 0x08U);
        _file_offset = info_offset + read_be32(bytes, info_offset + 0x0CU);
        _string_table_offset = info_offset + read_be32(bytes, info_offset + 0x14U);

        if (_dir_offset + (_dir_count * 0x10U) > bytes.size() || _file_offset + (_file_count * 0x14U) > bytes.size() || _string_table_offset > bytes.size()) {
            throw std::runtime_error("RARC table is outside archive");
        }

        walk_directory(0U, "");
    }

    void RarcArchive::walk_directory(std::uint32_t dir_index, std::string path) {
        const auto dir = dir_entry(dir_index);
        const auto file_count = read_be16(dir, 0x0AU);
        const auto first_file_index = read_be32(dir, 0x0CU);

        for (auto i = 0U; i < file_count; ++i) {
            const auto entry = file_entry(first_file_index + i);
            const auto flags = entry[0x04U];
            const auto name = file_name(read_be24(entry, 0x05U));
            if (name == "." || name == "..") {
                continue;
            }

            if ((flags & FILE_FLAG_FOLDER) != 0U) {
                const auto child_dir_index = read_be32(entry, 0x08U);
                walk_directory(child_dir_index, path + name + "/");
            } else if ((flags & FILE_FLAG_FILE) != 0U) {
                const auto full_path = path + name;
                auto directory = path;
                while (!directory.empty() && directory.back() == '/') {
                    directory.pop_back();
                }

                _entries.push_back(RarcEntry{
                    .path = full_path,
                    .name = name,
                    .directory = std::move(directory),
                    .file_id = read_be16(entry, 0x00U),
                    .name_hash = read_be16(entry, 0x02U),
                    .file_entry_index = first_file_index + i,
                    .data_offset = _file_data_start + read_be32(entry, 0x08U),
                    .data_size = read_be32(entry, 0x0CU),
                    .flags = flags,
                });
            }
        }
    }

    std::string RarcArchive::file_name(std::uint32_t name_offset) const {
        const auto start = _string_table_offset + name_offset;
        if (start >= _bytes.size()) {
            throw std::runtime_error("RARC string offset is outside archive");
        }

        auto end = start;
        while (end < _bytes.size() && _bytes[end] != 0U) {
            ++end;
        }
        if (end == _bytes.size()) {
            throw std::runtime_error("RARC string is not null terminated");
        }

        return std::string(reinterpret_cast<const char *>(_bytes.data() + start), end - start);
    }

    std::span<const std::uint8_t> RarcArchive::file_entry(std::uint32_t file_index) const {
        if (file_index >= _file_count) {
            throw std::runtime_error("RARC file index is outside table");
        }

        return std::span<const std::uint8_t>(_bytes).subspan(_file_offset + file_index * 0x14U, 0x14U);
    }

    std::span<const std::uint8_t> RarcArchive::dir_entry(std::uint32_t dir_index) const {
        if (dir_index >= _dir_count) {
            throw std::runtime_error("RARC directory index is outside table");
        }

        return std::span<const std::uint8_t>(_bytes).subspan(_dir_offset + dir_index * 0x10U, 0x10U);
    }

}  // namespace smgpc::resource
