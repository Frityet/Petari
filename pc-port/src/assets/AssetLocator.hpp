#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pcport {

struct AssetLocatorConfig {
    std::filesystem::path gameRoot;
    std::string version = "RMGK01";
    std::string language = "KrKorean";
};

enum class ArchiveKind {
    LayoutData,
    LanguageLayoutData,
};

class AssetLocator {
public:
    explicit AssetLocator(AssetLocatorConfig config);

    const AssetLocatorConfig& GetConfig() const;

    std::vector<std::filesystem::path> BuildCandidateArchivePaths(std::string_view archiveName, ArchiveKind kind) const;

    std::optional<std::filesystem::path> ResolveArchivePath(std::string_view archiveName, ArchiveKind kind) const;

private:
    std::filesystem::path BuildArchivePath(const std::filesystem::path& root, std::string_view archiveName, ArchiveKind kind) const;

    AssetLocatorConfig mConfig;
};

}  // namespace pcport
