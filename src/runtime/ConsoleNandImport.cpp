#include "runtime/ConsoleNandImport.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace smgpc::runtime {
    namespace {
        struct InputFile {
            std::filesystem::path host_path;
            std::string nand_path;
        };
        std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
            std::ifstream input(path, std::ios::binary | std::ios::ate);
            if (!input) throw std::runtime_error("Cannot open console NAND file: " + path.string());
            const auto end = input.tellg();
            if (end < 0 || static_cast<std::uintmax_t>(end) > std::numeric_limits<std::size_t>::max() ||
                static_cast<std::uintmax_t>(end) > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max()))
                throw std::runtime_error("Console NAND file size is not representable: " + path.string());
            std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
            input.seekg(0);
            if (!bytes.empty()) input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!input || input.peek() != std::char_traits<char>::eof())
                throw std::runtime_error("Console NAND file changed or could not be read completely: " + path.string());
            return bytes;
        }
    }
    NandImportResult import_console_nand_directory(
        aurora::NandFileSystem& nand, const std::filesystem::path& directory, NandImportExisting existing) {
        compat::JkrHostAllocationScope host;
        if (existing != NandImportExisting::Preserve && existing != NandImportExisting::Replace)
            throw std::invalid_argument("Unknown console NAND import policy");
        const auto root = std::filesystem::canonical(directory);
        if (!std::filesystem::is_directory(root))
            throw std::invalid_argument("Console NAND root must be an existing directory");
        std::vector<InputFile> files;
        std::set<std::string> paths;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            const auto status = entry.symlink_status();
            if (std::filesystem::is_directory(status)) continue;
            if (!std::filesystem::is_regular_file(status))
                throw std::invalid_argument("Console NAND import requires ordinary files/directories: " + entry.path().string());
            const auto relative = entry.path().lexically_relative(root);
            if (relative.empty() || relative.is_absolute())
                throw std::logic_error("Console NAND file is outside the selected root");
            const auto absolute = "/" + relative.generic_string();
            const auto normalized = nand.normalize_path(absolute);
            if (!paths.insert(normalized).second)
                throw std::invalid_argument("Console NAND paths collide after NAND normalization: " + normalized);
            files.push_back({entry.path(), normalized});
        }
        std::ranges::sort(files, {}, &InputFile::nand_path);
        // Copy the actual owner so every read/allocation can fail before commit.
        // NandFileSystem has no borrowed buffer references; its public reads copy.
        auto candidate = nand;
        NandImportResult result;
        for (const auto& file : files) {
            if (existing == NandImportExisting::Preserve && candidate.exists(file.nand_path)) {
                ++result.preserved_files;
                continue;
            }
            const auto bytes = read_file(file.host_path);
            const auto metadata = candidate.metadata(file.nand_path);
            // A plain dump contains bytes rather than NAND permission metadata.
            // Retain metadata for an existing identity; new identities use the
            // same explicit defaults as this owned NAND backend's write API.
            candidate.write_file(file.nand_path, bytes, metadata ? metadata->permission : 0x3C,
                                 metadata ? metadata->attribute : 0);
            ++result.imported_files;
            if (bytes.size() > std::numeric_limits<std::size_t>::max() - result.imported_bytes)
                throw std::overflow_error("Console NAND import byte count overflow");
            result.imported_bytes += bytes.size();
        }
        static_assert(std::is_nothrow_move_assignable_v<aurora::NandFileSystem>);
        nand = std::move(candidate);
        return result;
    }
}
