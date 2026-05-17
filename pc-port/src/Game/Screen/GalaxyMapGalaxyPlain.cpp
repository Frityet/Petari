#include "Game/Screen/GalaxyMapGalaxyPlain.hpp"

#include <string>

#include "Game/Util/LayoutUtil.hpp"

namespace {
    [[nodiscard]] std::wstring widen_ascii(const char* pText) {
        auto text = std::wstring{};
        if (pText == nullptr) {
            return text;
        }
        while (*pText != '\0') {
            text.push_back(static_cast< wchar_t >(static_cast< unsigned char >(*pText++)));
        }
        return text;
    }
}  // namespace

GalaxyMapGalaxyPlain::GalaxyMapGalaxyPlain(const LayoutActor* pHost) : LayoutActor("Galaxy情報簡易表示", true), mHost(pHost), mPaneName(nullptr) {
}

void GalaxyMapGalaxyPlain::init(const JMapInfoIter&) {
    initLayoutManager("GalaxyNamePlate", 1);
    kill();
}

void GalaxyMapGalaxyPlain::show(const char* pMessage, const char* pPaneName) {
    const auto wide = widen_ascii(pMessage);
    show(wide.c_str(), pPaneName);
}

void GalaxyMapGalaxyPlain::show(const wchar_t* pMessage, const char* pPaneName) {
    mPaneName = pPaneName;
    MR::setTextBoxMessageRecursive(this, nullptr, pMessage);
    LayoutActor::appear();
}

const char* GalaxyMapGalaxyPlain::getFollowPaneName() const {
    return mPaneName;
}
