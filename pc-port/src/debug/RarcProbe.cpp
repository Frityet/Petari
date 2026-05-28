#include "resource/RarcArchive.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace {

    [[nodiscard]] std::filesystem::path disc_files_root() {
        const auto cwd = std::filesystem::current_path();
        const std::filesystem::path candidates[]{
            cwd / "orig" / "RMGK01" / "files",
            cwd.parent_path() / "orig" / "RMGK01" / "files",
        };

        for (const auto &candidate : candidates) {
            std::error_code error {};
            const auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        throw std::runtime_error("could not locate orig/RMGK01/files from " + cwd.string());
    }

    [[nodiscard]] std::filesystem::path resolve_archive_path(std::string_view archive_name) {
        const auto requested = std::filesystem::path(archive_name);
        if (requested.is_absolute() || requested.has_parent_path()) {
            return requested;
        }

        return disc_files_root() / "ObjectData" / (std::string(archive_name) + ".arc");
    }

}  // namespace

int main(int argc, char **argv) try {
    const auto archive_name = argc > 1 ? std::string_view(argv[1]) : std::string_view("CometNearOrbitSky");
    const auto archive_path = resolve_archive_path(archive_name);
    const auto archive = smgpc::resource::RarcArchive::from_file(archive_path);

    if (argc == 4) {
        const auto entry_name = std::string_view(argv[2]);
        const auto output_path = std::filesystem::path(argv[3]);
        const auto data = archive.file_data(entry_name);
        if (!output_path.parent_path().empty()) {
            std::filesystem::create_directories(output_path.parent_path());
        }
        auto output = std::ofstream(output_path, std::ios::binary);
        output.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        std::cout << output_path.string() << ',' << data.size() << '\n';
        return 0;
    }

    std::cout << archive_path.string() << '\n';
    for (const auto &entry : archive.entries()) {
        std::cout << entry.path << ',' << entry.data_size << '\n';
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "RARC probe failed: " << e.what() << '\n';
    return 1;
}
