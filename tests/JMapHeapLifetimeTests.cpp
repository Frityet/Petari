#include "resource/JMapResource.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "NativeHeapFixture.hpp"
#include "JSystem/JKernel/JKRDisposer.hpp"
#include "resource/BcsvTable.hpp"

#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {




    constexpr auto name = "Native map metadata longer than small string storage";

    void require(bool value, const char* message) {
        if (!value) {
            aurora::allocation::HostAllocationScope host;
            throw std::runtime_error(message);
        }
    }

    void put32(std::vector<u8>& bytes, std::size_t at, u32 value) {
        for (unsigned i = 0; i < 4; ++i) bytes[at + i] = u8(value >> (24 - i * 8));
    }

    std::vector<u8> fixture() {
        std::vector<u8> bytes(48 + std::strlen(name) + 1);
        put32(bytes, 0, 1); put32(bytes, 4, 2); put32(bytes, 8, 40); put32(bytes, 12, 8);
        put32(bytes, 16, smgpc::resource::jmap_hash("name")); put32(bytes, 20, 0xffffffff);
        bytes[27] = JMAP_VALUE_TYPE_STRING_PTR;
        put32(bytes, 28, smgpc::resource::jmap_hash("value")); put32(bytes, 32, 0xffffffff);
        bytes[37] = 4; bytes[39] = JMAP_VALUE_TYPE_FLOAT;
        put32(bytes, 44, std::bit_cast<u32>(1.25F));
        std::memcpy(bytes.data() + 48, name, std::strlen(name));
        return bytes;
    }

    struct Heap {
        JKRHeap::Handle runtime = smgpc::test::create_native_root_heap(1024 * 1024);
        JKRHeap::Handle domain = smgpc::test::create_native_solid_heap(runtime, 256 * 1024);
        JKRHeap& get() const { return (*domain); }
        u32 count() const { return get().mDisposerList.getNumLinks(); }
    };

    bool registered(JKRHeap& heap, const void* object) {
        for (auto* link = heap.mDisposerList.getFirst(); link; link = link->getNext())
            if (dynamic_cast<void*>(link->getObject()) == object) return true;
        return false;
    }

    struct NativeValue final : JKRDisposer {
        std::shared_ptr<int> value;
        NativeValue() = default;
        NativeValue(const NativeValue&) = default;
        NativeValue(NativeValue&&) noexcept = default;
        NativeValue& operator=(const NativeValue&) = default;
        NativeValue& operator=(NativeValue&&) noexcept = default;
        ~NativeValue() override = default;
    };

    void test_reusable_identity_base() {
        Heap heap;
        NativeValue host;
        host.value = std::make_shared<int>(17);
        std::weak_ptr<int> payload = host.value;
        NativeValue* first;
        NativeValue* second;
        {
            const JKRHeap::CurrentHeapScope original(*(heap.domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            first = new NativeValue(host);
            second = new NativeValue(std::move(*first));
            require(heap.count() == 2 && registered(heap.get(), first) && registered(heap.get(), second),
                    "copy and move construct two new actual disposer identities");
            require(!registered(heap.get(), &host), "copy source on host must remain unregistered");
            *first = *second;
            host = std::move(*second);
            require(heap.count() == 2 && registered(heap.get(), first) && registered(heap.get(), second),
                    "copy and move assignment preserve both destination registrations");
            require(!registered(heap.get(), &host), "move assignment cannot steal a heap registration");
            delete second;
            require(heap.count() == 1 && registered(heap.get(), first), "explicit delete unlinks only its own identity");
        }
        heap.get().freeAll();
        require(heap.count() == 0 && payload.use_count() == 1, "bulk disposal releases heap values exactly once");
        host.value.reset();
        require(payload.expired(), "the independent host value remains the final shared owner");
    }

    void test_range_tail_and_explicit_delete() {
        Heap heap;
        const auto bytes = fixture();
        std::weak_ptr<const void> head_data, tail_data, deleted_data;
        JMapInfo* head;
        {
            const JKRHeap::CurrentHeapScope original(*(heap.domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            head = new JMapInfo(smgpc::resource::make_jmap_info(bytes));
            auto* removed = new JMapInfo(smgpc::resource::make_jmap_info(bytes));
            void* tail_storage = heap.get().alloc(sizeof(JMapInfo), -32);
            require(tail_storage, "actual original tail allocation succeeds");
            auto* tail = new (tail_storage) JMapInfo(smgpc::resource::make_jmap_info(bytes));
            head_data = head->mResourceOwner; tail_data = tail->mResourceOwner; deleted_data = removed->mResourceOwner;
            require(heap.count() == 3, "three heap parsers have three independent disposer links");
            delete removed;
            require(deleted_data.expired() && heap.count() == 2, "explicit parser delete releases ownership and unlinks");
        }
        heap.get().dispose(head, u32(sizeof(JMapInfo)));
        require(head_data.expired() && !tail_data.expired() && heap.count() == 1,
                "original range disposal dispatches the parser destructor only in its range");
        heap.get().freeTail();
        require(tail_data.expired() && heap.count() == 0, "original freeTail destroys the remaining tail parser");
        heap.get().freeAll();
        require(heap.count() == 0, "later freeAll cannot revisit explicitly deleted or range-disposed parsers");
    }

    void test_copy_move_registration_and_metadata() {
        Heap heap;
        const auto bytes = fixture();
        JMapInfo host = smgpc::resource::make_jmap_info(bytes);
        host.setName(name);
        const auto data = host.mResourceOwner;
        JMapInfo survivor;
        {
            const JKRHeap::CurrentHeapScope original(*(heap.domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            auto* copied = new JMapInfo(host);
            auto* moved = new JMapInfo(std::move(*copied));
            require(heap.count() == 2 && registered(heap.get(), copied) && registered(heap.get(), moved),
                    "JMap copy/move register the new storage instead of transferring the source link");
            require(moved->mResourceOwner == data && moved->getName() == name && *moved == host,
                    "JMap copy/move retain table identity and original borrowed name");
            *copied = *moved;
            survivor = std::move(*moved);
            require(heap.count() == 2 && !registered(heap.get(), &survivor),
                    "JMap assignment preserves heap destinations and never registers a host destination");
            require(survivor.mResourceOwner == data && std::string_view(survivor.getName()) == name,
                    "JMap assignment preserves shared data and copied name");
            *copied = *copied;
            *moved = std::move(*moved);
            require(heap.count() == 2 && registered(heap.get(), copied) && registered(heap.get(), moved),
                    "self assignment does not damage intrusive registration");
        }
        heap.get().freeAll();
        require(heap.count() == 0 && data.use_count() == 3,
                "bulk retirement releases exactly the heap parser's remaining shared reference");
        float value = 0;
        require(survivor.getValue(0, "value", &value) && value == 1.25F,
                "the original float resource survives source heap retirement");
    }

    void test_host_resource_and_borrowed_strings() {
        Heap heap;
        const auto bytes = fixture();
        JMapInfo survivor;
        std::weak_ptr<const void> resource;
        const char* borrowed = nullptr;
        {
            const JKRHeap::CurrentHeapScope original(*heap.domain);
            const aurora::allocation::ClientAllocationScope routing({true, true});
            auto* parser = new JMapInfo(smgpc::resource::make_jmap_info(bytes));
            const auto before_reads = heap.get().getFreeSize();
            parser->setName(name);
            require(parser->getName() == name, "the original name setter borrows its caller's string");
            require(parser->getValue(0, "name", &borrowed) &&
                        borrowed == reinterpret_cast<const char*>(parser->getData()) + 48,
                    "original string lookup returns the actual string-table address");
            survivor = *parser;
            resource = parser->mResourceOwner;
            require(heap.get().getFreeSize() == before_reads && heap.count() == 1,
                    "original getters and parser copies require no additional Game heap storage");
            require(JKRHeap::findFromRoot(const_cast<char*>(borrowed)) == nullptr,
                    "the bounded synthetic resource has host allocation provenance");
        }
        heap.get().freeAll();
        heap.domain.reset();
        heap.runtime.reset();
        const char* again = nullptr;
        require(resource.use_count() == 1 && survivor.getValue(0, "name", &again) &&
                    again == borrowed && std::string_view(again) == name,
                "a copied original reader retains bytes and string addresses after heap retirement");
        survivor = JMapInfo();
        require(resource.expired(), "the last original reader releases its host resource borrow");
    }

    void test_domain_retirement_releases_parser() {
        Heap heap;
        const auto bytes = fixture();
        std::weak_ptr<const void> data;
        {
            const JKRHeap::CurrentHeapScope original(*(heap.domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            auto* info = new JMapInfo(smgpc::resource::make_jmap_info(bytes));
            data = info->mResourceOwner;
            require(heap.count() == 1 && data.use_count() == 1, "only the actual heap parser owns its native metadata");
        }
        heap.domain.reset();
        require(data.expired(), "actual domain/solid-heap destruction invokes JMap metadata cleanup");
    }

    void test_resource_retirement_before_arena_release() {
        Heap heap;
        const auto bytes = fixture();
        JMapInfo survivor;
        JMapInfo* info;
        std::weak_ptr<const void> data;
        {
            const JKRHeap::CurrentHeapScope original(*heap.domain);
            const aurora::allocation::ClientAllocationScope routing({true, true});
            info = new JMapInfo(smgpc::resource::make_jmap_info(bytes));
            survivor = *info;
            data = info->mResourceOwner;
        }
        const auto used = heap.get().getFreeSize();
        heap.runtime->retireNativeResourceReferences();
        require(!info->dataExists() && registered(heap.get(), info) && heap.count() == 1 &&
                    heap.get().getFreeSize() == used && data.use_count() == 1,
                "retired scene parsers release native resources without destroying their original arena identity");
        const char* value = nullptr;
        require(survivor.getValue(0, "name", &value) && std::string_view(value) == name,
                "an independent live host parser keeps its own resource borrow");
        heap.runtime->retireNativeResourceReferences();
        heap.get().freeAll();
        require(heap.count() == 0 && !data.expired(), "repeat retirement and eventual destruction are safe");
        survivor = JMapInfo();
        require(data.expired(), "only the final independent borrower releases its retained metadata");
    }
}

int main() {
    const std::array tests{
        std::pair{"reusable disposer copy/move identity", test_reusable_identity_base},
        std::pair{"JMap range, tail and explicit deletion", test_range_tail_and_explicit_delete},
        std::pair{"JMap copy/move registration", test_copy_move_registration_and_metadata},
        std::pair{"host resource and original borrowed strings", test_host_resource_and_borrowed_strings},
        std::pair{"actual domain retirement", test_domain_retirement_releases_parser},
        std::pair{"native references before arena retirement", test_resource_retirement_before_arena_release},
    };
    for (const auto& [label, test] : tests) {
        try { test(); std::cout << "PASS " << label << '\n'; }
        catch (const std::exception& error) { std::cerr << "FAIL " << label << ": " << error.what() << '\n'; return 1; }
    }
}
