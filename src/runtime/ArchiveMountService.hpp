#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class JKRArchive;
class JKRMemArchive;
class JKRHeap;

namespace smgpc::resource { class RarcArchive; }

namespace smgpc::runtime {
    class DvdFileSystemService;

    class MountedArchive final {
    public:
        ~MountedArchive();
        MountedArchive(const MountedArchive&) = delete;
        MountedArchive& operator=(const MountedArchive&) = delete;
        [[nodiscard]] JKRMemArchive& archive() const noexcept;
        [[nodiscard]] JKRHeap* heap() const noexcept;
        [[nodiscard]] const resource::RarcArchive& source() const noexcept;
        [[nodiscard]] const std::filesystem::path& path() const noexcept;
    private:
        friend class ArchiveMountService;
        MountedArchive(std::shared_ptr<const resource::RarcArchive>, std::filesystem::path, JKRHeap*);
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

    // One explicit VFS lifetime, installed by RuntimeContext. Headless owners
    // can use the same service with an actual DvdFileSystemService.
    class ArchiveMountService final {
    public:
        explicit ArchiveMountService(DvdFileSystemService&);
        ~ArchiveMountService();
        ArchiveMountService(const ArchiveMountService&) = delete;
        ArchiveMountService& operator=(const ArchiveMountService&) = delete;
        [[nodiscard]] JKRMemArchive* mount(std::string_view, JKRHeap*);
        [[nodiscard]] JKRMemArchive* receive(std::string_view) const;
        [[nodiscard]] std::shared_ptr<const MountedArchive> retain(std::string_view) const;
        [[nodiscard]] std::size_t size() const;
        void remove_for_heap(JKRHeap*);
        [[nodiscard]] void* copy_archive_resource(JKRArchive&, const char*);
        [[nodiscard]] DvdFileSystemService& dvd() const noexcept;
        [[nodiscard]] static ArchiveMountService* active() noexcept;
    private:
        [[nodiscard]] std::filesystem::path key(std::string_view) const;
        DvdFileSystemService* _dvd;
        mutable std::mutex _mutex;
        std::map<std::filesystem::path, std::shared_ptr<MountedArchive>> _mounts;
        std::map<std::pair<const JKRArchive*, std::string>, std::vector<unsigned char>> _resource_copies;
    };
}
