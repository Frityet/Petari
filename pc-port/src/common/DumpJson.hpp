#pragma once

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

namespace smgpc::dump {

    using Json = nlohmann::ordered_json;

    [[nodiscard]] Json load_json_file(const std::filesystem::path &path);
    [[nodiscard]] std::string dump_json(const Json &json, int indent = 2);
    void write_json_file(const std::filesystem::path &path, const Json &json, int indent = 2);

}  // namespace smgpc::dump
