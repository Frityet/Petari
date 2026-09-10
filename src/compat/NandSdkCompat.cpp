#include <revolution.h>
#include <revolution/nand.h>
#include <aurora/allocation.hpp>

#include "runtime/RuntimeContext.hpp"
#include "Game/System/NANDManager.hpp"
#include "Game/System/NANDManagerThread.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    struct OpenFile {
        smgpc::runtime::RuntimeContext* runtime;
        std::string path;
        std::vector<u8> bytes;
        std::size_t position = 0;
        u8 access;
        bool dirty = false;
    };

    std::mutex s_file_mutex;
    std::map<s32, OpenFile> s_files;
    s32 s_next_descriptor = 1;

    smgpc::runtime::RuntimeContext& runtime() {
        auto* value = smgpc::runtime::RuntimeContext::try_instance();
        if (!value) throw std::logic_error("NAND requires the active process resource owner");
        return *value;
    }

    bool valid_path(const char* path) {
        return path && path[0] != '\0' && strnlen(path, NAND_MAX_PATH) < NAND_MAX_PATH;
    }

    template<class F> s32 invoke(F&& function) {
        aurora::allocation::HostAllocationScope host;
        std::lock_guard lock(s_file_mutex);
        try {
            return function();
        } catch (const std::bad_alloc&) {
            return NAND_RESULT_ALLOC_FAILED;
        } catch (const std::invalid_argument&) {
            return NAND_RESULT_INVALID;
        } catch (const std::exception&) {
            return NAND_RESULT_UNKNOWN;
        }
    }

    OpenFile* find_file(const NANDFileInfo* info) {
        if (!info) return nullptr;
        const auto it = s_files.find(info->fileDescriptor);
        if (it == s_files.end() || it->second.runtime != smgpc::runtime::RuntimeContext::try_instance()) return nullptr;
        return &it->second;
    }

    bool is_open(std::string_view path) {
        for (const auto& [descriptor, file] : s_files) {
            if (file.runtime == &runtime() && file.path == path) return true;
        }
        return false;
    }
}

NANDManager::~NANDManager() {
    // The original worker is cancelled and joined by OSThreadWrapper before
    // its queue, stack and any process resource owner can be released.
    delete mManagerThread;
    invoke([] {
        const auto* owner = smgpc::runtime::RuntimeContext::try_instance();
        std::erase_if(s_files, [owner](const auto& entry) { return entry.second.runtime == owner; });
        return s32{NAND_RESULT_OK};
    });
}

