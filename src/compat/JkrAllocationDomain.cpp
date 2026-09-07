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
        thread_local bool allocation_routing;
        thread_local unsigned allocation_scope_depth;
        thread_local unsigned heap_teardown_depth;
        struct OriginalHeapTeardown {
            bool previous = allocation_routing;
            OriginalHeapTeardown() { ++heap_teardown_depth; allocation_routing = true; }
            ~OriginalHeapTeardown() { allocation_routing = previous; --heap_teardown_depth; }
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

        constexpr std::size_t heap_alignment = 32;
        constexpr std::size_t heap_size_limit = std::numeric_limits<s32>::max();

        std::size_t checked_budget(std::size_t size, std::size_t minimum) {
            if (size < minimum || size > heap_size_limit) {
                throw std::invalid_argument("JKR host heap budget is outside the original signed-size range");
            }
            return size & ~(heap_alignment - 1);
        }

        constexpr std::size_t root_header_size =
            (sizeof(JKRExpHeap) + heap_alignment - 1) & ~(heap_alignment - 1);
    }

    struct JkrHeapRuntime::Storage {
        void* arena{};
        JKRExpHeap* root{};
        ~Storage() { std::free(arena); }
    };

    std::shared_ptr<JkrHeapRuntime> JkrHeapRuntime::create(std::size_t budget) {
        JkrHostAllocationScope host;
        return std::shared_ptr<JkrHeapRuntime>(new JkrHeapRuntime(budget));
    }

    JkrHeapRuntime::JkrHeapRuntime(std::size_t budget) : _storage(std::make_unique<Storage>()) {
        budget = checked_budget(budget, root_header_size + sizeof(JKRExpHeap::CMemBlock) + heap_alignment);
        HeapLock lock;
        if (JKRHeap::sRootHeap != nullptr || arena_begin.load(std::memory_order_relaxed) != 0) {
            throw std::logic_error("An original JKR root heap already exists");
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
        JkrHostAllocationScope host;
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
        _storage.reset();
    }

    JKRHeap& JkrHeapRuntime::root_heap() const noexcept { return *_storage->root; }

    struct JkrAllocationDomain::Storage {
        std::shared_ptr<JkrHeapRuntime> runtime;
        JKRSolidHeap* heap{};
        DomainRecord record;
    };

    std::shared_ptr<JkrAllocationDomain> JkrAllocationDomain::create(
        std::shared_ptr<JkrHeapRuntime> runtime, std::size_t budget) {
        JkrHostAllocationScope host;
        auto result = std::shared_ptr<JkrAllocationDomain>(new JkrAllocationDomain(std::move(runtime), budget));
        HeapLock lock;
        auto& record = result->_storage->record;
        record.heap = result->_storage->heap;
        record.owner = result;
        record.next = domains;
        domains = &record;
        return result;
    }

    JkrAllocationDomain::JkrAllocationDomain(std::shared_ptr<JkrHeapRuntime> runtime, std::size_t budget)
        : _storage(std::make_unique<Storage>()) {
        if (!runtime) throw std::invalid_argument("A JKR allocation domain requires its actual root owner");
        budget = checked_budget(budget, ((sizeof(JKRSolidHeap) + 31) & ~std::size_t(31)) + heap_alignment);
        _storage->runtime = std::move(runtime);
        HeapLock lock;
        _storage->heap = JKRSolidHeap::create(static_cast<u32>(budget), &_storage->runtime->root_heap(), false);
        if (!_storage->heap) throw std::bad_alloc();
    }

    JkrAllocationDomain::~JkrAllocationDomain() {
        JkrHostAllocationScope host;
        HeapLock lock;
        // Original heap destruction never selects a different current heap.
        // Disposer allocations therefore follow the existing original selection;
        // the base destructor may update that selection as it unlinks the heap.
        {
            OriginalHeapTeardown original;
            _storage->heap->destroy();
        }
        for (auto** record = &domains; *record != nullptr; record = &(*record)->next) {
            if (*record == &_storage->record) {
                *record = (*record)->next;
                break;
            }
        }
        _storage.reset();
    }

    JKRHeap& JkrAllocationDomain::heap() const noexcept { return *_storage->heap; }

    struct JkrAllocationScope::Storage {
        std::shared_ptr<JkrAllocationDomain> domain;
        std::shared_ptr<JkrAllocationDomain> previous_domain;
        bool previous_routing;
        HeapLock lock;
        std::optional<MR::CurrentHeapRestorer> restore;

        Storage(std::shared_ptr<JkrAllocationDomain> owner, bool previous)
            : domain(std::move(owner)), previous_routing(previous) {
            if (!domain) throw std::invalid_argument("A JKR allocation scope requires a retained domain");
            for (auto* record = domains; record != nullptr; record = record->next) {
                if (record->heap == JKRHeap::sCurrentHeap) {
                    previous_domain = record->owner.lock();
                    break;
                }
            }
            restore.emplace(&domain->heap());
        }
        ~Storage() { restore.reset(); }
    };

    JkrAllocationScope::JkrAllocationScope(std::shared_ptr<JkrAllocationDomain> domain) {
        const bool previous = allocation_routing;
        {
            JkrHostAllocationScope host;
            _storage = std::make_unique<Storage>(std::move(domain), previous);
        }
        ++allocation_scope_depth;
        allocation_routing = true;
    }

    JkrAllocationScope::~JkrAllocationScope() {
        const bool previous = _storage->previous_routing;
        allocation_routing = false;
        --allocation_scope_depth;
        _storage.reset();
        allocation_routing = previous;
    }

    JkrHostAllocationScope::JkrHostAllocationScope() noexcept : _previous_routing(allocation_routing) {
        allocation_routing = false;
    }
    JkrHostAllocationScope::~JkrHostAllocationScope() { allocation_routing = _previous_routing; }

    std::shared_ptr<JkrAllocationDomain> current_jkr_allocation_domain() noexcept {
        if (allocation_scope_depth == 0 && heap_teardown_depth == 0) return {};
        // A live scope/teardown holds the original mutex, including host escapes.
        for (auto* record = domains; record != nullptr; record = record->next) {
            if (record->heap == JKRHeap::sCurrentHeap) {
                auto owner = record->owner.lock();
                if (owner) return owner;
                break;
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
            if (explicit_heap != nullptr || allocation_routing) {
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
            std::free(memory);
        }
    }
}
