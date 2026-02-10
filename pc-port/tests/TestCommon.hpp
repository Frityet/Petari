#pragma once

#include "assets/MenuAssetPipeline.hpp"

#include <filesystem>
#include <stdexcept>

namespace smgpc::test {

inline std::filesystem::path FindRepoRoot() {
    std::filesystem::path cursor = std::filesystem::current_path();

    for (int i = 0; i < 10; ++i) {
        if (std::filesystem::exists(cursor / "smgpc" / "tools" / "prepare_menu_assets.py") &&
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

        MenuAssetPipelineConfigUration pipeline;
        pipeline.locator.gameRoot = repoRoot;
        pipeline.locator.version = "RMGK01";
        pipeline.locator.language = "KrKorean";
        pipeline.repoRoot = repoRoot;
        pipeline.cacheRoot = repoRoot / "smgpc" / ".cache" / "assets";
        pipeline.forceRebuild = false;

        return PrepareMenuAssets(pipeline);
    }();

    return assets;
}

}  // namespace smgpc::test
