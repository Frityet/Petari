#pragma once

#include "AssetLoader.hpp"
#include "compat/RuntimeContext.hpp"

namespace smgpc::game::compat {

class RuntimeAssetLoaderScope {
public:
    RuntimeAssetLoaderScope() {
        const auto& context = runtime_context();
        if (context.asset_loader) {
            _loader = context.asset_loader.get();
        }
    }

    [[nodiscard]] explicit operator bool() const {
        return _loader != nullptr;
    }

    [[nodiscard]] const assets::AssetLoader* get() const {
        return _loader;
    }

    [[nodiscard]] const assets::AssetLoader& operator*() const {
        return *_loader;
    }

    [[nodiscard]] const assets::AssetLoader* operator->() const {
        return _loader;
    }

private:
    const assets::AssetLoader* _loader{};
};

}  // namespace smgpc::game::compat
