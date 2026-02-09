#include "assets/MenuAssetPipeline.hpp"

#include "core/Logger.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace pcport {
namespace {

std::string QuoteShell(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('\'');
    for (const char c : value) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

void RequirePath(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Required path missing: " + path.string());
    }
}

}  // namespace

PreparedMenuAssets PrepareMenuAssets(const MenuAssetPipelineConfig& config) {
    const std::filesystem::path scriptPath = config.repoRoot / "pc-port" / "tools" / "prepare_menu_assets.py";
    if (!std::filesystem::exists(scriptPath)) {
        throw std::runtime_error("Asset prep script missing: " + scriptPath.string());
    }

    const std::string command = "python3 " + QuoteShell(scriptPath.string()) + " --game-root " + QuoteShell(config.locator.gameRoot.string()) +
                                " --version " + QuoteShell(config.locator.version) + " --language " + QuoteShell(config.locator.language) +
                                " --out-dir " + QuoteShell(config.cacheRoot.string()) + (config.forceRebuild ? " --force" : "");

    Log(LogLevel::Info, LogCategory::Assets, "Running asset prep: " + command);
    const int code = std::system(command.c_str());
    if (code != 0) {
        throw std::runtime_error("Asset prep command failed with code " + std::to_string(code));
    }

    const std::filesystem::path root = config.cacheRoot / config.locator.version / config.locator.language;
    PreparedMenuAssets assets = {
        .root = root,
        .pressStartDir = root / "PressStart",
        .fileSelectDir = root / "FileSelect",
        .titleLogoDir = root / "TitleLogo",
        .fontDir = root / "Font",
    };

    RequirePath(assets.pressStartDir / "blyt" / "pressstart.brlyt");
    RequirePath(assets.fileSelectDir / "blyt" / "fileselect.brlyt");
    RequirePath(assets.titleLogoDir / "blyt" / "titlelogo.brlyt");

    Log(LogLevel::Info, LogCategory::Assets, "Asset prep complete at " + assets.root.string());
    return assets;
}

}  // namespace pcport
