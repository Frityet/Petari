#include "Game/Util/LayoutUtil.hpp"

#include "Game/Screen/IconAButton.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "AssetLoader.hpp"
#include "compat/DecompIntegration.hpp"
#include "compat/LayoutTextureCompat.hpp"
#include "compat/RuntimeAssetLoader.hpp"
#include "compat/RuntimeContext.hpp"
#include "layout/Bmg.hpp"
#include "layout/LayoutArchiveLoader.hpp"
#include "layout/Tpl.hpp"

#include <array>
#include <cstdarg>
#include <cwchar>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

// SMGPC_INTEGRATION_BEGIN
SMGPC_STUB(src/Game/Effect/MultiEmitter.cpp);
// SMGPC_INTEGRATION_END

struct GameMessageCache {
    const void *asset_loader_identity {};
    bool loaded {};
    smgpc::assets::layout::BmgMessageMap messages {};
    std::unordered_map< std::string, std::wstring > wide_messages {};
};

struct TextTemplateKey {
    LayoutActor *actor {};
    std::string pane {};

    [[nodiscard]] bool operator==(const TextTemplateKey &other) const {
        return actor == other.actor && pane == other.pane;
    }
};

struct TextTemplateKeyHash {
    [[nodiscard]] std::size_t operator()(const TextTemplateKey &key) const {
        return std::hash< LayoutActor * > {}(key.actor) ^ (std::hash< std::string > {}(key.pane) << 1U);
    }
};

struct TextTemplateState {
    std::u16string text {};
    std::array< std::u16string, 10 > arguments {};
    std::array< bool, 10 > has_argument {};
};

std::unordered_map< TextTemplateKey, TextTemplateState, TextTemplateKeyHash > sTextTemplates {};

[[nodiscard]] std::u16string to_utf16_decimal(s32 number) {
    const auto text = std::to_string(number);
    std::u16string result {};
    result.reserve(text.size());
    for (const char ch : text) {
        result.push_back(static_cast<char16_t>(static_cast<unsigned char>(ch)));
    }
    return result;
}

[[nodiscard]] std::u16string to_utf16(const wchar_t *pText) {
    std::u16string result {};
    if (pText == nullptr) {
        return result;
    }

    for (const wchar_t *cursor = pText; *cursor != L'\0'; ++cursor) {
        const auto codepoint = static_cast<unsigned long>(*cursor);
        if (codepoint <= 0xFFFFUL) {
            result.push_back(static_cast<char16_t>(codepoint));
        } else {
            result.push_back(u'?');
        }
    }
    return result;
}

[[nodiscard]] std::u16string to_utf16(const u16 *pText) {
    std::u16string result {};
    if (pText == nullptr) {
        return result;
    }

    for (const u16 *cursor = pText; *cursor != 0U; ++cursor) {
        result.push_back(static_cast<char16_t>(*cursor));
    }
    return result;
}

[[nodiscard]] std::wstring to_wstring(std::u16string_view text) {
    std::wstring result {};
    result.reserve(text.size());
    for (const char16_t code_unit : text) {
        result.push_back(static_cast< wchar_t >(code_unit));
    }
    return result;
}

[[nodiscard]] std::u16string to_utf16_ascii(const char *pText) {
    std::u16string result {};
    if (pText == nullptr) {
        return result;
    }

    for (const char *cursor = pText; *cursor != '\0'; ++cursor) {
        result.push_back(static_cast<char16_t>(static_cast<unsigned char>(*cursor)));
    }
    return result;
}

[[nodiscard]] TextTemplateKey make_text_template_key(LayoutActor *pActor, const char *pPaneName) {
    return TextTemplateKey {
        .actor = pActor,
        .pane = pPaneName == nullptr ? std::string {} : std::string(pPaneName),
    };
}

[[nodiscard]] std::u16string render_text_template(const TextTemplateState &state) {
    std::u16string rendered {};
    rendered.reserve(state.text.size());

    for (std::size_t i = 0U; i < state.text.size();) {
        if (state.text[i] == u'{') {
            std::size_t cursor = i + 1U;
            std::size_t index = 0U;
            bool has_digit = false;
            while (cursor < state.text.size() && state.text[cursor] >= u'0' && state.text[cursor] <= u'9') {
                has_digit = true;
                index = index * 10U + static_cast< std::size_t >(state.text[cursor] - u'0');
                ++cursor;
            }
            if (has_digit && cursor < state.text.size() && state.text[cursor] == u'}' && index < state.arguments.size()) {
                if (state.has_argument[index]) {
                    rendered += state.arguments[index];
                }
                i = cursor + 1U;
                continue;
            }
        }

        rendered.push_back(state.text[i]);
        ++i;
    }

    return rendered;
}

