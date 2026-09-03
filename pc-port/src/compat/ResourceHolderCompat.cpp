#include "ResourceHolderCompat.hpp"

#include "Game/Animation/MaterialAnmBuffer.hpp"
#include "Game/System/StationedFileInfo.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.hpp"
#include "compat/J3dCommandScope.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/J3dAnimationResource.hpp"
#include "resource/J3dModelResource.hpp"
#include "resource/JMapResource.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>

namespace smgpc::compat {
    namespace {
        ResourceHolderService* active_service = nullptr;

        std::filesystem::path normalize_archive_request(std::string_view archive_name) {
            auto normalized = std::string(archive_name);
            std::ranges::replace(normalized, '\\', '/');
            while (!normalized.empty() && normalized.front() == '/') normalized.erase(normalized.begin());
            return std::filesystem::path(normalized).lexically_normal();
        }

        // Original Game manually balances this mutex around SDK loaders.
        // Host exceptions may cross that pair; restore only new acquisitions
        // by this thread, while the enclosing command scope owns the CPU gate.
        class LoadMutexRecovery final {
        public:
            LoadMutexRecovery() : _thread(OSGetCurrentThread()), _exceptions(std::uncaught_exceptions()) {
                const auto enabled = OSDisableInterrupts();
                _count = mutex().thread == _thread ? mutex().count : 0;
                OSRestoreInterrupts(enabled);
            }
            ~LoadMutexRecovery() {
                if (std::uncaught_exceptions() <= _exceptions) return;
                const auto enabled = OSDisableInterrupts();
                while (mutex().thread == _thread && mutex().count > _count) OSUnlockMutex(&mutex());
                OSRestoreInterrupts(enabled);
            }
        private:
            static OSMutex& mutex() { return MR::MutexHolder<0>::sMutex; }
            OSThread* _thread;
            int _exceptions;
            s32 _count;
        };

        enum class BackingKind { Raw, Animation, Model, Map };
        BackingKind backing_kind(std::string_view name) {
            // Same ordered, case-sensitive substring predicates as original
            // createAndRegisterObject. This selects storage, never table names.
            for (const auto ext : {".btp", ".bpk", ".btk", ".brk", ".blk", ".bck", ".bca"})
                if (name.find(ext) != name.npos) return BackingKind::Animation;
            if (name.find(".bas") != name.npos) return BackingKind::Raw;
            if (name.find(".bmt") != name.npos) return BackingKind::Model;
            if (name.find(".bva") != name.npos) return BackingKind::Animation;
            if (name.find(".banmt") != name.npos) return BackingKind::Map;
            if (name.find(".bdl") != name.npos || name.find(".bmd") != name.npos) return BackingKind::Model;
            return BackingKind::Raw;
        }
    }

    struct ResourceArchiveOwner::Storage {
        std::shared_ptr<JkrAllocationDomain> domain;
        std::shared_ptr<const resource::RarcArchive> source;
        std::filesystem::path path;
        std::unique_ptr<JKRMemArchive> archive;
        std::vector<resource::JMapResource> maps;
        std::vector<resource::JMapSourceRegistration> map_aliases;
        std::vector<resource::J3dAnimationResource> animations;
        std::vector<resource::J3dAnimationSourceRegistration> animation_aliases;
        std::vector<resource::J3dModelResource> models;
        std::vector<resource::J3dModelSourceRegistration> model_aliases;
        std::unique_ptr<ResourceHolder> holder;

        ~Storage() {
            JkrHostAllocationScope host;
            // Loaded materials can still point into MaterialAnmBuffer.
            // Destroy models before the actual animation array and holder.
            model_aliases.clear();
            models.clear();
            if (holder != nullptr) {
                JkrAllocationScope original(domain);
                if (holder->mMaterialBuf != nullptr) {
                    delete[] holder->mMaterialBuf->_0;
                    delete holder->mMaterialBuf;
                }
                delete holder->mBckCtrl;
                holder.reset();
            }
        }
    };

