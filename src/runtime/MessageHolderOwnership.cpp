#include "runtime/MessageHolderOwnership.hpp"
#include "Game/System/ErrorArchive.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/LanguageOwnership.hpp"
#include "resource/NativeBmgResource.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include <aurora/exception.hpp>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace smgpc::runtime {
    namespace {
        MessageHolder *active_holder = nullptr;
    }

    struct MessageHolderOwnership::Storage {
        std::unique_ptr<compat::LanguageOwnership> language;
        std::shared_ptr<compat::JkrAllocationDomain> domain;
        ArchiveMountService *mounts = nullptr;
        std::vector<std::shared_ptr<const MountedArchive>> archives;
        std::unique_ptr<MessageHolder> holder;

        ~Storage() {
            auto* original = holder.release();
            destroy_message_holder(original);
            if (mounts && domain)
                mounts->remove_for_heap(&domain->heap());
            // JMapInfo disposal precedes its borrowed archive data and the heap.
            archives.clear();
            domain.reset();
        }
    };

    MessageHolderOwnership::MessageHolderOwnership(
        std::shared_ptr<compat::JkrHeapRuntime> runtime, std::size_t byte_budget,
        ArchiveMountService &mounts, std::string_view game_archive_path,
        std::string_view language_prefix) {
        compat::JkrHostAllocationScope host;
        if (!runtime || ArchiveMountService::active() != &mounts)
            aurora::throw_host_exception<std::invalid_argument>("Original messages require the active archive service and real heap runtime");
        if (active_holder || SingletonHolder<GameSystem>::get())
            aurora::throw_host_exception<std::logic_error>("A standalone MessageHolder cannot coexist with another message or GameSystem owner");

        auto storage = std::make_unique<Storage>();
        storage->language = std::make_unique<compat::LanguageOwnership>(language_prefix);
        storage->domain = compat::JkrAllocationDomain::create(std::move(runtime), byte_budget);
        storage->mounts = &mounts;
        auto *embedded = mounts.mount_memory("ErrorMessageArchive.arc", cErrorArchive, &storage->domain->heap());
        // Original JKR path lookup case-folds archive components. RARC names in
        // this embedded resource are lowercase; basename lookup loses the locale.
        const auto system_path = "/" + std::string(language_prefix) + "/MessageData/System.arc";
        const auto *system_data = static_cast<const std::uint8_t *>(embedded->getResource(system_path.c_str()));
        if (!system_data)
            aurora::throw_host_exception<std::runtime_error>("Embedded message archive has no requested language: " + system_path);
        const auto system_bytes = std::span<const std::uint8_t>(system_data, embedded->getResSize(system_data));
        const auto &game_archive = mounts.dvd().archive_for_path(std::string(game_archive_path));
        (void)mounts.mount_memory("/Memory/SystemMessage.arc", system_bytes, &storage->domain->heap());
        (void)mounts.mount_memory("/MessageData/Message.arc", game_archive.bytes(), &storage->domain->heap());
        storage->archives.push_back(mounts.retain("/Memory/SystemMessage.arc"));
        storage->archives.push_back(mounts.retain("/MessageData/Message.arc"));
        storage->archives.push_back(mounts.retain("ErrorMessageArchive.arc"));
        for (const auto &archive : storage->archives) {
            if (!archive || archive->heap() != &storage->domain->heap())
                aurora::throw_host_exception<std::logic_error>("Original message archive identity is already owned by another heap");
        }
        {
            compat::JkrAllocationScope heap(storage->domain);
            storage->holder = std::make_unique<MessageHolder>();
            storage->holder->initSystemData();
            storage->holder->initGameData();
        }
        _storage = std::move(storage);
        active_holder = _storage->holder.get();
    }

    MessageHolderOwnership::~MessageHolderOwnership() {
        compat::JkrHostAllocationScope host;
        if (active_holder == _storage->holder.get())
            active_holder = nullptr;
        _storage.reset();
    }
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
    MessageHolder &MessageHolderOwnership::holder() const noexcept {
        return *_storage->holder;
    }
    MessageHolder *current_message_holder() noexcept {
        if (auto* system = SingletonHolder<GameSystem>::get())
            return system->mObjHolder ? system->mObjHolder->mMessageHolder : nullptr;
        return active_holder;
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
        if (active_holder == holder) active_holder = nullptr;
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
