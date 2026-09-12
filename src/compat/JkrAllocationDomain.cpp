#include <aurora/exception.hpp>
#include <aurora/mem2_arena.hpp>
#include <aurora/guest_thread.hpp>
#include <dolphin/ar.h>
#include "compat/JkrAllocationDomain.hpp"
#include "compat/JkrAllocationRouting.hpp"
#include "compat/JkrAllocationProvenance.hpp"
#include "compat/JkrDiagnostics.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "JSystem/JKernel/JKRExpHeap.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>

namespace smgpc::compat {
    namespace {
        using aurora::allocation::routing_state;
        using aurora::allocation::RoutingState;
        thread_local unsigned allocation_scope_depth;
        thread_local unsigned heap_teardown_depth;
        struct OriginalHeapTeardown {
            RoutingState previous = routing_state;
            OriginalHeapTeardown() { ++heap_teardown_depth; routing_state = {true, true}; }
            ~OriginalHeapTeardown() { routing_state = previous; --heap_teardown_depth; }
        };

        // This lock is the original Game current-heap mutex. Holding it before
        // CurrentHeapRestorer's snapshot also serializes native host threads.
        struct HeapLock {
            HeapLock() { OSLockMutex(&MR::MutexHolder<1>::sMutex); }
            ~HeapLock() { OSUnlockMutex(&MR::MutexHolder<1>::sMutex); }
            HeapLock(const HeapLock&) = delete;
            HeapLock& operator=(const HeapLock&) = delete;
        };

        struct DomainRecord {
            JKRHeap* heap{};
            std::weak_ptr<JkrAllocationDomain> owner;
            DomainRecord* next{};
        };
        DomainRecord* domains;
        std::atomic<std::uintptr_t> arena_begin;
        std::atomic<std::uintptr_t> arena_end;
        std::atomic<std::uintptr_t> mem2_begin;
        std::atomic<std::uintptr_t> mem2_end;

        constexpr std::size_t heap_alignment = 32;
        constexpr std::size_t heap_size_limit = std::numeric_limits<s32>::max();

        std::size_t checked_budget(std::size_t size, std::size_t minimum) {
            if (size < minimum || size > heap_size_limit) {
                aurora::throw_host_exception<std::invalid_argument>("JKR host heap budget is outside the original signed-size range");
            }
            return size & ~(heap_alignment - 1);
        }

        constexpr std::size_t root_header_size =
            (sizeof(JKRExpHeap) + heap_alignment - 1) & ~(heap_alignment - 1);
    }

    struct JkrHeapRuntime::Storage {
        void* arena{};
        void* mem2{};
        JKRExpHeap* root{};
        ~Storage() { std::free(mem2); std::free(arena); }
    };

    std::shared_ptr<JkrHeapRuntime> JkrHeapRuntime::create(std::size_t budget) {
        JkrHostAllocationScope host;
        return std::shared_ptr<JkrHeapRuntime>(new JkrHeapRuntime(budget));
    }

    JkrHeapRuntime::JkrHeapRuntime(std::size_t budget) : _storage(std::make_unique<Storage>()) {
        budget = checked_budget(budget, root_header_size + sizeof(JKRExpHeap::CMemBlock) + heap_alignment);
        HeapLock lock;
        if (JKRHeap::sRootHeap != nullptr || arena_begin.load(std::memory_order_relaxed) != 0) {
            aurora::throw_host_exception<std::logic_error>("An original JKR root heap already exists");
        }
        if (posix_memalign(&_storage->arena, heap_alignment, budget) != 0) {
            throw std::bad_alloc();
        }
        auto* data = static_cast<u8*>(_storage->arena) + root_header_size;
        _storage->root = new (_storage->arena) JKRExpHeap(data, static_cast<u32>(budget - root_header_size), nullptr, false);
        _storage->root->_6E = 1; // The explicit host owner owns this arena.
        JKRHeap::sRootHeap = _storage->root;
        const auto begin = reinterpret_cast<std::uintptr_t>(_storage->arena);
        arena_end.store(begin + budget, std::memory_order_relaxed);
        arena_begin.store(begin, std::memory_order_release);
    }

