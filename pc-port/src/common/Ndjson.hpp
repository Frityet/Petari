#pragma once

#include <filesystem>
#include <vector>

#include "DumpJson.hpp"

namespace smgpc::dump {

    void write_ndjson_file(const std::filesystem::path &path, const std::vector<Json> &records);
    [[nodiscard]] std::vector<Json> load_ndjson_file(const std::filesystem::path &path);

}  // namespace smgpc::dump
