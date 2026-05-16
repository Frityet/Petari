#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace smgpc::game {

struct RarcEntry {
    std::string path;
    std::uint32_t data_offset = 0U;
    std::uint32_t data_size = 0U;
    std::uint8_t flags = 0U;
};

class RarcArchive final {
public:
    static RarcArchive from_file(const std::filesystem::path &path);
    static RarcArchive from_bytes(std::vector<std::uint8_t> bytes);

    [[nodiscard]] const std::vector<RarcEntry> &entries() const;
    [[nodiscard]] bool contains(std::string_view path) const;
    [[nodiscard]] const RarcEntry *find(std::string_view path) const;
    [[nodiscard]] std::span<const std::uint8_t> file_data(const RarcEntry &entry) const;
    [[nodiscard]] std::span<const std::uint8_t> file_data(std::string_view path) const;

private:
    explicit RarcArchive(std::vector<std::uint8_t> bytes);

    void parse();
    void walk_directory(std::uint32_t dir_index, std::string path);

    [[nodiscard]] std::string file_name(std::uint32_t name_offset) const;
    [[nodiscard]] std::span<const std::uint8_t> file_entry(std::uint32_t file_index) const;
    [[nodiscard]] std::span<const std::uint8_t> dir_entry(std::uint32_t dir_index) const;

    std::vector<std::uint8_t> _bytes;
    std::vector<RarcEntry> _entries;
    std::uint32_t _dir_count = 0U;
    std::uint32_t _dir_offset = 0U;
    std::uint32_t _file_count = 0U;
    std::uint32_t _file_offset = 0U;
    std::uint32_t _string_table_offset = 0U;
    std::uint32_t _header_size = 0U;
    std::uint32_t _file_data_start = 0U;
};

}  // namespace smgpc::game