    JkrHeapRuntime::~JkrHeapRuntime() {
        const aurora::os::GuestThreadExecutionScope execution;
        JkrHostAllocationScope host;
        // A borrowed AR callback may still allocate from the live root. Finish
        // it before taking the heap mutex or destroying its allocation owner.
        if (_storage->mem2 != nullptr) ARReset();
        HeapLock lock;
        if (domains != nullptr || _storage->root->mChildTree.getNumChildren() != 0) {
            jkr_panic(__FILE__, __LINE__, "JKR runtime released with live child heaps");
        }
        {
            OriginalHeapTeardown original;
            _storage->root->~JKRExpHeap();
        }
        JKRHeap::sRootHeap = nullptr;
        JKRHeap::sCurrentHeap = nullptr;
        JKRHeap::sSystemHeap = nullptr;
        arena_begin.store(0, std::memory_order_release);
        arena_end.store(0, std::memory_order_relaxed);
        if (_storage->mem2 != nullptr) {
            aurora::unbind_mem2_arena(_storage->mem2);
            mem2_begin.store(0, std::memory_order_release);
            mem2_end.store(0, std::memory_order_relaxed);
        }
        _storage.reset();
    }

    JKRHeap& JkrHeapRuntime::root_heap() const noexcept { return *_storage->root; }

    void JkrHeapRuntime::prepare_mem2_arena(std::size_t budget) {
        JkrHostAllocationScope host;
        HeapLock lock;
        budget = checked_budget(budget, 0xE00000 + root_header_size + heap_alignment);
        if (_storage->mem2 != nullptr)
            aurora::throw_host_exception<std::logic_error>("The process MEM2 arena is already initialized");
        void* memory = nullptr;
        if (posix_memalign(&memory, heap_alignment, budget) != 0) throw std::bad_alloc();
        try { aurora::bind_mem2_arena(memory, budget); }
        catch (...) { std::free(memory); throw; }
        _storage->mem2 = memory;
        const auto begin = reinterpret_cast<std::uintptr_t>(memory);
        mem2_end.store(begin + budget, std::memory_order_relaxed);
        mem2_begin.store(begin, std::memory_order_release);
    }

    struct JkrAllocationDomain::Storage {
        std::shared_ptr<JkrHeapRuntime> runtime;
        JKRHeap* heap{};
        std::shared_ptr<void> parent_owner;
        bool owns_heap = true;
        DomainRecord record;
    };

    std::shared_ptr<JkrAllocationDomain> JkrAllocationDomain::create(
        std::shared_ptr<JkrHeapRuntime> runtime, std::size_t budget) {
        JkrHostAllocationScope host;
        auto result = std::shared_ptr<JkrAllocationDomain>(new JkrAllocationDomain(std::move(runtime), budget));
        register_owner(result);
        return result;
    }

    void JkrAllocationDomain::register_owner(const std::shared_ptr<JkrAllocationDomain>& result) {
        HeapLock lock;
        auto& record = result->_storage->record;
        record.heap = result->_storage->heap;
        record.owner = result;
        record.next = domains;
        domains = &record;
    }

    std::shared_ptr<JkrAllocationDomain> JkrAllocationDomain::create(
        std::shared_ptr<JkrAllocationDomain> parent, std::size_t budget) {
        JkrHostAllocationScope host;
        auto result = std::shared_ptr<JkrAllocationDomain>(new JkrAllocationDomain(std::move(parent), budget));
        register_owner(result);
        return result;
    }

    std::shared_ptr<JkrAllocationDomain> JkrAllocationDomain::retain_heap(
        std::shared_ptr<JkrHeapRuntime> runtime, JKRHeap& heap, std::shared_ptr<void> heap_owner) {
        JkrHostAllocationScope host;
        HeapLock lock;
        for (auto* record = domains; record != nullptr; record = record->next)
            if (record->heap == &heap) {
                if (auto owner = record->owner.lock()) return owner;
                aurora::throw_host_exception<std::logic_error>("Cannot retain a JKR heap whose owner is retiring");
            }
        auto result = std::shared_ptr<JkrAllocationDomain>(
            new JkrAllocationDomain(std::move(runtime), heap, std::move(heap_owner)));
        register_owner(result);
        return result;
    }

    std::shared_ptr<JkrAllocationDomain> JkrAllocationDomain::retain_heap(
        std::shared_ptr<JkrAllocationDomain> parent, JKRHeap& heap) {
        if (!parent) aurora::throw_host_exception<std::invalid_argument>("A retained subheap requires its actual parent owner");
        auto* ancestor = &heap;
        while (ancestor != nullptr && ancestor != &parent->heap()) ancestor = ancestor->getParent();
        if (ancestor == nullptr)
            aurora::throw_host_exception<std::invalid_argument>("The selected heap is not a child of its retained owner");
        auto runtime = parent->_storage->runtime;
        return retain_heap(std::move(runtime), heap, std::move(parent));
    }

