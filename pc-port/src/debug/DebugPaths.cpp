#include "DebugPaths.hpp"

#include <stdexcept>
#include <system_error>

namespace smgpc::debug {
    namespace {
        [[nodiscard]] bool is_pc_port_root(const std::filesystem::path &path) {
            auto error = std::error_code {};
            return std::filesystem::is_regular_file(path / "xmake.lua", error) &&
                   std::filesystem::is_directory(path / "src" / "Game", error);
        }

        [[nodiscard]] std::filesystem::path find_upward(std::filesystem::path start) {
            auto error = std::error_code {};
            start = std::filesystem::weakly_canonical(start, error);
            if (error) {
                start = std::filesystem::current_path();
            }

            for (auto candidate = start; !candidate.empty(); candidate = candidate.parent_path()) {
                if (is_pc_port_root(candidate)) {
                    return candidate;
                }

                if (is_pc_port_root(candidate / "pc-port")) {
                    return candidate / "pc-port";
                }

                if (candidate == candidate.parent_path()) {
                    break;
                }
            }

            throw std::runtime_error("could not locate pc-port root from " + start.string());
        }
    }  // namespace

    std::filesystem::path pc_port_root() {
        return find_upward(std::filesystem::current_path());
    }

    std::filesystem::path repo_root() {
        const auto pc_root = pc_port_root();
        if (pc_root.filename() == "pc-port") {
            return pc_root.parent_path();
        }
        return pc_root;
    }

    std::filesystem::path disc_files_root() {
        const auto root = repo_root();
        const auto pc_root = pc_port_root();
        const std::filesystem::path candidates[]{
            root / "orig" / "RMGK01" / "files",
            pc_root / ".." / "orig" / "RMGK01" / "files",
        };

        for (const auto &candidate : candidates) {
            auto error = std::error_code {};
            const auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        throw std::runtime_error("could not locate orig/RMGK01/files from " + std::filesystem::current_path().string());
    }

    std::filesystem::path cache_path(std::string_view tool_name) {
        return pc_port_root() / ".cache" / std::string(tool_name);
    }

}  // namespace smgpc::debug
