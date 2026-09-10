#include <aurora/exception.hpp>
#include "compat/ResourceHolderCompat.hpp"
#include "resource/BasResource.hpp"
#include "resource/BtiTextureData.hpp"
#include "camera/CameraAnimation.hpp"

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

        enum class BackingKind { Raw, Animation, Model, Map, Bas, Texture, CameraAnimation };
        BackingKind backing_kind(std::string_view name) {
            // Same ordered, case-sensitive substring predicates as original
            // createAndRegisterObject. This selects storage, never table names.
            for (const auto ext : {".btp", ".bpk", ".btk", ".brk", ".blk", ".bck", ".bca"})
                if (name.find(ext) != name.npos) return BackingKind::Animation;
            if (name.find(".bas") != name.npos) return BackingKind::Bas;
            if (name.find(".bmt") != name.npos) return BackingKind::Model;
            if (name.find(".bva") != name.npos) return BackingKind::Animation;
            if (name.find(".banmt") != name.npos) return BackingKind::Map;
            if (name.find(".bdl") != name.npos || name.find(".bmd") != name.npos) return BackingKind::Model;
            if (name.ends_with(".bti")) return BackingKind::Texture;
            if (name.ends_with(".canm")) return BackingKind::CameraAnimation;
            return BackingKind::Raw;
        }
    }

    struct ResourceArchiveOwner::Storage {
        std::shared_ptr<JkrAllocationDomain> domain;
        std::shared_ptr<const resource::RarcArchive> source;
        std::filesystem::path path;
        std::unique_ptr<JKRMemArchive> archive;
        std::vector<resource::BasResource> bas_resources;
        std::vector<resource::BtiTextureData> textures;
        std::vector<camera::NativeCameraAnimationData> camera_animations;
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
        if (!source || !domain || !mem1) aurora::throw_host_exception<std::invalid_argument>("ResourceHolder requires retained archive and heap owners");
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
                state.map_aliases.push_back(resource::register_jmap_source(bytes, state.source));
                break;
            case BackingKind::Bas:
                if (!bytes.empty()) state.bas_resources.emplace_back(bytes, state.source);
                break;
            case BackingKind::Texture:
                if (bytes.empty()) break;
                state.textures.emplace_back(bytes, mem1);
                // Publish through the archive's original retained-file cache
                // before ResourceHolder enumerates it. All lookup routes then
                // share the same native record and unchanged resource size.
                state.archive->mFiles[entry.file_entry_index].mFileData =
                    const_cast<ResTIMG*>(state.textures.back().image());
                break;
            case BackingKind::CameraAnimation:
                if (bytes.empty()) break;
                state.camera_animations.push_back(camera::CameraAnimation::from_bytes(bytes).native_data());
                // Original CameraAnim borrows native scalar records directly.
                // Publish once through the archive so every original lookup,
                // including ActorCameraUtil and loadResourceFromArc, shares it.
                state.archive->mFiles[entry.file_entry_index].mFileData =
                    const_cast<std::uint8_t*>(state.camera_animations.back().bytes().data());
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
        if (!_domain || !_mem1) aurora::throw_host_exception<std::invalid_argument>("ResourceHolder service requires explicit heap owners");
        if (active_service != nullptr) aurora::throw_host_exception<std::logic_error>("Only one ResourceHolder service may be active");
        active_service = this;
    }

    ResourceHolderService::~ResourceHolderService() {
        JkrHostAllocationScope host;
        if (active_service == this) active_service = nullptr;
        _holders.clear();
    }

    ResourceHolder* ResourceHolderService::create_and_add(std::string_view archive_name) {
        return create_and_add(archive_name, nullptr);
    }

    ResourceHolder* ResourceHolderService::create_and_add(std::string_view archive_name, JKRHeap* heap) {
        JkrHostAllocationScope host;
        const auto requested = normalize_archive_request(archive_name);
        if (requested.empty() || requested == "." || requested.filename().empty())
            aurora::throw_host_exception<std::invalid_argument>("ResourceHolder requires an exact archive name");
        const auto resolved = _dvd->find_first({std::filesystem::path("ObjectData") / requested,
                                               std::filesystem::path("MapPartsData") / requested, requested});
        if (!resolved) aurora::throw_host_exception<std::runtime_error>("Required ResourceHolder archive is unavailable: " + requested.generic_string());
        const auto key = _dvd->resolve(resolved->generic_string());
        if (const auto found = _holders.find(key); found != _holders.end()) return &found->second->holder();
        auto domain = _domain;
        if (heap != nullptr && heap != &domain->heap()) {
            auto process = current_jkr_allocation_domain();
            if (!process)
                aurora::throw_host_exception<std::logic_error>("Original resource heap has no retained process owner");
            domain = JkrAllocationDomain::retain_heap(std::move(process), *heap);
        }
        auto owner = std::make_shared<ResourceArchiveOwner>(_dvd->retain_archive_for_path(*resolved), key, std::move(domain), _mem1);
        auto* result = &owner->holder();
        _holders.emplace(key, std::move(owner));
        return result;
    }

    void ResourceHolderService::remove_for_heap(JKRHeap* heap) {
        JkrHostAllocationScope host;
        if (heap == nullptr) return;
        // A player archive cannot be reset while an actual model still borrows
        // its native backing. Check the whole operation before removing any.
        for (const auto& [path, owner] : _holders)
            if (owner->holder().mHeap == heap && owner.use_count() != 1)
                aurora::throw_host_exception<std::logic_error>("Cannot unload an original resource heap with live model owners");
        std::erase_if(_holders, [heap](const auto& entry) { return entry.second->holder().mHeap == heap; });
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
        aurora::throw_host_exception<std::invalid_argument>("ResourceHolder is not owned by this service");
    }
    const ResourceArchiveOwner& ResourceHolderService::backing(const ResourceHolder& holder) const { return *retain(holder); }
    const std::shared_ptr<JkrAllocationDomain>& ResourceHolderService::allocation_domain() const noexcept { return _domain; }
    ResourceHolderService* ResourceHolderService::active() noexcept { return active_service; }
}