    std::shared_ptr<JkrAllocationDomain> JkrAllocationDomain::retain_heap(JKRHeap& heap) {
        JkrHostAllocationScope host;
        HeapLock lock;
        for (auto* ancestor = &heap; ancestor != nullptr; ancestor = ancestor->getParent()) {
            for (auto* record = domains; record != nullptr; record = record->next) {
                if (record->heap != ancestor) continue;
                auto owner = record->owner.lock();
                if (!owner)
                    aurora::throw_host_exception<std::logic_error>("Cannot retain a JKR heap whose owner is retiring");
                if (ancestor == &heap) return owner;
                return retain_heap(std::move(owner), heap);
            }
        }
        aurora::throw_host_exception<std::logic_error>("The selected JKR heap has no retained owner");
    }

    JkrAllocationDomain::JkrAllocationDomain(std::shared_ptr<JkrHeapRuntime> runtime, std::size_t budget)
        : _storage(std::make_unique<Storage>()) {
        if (!runtime) aurora::throw_host_exception<std::invalid_argument>("A JKR allocation domain requires its actual root owner");
        budget = checked_budget(budget, ((sizeof(JKRSolidHeap) + 31) & ~std::size_t(31)) + heap_alignment);
        _storage->runtime = std::move(runtime);
        HeapLock lock;
        _storage->heap = JKRSolidHeap::create(static_cast<u32>(budget), &_storage->runtime->root_heap(), false);
        if (!_storage->heap) throw std::bad_alloc();
    }

    JkrAllocationDomain::JkrAllocationDomain(std::shared_ptr<JkrAllocationDomain> parent, std::size_t budget)
        : _storage(std::make_unique<Storage>()) {
        if (!parent) aurora::throw_host_exception<std::invalid_argument>("A child JKR domain requires its retained parent");
        budget = checked_budget(budget, ((sizeof(JKRSolidHeap) + 31) & ~std::size_t(31)) + heap_alignment);
        _storage->runtime = parent->_storage->runtime;
        _storage->parent_owner = parent;
        HeapLock lock;
        _storage->heap = JKRSolidHeap::create(static_cast<u32>(budget), &parent->heap(), false);
        if (!_storage->heap) throw std::bad_alloc();
    }

    JkrAllocationDomain::JkrAllocationDomain(std::shared_ptr<JkrHeapRuntime> runtime, JKRHeap& heap,
                                           std::shared_ptr<void> owner)
        : _storage(std::make_unique<Storage>()) {
        if (!runtime || !owner)
            aurora::throw_host_exception<std::invalid_argument>("An external JKR heap requires its actual lifetime owner");
        HeapLock lock;
        auto* root = &heap;
        while (root->getParent() != nullptr) root = root->getParent();
        if (root != &runtime->root_heap())
            aurora::throw_host_exception<std::invalid_argument>("The retained JKR heap belongs to another root");
        for (auto* record = domains; record != nullptr; record = record->next)
            if (record->heap == &heap)
                aurora::throw_host_exception<std::logic_error>("The actual JKR heap already has a retained domain");
        _storage->runtime = std::move(runtime);
        _storage->parent_owner = std::move(owner);
        _storage->heap = &heap;
        _storage->owns_heap = false;
    }

    JkrAllocationDomain::~JkrAllocationDomain() {
        JkrHostAllocationScope host;
        {
            HeapLock lock;
            // Original heap destruction never selects a different current heap.
            // Disposer allocations therefore follow the existing original selection;
            // the base destructor may update that selection as it unlinks the heap.
            {
                OriginalHeapTeardown original;
                if (_storage->owns_heap) _storage->heap->destroy();
            }
            for (auto** record = &domains; *record != nullptr; record = &(*record)->next) {
                if (*record == &_storage->record) {
                    *record = (*record)->next;
                    break;
                }
            }
        }
        // A retained parent may reach its final release here. Its callback
        // retirement must be able to wait without inheriting our heap lock.
        _storage.reset();
    }

    JKRHeap& JkrAllocationDomain::heap() const noexcept { return *_storage->heap; }

    struct JkrAllocationScope::Storage {
        std::shared_ptr<JkrAllocationDomain> domain;
        std::shared_ptr<JkrAllocationDomain> previous_domain;
        RoutingState previous_routing;
        HeapLock lock;
        std::optional<MR::CurrentHeapRestorer> restore;

