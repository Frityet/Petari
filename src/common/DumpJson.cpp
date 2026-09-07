#include "DumpJson.hpp"

#include <fstream>
#include <stdexcept>

namespace smgpc::dump {

    Json load_json_file(const std::filesystem::path &path) {
        auto file = std::ifstream(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("could not open JSON file " + path.string());
        }

        try {
            return Json::parse(file);
        } catch (const nlohmann::json::exception &e) {
            throw std::runtime_error("could not parse JSON file " + path.string() + ": " + e.what());
        }
    }

    std::string dump_json(const Json &json, int indent) {
        return json.dump(indent);
    }

    void write_json_file(const std::filesystem::path &path, const Json &json, int indent) {
        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path());
        }

        auto file = std::ofstream(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("could not open JSON output " + path.string());
        }

        file << dump_json(json, indent) << '\n';
        if (!file) {
            throw std::runtime_error("could not write JSON output " + path.string());
        }
    }

}  // namespace smgpc::dump