    ResourceArchiveOwner::ResourceArchiveOwner(std::shared_ptr<const resource::RarcArchive> source,
        std::filesystem::path path, std::shared_ptr<JkrAllocationDomain> domain,
        std::shared_ptr<resource::Mem1ResourceHeap> mem1) {
        JkrHostAllocationScope host;
        if (!source || !domain || !mem1) throw std::invalid_argument("ResourceHolder requires retained archive and heap owners");
        _storage = std::make_unique<Storage>();
        auto& state = *_storage;
        state.domain = std::move(domain);
        state.source = std::move(source);
        state.path = std::move(path);
        state.archive = std::make_unique<JKRMemArchive>(*state.source);
        for (const auto& entry : state.source->entries()) {
            const auto bytes = state.source->file_data(entry);
            switch (backing_kind(entry.name)) {
            case BackingKind::Animation:
                // JKRArchive returns null for a zero-size file. Preserve the
                // original loader's null dispatch (and BCK's explicit null
                // table entry) without registering an unrelated empty span.
                if (bytes.empty()) break;
                state.animations.emplace_back(bytes);
                state.animation_aliases.push_back(state.animations.back().register_source(bytes));
                break;
            case BackingKind::Model:
                state.models.emplace_back(bytes, state.domain, mem1);
                state.model_aliases.push_back(state.models.back().register_source(bytes));
                break;
            case BackingKind::Map:
                if (bytes.empty()) break; // Original JMapInfo::attach(nullptr).
                state.maps.emplace_back(bytes);
                state.map_aliases.push_back(state.maps.back().register_source(bytes));
                break;
            case BackingKind::Raw:
                if (!bytes.empty()) state.map_aliases.push_back(resource::register_jmap_source(bytes, state.source));
                break;
            }
        }
        JkrAllocationScope original(state.domain);
        J3dCommandScope commands;
        LoadMutexRecovery recovery;
        state.holder = std::make_unique<ResourceHolder>(*state.archive);
    }

    ResourceArchiveOwner::~ResourceArchiveOwner() {
        JkrHostAllocationScope host;
        _storage.reset();
    }
    ResourceHolder& ResourceArchiveOwner::holder() const noexcept { return *_storage->holder; }
    const resource::RarcArchive& ResourceArchiveOwner::archive() const noexcept { return *_storage->source; }
    const std::filesystem::path& ResourceArchiveOwner::resolved_path() const noexcept { return _storage->path; }

    ResourceHolderService::ResourceHolderService(runtime::DvdFileSystemService& dvd,
        std::shared_ptr<JkrAllocationDomain> domain, std::shared_ptr<resource::Mem1ResourceHeap> mem1)
        : _dvd(&dvd), _domain(std::move(domain)), _mem1(std::move(mem1)) {
        if (!_domain || !_mem1) throw std::invalid_argument("ResourceHolder service requires explicit heap owners");
        if (active_service != nullptr) throw std::logic_error("Only one ResourceHolder service may be active");
        active_service = this;
    }

    ResourceHolderService::~ResourceHolderService() {
        JkrHostAllocationScope host;
        if (active_service == this) active_service = nullptr;
        _holders.clear();
    }

    ResourceHolder* ResourceHolderService::create_and_add(std::string_view archive_name) {
        JkrHostAllocationScope host;
        const auto requested = normalize_archive_request(archive_name);
        if (requested.empty() || requested == "." || requested.filename().empty())
            throw std::invalid_argument("ResourceHolder requires an exact archive name");
        const auto resolved = _dvd->find_first({std::filesystem::path("ObjectData") / requested,
                                               std::filesystem::path("MapPartsData") / requested, requested});
        if (!resolved) throw std::runtime_error("Required ResourceHolder archive is unavailable: " + requested.generic_string());
        const auto key = _dvd->resolve(resolved->generic_string());
        if (const auto found = _holders.find(key); found != _holders.end()) return &found->second->holder();
        auto owner = std::make_shared<ResourceArchiveOwner>(_dvd->retain_archive_for_path(*resolved), key, _domain, _mem1);
        auto* result = &owner->holder();
        _holders.emplace(key, std::move(owner));
        return result;
    }

    std::vector<ResourceHolder*> ResourceHolderService::create_and_add_stationed(std::int32_t load_type) {
        JkrHostAllocationScope host;
        std::vector<ResourceHolder*> result;
        for (auto* info = MR::getStationedFileInfoTable(); info->mArchive != nullptr; ++info)
            if (info->mLoadType == load_type) result.push_back(create_and_add(info->mArchive));
        return result;
    }

    std::shared_ptr<const ResourceArchiveOwner> ResourceHolderService::retain(const ResourceHolder& holder) const {
        for (const auto& [path, owner] : _holders) if (&owner->holder() == &holder) return owner;
        throw std::invalid_argument("ResourceHolder is not owned by this service");
    }
    const ResourceArchiveOwner& ResourceHolderService::backing(const ResourceHolder& holder) const { return *retain(holder); }
    const std::shared_ptr<JkrAllocationDomain>& ResourceHolderService::allocation_domain() const noexcept { return _domain; }
    ResourceHolderService* ResourceHolderService::active() noexcept { return active_service; }
}
