#include "Game/Util/JMapInfo.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JKernel/JKRArchive.hpp"
#include "resource/RarcArchive.hpp"
#include <optional>

#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace {
    constexpr std::string_view name = "AnimationNameLongEnoughToRequireRetainedAllocatedStorage";
    constexpr std::string_view inline_name = "inline-animation";
    void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    void put32(std::vector<u8>& out, std::size_t offset, u32 value) {
        out[offset] = value >> 24; out[offset + 1] = value >> 16;
        out[offset + 2] = value >> 8; out[offset + 3] = value;
    }
    auto fixture() {
        std::vector<u8> out(40 + 36 + name.size() + 1);
        put32(out, 0, 1); put32(out, 4, 2); put32(out, 8, 40); put32(out, 12, 36);
        put32(out, 16, smgpc::resource::jmap_hash("inline")); put32(out, 20, 0xffffffff);
        out[27] = 1;
        put32(out, 28, smgpc::resource::jmap_hash("name")); put32(out, 32, 0xffffffff);
        out[37] = 32; out[39] = 6;
        std::memcpy(out.data() + 40, inline_name.data(), inline_name.size());
        put32(out, 72, 0);
        std::memcpy(out.data() + 76, name.data(), name.size());
        return out;
    }
    const char* read(const smgpc::resource::JMapResource& resource, const char* key) {
        JMapInfo local;
        require(local.attach(resource.data()), "actual unsized attach resolves retained bounded resource");
        const char* result = nullptr;
        require(local.getValue(0, key, &result), "actual string getter reads named field");
        return result;
    }
    void test_reader_lifetime() {
        auto resource = smgpc::resource::JMapResource(fixture());
        const char* borrowed = read(resource, "name");
        const char* short_borrowed = read(resource, "inline");
        require(std::string_view(borrowed) == name && std::string_view(short_borrowed) == inline_name,
                "original-style borrowed strings survive local JMapInfo destruction");
        for (int i = 0; i < 100; ++i) {
            require(read(resource, "name") == borrowed && read(resource, "inline") == short_borrowed,
                    "repeated reads and attaches retain exact character-pointer identity");
        }
    }
    void test_attached_reader_retains_data() {
        JMapInfo survivor;
        const char* borrowed;
        {
            auto resource = smgpc::resource::JMapResource(fixture());
            require(survivor.attach(resource.data()), "survivor attaches real table");
            borrowed = read(resource, "name");
        }
        require(std::string_view(borrowed) == name, "attached reader retains shared string owner after resource release");
        const char* again = nullptr;
        require(survivor.getValue(0, "name", &again) && again == borrowed, "surviving table shares same cache identity");
    }
    void test_rebind_keeps_other_owner() {
        auto first = smgpc::resource::JMapResource(fixture());
        auto second = smgpc::resource::JMapResource(fixture());
        JMapInfo info;
        require(info.attach(first.data()), "first attach");
        const char* borrowed = nullptr;
        require(info.getValue(0, "name", &borrowed), "first borrowed value");
        require(info.attach(second.data()), "second attach");
        const char* rebound = nullptr;
        require(info.getValue(0, "name", &rebound), "second borrowed value");
        require(borrowed != rebound && std::string_view(borrowed) == name && read(first, "name") == borrowed,
                "rebind cannot clear or replace strings retained by the old resource");
    }
    void test_concurrent_readers() {
        auto resource = smgpc::resource::JMapResource(fixture());
        std::array<const char*, 6> pointers{};
        std::array<std::thread, 6> readers;
        for (std::size_t i = 0; i < readers.size(); ++i) {
            readers[i] = std::thread([&, i] {
                for (int repeat = 0; repeat < 100; ++repeat) pointers[i] = read(resource, "name");
            });
        }
        for (auto& reader : readers) reader.join();
        for (auto* pointer : pointers) require(pointer == pointers[0], "concurrent readers share one stable cache entry");
    }
    void test_alias_lifetime() {
        using namespace smgpc::resource;
        auto raw = fixture();
        std::optional<JMapSourceRegistration> retained;
        JMapInfo survivor;
        const char* borrowed;
        {
            JMapResource owner(raw);
            retained.emplace(owner.register_source(raw));
            {
                auto second = owner.register_source(raw);
                require(survivor.attach(raw.data()), "archive alias resolves actual table");
                require(survivor.getValue(0, "name", &borrowed), "aliased reader reads original field");
            }
            require(find_jmap_resource(raw.data()) != nullptr, "nested registration release preserves retained alias");
        }
        require(find_jmap_resource(raw.data()) != nullptr && std::string_view(borrowed) == name,
                "registration retains decoded table after native owner handle leaves");
        retained.reset();
        require(find_jmap_resource(raw.data()) == nullptr, "last alias release removes exact raw identity");
        const char* again;
        require(survivor.getValue(0,"name",&again) && again == borrowed,
                "attached reader retains shared table after raw identity deregistration");
    }
    void test_alias_validation_and_reuse() {
        using namespace smgpc::resource;
        auto raw = fixture();
        JMapResource first(raw), second(raw);
        auto registered = first.register_source(raw);
        bool failed = false;
        try { auto wrong = second.register_source(raw); } catch (const std::logic_error&) { failed = true; }
        require(failed, "an active alias cannot change resource owners");
        auto altered = raw; altered.back() ^= 1;
        failed = false;
        try { auto wrong = first.register_source(altered); } catch (const std::invalid_argument&) { failed = true; }
        require(failed, "alias validates complete bytes");
        failed = false;
        try { auto wrong = first.register_source(std::span(raw).first(raw.size()-1)); } catch (const std::invalid_argument&) { failed = true; }
        require(failed, "alias validates complete extent");
        std::optional<JMapSourceRegistration> self;
        self.emplace(first.register_source(first.bytes()));
        self.reset();
        require(find_jmap_resource(first.data()) != nullptr, "owned identity baseline survives optional self alias");
    }
    void test_native_heap_boundary() {
        using namespace smgpc::compat;
        std::optional<smgpc::resource::JMapResource> resource;
        auto raw = fixture();
        auto runtime = JkrHeapRuntime::create(1024*1024);
        auto domain = JkrAllocationDomain::create(runtime, 256*1024);
        const char* borrowed;
        bool native_cache;
        {
            JkrAllocationScope original(domain);
            resource.emplace(raw);
            borrowed = read(*resource, "name");
            native_cache = JKRHeap::findFromRoot(const_cast<char*>(borrowed)) == nullptr;
            require(JKRHeap::findFromRoot(const_cast<void*>(resource->data())) == nullptr,
                    "JMap resource bytes are host owned inside Game allocation scope");
        }
        // Safely dispose the old implementation's bad cache before retiring
        // its arena, so the negative oracle reports ownership rather than UB.
        if (!native_cache) resource.reset();
        domain.reset();runtime.reset();
        require(native_cache, "lazy shared JMap string cache escaped Game heap routing");
        require(std::string_view(borrowed)==name && read(*resource,"name")==borrowed,
                "cached names survive the heap that was selected for the first read");
    }
    void test_deferred_archive_lifetime() {
        using namespace smgpc::resource;
        auto raw = std::make_shared<const std::vector<u8>>(fixture());
        std::weak_ptr<const std::vector<u8>> weak = raw;
        const void* identity = raw->data();
        std::optional<JMapSourceRegistration> first, second;
        first.emplace(register_jmap_source(*raw, raw));
        second.emplace(register_jmap_source(*raw, raw));
        raw.reset();
        require(!weak.expired(), "deferred archive registration retains source before the first read");
        JMapInfo survivor;
        require(survivor.attach(identity), "original unsized attach decodes retained archive bytes on demand");
        const char* value = nullptr;
        require(survivor.getValue(0, "name", &value) && std::string_view(value) == name,
                "deferred source decodes the original table and string");
        first.reset();
        require(find_jmap_resource(identity) != nullptr, "duplicate registration retains the same source");
        second.reset();
        require(!weak.expired() && find_jmap_resource(identity) == nullptr,
                "unpublication removes lookup while the attached reader retains actual source bytes");
        const char* again = nullptr;
        require(survivor.getValue(0, "name", &again) && again == value,
                "attached reader retains decoded table after archive source unpublication");
        survivor = JMapInfo();
        require(weak.expired(), "last reader releases the retained source lease");
    }
    void test_deferred_validation() {
        using namespace smgpc::resource;
        auto raw = std::make_shared<const std::vector<u8>>(std::initializer_list<u8>{1, 2, 3});
        auto registration = register_jmap_source(*raw, raw);
        // FileInfoTable contains arbitrary raw assets. Registration must not
        // interpret them until a real JMapInfo consumer requests that identity.
        bool rejected = false;
        try { JMapInfo info; (void)info.attach(raw->data()); }
        catch (const std::exception&) { rejected = true; }
        require(rejected, "requested malformed table is rejected within its retained three-byte extent");
        rejected = false;
        try { auto wrong = register_jmap_source(std::span(*raw).first(2), raw); }
        catch (const std::logic_error&) { rejected = true; }
        require(rejected, "existing source cannot silently change its extent");
        rejected = false;
        try { auto wrong = register_jmap_source(*raw, std::make_shared<int>(1)); }
        catch (const std::logic_error&) { rejected = true; }
        require(rejected, "existing source cannot silently change its owner");
    }
    void test_concurrent_deferred_readers() {
        auto raw = std::make_shared<const std::vector<u8>>(fixture());
        auto registration = smgpc::resource::register_jmap_source(*raw, raw);
        std::array<const char*, 6> pointers{};
        std::array<std::thread, 6> readers;
        for (std::size_t i = 0; i < readers.size(); ++i) readers[i] = std::thread([&, i] {
            JMapInfo info;
            require(info.attach(raw->data()), "concurrent lazy attach");
            require(info.getValue(0, "name", &pointers[i]), "concurrent lazy string read");
        });
        for (auto& reader : readers) reader.join();
        for (auto* pointer : pointers)
            require(pointer == pointers[0], "concurrent first readers publish one decoded table/cache");
    }
    void test_raw_source_identity() {
        using namespace smgpc::resource;
        auto first = fixture(), second = first;
        JMapResource owner(first);
        auto reg1 = owner.register_source(first), reg2 = owner.register_source(second);
        JMapInfo a, b;
        require(a.attach(first.data()) && b.attach(second.data()), "both raw aliases attach");
        require(a.mData == b.mData && !(a == b), "shared decoded cache does not conflate distinct raw tables");
        require(a.getData() == first.data() && b.getData() == second.data(), "source addresses remain exact");
        require(a.getEntryData(0) == reinterpret_cast<const char*>(first.data() + 40), "entry starts at original data offset");
        require(a.getEntryData(1) == reinterpret_cast<const char*>(first.data() + 76) && a.getDataSize() == 76,
                "original row stride and row-range extent exclude the string table");
        const char *av, *bv;
        require(a.getValue(0, "name", &av) && b.getValue(0, "name", &bv) && av == bv, "raw aliases keep shared cached strings");
        JMapInfo copy(a), moved(std::move(copy));
        require(moved == a && moved.getData() == first.data() && copy.getData() == nullptr,
                "copy and move preserve raw identity without retaining a moved-from alias");
        JMapInfo owned = JMapInfo::from_bcsv(fixture());
        require(owned.getData() != first.data() && std::memcmp(owned.getData(), first.data(), first.size()) == 0,
                "direct parser owns the complete original byte image");
    }
    void test_raw_source_heap_retirement() {
        using namespace smgpc::compat;
        auto runtime = JkrHeapRuntime::create(1 << 20);
        auto domain = JkrAllocationDomain::create(runtime, 1 << 18);
        auto raw = std::make_shared<const std::vector<u8>>(fixture());
        std::weak_ptr<const std::vector<u8>> weak = raw;
        std::optional<smgpc::resource::JMapSourceRegistration> registration;
        registration.emplace(smgpc::resource::register_jmap_source(*raw, raw));
        {
            JkrAllocationScope original(domain);
            auto* info = new JMapInfo;
            require(info->attach(raw->data()), "actual Game-heap parser attaches retained source");
            require(JKRHeap::findFromRoot(info) == &domain->heap(), "parser belongs to original heap");
            require(JKRHeap::findFromRoot(const_cast<void*>(info->getData())) == nullptr, "archive source stays host-owned");
        }
        registration.reset(); raw.reset();
        require(!weak.expired(), "Game parser retains source after unpublication");
        domain->heap().freeAll();
        require(weak.expired(), "original disposer dispatch releases raw source before arena reuse");
        {
            JkrAllocationScope original(domain);
            auto* reused = new JMapInfo;
            require(reused->getData() == nullptr, "reused arena starts with no stale raw identity");
        }
        domain.reset(); runtime.reset();
    }
    void test_original_archive_index() {
        using namespace smgpc::resource;
        const auto table = fixture();
        constexpr std::size_t dirs = 0x40, files = 0x50, strings = 0x8c, data = 0xc0;
        constexpr std::array<u8, 24> names{'r','o','o','t',0,'.',0,'.','.',0,'t','a','b','l','e','.','b','c','s','v',0,0,0,0};
        std::vector<u8> bytes(data + table.size());
        auto put16 = [&](std::size_t off, u16 value) { bytes.at(off) = value >> 8; bytes.at(off + 1) = value; };
        std::memcpy(bytes.data(), "RARC", 4); put32(bytes, 4, bytes.size()); put32(bytes, 8, 0x20);
        put32(bytes, 12, data - 0x20); put32(bytes, 16, table.size());
        put32(bytes, 0x20, 1); put32(bytes, 0x24, dirs - 0x20); put32(bytes, 0x28, 3); put32(bytes, 0x2c, files - 0x20);
        put32(bytes, 0x30, names.size()); put32(bytes, 0x34, strings - 0x20); put16(0x38, 101);
        std::memcpy(bytes.data() + dirs, "ROOT", 4); put32(bytes, dirs + 4, 0); put16(dirs + 8, RarcArchive::hash_name("root"));
        put16(dirs + 10, 3); put32(bytes, dirs + 12, 0);
        auto entry = [&](unsigned i, u16 id, u32 name_offset, u8 flags, u32 offset, u32 size) {
            const auto p = files + i * 20; put16(p, id); put16(p + 2, RarcArchive::hash_name(reinterpret_cast<const char*>(names.data() + name_offset)));
            put32(bytes, p + 4, (u32(flags) << 24) | name_offset); put32(bytes, p + 8, offset); put32(bytes, p + 12, size);
        };
        entry(0, 100, 10, 0x11, 0, table.size()); entry(1, 0xffff, 5, 2, 0, 16); entry(2, 0xffff, 7, 2, 0xffffffff, 16);
        std::copy(names.begin(), names.end(), bytes.begin() + strings); std::copy(table.begin(), table.end(), bytes.begin() + data);
        auto archive = std::make_shared<RarcArchive>(RarcArchive::from_bytes(std::move(bytes)));
        JKRMemArchive original(*archive);
        auto* index = original.getIdxResource(0);
        require(index == archive->file_data_start() && index == original.getResource(u16(100)), "index fetch returns actual memory-archive bytes");
        require(original.getIdxResource(100) == nullptr && original.getResource(u16(0)) == nullptr, "index and file ID remain distinct domains");
        require(original.getIdxResource(3) == nullptr, "original index bounds reject end of file table");
        u32 size = 0; require(original.fetchResource(original.findIdxResource(0), &size) == index && size == table.size(),
                             "original cached memory fetch returns exact pointer and full file size");
        auto registration = register_jmap_source(archive->file_data(archive->entries().front()), archive);
        JMapInfo info; require(info.attach(index) && info.getData() == index, "index-fed parser retains real raw file identity");
    }

}
int main() {
    const std::array tests{
        std::pair{"raw source identity and original row ranges", test_raw_source_identity},
        std::pair{"raw source original heap retirement", test_raw_source_heap_retirement},
        std::pair{"original memory-archive index fetch", test_original_archive_index},
        std::pair{"borrowed name outlives local reader", test_reader_lifetime},
        std::pair{"attached reader retains released resource", test_attached_reader_retains_data},
        std::pair{"rebind preserves other owner", test_rebind_keeps_other_owner},
        std::pair{"concurrent shared readers", test_concurrent_readers},
        std::pair{"archive alias retention", test_alias_lifetime},
        std::pair{"archive alias validation", test_alias_validation_and_reuse},
        std::pair{"Game heap cache isolation", test_native_heap_boundary},
        std::pair{"deferred archive lifetime", test_deferred_archive_lifetime},
        std::pair{"deferred archive validation", test_deferred_validation},
        std::pair{"concurrent deferred archive readers", test_concurrent_deferred_readers},
    };
    for (const auto& [label, test] : tests) {
        try { test(); std::cout << "PASS " << label << '\n'; }
        catch (const std::exception& error) { std::cerr << "FAIL " << label << ": " << error.what() << '\n'; return 1; }
    }
}
