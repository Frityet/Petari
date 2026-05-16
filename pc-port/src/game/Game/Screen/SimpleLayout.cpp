#include "Game/Screen/SimpleLayout.hpp"

#include <memory>
#include <string>
#include <string_view>

#include "Logger.hpp"
#include "ServiceProvider.hpp"
#include "compat/RuntimeAssetLoader.hpp"
#include "compat/RuntimeContext.hpp"
#include "layout/LayoutArchiveLoader.hpp"
#include "layout/LayoutRuntimeActor.hpp"

namespace {

[[nodiscard]] bool ends_with_arc(std::string_view text) {
    return text.size() >= 4U && text.substr(text.size() - 4U) == ".arc";
}

}  // namespace

SimpleLayout::SimpleLayout(const char *pName, const char *pArchiveName, int a3, int a4)
    : LayoutActor(pName, loadRuntimeActor(pArchiveName)) {
    (void)a3;
    (void)a4;
}

void SimpleLayout::initWithoutIter() {
}

std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> SimpleLayout::loadRuntimeActor(const char *pArchiveName) {
    if (pArchiveName == nullptr) {
        return nullptr;
    }

    const auto &context = smgpc::game::compat::runtime_context();
    const smgpc::game::compat::RuntimeAssetLoaderScope asset_loader {};
    if (not asset_loader) {
        return nullptr;
    }

    std::string archive_path;
    const std::string archive_name(pArchiveName);
    if (ends_with_arc(archive_name)) {
        auto path = asset_loader->layout_archive_file_name(archive_name);
        archive_path = path.value_or("/LayoutData/" + archive_name);
    } else {
        auto path = asset_loader->layout_archive_file_name_from_prefix(archive_name, true);
        archive_path = path.value_or("/LayoutData/" + archive_name + ".arc");
    }

    auto loader = smgpc::game::layout::LayoutArchiveLoader(
        smgpc::di::DependencyReference<const smgpc::assets::AssetLoader>(*asset_loader),
        context.logger);
    smgpc::game::layout::LayoutArchiveLoadRequest request {
        .archive_path = std::move(archive_path),
    };

    auto loaded = loader.load(request);
    if (not loaded) {
        if (context.logger) {
            context.logger->error(
                __FILE__,
                __LINE__,
                smgpc::logging::Category::GAME,
                "Failed to load layout archive {}: {}",
                pArchiveName,
                loaded.failure().message);
        }
        return nullptr;
    }

    return std::make_shared<smgpc::game::layout::LayoutRuntimeActor>(*loaded);
}
