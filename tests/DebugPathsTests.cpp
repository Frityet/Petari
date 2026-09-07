#include "debug/DebugPaths.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
    namespace fs = std::filesystem;

    struct Workspace {
        fs::path original = fs::current_path();
        fs::path root = fs::temp_directory_path() /
            ("smgpc-paths-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

        Workspace() {
            fs::create_directories(root / "src/Game");
            fs::create_directories(root / "build/nested");
            std::ofstream(root / "xmake.lua") << "set_project(\"test\")\n";
            root = fs::canonical(root);
        }

        ~Workspace() {
            auto error = std::error_code {};
            fs::current_path(original, error);
            fs::remove_all(root, error);
        }
    };

    void require(bool condition, const char *message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }
}

int main() try {
    const auto workspace = Workspace {};
    for (const auto &directory : {workspace.root, workspace.root / "build/nested"}) {
        fs::current_path(directory);
        require(smgpc::debug::pc_port_root() == workspace.root, "PC root discovery failed");
        require(smgpc::debug::repo_root() == workspace.root, "repository root must be the PC root");
        require(smgpc::debug::cache_path("probe") == workspace.root / ".cache/probe",
                "probe cache must stay inside the repository");
    }

    const auto container_files = workspace.root / "container/orig/RMGK01/files";
    fs::create_directories(container_files / "ObjectData");
    require(smgpc::debug::disc_files_root() == container_files, "container fixture discovery failed");

    const auto original_files = workspace.root / "orig/RMGK01/files";
    fs::create_directories(original_files / "ObjectData");
    require(smgpc::debug::disc_files_root() == original_files, "root fixtures should take precedence");

    fs::remove_all(original_files);
    fs::remove_all(container_files / "ObjectData");
    fs::create_directories(container_files / "LayoutData");
    require(smgpc::debug::disc_files_root() == container_files, "layout-only extraction should be usable");

    fs::remove_all(container_files);
    auto missing_reported = false;
    try {
        (void)smgpc::debug::disc_files_root();
    } catch (const std::runtime_error &) {
        missing_reported = true;
    }
    require(missing_reported, "missing fixtures must be reported");
    std::cout << "Debug path tests passed: root/nested discovery, cache, fixtures, and missing data\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
