#include "Game/Util/MessageUtil.hpp"

#include "compat/MessageUtilCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <string>
#include <string_view>

namespace {
    [[nodiscard]] const wchar_t* resolve_raw_message(const char* message_id) {
        if (message_id == nullptr) return nullptr;
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) return nullptr;
        const auto* message = runtime->messages().message_raw_wide(message_id);
        return message != nullptr ? message->c_str() : nullptr;
    }
} // namespace

namespace smgpc::compat {
    const char* layout_message_id_for_pointer(const wchar_t* message) noexcept {
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr ? runtime->messages().message_id_for_wide_pointer(message) : nullptr;
    }
} // namespace smgpc::compat

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
