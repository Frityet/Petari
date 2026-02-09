#include "assets/AssetLocator.hpp"

#include "core/Logger.hpp"

#include <string>

namespace pcport {

AssetLocator::AssetLocator(AssetLocatorConfig config) : mConfig(std::move(config)) {}

const AssetLocatorConfig& AssetLocator::GetConfig() const {
    return mConfig;
}

std::filesystem::path AssetLocator::BuildArchivePath(const std::filesystem::path& root, std::string_view archiveName, ArchiveKind kind) const {
    const std::string fileName = std::string(archiveName) + ".arc";
    if (kind == ArchiveKind::LanguageLayoutData) {
        return root / mConfig.language / "LayoutData" / fileName;
    }
    return root / "LayoutData" / fileName;
}

std::vector<std::filesystem::path> AssetLocator::BuildCandidateArchivePaths(std::string_view archiveName, ArchiveKind kind) const {
    std::vector<std::filesystem::path> candidates;

    const std::filesystem::path origFiles = mConfig.gameRoot / "orig" / mConfig.version / "files";
    const std::filesystem::path dumpFiles = mConfig.gameRoot / "dump" / "DATA" / "files";
    const std::filesystem::path directFiles = mConfig.gameRoot / "files";

    candidates.emplace_back(BuildArchivePath(origFiles, archiveName, kind));
    candidates.emplace_back(BuildArchivePath(dumpFiles, archiveName, kind));
    candidates.emplace_back(BuildArchivePath(directFiles, archiveName, kind));
    candidates.emplace_back(BuildArchivePath(mConfig.gameRoot, archiveName, kind));

    if (kind == ArchiveKind::LanguageLayoutData) {
        candidates.emplace_back(BuildArchivePath(origFiles, archiveName, ArchiveKind::LayoutData));
        candidates.emplace_back(BuildArchivePath(dumpFiles, archiveName, ArchiveKind::LayoutData));
        candidates.emplace_back(BuildArchivePath(directFiles, archiveName, ArchiveKind::LayoutData));
        candidates.emplace_back(BuildArchivePath(mConfig.gameRoot, archiveName, ArchiveKind::LayoutData));
    }

    return candidates;
}

std::optional<std::filesystem::path> AssetLocator::ResolveArchivePath(std::string_view archiveName, ArchiveKind kind) const {
    for (const auto& candidate : BuildCandidateArchivePaths(archiveName, kind)) {
        if (std::filesystem::exists(candidate)) {
            Log(LogLevel::Debug, LogCategory::Assets,
                "Resolved archive " + std::string(archiveName) + " -> " + candidate.string());
            return candidate;
        }
    }

    Log(LogLevel::Warn, LogCategory::Assets, "Failed to resolve archive " + std::string(archiveName));
    return std::nullopt;
}

}  // namespace pcport
