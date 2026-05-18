#include "Game/Util/MessageUtil.hpp"

#include <string_view>

#include "Game/compat/RuntimeContext.hpp"
#include "Game/compat/TextEncoding.hpp"

namespace {
    [[nodiscard]] std::wstring wide_from_utf16(std::u16string_view text) {
        auto wide = std::wstring{};
        wide.reserve(text.size());
        for (const auto code : text) {
            wide.push_back(static_cast<wchar_t>(code));
        }
        return wide;
    }

    [[nodiscard]] const wchar_t* raw_message_direct(const char* pMessageId) {
        thread_local auto message = std::wstring{};
        const auto tag = pMessageId != nullptr ? std::string_view(pMessageId) : std::string_view{};
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            message = wide_from_utf16(runtime->messages().message_raw_utf16_or(tag, smgpc::game::utf16_from_utf8_lossy(tag)));
        } else {
            message = wide_from_utf16(smgpc::game::utf16_from_utf8_lossy(tag));
        }
        return message.c_str();
    }
}  // namespace

namespace MR {
    const wchar_t* getSystemMessageDirect(const char* pMessageId) {
        return raw_message_direct(pMessageId);
    }

    const wchar_t* getGameMessageDirect(const char* pMessageId) {
        return raw_message_direct(pMessageId);
    }

    const wchar_t* getLayoutMessageDirect(const char* pMessageId) {
        return raw_message_direct(pMessageId);
    }

    bool isExistGameMessage(const char* pMessageId) {
        if (pMessageId == nullptr) {
            return false;
        }
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            return runtime->messages().message(std::string_view(pMessageId)) != nullptr;
        }
        return false;
    }
}  // namespace MR
