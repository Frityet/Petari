#include "compat/JkrAllocationProvenance.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <mutex>

namespace smgpc::compat::detail {
    namespace {
        struct Record {
            void* memory;
            JKRHeap* heap;
            Record* next;
            bool from_tail;
        };
        constexpr std::size_t bucket_count = 2048;
        Record* buckets[bucket_count];
        std::mutex& mutex() {
            // Global delete remains callable after other static destructors.
            // This tiny process synchronization owner intentionally persists;
            // constructing it never re-enters an ordinary operator new.
            static std::mutex* instance = [] {
                void* memory = std::malloc(sizeof(std::mutex));
                if (memory == nullptr) std::abort();
                return new (memory) std::mutex;
            }();
            return *instance;
        }

        std::size_t bucket(void* memory) {
            auto value = reinterpret_cast<std::uintptr_t>(memory) >> 3;
            value ^= value >> 17;
            return value & (bucket_count - 1);
        }
        struct Lock {
            Lock() { mutex().lock(); }
            ~Lock() { mutex().unlock(); }
        };
    }

    void record_jkr_allocation(void* memory, JKRHeap* heap, int alignment) {
        if (memory == nullptr) return;
        auto* fresh = static_cast<Record*>(std::malloc(sizeof(Record)));
        if (fresh == nullptr) {
            heap->free(memory);
            throw std::bad_alloc();
        }
        *fresh = {memory, heap, nullptr, alignment < 0};
        Lock lock;
        auto** slot = &buckets[bucket(memory)];
        for (auto* record = *slot; record != nullptr; record = record->next) {
            if (record->memory == memory) {
                // Original bulk-free operations may reclaim a pointer without
                // an individual free. A new allocation supersedes that record.
                record->heap = heap;
                record->from_tail = alignment < 0;
                std::free(fresh);
                return;
            }
        }
        fresh->next = *slot;
        *slot = fresh;
    }

    JKRHeap* forget_jkr_allocation(void* memory) noexcept {
        if (memory == nullptr) return nullptr;
        Lock lock;
        for (auto** slot = &buckets[bucket(memory)]; *slot != nullptr; slot = &(*slot)->next) {
            auto* record = *slot;
            if (record->memory == memory) {
                auto* heap = record->heap;
                *slot = record->next;
                std::free(record);
                return heap;
            }
        }
        return nullptr;
    }

    void retire_jkr_heap_allocations(JKRHeap* heap, bool tail_only) noexcept {
        Lock lock;
        for (auto& head : buckets) {
            auto** slot = &head;
            while (*slot != nullptr) {
                auto* record = *slot;
                if (record->heap == heap && (!tail_only || record->from_tail)) {
                    *slot = record->next;
                    std::free(record);
                } else {
                    slot = &record->next;
                }
            }
        }
    }
}
