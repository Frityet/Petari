#pragma once

#include <filesystem>
#include <string_view>

namespace smgpc::debug {

    [[nodiscard]] std::filesystem::path pc_port_root();
    [[nodiscard]] std::filesystem::path repo_root();
    [[nodiscard]] std::filesystem::path disc_files_root();
    [[nodiscard]] std::filesystem::path cache_path(std::string_view tool_name);

}  // namespace smgpc::debug
