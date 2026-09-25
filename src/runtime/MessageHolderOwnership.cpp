#include "runtime/MessageHolderOwnership.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/NativeBmgResource.hpp"
#include <aurora/exception.hpp>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace smgpc::runtime {
    SceneMessageBinding::SceneMessageBinding(MessageHolder &holder)
        : _holder(holder), _previous(holder.mSceneMessageData) {
        if (!holder.mGameMessageData)
            aurora::throw_host_exception<std::logic_error>("Scene messages require initialized original game message data");
        _holder.initSceneData();
    }
    SceneMessageBinding::~SceneMessageBinding() {
        _holder.destroySceneData();
        _holder.mSceneMessageData = _previous;
    }
    MessageHolder *current_message_holder() noexcept {
        if (auto* system = SingletonHolder<GameSystem>::get())
            return system->mObjHolder ? system->mObjHolder->mMessageHolder : nullptr;
        return nullptr;
    }
    MessageHolder &require_message_holder() {
        auto* holder = current_message_holder();
        if (!holder)
            aurora::throw_host_exception<std::logic_error>("Original message access requires an initialized MessageHolder owner");
        return *holder;
    }
    void destroy_message_holder(MessageHolder*& holder) noexcept {
        if (!holder) return;
        const compat::JkrHostAllocationScope host;
        holder->destroySceneData();
        delete std::exchange(holder->mGameMessageData, nullptr);
        delete std::exchange(holder->mSystemMessageData, nullptr);
        delete std::exchange(holder, nullptr);
    }
    const char *message_id_for_pointer(const wchar_t *pointer) noexcept {
        const auto* holder = current_message_holder();
        if (!pointer || !holder)
            return nullptr;
        for (const auto *data : {holder->mGameMessageData, holder->mSystemMessageData}) {
            if (!data)
                continue;
            const auto index = data->mNativeResource->message_index(pointer);
            if (!index)
                continue;
            for (int row = 0; row < data->mIDTable->getNumEntries(); ++row) {
                s32 row_index = -1;
                const char *id = nullptr;
                if (data->mIDTable->getValue(row, "Index", &row_index) &&
                    row_index == static_cast<s32>(*index) && data->mIDTable->getValue(row, "MessageId", &id))
                    return id;
            }
        }
        return nullptr;
    }
}  // namespace smgpc::runtime
