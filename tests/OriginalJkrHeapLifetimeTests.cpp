#include "NativeHeapFixture.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "JSystem/JKernel/JKRExpHeap.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    struct Tracked final : JKRDisposer {
        int& destroyed;
        explicit Tracked(int& counter) : destroyed(counter) {}
        ~Tracked() override { ++destroyed; }
    };
    struct alignas(128) Aligned { std::array<std::uint64_t, 16> words; };

    void test_exp_reclaim_and_resize() {
        auto runtime = smgpc::test::create_native_root_heap(1 << 20);
        auto& heap = static_cast<JKRExpHeap&>((*runtime));
        const auto initial = heap.getTotalFreeSize();
        auto* a = static_cast<u8*>(heap.alloc(19, 4));
        auto* b = static_cast<u8*>(heap.alloc(41, 64));
        auto* c = static_cast<u8*>(heap.alloc(35, -128));
        require(a && b && c && heap.check(), "original Exp allocations preserve complete block accounting");
        require(std::uintptr_t(a) % alignof(void*) == 0 && std::uintptr_t(b) % 64 == 0 &&
                    std::uintptr_t(c) % 128 == 0, "both directions preserve full native pointer alignment");
        a[0] = 0x12;
        a[18] = 0x34;
        heap.free(b);
        require(heap.resize(a, 160) >= 160 && a[0] == 0x12 && a[18] == 0x34,
                "resize merges the adjacent free block and preserves existing bytes");
        require(heap.resize(a, 16) >= 16 && heap.check(), "shrinking returns a split free block");
        heap.free(c);
        heap.free(a);
        require(heap.isEmpty() && heap.getTotalFreeSize() == initial && heap.check(),
                "nonsequential frees coalesce to original capacity");
        JKRExpHeap* child = JKRExpHeap::create(8192, &heap, false);
        require(child != nullptr, "actual Exp child construction");
        void* retained = child->alloc(200, 16);
        const auto before = child->mSize;
        const auto adjusted = child->adjustSize();
        require(adjusted > 0 && child->mSize < before && child->find(retained) == child && child->check(),
                "retail adjustSize keeps live allocation and returns parent capacity");
        child->destroy();
        require(heap.getTotalFreeSize() == initial && heap.check(), "Exp child destruction returns storage to parent");
    }

    void test_heap_bulk_and_typed_lifetime() {
        auto runtime = smgpc::test::create_native_root_heap(1 << 20);
        const auto initial = (*runtime).getTotalFreeSize();
        auto domain = smgpc::test::create_native_solid_heap(runtime, 16384);
        int destroyed = 0;
        Tracked* explicit_value;
        {
            const JKRHeap::CurrentHeapScope scope(*(domain));
            const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
            explicit_value = new Tracked(destroyed);
            new Tracked(destroyed); // Original disposer list owns this destructor.
            new std::byte[257]; // Original Solid bulk release owns discarded allocation.
            auto* aligned = new Aligned;
            require(std::uintptr_t(aligned) % 128 == 0 && JKRHeap::findFromRoot(aligned) == &(*domain),
                    "ordinary over-aligned new selects the actual domain");
            delete aligned;
        }
        require(destroyed == 0, "leaving allocation scope retains original objects");
        delete explicit_value;
        require(destroyed == 1, "ordinary delete outside scope uses original allocation provenance");
        runtime.reset();
        require((*domain).check(), "domain retains the original root owner");
        domain.reset();
        require(destroyed == 2 && JKRHeap::sRootHeap == nullptr, "last domain release disposes and releases its root");
        auto replacement = smgpc::test::create_native_root_heap(1 << 20);
        require((*replacement).getTotalFreeSize() == initial, "released root can be recreated with the same capacity");
    }

    void test_scope_selection_and_host_escape() {
        auto runtime = smgpc::test::create_native_root_heap(1 << 20);
        auto a = smgpc::test::create_native_solid_heap(runtime, 16384);
        auto b = smgpc::test::create_native_solid_heap(runtime, 16384);
        require(&MR::MutexHolder<1>::sMutex == &JKRHeap::sCurrentHeapMutex,
                "original Game and J3D wrappers alias the SDK current-heap mutex");
        require(!JKRHeap::retainCurrentNativeLifetime(), "no domain is published outside a Game allocation scope");
        std::vector<std::uint64_t> host_values;
        void* root_before = JKRHeap::sCurrentHeap;
        {
            const JKRHeap::CurrentHeapScope scope(*(a));
            const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
            require(JKRHeap::retainCurrentNativeLifetime() == a, "Game scope selects its actual heap");
            auto* av = new int(17);
            {
                MR::CurrentHeapRestorer original_select(&(*b));
                require(JKRHeap::retainCurrentNativeLifetime() == b, "original heap selector changes the published domain");
                auto* bv = new int(29);
                require(JKRHeap::findFromRoot(bv) == &(*b), "original selector controls ordinary new");
                try {
                    JKRHeap::CurrentHeapScope sdk_select((*a));
                    auto* sdk_value = new int(37);
                    require(JKRHeap::retainCurrentNativeLifetime() == a && JKRHeap::findFromRoot(sdk_value) == &(*a),
                            "SDK heap scope nests inside the original MR heap restorer");
                    delete sdk_value;
                    throw 37;
                } catch (int value) {
                    require(value == 37 && JKRHeap::sCurrentHeap == &(*b),
                            "SDK scope restores the original selected heap while unwinding");
                }
                {
                    aurora::allocation::HostAllocationScope host;
                    host_values.resize(1000, 0x1234);
                    require(!JKRHeap::findFromRoot(host_values.data()) && JKRHeap::retainCurrentNativeLifetime() == b,
                            "native metadata escapes allocation while retaining observable original heap");
                    {
                        const JKRHeap::CurrentHeapScope nested(*(a));
                        const aurora::allocation::ClientAllocationScope nestedRouting({true, true});
                        auto* value = new int(31);
                        require(JKRHeap::findFromRoot(value) == &(*a), "nested Game scope re-enters from host escape");
                        delete value;
                    }
                    auto* escaped = new int(43);
                    require(!JKRHeap::findFromRoot(escaped), "nested Game scope restores host allocation escape");
                    delete escaped;
                }
                delete bv;
            }
            require(JKRHeap::sCurrentHeap == &(*a), "original restoration returns to enclosing selected heap");
            delete av;
        }
        require(JKRHeap::sCurrentHeap == root_before && !JKRHeap::retainCurrentNativeLifetime(), "native scope restores original selection");
        a.reset(); b.reset(); runtime.reset();
        require(host_values[999] == 0x1234, "native metadata survives original heap teardown");
    }

    void test_external_explicit_and_placement() {
        auto runtime = smgpc::test::create_native_root_heap(1 << 20);
        alignas(128) std::array<u8, 8192> external{};
        JKRSolidHeap external_heap(external.data(), external.size(), &(*runtime), false);
        const auto initial = external_heap.getFreeSize();
        int destroyed = 0;
        auto* explicit_value = new (&external_heap, -64) Tracked(destroyed);
        require(std::uintptr_t(explicit_value) % 64 == 0 && explicit_value->mHeap == &external_heap,
                "explicit tail heap new honors actual external-buffer owner");
        auto* placed = new (external_heap.alloc(sizeof(Tracked), 16)) Tracked(destroyed);
        const auto remaining = external_heap.getFreeSize();
        delete explicit_value;
        delete placed;
        require(destroyed == 2 && external_heap.getFreeSize() == remaining,
                "ordinary delete invokes actual external Solid free without freeing host pointers");
        external_heap.freeAll();
        require(external_heap.getFreeSize() == initial, "external heap bulk release retires its provenance");
        auto* reused = new (&external_heap, 16) Tracked(destroyed);
        delete reused;
        require(destroyed == 3, "bulk-reused address receives new allocation provenance");
    }

    void test_disposer_allocations_preserve_current_heap() {
        struct AllocatingDisposer final : JKRDisposer {
            JKRHeap*& observed;
            explicit AllocatingDisposer(JKRHeap*& result) : observed(result) {}
            ~AllocatingDisposer() override {
                auto* scratch = new std::byte[32];
                observed = JKRHeap::findFromRoot(scratch);
                delete[] scratch;
            }
        };
        auto runtime = smgpc::test::create_native_root_heap(1 << 20);
        const auto capacity = (*runtime).getTotalFreeSize();
        auto a = smgpc::test::create_native_solid_heap(runtime, 4096);
        auto b = smgpc::test::create_native_solid_heap(runtime, 4096);
        JKRHeap* observed = nullptr;
        {
            const JKRHeap::CurrentHeapScope scope(*(a));
            const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
            new AllocatingDisposer(observed);
        }
        (*runtime).becomeCurrentHeap();
        {
            const JKRHeap::CurrentHeapScope scope(*(b));
            const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
            a.reset();
            require(observed == &(*b), "disposer allocation preserves the actual original current heap");
        }
        auto previous = smgpc::test::create_native_solid_heap(runtime, 4096);
        std::weak_ptr<JKRHeap> previous_lifetime = previous;
        (*previous).becomeCurrentHeap();
        {
            const JKRHeap::CurrentHeapScope scope(*(b));
            const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
            previous.reset();
            require(!previous_lifetime.expired(), "native scope retains the original heap it will restore");
        }
        require(previous_lifetime.expired(), "restored heap can release after its restoration scope ends");
        b.reset();
        require((*runtime).getTotalFreeSize() == capacity,
                "teardown metadata and transient disposer allocation do not escape retained heaps");
    }

    void test_failure_restore_and_root_budget() {
        auto runtime = smgpc::test::create_native_root_heap(1 << 20);
        bool duplicate_rejected = false;
        try { auto duplicate = smgpc::test::create_native_root_heap(1 << 20); } catch (const std::logic_error&) { duplicate_rejected = true; }
        require(duplicate_rejected, "a second authoritative root is rejected");
        auto domain = smgpc::test::create_native_solid_heap(runtime, 2048);
        bool exhausted = false;
        JKRHeap* before = JKRHeap::sCurrentHeap;
        try {
            const JKRHeap::CurrentHeapScope scope(*(domain));
            const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
            (void)::operator new(1 << 20);
        } catch (const std::bad_alloc&) { exhausted = true; }
        require(exhausted && JKRHeap::sCurrentHeap == before && !JKRHeap::retainCurrentNativeLifetime(),
                "native allocation failure restores original current heap and routing");
        auto* host = new int(11);
        require(!JKRHeap::findFromRoot(host), "subsequent ordinary allocation remains on host");
        delete host;
    }

    void test_native_threads_share_original_mutex() {
        auto runtime = smgpc::test::create_native_root_heap(1 << 20);
        auto a = smgpc::test::create_native_solid_heap(runtime, 65536);
        auto b = smgpc::test::create_native_solid_heap(runtime, 65536);
        std::atomic<unsigned> ready{0};
        std::atomic<bool> okay{true};
        auto worker = [&](const JKRHeap::Handle& domain) {
            ready.fetch_add(1);
            while (ready.load() != 2) std::this_thread::yield();
            for (unsigned n = 0; n < 64; ++n) {
                const JKRHeap::CurrentHeapScope scope(*(domain));
                const aurora::allocation::ClientAllocationScope scopeRouting({true, true});
                auto* value = new std::uint64_t(n);
                if (JKRHeap::retainCurrentNativeLifetime() != domain || JKRHeap::findFromRoot(value) != &(*domain) || *value != n) okay = false;
                delete value;
            }
        };
        std::thread first(worker, std::cref(a)), second(worker, std::cref(b));
        first.join(); second.join();
        require(okay && (*a).check() && (*b).check(), "native threads serialize original heap selection and allocation");
    }
}

int main() {
    try {
        test_exp_reclaim_and_resize();
        test_heap_bulk_and_typed_lifetime();
        test_scope_selection_and_host_escape();
        test_external_explicit_and_placement();
        test_disposer_allocations_preserve_current_heap();
        test_failure_restore_and_root_budget();
        test_native_threads_share_original_mutex();
        std::cout << "[pass] 7 retained JKR heap lifetime groups\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
