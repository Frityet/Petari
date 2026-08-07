#include "Game/Util/MessageUtil.hpp"

#include "runtime/RuntimeContext.hpp"

#include <string>
#include <string_view>

namespace {
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
}  // namespace

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
        return resolve_raw_message(message_id);
    }

    bool isExistGameMessage(const char* message_id) {
        if (message_id == nullptr) {
            return false;
        }

        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr && runtime->messages().message_raw_utf16(std::string_view(message_id)) != nullptr;
    }
}  // namespace MR
