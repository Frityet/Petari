#pragma once

#include <aurora/nand.hpp>
#include <cstddef>
#include <filesystem>

namespace smgpc::runtime {
    enum class NandImportExisting { Preserve, Replace };
    struct NandImportResult {
        std::size_t imported_files = 0;
        std::size_t preserved_files = 0;
        std::size_t imported_bytes = 0;
    };
    // Import an actual console-root directory into the same owned NAND view.
    // Every path is absolute from that root. Existing title-save mapping is
    // unchanged. Failed imports leave the complete destination untouched.
    [[nodiscard]] NandImportResult import_console_nand_directory(
        aurora::NandFileSystem&, const std::filesystem::path&, NandImportExisting);
}
