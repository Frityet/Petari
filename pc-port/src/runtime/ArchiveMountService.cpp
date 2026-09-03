#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/JMapResource.hpp"
#include "resource/RarcArchive.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include <atomic>
#include <stdexcept>

namespace smgpc::runtime {
    namespace { std::atomic<ArchiveMountService*> active_service; }

    struct MountedArchive::Storage {
        std::shared_ptr<const resource::RarcArchive> source;
        std::filesystem::path path;
        JKRHeap* heap;
        std::vector<resource::JMapSourceRegistration> registrations;
        std::unique_ptr<JKRMemArchive> archive;
    };

    MountedArchive::MountedArchive(std::shared_ptr<const resource::RarcArchive> source,
                                   std::filesystem::path path, JKRHeap* heap) {
        compat::JkrHostAllocationScope host;
        _storage = std::make_unique<Storage>();
        _storage->source = std::move(source);
        _storage->path = std::move(path);
        _storage->heap = heap;
        for (const auto& entry : _storage->source->entries()) {
            const auto bytes = _storage->source->file_data(entry);
            if (!bytes.empty())
                _storage->registrations.push_back(resource::register_jmap_source(bytes, _storage->source));
        }
        _storage->archive = std::make_unique<JKRMemArchive>(*_storage->source);
    }
    MountedArchive::~MountedArchive() {
        compat::JkrHostAllocationScope host;
        _storage.reset();
    }
    JKRMemArchive& MountedArchive::archive() const noexcept { return *_storage->archive; }
    JKRHeap* MountedArchive::heap() const noexcept { return _storage->heap; }
    const resource::RarcArchive& MountedArchive::source() const noexcept { return *_storage->source; }
    const std::filesystem::path& MountedArchive::path() const noexcept { return _storage->path; }

    ArchiveMountService::ArchiveMountService(DvdFileSystemService& dvd) : _dvd(&dvd) {
        ArchiveMountService* expected = nullptr;
        if (!active_service.compare_exchange_strong(expected, this))
            throw std::logic_error("An archive mount owner is already installed");
    }
    ArchiveMountService::~ArchiveMountService() {
        compat::JkrHostAllocationScope host;
        active_service.store(nullptr);
        _resource_copies.clear();
        _mounts.clear();
    }
    ArchiveMountService* ArchiveMountService::active() noexcept { return active_service.load(); }
    DvdFileSystemService& ArchiveMountService::dvd() const noexcept { return *_dvd; }
    std::filesystem::path ArchiveMountService::key(std::string_view path) const {
        return _dvd->resolve(_dvd->normalize_disc_path_string(path)).lexically_normal();
    }
    JKRMemArchive* ArchiveMountService::mount(std::string_view path, JKRHeap* heap) {
        compat::JkrHostAllocationScope host;
        const auto resolved = key(path);
        const std::lock_guard lock(_mutex);
        if (const auto found = _mounts.find(resolved); found != _mounts.end()) return &found->second->archive();
        auto mounted = std::shared_ptr<MountedArchive>(new MountedArchive(_dvd->retain_archive_for_path(resolved), resolved, heap));
        auto* result = &mounted->archive();
        _mounts.emplace(resolved, std::move(mounted));
        return result;
    }
    std::shared_ptr<const MountedArchive> ArchiveMountService::retain(std::string_view path) const {
        compat::JkrHostAllocationScope host;
        const auto resolved = key(path);
        const std::lock_guard lock(_mutex);
        const auto found = _mounts.find(resolved);
        return found == _mounts.end() ? nullptr : found->second;
    }
    JKRMemArchive* ArchiveMountService::receive(std::string_view path) const {
        const auto mounted = retain(path);
        return mounted ? &mounted->archive() : nullptr;
    }
    std::size_t ArchiveMountService::size() const {
        const std::lock_guard lock(_mutex);
        return _mounts.size();
    }
    void ArchiveMountService::remove_for_heap(JKRHeap* heap) {
        compat::JkrHostAllocationScope host;
        const std::lock_guard lock(_mutex);
        for (auto it = _mounts.begin(); it != _mounts.end();) {
            if (it->second->heap() == heap) it = _mounts.erase(it);
            else ++it;
        }
    }
    void* ArchiveMountService::copy_archive_resource(JKRArchive& archive, const char* path) {
        compat::JkrHostAllocationScope host;
        const auto* data = static_cast<const u8*>(archive.getResource(path));
        const auto size = archive.getResSize(data);
        if (!data || !size) return nullptr;
        const std::lock_guard lock(_mutex);
        auto& bytes = _resource_copies[{&archive, path}];
        bytes.assign(data, data + size);
        return bytes.data();
    }
}
