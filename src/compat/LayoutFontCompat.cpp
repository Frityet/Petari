#include <aurora/exception.hpp>
#include "Game/Util/LayoutUtil.hpp"

#include <stdexcept>

#include "layout/LayoutHost.hpp"
#include "layout/LayoutRuntime.hpp"
#include "nw4r/ut/Font.h"

namespace MR {

    void setTextBoxFontRecursive(LayoutActor* pLayout, const char* pPaneName, nw4r::ut::Font* pFont) {
        if (pFont == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Setting a text-box font requires a real font");
        }
        smgpc::layout::require_layout_runtime(pLayout, "Setting a text-box font")
            .setTextBoxFontRecursive(pPaneName, *pFont);
    }

}  // namespace MR
