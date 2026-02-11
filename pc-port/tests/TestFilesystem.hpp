#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace smgpc::test {

class TempDirectory final {
public:
    TempDirectory() {
        static std::atomic_uint64_t counter {0};

        const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
        const auto unique_id = counter.fetch_add(1);

        _path = std::filesystem::temp_directory_path() /
            ("smgpc-tests-" + std::to_string(now_ns.count()) + "-" + std::to_string(unique_id));

        std::filesystem::create_directories(_path);
    }

    TempDirectory(const TempDirectory &) = delete;
    TempDirectory &operator=(const TempDirectory &) = delete;

    TempDirectory(TempDirectory &&other) noexcept
        : _path(std::move(other._path)) {
        other._path.clear();
    }

    TempDirectory &operator=(TempDirectory &&other) noexcept {
        if (this == &other) {
            return *this;
        }

        cleanup();
        _path = std::move(other._path);
        other._path.clear();
        return *this;
    }

    ~TempDirectory() {
        cleanup();
    }

    [[nodiscard]] const std::filesystem::path &path() const noexcept {
        return _path;
    }

private:
    void cleanup() noexcept {
        if (_path.empty()) {
            return;
        }

        std::error_code remove_error {};
        std::filesystem::remove_all(_path, remove_error);
        _path.clear();
    }

    std::filesystem::path _path {};
};

inline std::vector<std::byte> bytes_from_string(std::string_view text) {
    std::vector<std::byte> bytes {};
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

inline std::string string_from_bytes(std::span<const std::byte> bytes) {
    std::string result {};
    result.reserve(bytes.size());
    for (const std::byte value : bytes) {
        result.push_back(static_cast<char>(std::to_integer<unsigned char>(value)));
    }
    return result;
}

inline void write_bytes(const std::filesystem::path &path, std::span<const std::byte> bytes) {
    const auto parent_path = path.parent_path();
    if (not parent_path.empty()) {
        std::filesystem::create_directories(parent_path);
    }

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (not stream.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + path.string());
    }

    if (not bytes.empty()) {
        stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (not stream) {
            throw std::runtime_error("Failed to write bytes to: " + path.string());
        }
    }
}

}  // namespace smgpc::test