extern "C" {
    void NANDInitBanner(NANDBanner* banner, u32 flags, const u16* title, const u16* comment) {
        static_assert(sizeof(NANDBanner) == 0xf0a0);
        std::memset(banner, 0, sizeof(*banner));
        banner->signature = NAND_BANNER_SIGNATURE;
        banner->flag = flags;
        const u16* lines[] = {title, comment};
        for (std::size_t line = 0; line < 2; ++line) {
            const auto* source = lines[line];
            if (*source == 0) {
                banner->comment[line][0] = u16{' '};
            } else {
                for (std::size_t i = 0; i < NAND_BANNER_COMMENT_SIZE && source[i] != 0; ++i) {
                    banner->comment[line][i] = source[i];
                }
            }
        }
    }

    s32 NANDInit() {
        return invoke([] { (void)runtime().save_data(); return s32{NAND_RESULT_OK}; });
    }

    s32 NANDCreate(const char* path, u8 permission, u8 attribute) {
        return invoke([&] {
            if (!valid_path(path) || (permission & ~0x3fU)) return s32{NAND_RESULT_INVALID};
            return runtime().save_data().create_nand_file(path, permission, attribute);
        });
    }

    s32 NANDOpen(const char* path, NANDFileInfo* info, u8 access) {
        return invoke([&] {
            if (!valid_path(path) || !info || access < NAND_ACCESS_READ || access > NAND_ACCESS_RW) return s32{NAND_RESULT_INVALID};
            auto& owner = runtime();
            auto& save = owner.save_data();
            const auto normalized = save.nand().normalize_path(path);
            if (normalized.size() >= NAND_MAX_PATH) return s32{NAND_RESULT_INVALID};
            if (is_open(normalized)) return s32{NAND_RESULT_OPENFD};
            const auto metadata = save.nand().metadata(path);
            if (metadata && (((access & NAND_ACCESS_READ) && !(metadata->permission & NAND_PERM_RUSR)) ||
                             ((access & NAND_ACCESS_WRITE) && !(metadata->permission & NAND_PERM_WUSR)))) return s32{NAND_RESULT_ACCESS};
            // A newly created empty file has no encoded container to translate yet.
            auto bytes = metadata && metadata->size == 0 ? std::optional<std::vector<u8>>(std::vector<u8>{}) : save.read_nand_file(path);
            if (!bytes) return s32{NAND_RESULT_NOEXISTS};
            if (s_next_descriptor == std::numeric_limits<s32>::max()) return s32{NAND_RESULT_MAXFD};
            const s32 descriptor = s_next_descriptor++;
            s_files.emplace(descriptor, OpenFile{&owner, normalized, std::move(*bytes), 0, access});
            std::memset(info, 0, sizeof(*info));
            info->fileDescriptor = descriptor;
            info->origFd = descriptor;
            info->accType = access;
            std::memcpy(info->origPath, normalized.c_str(), normalized.size() + 1);
            return s32{NAND_RESULT_OK};
        });
    }

    s32 NANDRead(NANDFileInfo* info, void* destination, u32 size) {
        return invoke([&] {
            auto* file = find_file(info);
            if (!file || (!destination && size)) return s32{NAND_RESULT_INVALID};
            if (!(file->access & NAND_ACCESS_READ)) return s32{NAND_RESULT_ACCESS};
            const auto count = std::min<std::size_t>(size, file->bytes.size() - file->position);
            if (count > static_cast<std::size_t>(std::numeric_limits<s32>::max())) return s32{NAND_RESULT_INVALID};
            if (count) std::memcpy(destination, file->bytes.data() + file->position, count);
            file->position += count;
            return static_cast<s32>(count);
        });
    }

    s32 NANDWrite(NANDFileInfo* info, const void* source, u32 size) {
        return invoke([&] {
            auto* file = find_file(info);
            if (!file || (!source && size) || size > static_cast<u32>(std::numeric_limits<s32>::max())) return s32{NAND_RESULT_INVALID};
            if (!(file->access & NAND_ACCESS_WRITE)) return s32{NAND_RESULT_ACCESS};
            const auto end = file->position + size;
            const auto added_blocks = (end + 0x3fffU) / 0x4000U - (file->bytes.size() + 0x3fffU) / 0x4000U;
            if (end > file->bytes.size()) {
                const auto capacity = runtime().save_data().nand().check(static_cast<u32>(added_blocks), 0);
                if (capacity.result != NAND_RESULT_OK) return capacity.result;
                file->bytes.resize(end);
            }
            if (size) std::memcpy(file->bytes.data() + file->position, source, size);
            file->position = end;
            file->dirty = file->dirty || size != 0;
            return static_cast<s32>(size);
        });
    }

    s32 NANDGetLength(NANDFileInfo* info, u32* length) {
        return invoke([&] {
            const auto* file = find_file(info);
            if (!file || !length || file->bytes.size() > std::numeric_limits<u32>::max()) return s32{NAND_RESULT_INVALID};
            *length = static_cast<u32>(file->bytes.size());
            return s32{NAND_RESULT_OK};
        });
    }

    s32 NANDClose(NANDFileInfo* info) {
        return invoke([&] {
            if (!find_file(info)) return s32{NAND_RESULT_INVALID};
            auto node = s_files.extract(info->fileDescriptor);
            info->fileDescriptor = -1;
            info->origFd = -1;
            auto& file = node.mapped();
            if (file.dirty) {
                auto& save = runtime().save_data();
                const auto metadata = save.nand().metadata(file.path);
                save.write_nand_file(file.path, file.bytes);
                if (metadata) {
                    const auto raw_bytes = save.nand().read_file(file.path);
                    save.nand().write_file(file.path, *raw_bytes, metadata->permission, metadata->attribute);
                }
            }
            return s32{NAND_RESULT_OK};
        });
    }

    s32 NANDDelete(const char* path) {
        return invoke([&] {
            if (!valid_path(path)) return s32{NAND_RESULT_INVALID};
            auto& save = runtime().save_data();
            if (is_open(save.nand().normalize_path(path))) return s32{NAND_RESULT_OPENFD};
            return s32{save.erase_nand_file(path) ? NAND_RESULT_OK : NAND_RESULT_NOEXISTS};
        });
    }

    s32 NANDMove(const char* source, const char* destination_directory) {
        return invoke([&] {
            if (!valid_path(source) || !valid_path(destination_directory)) return s32{NAND_RESULT_INVALID};
            auto& save = runtime().save_data();
            const auto normalized_source = save.nand().normalize_path(source);
            auto destination = save.nand().normalize_path(destination_directory);
            if (destination != "/") destination += '/';
            destination += aurora::NandFileSystem::file_name(normalized_source);
            if (destination.size() >= NAND_MAX_PATH) return s32{NAND_RESULT_INVALID};
            if (is_open(normalized_source) || is_open(destination)) return s32{NAND_RESULT_OPENFD};
            return save.move_nand_file(normalized_source, destination);
        });
    }

    s32 NANDCheck(u32 blocks, u32 inodes, u32* answer) {
        return invoke([&] {
            if (!answer) return s32{NAND_RESULT_INVALID};
            const auto& nand = runtime().save_data().nand();
            const auto home = nand.usage(aurora::NandFileSystem::title_data_root());
            aurora::NandUsage user;
            for (const auto* root : {"/meta", "/ticket", "/title/00010000", "/title/00010001", "/title/00010003",
                                     "/title/00010004", "/title/00010005", "/title/00010006", "/title/00010007", "/shared2/title"}) {
                const auto usage = nand.usage(root);
                user.blocks += usage.blocks;
                user.inodes += usage.inodes;
            }
            *answer = 0;
            if (static_cast<u64>(home.blocks) + blocks > 0x400U) *answer |= NAND_CHECK_HOME_INSSPACE;
            if (static_cast<u64>(home.inodes) + inodes > 0x21U) *answer |= NAND_CHECK_HOME_INSINODE;
            if (static_cast<u64>(user.blocks) + blocks > 0x4400U) *answer |= NAND_CHECK_SYS_INSSPACE;
            if (static_cast<u64>(user.inodes) + inodes > 0xfa0U) *answer |= NAND_CHECK_SYS_INSINODE;
            // Capacity exhaustion is reported in the answer bits, not as an I/O error.
            return s32{NAND_RESULT_OK};
        });
    }

    s32 NANDGetHomeDir(char* path) {
        return invoke([&] {
            if (!path) return s32{NAND_RESULT_INVALID};
            (void)runtime();
            const auto directory = aurora::NandFileSystem::title_data_root();
            if (directory.size() >= NAND_MAX_PATH) return s32{NAND_RESULT_INVALID};
            std::memcpy(path, directory.c_str(), directory.size() + 1);
            return s32{NAND_RESULT_OK};
        });
    }
}
