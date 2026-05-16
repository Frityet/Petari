#include "compat/HostNandStorage.hpp"

#include "Game/System/NANDManager.hpp"
#include "compat/RuntimeAssetLoader.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

namespace smgpc::game::compat {
namespace {

[[nodiscard]] std::filesystem::path default_save_root() {
    if (const char *env = std::getenv("SMGPC_SAVE_DIR"); env != nullptr && env[0] != '\0') {
        return std::filesystem::path(env);
    }

    const RuntimeAssetLoaderScope asset_loader{};
    if (asset_loader) {
        const auto &configuration = asset_loader->configuration();
        if (!configuration.game_root.empty() && !configuration.version.empty()) {
            return configuration.game_root / "pc-port" / "saves" / configuration.version;
        }
    }

    return std::filesystem::current_path() / "pc-port" / "saves" / "RMGK01";
}

[[nodiscard]] std::string strip_leading_slash(std::string path) {
    while (!path.empty() && path.front() == '/') {
        path.erase(path.begin());
    }
    return path;
}

[[nodiscard]] std::string basename(std::string_view path) {
    std::string text(path);
    std::replace(text.begin(), text.end(), '\\', '/');
    const auto slash = text.find_last_of('/');
    if (slash == std::string::npos) {
        return text;
    }
    return text.substr(slash + 1U);
}

}  // namespace

HostNandStorage::HostNandStorage(std::filesystem::path root)
    : _root(std::move(root)) {
}

const std::filesystem::path &HostNandStorage::root() const {
    return _root;
}

std::filesystem::path HostNandStorage::resolve(std::string_view nand_path) const {
    std::string path(nand_path);
    std::replace(path.begin(), path.end(), '\\', '/');

    if (path.starts_with("/tmp/")) {
        return _root / "tmp" / strip_leading_slash(path.substr(5U));
    }
    if (path.starts_with("/title/00010000/524d474b/data/")) {
        return _root / "data" / strip_leading_slash(path.substr(31U));
    }
    if (path == "/title/00010000/524d474b/data") {
        return _root / "data";
    }

    return _root / "data" / strip_leading_slash(std::move(path));
}

std::string HostNandStorage::home_dir() const {
    return "/title/00010000/524d474b/data";
}

s32 HostNandStorage::check(u32, u32, u32 *pAnswer) const {
    if (pAnswer != nullptr) {
        *pAnswer = 0U;
    }
    return NAND_RESULT_OK;
}

s32 HostNandStorage::read(std::string_view nand_path, void *pDst, u32 max_length, u32 *pLength) const {
    const auto path = resolve(nand_path);
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return NAND_RESULT_NOEXISTS;
    }

    const std::vector<char> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (pLength != nullptr) {
        *pLength = static_cast<u32>(bytes.size());
    }
    if (bytes.size() > max_length) {
        return NAND_RESULT_AUTHENTICATION;
    }
    if (!bytes.empty() && pDst != nullptr) {
        std::copy(bytes.begin(), bytes.end(), static_cast<char *>(pDst));
    }
    return NAND_RESULT_OK;
}

s32 HostNandStorage::write(std::string_view nand_path, const void *pSrc, u32 length) const {
    const auto path = resolve(nand_path);
    std::error_code error {};
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return NAND_RESULT_ACCESS;
    }

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return NAND_RESULT_ACCESS;
    }
    if (length != 0U && pSrc != nullptr) {
        stream.write(static_cast<const char *>(pSrc), static_cast<std::streamsize>(length));
    }
    return stream ? NAND_RESULT_OK : NAND_RESULT_ACCESS;
}

s32 HostNandStorage::remove(std::string_view nand_path) const {
    const auto path = resolve(nand_path);
    std::error_code error {};
    const bool removed = std::filesystem::remove(path, error);
    if (error) {
        return NAND_RESULT_ACCESS;
    }
    return removed ? NAND_RESULT_OK : NAND_RESULT_NOEXISTS;
}

s32 HostNandStorage::move(std::string_view nand_path, std::string_view dest_dir) const {
    const auto source = resolve(nand_path);
    const auto dest = resolve(dest_dir) / basename(nand_path);
    std::error_code error {};
    std::filesystem::create_directories(dest.parent_path(), error);
    if (error) {
        return NAND_RESULT_ACCESS;
    }

    std::filesystem::rename(source, dest, error);
    if (!error) {
        return NAND_RESULT_OK;
    }

    std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing, error);
    if (error) {
        return NAND_RESULT_ACCESS;
    }
    std::filesystem::remove(source, error);
    return NAND_RESULT_OK;
}

HostNandStorage &HostNandStorage::instance() {
    static HostNandStorage storage(default_save_root());
    const auto desired_root = default_save_root();
    if (storage._root != desired_root) {
        storage._root = desired_root;
    }
    return storage;
}

}  // namespace smgpc::game::compat
