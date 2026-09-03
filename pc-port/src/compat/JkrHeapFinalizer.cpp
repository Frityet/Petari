#include "compat/JkrHeapFinalizer.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <atomic>
#include <cstdlib>
#include <mutex>
#include <new>
#include <stdexcept>

namespace smgpc::compat {
    namespace {
        struct Record {
            void* object;
            JKRHeap* heap;
            void (*finalize)(void*) noexcept;
            Record* next;
        };
        Record* records;
        std::atomic<std::size_t> record_count;
        std::mutex& mutex() {
            // Native SDK destruction remains legal during process teardown.
            // This synchronization object never uses a Game allocation.
            static auto* instance = [] {
                void* memory = std::malloc(sizeof(std::mutex));
                if (!memory) std::abort();
                return new (memory) std::mutex;
            }();
            return *instance;
        }
        struct HeapLock {
            JKRHeap& heap;
            explicit HeapLock(JKRHeap& value) : heap(value) { heap.lock(); }
            ~HeapLock() { heap.unlock(); }
        };
        void finalize(JKRHeap* heap, std::uintptr_t begin, std::uintptr_t end, bool entire_heap) noexcept {
            JkrHostAllocationScope host;
            for (;;) {
                if (record_count.load(std::memory_order_acquire) == 0) return;
                Record* removed = nullptr;
                {
                    std::lock_guard lock(mutex());
                    for (auto** slot = &records; *slot; slot = &(*slot)->next) {
                        auto* record = *slot;
                        auto address = reinterpret_cast<std::uintptr_t>(record->object);
                        if (record->heap == heap && (entire_heap || (begin <= address && address < end))) {
                            removed = record;
                            *slot = record->next;
                            record_count.fetch_sub(1, std::memory_order_release);
                            break;
                        }
                    }
                }
                if (!removed) return;
                auto* object = removed->object;
                auto callback = removed->finalize;
                std::free(removed);
                // Re-scan after each callback: it can destroy and unregister
                // other objects. Never hold the registry lock across GX drain.
                callback(object);
            }
        }
    }
    void register_jkr_heap_finalizer(void* object, void (*callback)(void*) noexcept) {
        JkrHostAllocationScope host;
        if (!object || !callback) throw std::invalid_argument("Heap finalizer requires an object and callback");
        auto* heap = JKRHeap::findFromRoot(object);
        if (!heap) return;
        HeapLock heap_lock(*heap);
        auto* record = static_cast<Record*>(std::malloc(sizeof(Record)));
        if (!record) throw std::bad_alloc();
        *record = {object, heap, callback, nullptr};
        std::lock_guard lock(mutex());
        for (auto* current = records; current; current = current->next) {
            if (current->object == object) {
                std::free(record);
                throw std::logic_error("Object already has a JKR heap finalizer");
            }
        }
        record->next = records;
        records = record;
        record_count.fetch_add(1, std::memory_order_release);
    }
    void unregister_jkr_heap_finalizer(void* object) noexcept {
        if (record_count.load(std::memory_order_acquire) == 0) return;
        std::lock_guard lock(mutex());
        for (auto** slot = &records; *slot; slot = &(*slot)->next) {
            if ((*slot)->object == object) {
                auto* removed = *slot;
                *slot = removed->next;
                record_count.fetch_sub(1, std::memory_order_release);
                std::free(removed);
                return;
            }
        }
    }
    namespace detail {
        void finalize_jkr_heap_objects(JKRHeap* heap) noexcept { finalize(heap, 0, 0, true); }
        void finalize_jkr_heap_objects(JKRHeap* heap, std::uintptr_t begin, std::uintptr_t end) noexcept {
            finalize(heap, begin, end, false);
        }
    }
}
