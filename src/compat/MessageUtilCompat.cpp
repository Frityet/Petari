#include "compat/MessageUtilCompat.hpp"
#include "runtime/MessageHolderOwnership.hpp"
#include "resource/NativeBmgResource.hpp"
#include "Game/System/MessageHolder.hpp"

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
        return data->mNativeResource->message_utf16(index);
    }

    const u16* getSystemMessageDirectUtf16(const char* message_id) {
        if (message_id == nullptr) return nullptr;
        auto* holder = smgpc::runtime::current_message_holder();
        if (holder == nullptr) return nullptr;
        const auto* data = holder->mSystemMessageData;
        const auto index = data->findMessageIndex(message_id);
        if (index < 0 || index >= data->mInfoBlock->mItemCount) return nullptr;
        return data->mNativeResource->message_utf16(index);
    }

}  // namespace MR
