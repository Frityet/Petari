#include "Game/Util/MessageUtil.hpp"

#include "compat/MessageUtilCompat.hpp"
#include "runtime/MessageHolderOwnership.hpp"
#include "resource/NativeBmgResource.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/NPC/TalkMessageInfo.hpp"

#include <string>
#include <string_view>

namespace {
    [[nodiscard]] const wchar_t* resolve_raw_message(const char* message_id,
        bool (*lookup)(TalkMessageInfo*, const char*)) {
        if (!message_id || !smgpc::runtime::current_message_holder()) return nullptr;
        TalkMessageInfo info;
        return lookup(&info, message_id) ? reinterpret_cast<const wchar_t*>(info._0) : nullptr;
    }
} // namespace

namespace smgpc::compat {
    const char* layout_message_id_for_pointer(const wchar_t* message) noexcept {
        return smgpc::runtime::message_id_for_pointer(message);
    }
} // namespace smgpc::compat

namespace MR {
    const u16* getGameMessageDirectUtf16(const char* message_id) {
        if (message_id == nullptr) {
            return nullptr;
        }

        auto* holder = smgpc::runtime::current_message_holder();
        if (holder == nullptr) {
            return nullptr;
        }

        const auto* data = holder->mGameMessageData;
        const auto index = data->findMessageIndex(message_id);
        if (index < 0 || index >= data->mInfoBlock->mItemCount) return nullptr;
        return reinterpret_cast<const u16*>(data->mNativeResource->message_utf16(index));
    }

    const u16* getSystemMessageDirectUtf16(const char* message_id) {
        if (message_id == nullptr) return nullptr;
        auto* holder = smgpc::runtime::current_message_holder();
        if (holder == nullptr) return nullptr;
        const auto* data = holder->mSystemMessageData;
        const auto index = data->findMessageIndex(message_id);
        if (index < 0 || index >= data->mInfoBlock->mItemCount) return nullptr;
        return reinterpret_cast<const u16*>(data->mNativeResource->message_utf16(index));
    }

    const wchar_t* getSystemMessageDirect(const char* message_id) {
        return resolve_raw_message(message_id, MessageSystem::getSystemMessageDirect);
    }

    const wchar_t* getGameMessageDirect(const char* message_id) {
        return resolve_raw_message(message_id, MessageSystem::getGameMessageDirect);
    }

    const wchar_t* getLayoutMessageDirect(const char* message_id) {
        return resolve_raw_message(message_id, MessageSystem::getLayoutMessageDirect);
    }

    bool isExistGameMessage(const char* message_id) {
        if (message_id == nullptr) {
            return false;
        }

        return getGameMessageDirect(message_id) != nullptr;
    }
}  // namespace MR