void erase_text_template(LayoutActor *pActor, const char *pPaneName) {
    sTextTemplates.erase(make_text_template_key(pActor, pPaneName));
}

void set_text_template(LayoutActor *pActor, const char *pPaneName, const std::u16string &text) {
    auto &state = sTextTemplates[make_text_template_key(pActor, pPaneName)];
    state.text = text;
    state.arguments = {};
    state.has_argument = {};
    pActor->setTextBoxTextRecursive(pPaneName, render_text_template(state));
}

bool set_text_template_argument(LayoutActor *pActor, const char *pPaneName, std::u16string argument, s32 index) {
    if (index < 0) {
        return false;
    }

    const auto key = make_text_template_key(pActor, pPaneName);
    auto found = sTextTemplates.find(key);
    if (found == sTextTemplates.end()) {
        return false;
    }

    const auto argument_index = static_cast< std::size_t >(index);
    if (argument_index >= found->second.arguments.size()) {
        return false;
    }

    found->second.arguments[argument_index] = std::move(argument);
    found->second.has_argument[argument_index] = true;
    pActor->setTextBoxTextRecursive(pPaneName, render_text_template(found->second));
    return true;
}

[[nodiscard]] GameMessageCache *current_game_message_cache() {
    const auto &context = smgpc::game::compat::runtime_context();
    const void *asset_loader_identity = nullptr;
    if (context.asset_loader) {
        asset_loader_identity = context.asset_loader.get();
    }

    if (asset_loader_identity == nullptr) {
        return nullptr;
    }

    static GameMessageCache sCache {};
    if (sCache.asset_loader_identity != asset_loader_identity) {
        sCache = GameMessageCache {};
        sCache.asset_loader_identity = asset_loader_identity;
    }

    return &sCache;
}

[[nodiscard]] bool ensure_game_message_cache_loaded(GameMessageCache *cache) {
    if (cache == nullptr) {
        return false;
    }
    if (cache->loaded) {
        return true;
    }

    const smgpc::game::compat::RuntimeAssetLoaderScope asset_loader{};
    if (!asset_loader) {
        cache->loaded = true;
        return false;
    }

    auto parsed_messages = asset_loader->bmg_messages("/MessageData/Message.arc");
    if (!parsed_messages.has_value()) {
        cache->loaded = true;
        return false;
    }

    cache->messages = std::move(*parsed_messages);
    cache->wide_messages.clear();
    cache->loaded = true;
    return true;
}

[[nodiscard]] const std::u16string *find_game_message(std::string_view message_name) {
    if (message_name.empty()) {
        return nullptr;
    }

    auto *cache = current_game_message_cache();
    if (!ensure_game_message_cache_loaded(cache)) {
        return nullptr;
    }

    const auto found = cache->messages.find(std::string(message_name));
    if (found == cache->messages.end()) {
        return nullptr;
    }

    return &found->second;
}

[[nodiscard]] std::string layout_archive_path(const char *pArchiveName) {
    if (pArchiveName == nullptr || *pArchiveName == '\0') {
        return {};
    }

    std::string path {pArchiveName};
    if (!path.empty() && path.front() == '/') {
        return path;
    }
    if (path.find('/') == std::string::npos) {
        return "/LayoutData/" + path;
    }
    return "/" + path;
}

[[nodiscard]] std::vector<std::string> texture_entry_candidates(const char *pTextureName) {
    std::vector<std::string> candidates {};
    if (pTextureName == nullptr || *pTextureName == '\0') {
        return candidates;
    }

    std::string path {pTextureName};
    candidates.push_back(path);
    if (path.find('/') == std::string::npos) {
        candidates.push_back("timg/" + path);
    }
    return candidates;
}

}  // namespace