        Storage(std::shared_ptr<JkrAllocationDomain> owner, RoutingState previous)
            : domain(std::move(owner)), previous_routing(previous) {
            if (!domain) aurora::throw_host_exception<std::invalid_argument>("A JKR allocation scope requires a retained domain");
            for (auto* heap = JKRHeap::sCurrentHeap; heap != nullptr && !previous_domain; heap = heap->getParent()) {
                for (auto* record = domains; record != nullptr; record = record->next) {
                    if (record->heap == heap) {
                        previous_domain = record->owner.lock();
                        break;
                    }
                }
            }
            restore.emplace(&domain->heap());
        }
        ~Storage() { restore.reset(); }
    };

    JkrAllocationScope::JkrAllocationScope(std::shared_ptr<JkrAllocationDomain> domain) {
        const RoutingState previous = routing_state;
        {
            JkrHostAllocationScope host;
            _storage = std::make_unique<Storage>(std::move(domain), previous);
        }
        ++allocation_scope_depth;
        routing_state = {true, true};
    }

    JkrAllocationScope::~JkrAllocationScope() {
        const RoutingState previous = _storage->previous_routing;
        routing_state.guest = false;
        --allocation_scope_depth;
        _storage.reset();
        routing_state = previous;
    }

    std::shared_ptr<JkrAllocationDomain> current_jkr_allocation_domain() noexcept {
        if (allocation_scope_depth == 0 && heap_teardown_depth == 0 && !routing_state.callbackGuest) return {};
        // Actual SDK workers have guest routing without a host allocation
        // scope. Take only the lookup lock; never hold it across OS waits.
        HeapLock lock;
        for (auto* heap = JKRHeap::sCurrentHeap; heap != nullptr; heap = heap->getParent()) {
            for (auto* record = domains; record != nullptr; record = record->next) {
                if (record->heap == heap) {
                    auto owner = record->owner.lock();
                    if (owner) return owner;
                    break;
                }
            }
        }
        if (heap_teardown_depth != 0) {
            jkr_panic(__FILE__, __LINE__, "Cannot retain a new resource in a heap whose last owner is releasing");
        }
        return {};
    }

    namespace detail {
        void* allocate_jkr_or_host(std::size_t size, std::size_t alignment,
                                  bool from_tail, JKRHeap* explicit_heap) {
            if (alignment == 0 || (alignment & (alignment - 1)) != 0) throw std::bad_alloc();
            if (explicit_heap != nullptr || routing_state.guest) {
                if (size > heap_size_limit || alignment > heap_size_limit) return nullptr;
                HeapLock lock;
                JKRHeap* heap = explicit_heap != nullptr ? explicit_heap : JKRHeap::sCurrentHeap;
                if (heap == nullptr) jkr_panic(__FILE__, __LINE__, "Original allocation has no current JKR heap");
                // C++ objects require native alignment even where an original
                // placement-new spelling requested only four bytes.
                alignment = alignment < alignof(void*) ? alignof(void*) : alignment;
                const int signed_alignment = from_tail ? -static_cast<int>(alignment) : static_cast<int>(alignment);
                return heap->alloc(static_cast<u32>(size), signed_alignment);
            }
            alignment = alignment < alignof(void*) ? alignof(void*) : alignment;
            void* result = nullptr;
            if (posix_memalign(&result, alignment, size == 0 ? 1 : size) != 0) return nullptr;
            return result;
        }

        void deallocate_jkr_or_host(void* memory) noexcept {
            if (memory == nullptr) return;
            if (JKRHeap* heap = forget_jkr_allocation(memory)) {
                // The provenance lock has been released. The original heap's
                // own mutex serializes its free; no global Game lock is needed.
                heap->free(memory);
                return;
            }
            const auto address = reinterpret_cast<std::uintptr_t>(memory);
            const auto begin = arena_begin.load(std::memory_order_acquire);
            if (begin != 0 && begin <= address && address < arena_end.load(std::memory_order_relaxed)) {
                jkr_panic(__FILE__, __LINE__, "Delete has no original allocation provenance: %p", memory);
            }
            const auto mem2 = mem2_begin.load(std::memory_order_acquire);
            if (mem2 != 0 && mem2 <= address && address < mem2_end.load(std::memory_order_relaxed)) {
                jkr_panic(__FILE__, __LINE__, "Delete has no original MEM2 allocation provenance: %p", memory);
            }
            std::free(memory);
        }
    }
}
