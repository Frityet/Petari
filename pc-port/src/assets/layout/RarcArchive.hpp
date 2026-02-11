#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "AssetServices.hpp"

namespace smgpc::assets::layout {

class RarcArchive {
public:
    struct Entry {
        std::string path {};
        std::size_t offset {};
        std::size_t size {};
    };

    [[nodiscard]] static AssetResult<RarcArchive> parse(std::vector<std::byte> bytes);

    [[nodiscard]] std::span<const std::byte> find_entry(std::string_view path) const;
    [[nodiscard]] const std::vector<Entry> &entries() const;

private:
    static void normalize_path(std::string *path);

    std::vector<std::byte> _bytes {};
    std::vector<Entry> _entries {};
    std::unordered_map<std::string, std::size_t> _index_by_path {};
};

}  // namespace smgpc::assets::layout