namespace MR {

void startAnim(LayoutActor *pActor, const char *pAnimationName, u32 layer) {
    if (pActor == nullptr || pAnimationName == nullptr) {
        return;
    }

    pActor->startAnim(pAnimationName, layer);
}

void invalidateParentAnim(LayoutActor *pActor) {
    (void)pActor;
}

bool isAnimStopped(const LayoutActor *pActor, u32 layer) {
    if (pActor == nullptr) {
        return true;
    }

    return pActor->isAnimStopped(layer);
}

void setAnimFrameAndStop(LayoutActor *pActor, f32 frame, u32 layer) {
    if (pActor == nullptr) {
        return;
    }

    pActor->setAnimFrameAndStop(frame, layer);
}

void setAnimFrame(LayoutActor *pActor, f32 frame, u32 layer) {
    if (pActor == nullptr) {
        return;
    }

    pActor->setAnimFrame(frame, layer);
}

void setAnimRate(LayoutActor *pActor, f32 rate, u32 layer) {
    if (pActor == nullptr) {
        return;
    }

    pActor->setAnimRate(rate, layer);
}

f32 getAnimFrame(const LayoutActor *pActor, u32 layer) {
    if (pActor == nullptr) {
        return 0.0F;
    }

    return pActor->getAnimFrame(layer);
}

f32 getAnimRate(const LayoutActor *pActor, u32 layer) {
    if (pActor == nullptr) {
        return 0.0F;
    }

    return pActor->getAnimRate(layer);
}

f32 getAnimFrameMax(const LayoutActor *pActor, const char *pAnimationName) {
    if (pActor == nullptr) {
        return 0.0F;
    }

    return pActor->getAnimFrameMax(pAnimationName);
}

J3DFrameCtrl *getAnimCtrl(const LayoutActor *pActor, u32 layer) {
    static J3DFrameCtrl sFrameCtrl {};
    sFrameCtrl.mFrame = getAnimFrame(pActor, layer);
    sFrameCtrl.mEnd = pActor != nullptr ? pActor->getAnimFrameMax(layer) : 0.0F;
    sFrameCtrl.mRate = getAnimRate(pActor, layer);
    return &sFrameCtrl;
}

J3DFrameCtrl *getPaneAnimCtrl(const LayoutActor *pActor, const char *pPaneName, u32 slot) {
    static J3DFrameCtrl sFrameCtrl {};
    sFrameCtrl.mFrame = getPaneAnimFrame(pActor, pPaneName, slot);
    sFrameCtrl.mEnd = pActor != nullptr ? pActor->getAnimFrameMax(slot) : 0.0F;
    sFrameCtrl.mRate = getAnimRate(pActor, slot);
    return &sFrameCtrl;
}

void emitEffect(LayoutActor *pActor, const char *pEffectName) {
    if (pActor == nullptr) {
        return;
    }

    pActor->emitEffect(pEffectName);
}

void deleteEffectAll(LayoutActor *pActor) {
    if (pActor == nullptr) {
        return;
    }

    pActor->deleteEffectAll();
}

void setLayoutScalePosAtPaneScaleTransIfExecCalcAnim(LayoutActor *pActor, const LayoutActor *pParent, const char *pPaneName) {
    setLayoutScalePosAtPaneScaleTrans(pActor, pParent, pPaneName);
}

void setLayoutPosAtPaneTrans(LayoutActor *pActor, const LayoutActor *pParent, const char *pPaneName) {
    setLayoutScalePosAtPaneScaleTrans(pActor, pParent, pPaneName);
}

void showLayout(LayoutActor *pActor) {
    if (pActor != nullptr) {
        pActor->appear();
    }
}

void hideLayout(LayoutActor *pActor) {
    if (pActor != nullptr) {
        pActor->kill();
    }
}

void showPane(LayoutActor *pActor, const char *pPaneName) {
    if (pActor == nullptr) {
        return;
    }

    pActor->setPaneVisible(pPaneName, true);
}

void hidePane(LayoutActor *pActor, const char *pPaneName) {
    if (pActor == nullptr) {
        return;
    }

    pActor->setPaneVisible(pPaneName, false);
}

void showPaneRecursive(LayoutActor *pActor, const char *pPaneName) {
    if (pActor == nullptr) {
        return;
    }

    pActor->setPaneVisibleRecursive(pPaneName, true);
}

void hidePaneRecursive(LayoutActor *pActor, const char *pPaneName) {
    if (pActor == nullptr) {
        return;
    }

    pActor->setPaneVisibleRecursive(pPaneName, false);
}

void setFollowPos(const TVec2f *pFollowPos, const LayoutActor *pActor, const char *pPaneName) {
    if (pActor != nullptr) {
        pActor->setPaneFollowPos(pPaneName, pFollowPos);
    }
}

void copyPaneTrans(TVec2f *pOut, const LayoutActor *pActor, const char *pPaneName) {
    if (pOut == nullptr) {
        return;
    }

    if (pActor != nullptr && pPaneName != nullptr && pActor->getPaneTrans(pPaneName, pOut)) {
        return;
    }

    if (pActor != nullptr) {
        *pOut = pActor->getTrans();
        return;
    }

    pOut->set(0.0F, 0.0F);
}

void setLayoutScalePosAtPaneScaleTrans(LayoutActor *pActor, const LayoutActor *pParent, const char *pPaneName) {
    if (pActor == nullptr || pParent == nullptr || pPaneName == nullptr) {
        return;
    }

    TVec2f pane_position {};
    if (pParent->getPaneTrans(pPaneName, &pane_position)) {
        pActor->setTrans(pane_position);
    }
}

void setTextBoxGameMessageRecursive(LayoutActor *pActor, const char *pPaneName, const char *pMessageName) {
    if (pActor == nullptr) {
        return;
    }
    if (pMessageName == nullptr) {
        erase_text_template(pActor, pPaneName);
        pActor->setTextBoxTextRecursive(pPaneName, {});
        return;
    }

    const auto *message = find_game_message(pMessageName);
    if (message != nullptr) {
        set_text_template(pActor, pPaneName, *message);
        return;
    }

    erase_text_template(pActor, pPaneName);
    pActor->setTextBoxTextRecursive(pPaneName, to_utf16_ascii(pMessageName));
}

void setTextBoxLayoutMessageRecursive(LayoutActor *pActor, const char *pPaneName, const char *pMessageName) {
    setTextBoxGameMessageRecursive(pActor, pPaneName, pMessageName);
}

void clearTextBoxMessageRecursive(LayoutActor *pActor, const char *pPaneName) {
    if (pActor == nullptr) {
        return;
    }

    erase_text_template(pActor, pPaneName);
    pActor->clearTextBoxTextRecursive(pPaneName);
}

void setTextBoxNumberRecursive(LayoutActor *pActor, const char *pPaneName, s32 number) {
    if (pActor == nullptr) {
        return;
    }

    erase_text_template(pActor, pPaneName);
    pActor->setTextBoxTextRecursive(pPaneName, to_utf16_decimal(number));
}

void setTextBoxMessageRecursive(LayoutActor *pActor, const char *pPaneName, const wchar_t *pMessage) {
    if (pActor == nullptr) {
        return;
    }

    erase_text_template(pActor, pPaneName);
    pActor->setTextBoxTextRecursive(pPaneName, to_utf16(pMessage));
}

void setTextBoxMessageRecursive(LayoutActor *pActor, const char *pPaneName, const u16 *pMessage) {
    if (pActor == nullptr) {
        return;
    }

    erase_text_template(pActor, pPaneName);
    pActor->setTextBoxTextRecursive(pPaneName, to_utf16(pMessage));
}

void setTextBoxFormatRecursive(LayoutActor *pActor, const char *pPaneName, const wchar_t *pFormat, ...) {
    if (pActor == nullptr || pFormat == nullptr) {
        return;
    }

    wchar_t buffer[256]{};
    va_list args;
    va_start(args, pFormat);
    const int result = std::vswprintf(buffer, std::size(buffer), pFormat, args);
    va_end(args);

    if (result < 0) {
        clearTextBoxMessageRecursive(pActor, pPaneName);
        return;
    }

    setTextBoxMessageRecursive(pActor, pPaneName, buffer);
}

void setTextBoxArgNumberRecursive(LayoutActor *pActor, const char *pPaneName, s32 number, s32 index) {
    if (pActor == nullptr) {
        return;
    }

    if (!set_text_template_argument(pActor, pPaneName, to_utf16_decimal(number), index)) {
        pActor->setTextBoxTextRecursive(pPaneName, to_utf16_decimal(number));
    }
}

void setTextBoxArgStringRecursive(LayoutActor *pActor, const char *pPaneName, const wchar_t *pMessage, s32 index) {
    if (pActor == nullptr) {
        return;
    }

    if (!set_text_template_argument(pActor, pPaneName, to_utf16(pMessage), index)) {
        setTextBoxMessageRecursive(pActor, pPaneName, pMessage);
    }
}

const wchar_t *getGameMessageDirect(const char *pMessageName) {
    if (pMessageName == nullptr) {
        return nullptr;
    }

    auto *cache = current_game_message_cache();
    if (!ensure_game_message_cache_loaded(cache)) {
        return nullptr;
    }

    const auto found_message = cache->messages.find(pMessageName);
    if (found_message == cache->messages.end()) {
        return nullptr;
    }

    const auto found_wide = cache->wide_messages.find(pMessageName);
    if (found_wide != cache->wide_messages.end()) {
        return found_wide->second.c_str();
    }

    const auto inserted = cache->wide_messages.emplace(pMessageName, to_wstring(found_message->second));
    return inserted.first->second.c_str();
}

bool isExistPaneCtrl(const LayoutActor *pActor, const char *pPaneName) {
    if (pActor == nullptr) {
        return false;
    }
    return pActor->isExistPaneCtrl(pPaneName);
}

void createAndAddPaneCtrl(LayoutActor *pActor, const char *pPaneName, u32 slotCount) {
    (void)pActor;
    (void)pPaneName;
    (void)slotCount;
}

void startPaneAnim(LayoutActor *pActor, const char *pPaneName, const char *pAnimName, u32 slot) {
    if (pActor != nullptr) {
        pActor->startPaneAnim(pPaneName, pAnimName, slot);
    }
}

bool isPaneAnimStopped(const LayoutActor *pActor, const char *pPaneName, u32 slot) {
    if (pActor == nullptr) {
        return true;
    }
    return pActor->isPaneAnimStopped(pPaneName, slot);
}

void setPaneAnimFrameAndStop(LayoutActor *pActor, const char *pPaneName, f32 frame, u32 slot) {
    if (pActor != nullptr) {
        pActor->setPaneAnimFrame(pPaneName, frame, slot);
        pActor->setPaneAnimRate(pPaneName, 0.0F, slot);
    }
}

void setPaneAnimFrame(LayoutActor *pActor, const char *pPaneName, f32 frame, u32 slot) {
    if (pActor != nullptr) {
        pActor->setPaneAnimFrame(pPaneName, frame, slot);
    }
}

f32 getPaneAnimFrame(const LayoutActor *pActor, const char *pPaneName, u32 slot) {
    if (pActor == nullptr) {
        return 0.0F;
    }
    return pActor->getPaneAnimFrame(pPaneName, slot);
}

void setPaneAnimRate(LayoutActor *pActor, const char *pPaneName, f32 rate, u32 slot) {
    if (pActor != nullptr) {
        pActor->setPaneAnimRate(pPaneName, rate, slot);
    }
}

nw4r::lyt::TexMap *createLytTexMap(const char *pArchiveName, const char *pTextureName) {
    const smgpc::game::compat::RuntimeAssetLoaderScope asset_loader{};
    if (!asset_loader) {
        return nullptr;
    }

    auto decoded = asset_loader->first_tpl_image(layout_archive_path(pArchiveName), texture_entry_candidates(pTextureName));
    if (decoded.has_value()) {
        return new nw4r::lyt::TexMap(std::move(*decoded));
    }

    return nullptr;
}

nw4r::lyt::TexMap *createLytTexMap(ResTIMG *pImage) {
    if (pImage == nullptr) {
        return nullptr;
    }
    return new nw4r::lyt::TexMap(pImage);
}

nw4r::lyt::TexMap *getLytTexMap(LayoutActor *pActor, const char *pPaneName, u8 slot) {
    if (pActor == nullptr) {
        return nullptr;
    }
    return pActor->getPaneTexture(pPaneName, slot);
}

void replacePaneTexture(LayoutActor *pActor, const char *pPaneName, const nw4r::lyt::TexMap *pTexMap, u8 slot) {
    if (pActor != nullptr) {
        pActor->replacePaneTexture(pPaneName, pTexMap, slot);
    }
}

IconAButton *createAndSetupIconAButton(LayoutActor *pParent, bool connectToScene, bool connectToPause) {
    auto *button = new IconAButton(connectToScene, connectToPause);
    button->initWithoutIter();
    if (pParent != nullptr) {
        button->setFollowActorPane(pParent, "AButtonPosition");
    }
    return button;
}

bool isDead(const LayoutActor *pActor) {
    if (pActor == nullptr) {
        return true;
    }

    return pActor->isDead();
}

}  // namespace MR
