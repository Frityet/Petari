#include "Game/Util/MessageUtil.hpp"

#include "compat/MessageUtilCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <string>
#include <string_view>

namespace {
    thread_local auto sLayoutMessage = std::wstring{};
    thread_local auto sLayoutMessageId = std::string{};

    [[nodiscard]] const wchar_t* resolve_raw_message(const char* message_id) {
        if (message_id == nullptr) {
            return nullptr;
        }

        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            return nullptr;
        }

        const auto* utf16 = runtime->messages().message_raw_utf16(std::string_view(message_id));
        if (utf16 == nullptr) {
            return nullptr;
        }

        thread_local auto message = std::wstring{};
        message.clear();
        message.reserve(utf16->size());
        for (const auto code : *utf16) {
            message.push_back(static_cast<wchar_t>(code));
        }
        return message.c_str();
    }

    [[nodiscard]] const wchar_t* resolve_layout_message(const char* message_id) {
        sLayoutMessage.clear();
        sLayoutMessageId.clear();
        if (message_id == nullptr) {
            return nullptr;
        }

        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            return nullptr;
        }

        const auto* utf16 =
            runtime->messages().message_raw_utf16(std::string_view(message_id));
        if (utf16 == nullptr) {
            return nullptr;
        }

        sLayoutMessage.reserve(utf16->size());
        for (const auto code : *utf16) {
            sLayoutMessage.push_back(static_cast<wchar_t>(code));
        }
        sLayoutMessageId = message_id;
        return sLayoutMessage.c_str();
    }
}  // namespace

namespace smgpc::compat {

    const char*
    layout_message_id_for_pointer(const wchar_t* message) noexcept {
        if (message == nullptr || sLayoutMessageId.empty() ||
            message != sLayoutMessage.c_str()) {
            return nullptr;
        }
        return sLayoutMessageId.c_str();
    }

}  // namespace smgpc::compat

namespace MR {
    const u16* getGameMessageDirectUtf16(const char* message_id) {
        if (message_id == nullptr) {
            return nullptr;
        }

        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            return nullptr;
        }

        const auto* message = runtime->messages().message_raw_utf16(std::string_view(message_id));
        return message != nullptr ? reinterpret_cast< const u16* >(message->c_str()) : nullptr;
    }

    const wchar_t* getSystemMessageDirect(const char* message_id) {
        return resolve_raw_message(message_id);
    }

    const wchar_t* getGameMessageDirect(const char* message_id) {
        return resolve_raw_message(message_id);
    }

    const wchar_t* getLayoutMessageDirect(const char* message_id) {
        return resolve_layout_message(message_id);
    }

    bool isExistGameMessage(const char* message_id) {
        if (message_id == nullptr) {
            return false;
        }

        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr && runtime->messages().message_raw_utf16(std::string_view(message_id)) != nullptr;
    }
}  // namespace MR
