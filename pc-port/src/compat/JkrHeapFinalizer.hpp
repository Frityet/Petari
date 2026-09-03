#pragma once
#include <cstdint>
class JKRHeap;
namespace smgpc::compat {
    // Native resources attached to an object in an actual original heap. The
    // callback runs after original disposers, before that storage is reused.
    // Host/stack objects are not registered. Does not retain the heap itself.
    void register_jkr_heap_finalizer(void* object, void (*finalize)(void*) noexcept);
    void unregister_jkr_heap_finalizer(void* object) noexcept;
    namespace detail {
        void finalize_jkr_heap_objects(JKRHeap*) noexcept;
        void finalize_jkr_heap_objects(JKRHeap*, std::uintptr_t begin, std::uintptr_t end) noexcept;
    }
}
