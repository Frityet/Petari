#include "Ndjson.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace smgpc::dump {

    void write_ndjson_file(const std::filesystem::path &path, const std::vector<Json> &records) {
        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path());
        }

        auto file = std::ofstream(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("could not open NDJSON output " + path.string());
        }

        for (const auto &record : records) {
            file << record.dump() << '\n';
        }
        if (!file) {
            throw std::runtime_error("could not write NDJSON output " + path.string());
        }
    }

    std::vector<Json> load_ndjson_file(const std::filesystem::path &path) {
        auto file = std::ifstream(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("could not open NDJSON file " + path.string());
        }

        auto records = std::vector<Json>{};
        auto line = std::string {};
        auto line_number = 0U;
        while (std::getline(file, line)) {
            ++line_number;
            if (line.empty()) {
                continue;
            }

            try {
                records.push_back(Json::parse(line));
            } catch (const nlohmann::json::exception &e) {
                throw std::runtime_error("could not parse NDJSON file " + path.string() + " line " + std::to_string(line_number) + ": " + e.what());
            }
        }

        if (!file.eof()) {
            throw std::runtime_error("could not read NDJSON file " + path.string());
        }

        return records;
    }

}  // namespace smgpc::dump
