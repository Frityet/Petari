#include "RarcArchive.hpp"

#include <functional>
#include <string>

#include "Binary.hpp"

namespace smgpc::assets::layout {
namespace {

struct DirEntry {
    std::uint32_t name_offset {};
    std::uint16_t file_count {};
    std::uint32_t first_file_index {};
};

struct FileEntry {
    std::uint8_t flags {};
    std::uint32_t name_offset {};
    std::uint32_t data_offset {};
    std::uint32_t data_size {};
};

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message)
    };
}

[[nodiscard]] bool is_folder(const FileEntry &entry) {
    return (entry.flags & (1U << 1U)) != 0U;
}

[[nodiscard]] bool is_file(const FileEntry &entry) {
    return (entry.flags & (1U << 0U)) != 0U;
}

}  // namespace

AssetResult<RarcArchive> RarcArchive::parse(std::vector<std::byte> bytes) {
    using namespace binary;

    if (bytes.size() < 0x40U) {
        return make_error("RARC buffer too small for header.");
    }

    const auto source = std::span<const std::byte>(bytes);
    if (not fourcc_equals(source, 0U, "RARC")) {
        return make_error("RARC magic mismatch.");
    }

    const auto header_size = static_cast<std::size_t>(read_u32_be(source, 0x08U));
    const auto file_data_offset = static_cast<std::size_t>(read_u32_be(source, 0x0CU));
    const auto info_offset = header_size;

    if (not has_bytes(source, info_offset, 0x20U)) {
        return make_error("RARC info block is out of bounds.");
    }

    const auto directory_count = static_cast<std::size_t>(read_u32_be(source, info_offset + 0x00U));
    const auto dir_offset = static_cast<std::size_t>(read_u32_be(source, info_offset + 0x04U));
    const auto file_count = static_cast<std::size_t>(read_u32_be(source, info_offset + 0x08U));
    const auto file_offset = static_cast<std::size_t>(read_u32_be(source, info_offset + 0x0CU));
    const auto string_table_offset = static_cast<std::size_t>(read_u32_be(source, info_offset + 0x14U));

    const auto dir_table = info_offset + dir_offset;
    const auto file_table = info_offset + file_offset;
    const auto string_table = info_offset + string_table_offset;
    const auto data_table = header_size + file_data_offset;

    if (not has_bytes(source, dir_table, directory_count * 0x10U)) {
        return make_error("RARC directory table exceeds archive bounds.");
    }
    if (not has_bytes(source, file_table, file_count * 0x14U)) {
        return make_error("RARC file table exceeds archive bounds.");
    }
    if (string_table >= source.size()) {
        return make_error("RARC string table offset exceeds archive bounds.");
    }
    if (data_table > source.size()) {
        return make_error("RARC data table offset exceeds archive bounds.");
    }

    std::vector<DirEntry> directories {};
    directories.reserve(directory_count);
    for (std::size_t i = 0; i < directory_count; ++i) {
        const std::size_t entry_offset = dir_table + i * 0x10U;
        directories.push_back(DirEntry {
            .name_offset = read_u32_be(source, entry_offset + 0x04U),
            .file_count = read_u16_be(source, entry_offset + 0x0AU),
            .first_file_index = read_u32_be(source, entry_offset + 0x0CU),
        });
    }

    std::vector<FileEntry> files {};
    files.reserve(file_count);
    for (std::size_t i = 0; i < file_count; ++i) {
        const std::size_t entry_offset = file_table + i * 0x14U;
        const auto packed_flag_name = read_u32_be(source, entry_offset + 0x04U);
        files.push_back(FileEntry {
            .flags = static_cast<std::uint8_t>((packed_flag_name >> 24U) & 0xFFU),
            .name_offset = packed_flag_name & 0x00FFFFFFU,
            .data_offset = read_u32_be(source, entry_offset + 0x08U),
            .data_size = read_u32_be(source, entry_offset + 0x0CU),
        });
    }

    RarcArchive archive {};
    archive._bytes = std::move(bytes);

    std::function<AssetResult<void>(std::size_t, const std::string &)> walk;
    walk = [&](std::size_t dir_index, const std::string &path_prefix) -> AssetResult<void> {
        if (dir_index >= directories.size()) {
            return make_error("RARC directory index points out of bounds.");
        }

        const auto &dir = directories[dir_index];
        if (static_cast<std::size_t>(dir.first_file_index) + static_cast<std::size_t>(dir.file_count) > files.size()) {
            return make_error("RARC directory references file entries out of bounds.");
        }

        for (std::size_t i = 0; i < dir.file_count; ++i) {
            const auto &file = files[dir.first_file_index + i];
            const auto name = read_c_string(source, string_table + static_cast<std::size_t>(file.name_offset));
            if (name.empty()) {
                continue;
            }

            if (is_folder(file)) {
                if (name == "." or name == "..") {
                    continue;
                }

                const auto next_path = path_prefix.empty() ? name : path_prefix + "/" + name;
                const auto walk_result = walk(static_cast<std::size_t>(file.data_offset), next_path);
                if (not walk_result) {
                    return walk_result;
                }
                continue;
            }

            if (not is_file(file)) {
                continue;
            }

            const auto entry_data_offset = data_table + static_cast<std::size_t>(file.data_offset);
            const auto entry_data_size = static_cast<std::size_t>(file.data_size);
            if (not has_bytes(source, entry_data_offset, entry_data_size)) {
                return make_error("RARC file entry data exceeds archive bounds.");
            }

            const auto entry_path = path_prefix.empty() ? name : path_prefix + "/" + name;
            archive._entries.push_back(Entry {
                .path = entry_path,
                .offset = entry_data_offset,
                .size = entry_data_size,
            });
        }

        return {};
    };

    const auto walk_result = walk(0U, {});
    if (not walk_result) {
        return walk_result.failure();
    }

    for (std::size_t i = 0; i < archive._entries.size(); ++i) {
        auto normalized = archive._entries[i].path;
        normalize_path(&normalized);
        archive._index_by_path.emplace(std::move(normalized), i);
    }

    return archive;
}

std::span<const std::byte> RarcArchive::find_entry(std::string_view path) const {
    auto normalized = std::string(path);
    normalize_path(&normalized);

    const auto found = _index_by_path.find(normalized);
    if (found == _index_by_path.end()) {
        return {};
    }

    const auto &entry = _entries[found->second];
    return std::span<const std::byte>(_bytes).subspan(entry.offset, entry.size);
}

const std::vector<RarcArchive::Entry> &RarcArchive::entries() const {
    return _entries;
}

void RarcArchive::normalize_path(std::string *path) {
    if (path == nullptr) {
        return;
    }

    for (auto &character : *path) {
        if (character == '\\') {
            character = '/';
        }
    }

    while (path->starts_with("./")) {
        path->erase(0, 2);
    }
    while (path->starts_with('/')) {
        path->erase(path->begin());
    }

    *path = binary::to_lower_ascii(*path);
}

}  // namespace smgpc::assets::layout
