#pragma once

#include "compat/Types.hpp"

#include <filesystem>
#include <string_view>
#include <vector>

namespace smgpc::game::compat {

class HostNandStorage {
public:
    explicit HostNandStorage(std::filesystem::path root);

    [[nodiscard]] const std::filesystem::path &root() const;
    [[nodiscard]] std::filesystem::path resolve(std::string_view nand_path) const;
    [[nodiscard]] std::string home_dir() const;

    s32 check(u32 fsBlock, u32 inode, u32 *pAnswer) const;
    s32 read(std::string_view nand_path, void *pDst, u32 max_length, u32 *pLength) const;
    s32 write(std::string_view nand_path, const void *pSrc, u32 length) const;
    s32 remove(std::string_view nand_path) const;
    s32 move(std::string_view nand_path, std::string_view dest_dir) const;

    static HostNandStorage &instance();

private:
    std::filesystem::path _root;
};

}  // namespace smgpc::game::compat
