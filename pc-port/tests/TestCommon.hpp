#pragma once

#include "assets/MenuAssetPipeline.hpp"

#include <filesystem>
#include <stdexcept>

namespace pcport::test {

inline std::filesystem::path FindRepoRoot() {
    std::filesystem::path cursor = std::filesystem::current_path();

    for (int i = 0; i < 10; ++i) {
        if (std::filesystem::exists(cursor / "pc-port" / "tools" / "prepare_menu_assets.py") &&
            std::filesystem::exists(cursor / "build" / "tools" / "dtk")) {
            return cursor;
        }

        if (cursor.has_parent_path()) {
            cursor = cursor.parent_path();
        } else {
            break;
        }
    }

    throw std::runtime_error("Unable to locate repository root for tests");
}

inline const PreparedMenuAssets& GetPreparedAssets() {
    static PreparedMenuAssets assets = [] {
        const std::filesystem::path repoRoot = FindRepoRoot();

        MenuAssetPipelineConfig pipeline;
        pipeline.locator.gameRoot = repoRoot;
        pipeline.locator.version = "RMGK01";
        pipeline.locator.language = "KrKorean";
        pipeline.repoRoot = repoRoot;
        pipeline.cacheRoot = repoRoot / "pc-port" / ".cache" / "assets";
        pipeline.forceRebuild = false;

        return PrepareMenuAssets(pipeline);
    }();

    return assets;
}

}  // namespace pcport::test
