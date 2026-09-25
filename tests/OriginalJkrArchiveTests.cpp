#include "JSystem/JKernel/JKRArchive.hpp"
#include "JSystem/JKernel/JKRFileFinder.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    void require(bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    }
    void put16(std::vector<u8>& out, std::size_t offset, u16 value) {
        out[offset] = value >> 8; out[offset + 1] = value;
    }
    void put32(std::vector<u8>& out, std::size_t offset, u32 value) {
        out[offset] = value >> 24; out[offset + 1] = value >> 16;
        out[offset + 2] = value >> 8; out[offset + 3] = value;
    }
    std::vector<u8> fixture() {
        const std::array names{"actual_root", "a.bck", "nested", "empty", ".", "..", "b.pa"};
        std::vector<u8> strings;
        std::array<u32, names.size()> offsets{};
        for (std::size_t i = 0; i < names.size(); ++i) {
            offsets[i] = strings.size();
            strings.insert(strings.end(), names[i], names[i] + std::strlen(names[i]) + 1);
        }
        constexpr u32 dirs = 0x40, files = 0x70, str = files + 10 * 0x14;
        const u32 data = (str + strings.size() + 31) & ~31U;
        std::vector<u8> out(data + 2);
        put32(out, 0, 0x52415243); put32(out, 4, out.size()); put32(out, 8, 0x20);
        put32(out, 12, data - 0x20); put32(out, 16, 2);
        put32(out, 0x20, 3); put32(out, 0x24, dirs - 0x20);
        put32(out, 0x28, 10); put32(out, 0x2c, files - 0x20);
        put32(out, 0x30, strings.size()); put32(out, 0x34, str - 0x20);
        put16(out, 0x38, 43);
        const auto directory = [&](u32 i, u32 name, u16 count, u32 first) {
            const u32 offset = dirs + i * 16;
            put32(out, offset, i == 0 ? 0x524f4f54 : 0x44495220);
            put32(out, offset + 4, offsets[name]);
            put16(out, offset + 8, smgpc::resource::RarcArchive::hash_name(names[name]));
            put16(out, offset + 10, count); put32(out, offset + 12, first);
        };
        directory(0, 0, 5, 0); directory(1, 2, 3, 5); directory(2, 3, 2, 8);
        const auto file = [&](u32 i, u16 id, u32 name, u8 flags, u32 start, u32 size) {
            const u32 offset = files + i * 20;
            put16(out, offset, id);
            put16(out, offset + 2, smgpc::resource::RarcArchive::hash_name(names[name]));
            put32(out, offset + 4, (u32(flags) << 24) | offsets[name]);
            put32(out, offset + 8, start); put32(out, offset + 12, size);
        };
        file(0, 42, 1, 0x11, 0, 1); file(1, 0xffff, 2, 2, 1, 16);
        file(2, 0xffff, 3, 2, 2, 16); file(3, 0xffff, 4, 2, 0, 16);
        file(4, 0xffff, 5, 2, 0xffffffff, 16);
        file(5, 7, 6, 0x21, 1, 1); file(6, 0xffff, 4, 2, 1, 16);
        file(7, 0xffff, 5, 2, 0, 16); file(8, 0xffff, 4, 2, 2, 16);
        file(9, 0xffff, 5, 2, 0, 16);
        std::copy(strings.begin(), strings.end(), out.begin() + str);
        out[data] = 0x42; out[data + 1] = 0x07;
        return out;
    }
    auto archive() { return JKRMemArchive(smgpc::resource::RarcArchive::from_bytes(fixture())); }
    std::vector<std::string> names(JKRArchive& arc, const char* path) {
        std::unique_ptr<JKRArcFinder> finder(arc.getFirstFile(path));
        require(finder != nullptr, "existing directory requires a real finder");
        std::vector<std::string> result;
        while (finder->mHasMoreFiles) {
            result.emplace_back(finder->mName);
            finder->findNextFile();
        }
        require(!finder->findNextFile(), "exhausted finder must stay exhausted");
        return result;
    }
    void test_catalog() {
        auto arc = archive();
        require(std::strcmp(arc.mLoaderName, "actual_root") == 0, "loader name comes from authored root record");
        require(arc.mInfoBlock->mNrDirs == 3 && arc.mInfoBlock->mNrFiles == 10, "complete raw catalog survives");
        require(arc.countResource() == 2, "resource count excludes folders and dot entries");
        require(arc.countFile("/") == 5 && arc.countFile("/nested") == 3 && arc.countFile("/empty") == 2,
                "directory count includes original dot records and empty folders");
        require(arc.countFile("/missing") == 0 && arc.getFirstFile("/missing") == nullptr,
                "missing directory stays absent");
        require(arc.countFile("/..") == 0 && arc.getFirstFile("/..") == nullptr,
                "root parent sentinel must not become a native array pointer");
    }
    void test_finder() {
        auto arc = archive();
        require(names(arc, "/") == std::vector<std::string>{"a.bck", "nested", "empty", ".", ".."},
                "finder preserves authored directory order");
        require(names(arc, "/NESTED") == std::vector<std::string>{"b.pa", ".", ".."},
                "original hash/name lookup lowercases request");
        require(names(arc, "/empty") == std::vector<std::string>{".", ".."}, "empty folder is retained");
        std::unique_ptr<JKRArcFinder> finder(arc.getFirstFile("/nested"));
        require(finder->mFileID == 7 && finder->mDirIndex == 5 && finder->mFileFlag == 0x21 && !finder->mFileIsFolder,
                "file ID remains distinct from catalog index and flags");
        finder->findNextFile();
        require(finder->mFileIsFolder && finder->mFileID == 0xffff, "folder flag and ID are preserved");
    }
    void test_ids_and_data() {
        auto arc = archive();
        const auto* a = static_cast<const u8*>(arc.getResource(u16(42)));
        const auto* b = static_cast<const u8*>(arc.getResource(u16(7)));
        require(a && *a == 0x42 && b && *b == 7, "non-index file IDs resolve correct owned bytes");
        require(arc.getResource(u16(0xffff)) == nullptr && arc.getResource(u16(99)) == nullptr, "unknown file IDs stay absent");
        require(arc.getFileAttribute(0) == 0x11 && arc.getFileAttribute(5) == 0x21 && arc.getFileAttribute(42) == 0,
                "getFileAttribute uses table index rather than file ID");
        JKRArchive::SDirEntry entry{};
        require(arc.getDirEntry(&entry, 5) && entry.mFileID == 7 && entry.mFileFlag == 0x21 &&
                    std::strcmp(entry.mName, "b.pa") == 0, "getDirEntry exposes retained typed metadata");
        require(!arc.getDirEntry(&entry, 10), "out-of-range directory entry is absent");
        require(entry.mFileID == 7, "failed directory entry read preserves output");
        require(arc.getResSize(a) == 1 && arc.getResSize(b) == 1, "resource size uses same retained byte identity");
    }
    void test_relative_directory() {
        auto arc = archive();
        JKRArchive::sCurrentDirID = 1;
        require(names(arc, ".") == std::vector<std::string>{"b.pa", ".", ".."}, "relative dot uses original current directory ID");
        require(arc.countFile("..") == 5 && arc.countFile("/empty") == 2, "parent and absolute directory semantics");
        JKRArchive::sCurrentDirID = 0;
    }
    void test_lifetime() {
        auto retained = std::make_unique<JKRMemArchive>(smgpc::resource::RarcArchive::from_bytes(fixture()));
        std::unique_ptr<JKRArcFinder> finder(retained->getFirstFile("/nested"));
        const char* name = finder->mName;
        const auto* resource = static_cast<const u8*>(retained->getResource(u16(7)));
        for (int i = 0; i < 32; ++i) { auto unrelated = archive(); require(unrelated.countResource() == 2, "independent catalog"); }
        require(std::strcmp(name, "b.pa") == 0 && *resource == 7, "catalog and payload survive source destruction and unrelated owners");
    }
    void test_bounded_lookup_names() {
        struct GuardedName {
            JKRArchive::CArcName name;
            std::array<u8, 8> guard;
        } value{};
        value.guard.fill(0xCD);
        const std::string maximum(255, 'A');
        value.name.store(maximum.c_str());
        require(value.name.mLength == 255 && std::strlen(value.name.mName) == 255 &&
                    value.name.mName[0] == 'a' && value.name.mName[254] == 'a',
                "the original maximum name remains lowercased and terminated");
        const std::string path = maximum + "/tail";
        require(value.name.store(path.c_str(), '/') == path.c_str() + 256 && value.name.mLength == 255,
                "maximum-length path component preserves the original next-component pointer");
        const std::string oversized(256, 'A');
        bool whole_rejected = false, component_rejected = false;
        try { value.name.store(oversized.c_str()); }
        catch (const std::invalid_argument&) { whole_rejected = true; }
        try { (void)value.name.store((oversized + "/tail").c_str(), '/'); }
        catch (const std::invalid_argument&) { component_rejected = true; }
        require(whole_rejected && component_rejected, "both lookup overloads must reject an oversized name before writing its terminator");
        require(std::all_of(value.guard.begin(), value.guard.end(), [](u8 byte) { return byte == 0xCD; }),
                "lookup names must never overwrite neighboring storage");
    }
    void test_fixed_mount_volume_and_byte_identity() {
        auto bytes = fixture();
        auto parsed = smgpc::resource::RarcArchive::from_borrowed(bytes);
        const auto count = JKRFileLoader::sVolumeList.getNumLinks();
        auto* current = JKRFileLoader::gCurrentFileLoader;
        {
            JKRMemArchive mounted;
            require(mounted.mountFixed(std::span<const u8>(bytes), JKR_MEM_BREAK_FLAG_0),
                    "a borrowed fixed mount must publish successfully");
            require(JKRFileLoader::sVolumeList.getNumLinks() == count + 1 &&
                        mounted.mEntryNum == reinterpret_cast<std::uintptr_t>(bytes.data()),
                    "the volume list retains native-width buffer identity");
            JKRFileLoader::initializeVolumeList();
            require(JKRFileLoader::sVolumeList.getNumLinks() == count + 1,
                    "process initialization cannot reset a pre-existing mounted volume");
            auto* data = static_cast<u8*>(mounted.getIdxResource(0));
            require(data == parsed.file_data_start() && mounted.getResource(u16(42)) == data && mounted.getResSize(data) == 1,
                    "indexed, ID and size lookups share the original borrowed resource bytes");
            *data = 0xA5;
            require(*parsed.file_data_start() == 0xA5 && JKRFileLoader::getGlbResource("a.bck") == data,
                    "global lookup observes the mounted buffer without a payload copy");
            JKRMemArchive duplicate;
            require(!duplicate.mountFixed(std::span<const u8>(bytes), JKR_MEM_BREAK_FLAG_0) &&
                        mounted._34 == 2 && !duplicate.mIsMounted,
                    "duplicate fixed mount preserves the original reference-count increment and rejects publication");
            mounted.unmount();
            require(mounted._34 == 1 && mounted.mIsMounted,
                    "releasing the duplicate reference preserves the first mount");
        }
        require(JKRFileLoader::sVolumeList.getNumLinks() == count && JKRFileLoader::gCurrentFileLoader == current,
                "archive retirement removes the volume while preserving the prior current loader");
        require(*parsed.file_data_start() == 0xA5, "borrowed bytes remain owned by their caller after archive retirement");
    }
    void test_missing_string_extent() {
        auto bytes = fixture();
        put32(bytes, 0x30, 1);
        auto parsed = smgpc::resource::RarcArchive::from_bytes(std::move(bytes));
        bool rejected = false;
        try { JKRMemArchive arc(parsed); } catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "bounded native catalog rejects unterminated names in declared string extent");
    }
    void test_native_cache_override_retirement() {
        std::array<unsigned, 3> order{0, 1, 2};
        do {
            auto arc = archive();
            void* original = arc.getIdxResource(0);
            std::array<u8, 3> replacements{0x61, 0x62, 0x63};
            std::array<std::shared_ptr<const void>, 3> handles;
            std::array<bool, 3> live{true, true, true};
            for (unsigned i = 0; i < handles.size(); ++i)
                handles[i] = arc.overrideNativeResource(0, &replacements[i]);
            auto copy = handles[0];
            // Copying an override handle shares its one archive borrow; it does
            // not represent another converted file owned by the holder.
            arc.validateNativeRetirement(3);
            bool rejected = false;
            try { arc.validateNativeRetirement(2); }
            catch (const std::logic_error&) { rejected = true; }
            require(rejected, "preflight discounts exactly one archive token for each live cache override");
            copy.reset();
            for (const auto retiring : order) {
                handles[retiring].reset();
                live[retiring] = false;
                void* expected = original;
                for (unsigned i = 0; i < live.size(); ++i)
                    if (live[i]) expected = &replacements[i];
                require(arc.getIdxResource(0) == expected && arc.getResource(u16(42)) == expected &&
                            arc.getResource("/a.bck") == expected && arc.getResSize(expected) == 1,
                        "every retirement permutation restores the newest live resource across original lookup APIs");
            }
            arc.validateNativeRetirement();
        } while (std::next_permutation(order.begin(), order.end()));

        auto arc = archive();
        u8 first = 0x31, second = 0x32, independent = 0x33;
        auto earlier = arc.overrideNativeResource(0, &first);
        auto later = arc.overrideNativeResource(0, &second);
        arc.mFiles[0].mFileData = &independent;
        earlier.reset(); later.reset();
        require(arc.getIdxResource(0) == &independent,
                "retiring an override cannot overwrite a separately replaced native cache");
        arc.validateNativeRetirement();
        bool folder_rejected = false, index_rejected = false, null_rejected = false;
        try { (void)arc.overrideNativeResource(1, &first); }
        catch (const std::invalid_argument&) { folder_rejected = true; }
        try { (void)arc.overrideNativeResource(10, &first); }
        catch (const std::invalid_argument&) { index_rejected = true; }
        try { (void)arc.overrideNativeResource(0, nullptr); }
        catch (const std::invalid_argument&) { null_rejected = true; }
        require(folder_rejected && index_rejected && null_rejected && arc.getIdxResource(0) == &independent,
                "invalid native overrides preserve the existing cache and lifetime state");
    }
}
int main() {
    const std::array tests{
        std::pair{"full typed catalog", test_catalog}, std::pair{"original finder", test_finder},
        std::pair{"IDs and resource bytes", test_ids_and_data}, std::pair{"relative directories", test_relative_directory},
        std::pair{"retained archive lifetime", test_lifetime}, std::pair{"bounded string table", test_missing_string_extent},
        std::pair{"bounded lookup names", test_bounded_lookup_names},
        std::pair{"fixed mount volume and byte identity", test_fixed_mount_volume_and_byte_identity},
        std::pair{"native cache override lifetime and arbitrary retirement", test_native_cache_override_retirement},
    };
    for (const auto& [name, test] : tests) {
        try { test(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& error) { std::cerr << "FAIL " << name << ": " << error.what() << '\n'; return 1; }
    }
}
