#pragma once

#include "assets/AssetLocator.hpp"

#include <filesystem>

namespace pcport {

struct MenuAssetPipelineConfig {
    AssetLocatorConfig locator;
    std::filesystem::path repoRoot;
    std::filesystem::path cacheRoot;
    bool forceRebuild = false;
};

struct PreparedMenuAssets {
    std::filesystem::path root;
    std::filesystem::path pressStartDir;
    std::filesystem::path fileSelectDir;
    std::filesystem::path titleLogoDir;
    std::filesystem::path fontDir;
};

PreparedMenuAssets PrepareMenuAssets(const MenuAssetPipelineConfig& config);

}  // namespace pcport
