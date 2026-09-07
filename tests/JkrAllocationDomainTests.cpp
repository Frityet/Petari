#include "compat/JkrAllocationDomain.hpp"
#include "Game/Util/MemoryUtil.hpp"
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
    using namespace smgpc::compat;
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
        auto runtime = JkrHeapRuntime::create(1 << 20);
        auto& heap = static_cast<JKRExpHeap&>(runtime->root_heap());
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

    void test_domain_bulk_and_typed_lifetime() {
        auto runtime = JkrHeapRuntime::create(1 << 20);
        const auto initial = runtime->root_heap().getTotalFreeSize();
        auto domain = JkrAllocationDomain::create(runtime, 16384);
        int destroyed = 0;
        Tracked* explicit_value;
        {
            JkrAllocationScope scope(domain);
            explicit_value = new Tracked(destroyed);
            new Tracked(destroyed); // Original disposer list owns this destructor.
            new std::byte[257]; // Original Solid bulk release owns discarded allocation.
            auto* aligned = new Aligned;
            require(std::uintptr_t(aligned) % 128 == 0 && JKRHeap::findFromRoot(aligned) == &domain->heap(),
                    "ordinary over-aligned new selects the actual domain");
            delete aligned;
        }
        require(destroyed == 0, "leaving allocation scope retains original objects");
        delete explicit_value;
        require(destroyed == 1, "ordinary delete outside scope uses original allocation provenance");
        runtime.reset();
        require(domain->heap().check(), "domain retains the original root owner");
        domain.reset();
        require(destroyed == 2 && JKRHeap::sRootHeap == nullptr, "last domain release disposes and releases its root");
        auto replacement = JkrHeapRuntime::create(1 << 20);
        require(replacement->root_heap().getTotalFreeSize() == initial, "released root can be recreated with the same capacity");
    }

    void test_scope_selection_and_host_escape() {
        auto runtime = JkrHeapRuntime::create(1 << 20);
        auto a = JkrAllocationDomain::create(runtime, 16384);
        auto b = JkrAllocationDomain::create(runtime, 16384);
        require(!current_jkr_allocation_domain(), "no domain is published outside a Game allocation scope");
        std::vector<std::uint64_t> host_values;
        void* root_before = JKRHeap::sCurrentHeap;
        {
            JkrAllocationScope scope(a);
            require(current_jkr_allocation_domain() == a, "Game scope selects its actual heap");
            auto* av = new int(17);
            {
                MR::CurrentHeapRestorer original_select(&b->heap());
                require(current_jkr_allocation_domain() == b, "original heap selector changes the published domain");
                auto* bv = new int(29);
                require(JKRHeap::findFromRoot(bv) == &b->heap(), "original selector controls ordinary new");
                {
                    JkrHostAllocationScope host;
                    host_values.resize(1000, 0x1234);
                    require(!JKRHeap::findFromRoot(host_values.data()) && current_jkr_allocation_domain() == b,
                            "native metadata escapes allocation while retaining observable original heap");
                    {
                        JkrAllocationScope nested(a);
                        auto* value = new int(31);
                        require(JKRHeap::findFromRoot(value) == &a->heap(), "nested Game scope re-enters from host escape");
                        delete value;
                    }
                    auto* escaped = new int(43);
                    require(!JKRHeap::findFromRoot(escaped), "nested Game scope restores host allocation escape");
                    delete escaped;
                }
                delete bv;
            }
            require(JKRHeap::sCurrentHeap == &a->heap(), "original restoration returns to enclosing selected heap");
            delete av;
        }
        require(JKRHeap::sCurrentHeap == root_before && !current_jkr_allocation_domain(), "native scope restores original selection");
        a.reset(); b.reset(); runtime.reset();
        require(host_values[999] == 0x1234, "native metadata survives original heap teardown");
    }

    void test_external_explicit_and_placement() {
        auto runtime = JkrHeapRuntime::create(1 << 20);
        alignas(128) std::array<u8, 8192> external{};
        JKRSolidHeap external_heap(external.data(), external.size(), &runtime->root_heap(), false);
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
        auto runtime = JkrHeapRuntime::create(1 << 20);
        const auto capacity = runtime->root_heap().getTotalFreeSize();
        auto a = JkrAllocationDomain::create(runtime, 4096);
        auto b = JkrAllocationDomain::create(runtime, 4096);
        JKRHeap* observed = nullptr;
        {
            JkrAllocationScope scope(a);
            new AllocatingDisposer(observed);
        }
        runtime->root_heap().becomeCurrentHeap();
        {
            JkrAllocationScope scope(b);
            a.reset();
            require(observed == &b->heap(), "disposer allocation preserves the actual original current heap");
        }
        auto previous = JkrAllocationDomain::create(runtime, 4096);
        std::weak_ptr<JkrAllocationDomain> previous_lifetime = previous;
        previous->heap().becomeCurrentHeap();
        {
            JkrAllocationScope scope(b);
            previous.reset();
            require(!previous_lifetime.expired(), "native scope retains the original heap it will restore");
        }
        require(previous_lifetime.expired(), "restored heap can release after its restoration scope ends");
        b.reset();
        require(runtime->root_heap().getTotalFreeSize() == capacity,
                "teardown metadata and transient disposer allocation do not escape retained heaps");
    }

    void test_failure_restore_and_root_budget() {
        auto runtime = JkrHeapRuntime::create(1 << 20);
        bool duplicate_rejected = false;
        try { auto duplicate = JkrHeapRuntime::create(1 << 20); } catch (const std::logic_error&) { duplicate_rejected = true; }
        require(duplicate_rejected, "a second authoritative root is rejected");
        auto domain = JkrAllocationDomain::create(runtime, 2048);
        bool exhausted = false;
        JKRHeap* before = JKRHeap::sCurrentHeap;
        try {
            JkrAllocationScope scope(domain);
            (void)::operator new(1 << 20);
        } catch (const std::bad_alloc&) { exhausted = true; }
        require(exhausted && JKRHeap::sCurrentHeap == before && !current_jkr_allocation_domain(),
                "native allocation failure restores original current heap and routing");
        auto* host = new int(11);
        require(!JKRHeap::findFromRoot(host), "subsequent ordinary allocation remains on host");
        delete host;
    }

    void test_native_threads_share_original_mutex() {
        auto runtime = JkrHeapRuntime::create(1 << 20);
        auto a = JkrAllocationDomain::create(runtime, 65536);
        auto b = JkrAllocationDomain::create(runtime, 65536);
        std::atomic<unsigned> ready{0};
        std::atomic<bool> okay{true};
        auto worker = [&](const std::shared_ptr<JkrAllocationDomain>& domain) {
            ready.fetch_add(1);
            while (ready.load() != 2) std::this_thread::yield();
            for (unsigned n = 0; n < 64; ++n) {
                JkrAllocationScope scope(domain);
                auto* value = new std::uint64_t(n);
                if (current_jkr_allocation_domain() != domain || JKRHeap::findFromRoot(value) != &domain->heap() || *value != n) okay = false;
                delete value;
            }
        };
        std::thread first(worker, std::cref(a)), second(worker, std::cref(b));
        first.join(); second.join();
        require(okay && a->heap().check() && b->heap().check(), "native threads serialize original heap selection and allocation");
    }
}

int main() {
    try {
        test_exp_reclaim_and_resize();
        test_domain_bulk_and_typed_lifetime();
        test_scope_selection_and_host_escape();
        test_external_explicit_and_placement();
        test_disposer_allocations_preserve_current_heap();
        test_failure_restore_and_root_budget();
        test_native_threads_share_original_mutex();
        std::cout << "[pass] 7 retained JKR allocation domain groups\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
