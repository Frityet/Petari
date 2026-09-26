#include "Game/Util/JMapInfo.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"
#include "NativeHeapFixture.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JKernel/JKRArchive.hpp"
#include "resource/RarcArchive.hpp"
#include <optional>

#include <array>
#include <atomic>
#include <bit>
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

        std::optional<smgpc::resource::JMapResource> resource;
        auto raw = fixture();
        auto runtime = smgpc::test::create_native_root_heap(1024*1024);
        auto domain = smgpc::test::create_native_solid_heap(runtime, 256*1024);
        const char* borrowed;
        bool native_cache;
        {
            const JKRHeap::CurrentHeapScope original(*(domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
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
        require(a.mData != b.mData && !(a == b), "original table identity follows each distinct raw resource");
        require(a.getData() == first.data() && b.getData() == second.data(), "source addresses remain exact");
        require(a.getEntryData(0) == reinterpret_cast<const char*>(first.data() + 40), "entry starts at original data offset");
        require(a.getEntryData(1) == reinterpret_cast<const char*>(first.data() + 76) && a.getDataSize() == 76,
                "original row stride and row-range extent exclude the string table");
        const char *av, *bv;
        require(a.getValue(0, "name", &av) && b.getValue(0, "name", &bv) &&
                    av == reinterpret_cast<const char*>(first.data() + 76) &&
                    bv == reinterpret_cast<const char*>(second.data() + 76),
                "original string readers borrow each resource's actual string table");
        JMapInfo copy(a), moved(std::move(copy));
        require(moved == a && moved.getData() == first.data() && copy.getData() == nullptr,
                "copy and move preserve raw identity without retaining a moved-from alias");
        JMapInfo owned = smgpc::resource::make_jmap_info(fixture());
        require(owned.getData() != first.data() && std::memcmp(owned.getData(), first.data(), first.size()) == 0,
                "direct parser owns the complete original byte image");
    }
    void test_original_field_rules_and_binary_search() {
        static_assert(sizeof(JMapItem) == 12 && sizeof(JMapData) == 16 && alignof(JMapData) == 1);
        struct Field { const char* name; u8 type; u32 mask; u8 shift; u32 word; };
        const std::array fields{
            Field{"signed", 0, 0xffffffff, 0, 0xffff8001},
            Field{"short", 4, 0xffff, 0, 0x80010000},
            Field{"byte", 5, 0xff, 0, 0xfe000000},
            Field{"unsigned", 3, 0x0000ff00, 8, 0x12345600},
            Field{"float", 2, 0xffffffff, 0, std::bit_cast<u32>(-1.5F)},
            Field{"packed", 0, 0xf0, 4, 0xab},
            Field{"flag", 0, 0x80000000, 31, 0x80000000},
        };
        const u32 data_offset = 16 + fields.size() * 12;
        std::vector<u8> bytes(data_offset + fields.size() * 4);
        put32(bytes, 0, 1); put32(bytes, 4, fields.size());
        put32(bytes, 8, data_offset); put32(bytes, 12, fields.size() * 4);
        for (std::size_t i = 0; i < fields.size(); ++i) {
            const auto header = 16 + i * 12;
            put32(bytes, header, smgpc::resource::jmap_hash(fields[i].name));
            put32(bytes, header + 4, fields[i].mask);
            bytes[header + 9] = i * 4;
            bytes[header + 10] = fields[i].shift;
            bytes[header + 11] = fields[i].type;
            put32(bytes, data_offset + i * 4, fields[i].word);
        }
        const auto info = smgpc::resource::make_jmap_info(bytes);
        s32 signed_value = 0;
        require(info.getValue(0, "signed", &signed_value) && signed_value == -32767,
                "original signed words preserve their full two's-complement value");
        require(info.getValue(0, "short", &signed_value) && signed_value == -32767 &&
                    info.getValue(0, "byte", &signed_value) && signed_value == -2,
                "original short and byte fields sign-extend only at full width");
        signed_value = 123;
        require(!info.getValue(0, "unsigned", &signed_value) && !info.getValue(0, "packed", &signed_value) && signed_value == 123,
                "original signed getters reject unsigned or packed fields without changing the output");
        u32 value = 0;
        require(info.getValue(0, "unsigned", &value) && value == 0x56 &&
                    info.getValue(0, "packed", &value) && value == 10,
                "original unsigned getters apply the authored mask and shift");
        float floating = 0;
        bool flag = false;
        require(info.getValue(0, "float", &floating) && floating == -1.5F && info.getValue(0, "flag", &flag) && flag,
                "original float and bool getters read big-endian payloads directly");

        const std::array<const char*, 5> names{"Alpha", "Duplicate", "Duplicate", "Duplicate", "Omega"};
        bytes.assign(28 + names.size() * 4, 0);
        put32(bytes, 0, names.size()); put32(bytes, 4, 1); put32(bytes, 8, 28); put32(bytes, 12, 4);
        put32(bytes, 16, smgpc::resource::jmap_hash("name")); put32(bytes, 20, 0xffffffff); bytes[27] = 6;
        for (std::size_t row = 0; row < names.size(); ++row) {
            put32(bytes, 28 + row * 4, bytes.size() - 48);
            bytes.insert(bytes.end(), names[row], names[row] + std::strlen(names[row]) + 1);
        }
        const auto sorted = smgpc::resource::make_jmap_info(bytes);
        require(sorted.findElementBinary("name", "Duplicate").mIndex == 2 &&
                    sorted.findElement("name", "Duplicate", 0).mIndex == 1,
                "the restored binary search selects the original midpoint among duplicate keys");
        require(sorted.findElementBinary("name", "Missing") == sorted.end() &&
                    MR::findJMapInfoElementNoCase(&sorted, "name", "oMEGA", 0).mIndex == 4,
                "original absent and case-insensitive searches retain their iterator contracts");
    }
    void test_raw_source_heap_retirement() {

        auto runtime = smgpc::test::create_native_root_heap(1 << 20);
        auto domain = smgpc::test::create_native_solid_heap(runtime, 1 << 18);
        auto raw = std::make_shared<const std::vector<u8>>(fixture());
        std::weak_ptr<const std::vector<u8>> weak = raw;
        std::optional<smgpc::resource::JMapSourceRegistration> registration;
        registration.emplace(smgpc::resource::register_jmap_source(*raw, raw));
        {
            const JKRHeap::CurrentHeapScope original(*(domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            auto* info = new JMapInfo;
            require(info->attach(raw->data()), "actual Game-heap parser attaches retained source");
            require(JKRHeap::findFromRoot(info) == &(*domain), "parser belongs to original heap");
            require(JKRHeap::findFromRoot(const_cast<void*>(info->getData())) == nullptr, "archive source stays host-owned");
        }
        registration.reset(); raw.reset();
        require(!weak.expired(), "Game parser retains source after unpublication");
        (*domain).freeAll();
        require(weak.expired(), "original disposer dispatch releases raw source before arena reuse");
        {
            const JKRHeap::CurrentHeapScope original(*(domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            auto* reused = new JMapInfo;
            require(reused->getData() == nullptr, "reused arena starts with no stale raw identity");
        }
        domain.reset(); runtime.reset();
    }
    auto archive_fixture() {
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
        return bytes;
    }
    void test_original_archive_index() {
        using namespace smgpc::resource;
        const auto table = fixture();
        auto archive = std::make_shared<RarcArchive>(RarcArchive::from_bytes(archive_fixture()));
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
    void test_fixed_archive_attachment_leases() {
        using namespace smgpc::resource;
        auto raw = archive_fixture();
        const auto expected_table = fixture();
        auto archive = std::make_unique<JKRMemArchive>();
        require(archive->mountFixed(std::span<const u8>(raw), JKR_MEM_BREAK_FLAG_0), "bounded fixed archive mounts");
        auto source = archive->retainSource();
        std::weak_ptr<const RarcArchive> weak_source = source;
        const std::weak_ptr<const void> lifetime = archive->retainNativeResources();
        const auto bytes = source->file_data(source->entries().front());
        std::optional<JMapSourceRegistration> registration;
        registration.emplace(register_jmap_source(bytes, source, [lifetime] { return lifetime.lock(); }));
        source.reset();
        archive->validateNativeRetirement();

        JMapInfo first;
        require(first.attach(bytes.data()) && first.getData() == bytes.data() &&
                    first.getEntryData(0) == reinterpret_cast<const char*>(bytes.data() + 40),
                "fixed attachment preserves the archive's exact raw and entry identities");
        JMapInfo copy(first);
        first = JMapInfo();
        archive->validateNativeRetirement(1);
        JMapInfo independent;
        require(independent.attach(bytes.data()), "a second original attach acquires its own lease");
        archive->validateNativeRetirement(2);
        bool rejected = false;
        try { archive->validateNativeRetirement(1); }
        catch (const std::logic_error&) { rejected = true; }
        require(rejected, "copied readers share one lease while independently attached readers own separate leases");

        registration.reset();
        require(!find_jmap_resource(bytes.data()) && !weak_source.expired(),
                "unpublication removes raw lookup while existing readers retain bounded metadata");
        rejected = false;
        try { archive->validateNativeRetirement(); }
        catch (const std::logic_error&) { rejected = true; }
        require(rejected && bytes.size() == expected_table.size() &&
                    std::memcmp(copy.getData(), expected_table.data(), expected_table.size()) == 0,
                "live fixed-buffer attachments prevent archive retirement rather than dangling their raw identity");
        copy = JMapInfo();
        archive->validateNativeRetirement(1);
        JMapResource replacement(fixture());
        require(independent.attach(replacement.data()), "rebinding to a separately owned table succeeds");
        archive->validateNativeRetirement();
        archive.reset();
        require(lifetime.expired() && weak_source.expired(),
                "final detach releases archive and metadata before the fixed caller buffer can retire");
        const char* value = nullptr;
        require(independent.getValue(0, "name", &value) && std::string_view(value) == name,
                "rebound reader retains its independently owned table");
    }
    void test_attachment_lease_validation() {
        using namespace smgpc::resource;
        auto raw = std::make_shared<const std::vector<u8>>(fixture());
        auto lease = std::make_shared<const int>(7);
        std::weak_ptr<const void> weak = lease;
        auto registration = register_jmap_source(*raw, raw, [weak] { return weak.lock(); });
        bool policy_rejected = false;
        try { auto changed = register_jmap_source(*raw, raw); }
        catch (const std::logic_error&) { policy_rejected = true; }
        require(policy_rejected, "duplicate raw registration cannot silently discard its required attachment lease");
        lease.reset();
        bool expired_rejected = false;
        try { JMapInfo reader; (void)reader.attach(raw->data()); }
        catch (const std::logic_error&) { expired_rejected = true; }
        require(expired_rejected, "an expired raw lifetime rejects attachment before decoding source bytes");
    }

}
int main() {
    const std::array tests{
        std::pair{"original field rules and binary search", test_original_field_rules_and_binary_search},
        std::pair{"raw source identity and original row ranges", test_raw_source_identity},
        std::pair{"raw source original heap retirement", test_raw_source_heap_retirement},
        std::pair{"original memory-archive index fetch", test_original_archive_index},
        std::pair{"fixed archive attachment retirement leases", test_fixed_archive_attachment_leases},
        std::pair{"attachment lease expiry and duplicate policy", test_attachment_lease_validation},
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
