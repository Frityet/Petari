from pathlib import Path
from edit import write

path='src/JSystem/JKernel/JKRHeap.hpp'
s=Path(path).read_text().replace('#include <cstddef>','#include <atomic>\n#include <cstddef>')
s=s.replace('return mUserRamStart;', 'return mUserRamStart.load(std::memory_order_acquire);')
s=s.replace('return mUserRamEnd;', 'return mUserRamEnd.load(std::memory_order_relaxed);')
s=s.replace('    static void* mUserRamStart;\n    static void* mUserRamEnd;', '''    // The original root-arena bounds also identify pointers lacking native
    // allocation provenance. Atomic publication keeps global delete lock-free.
    static std::atomic< void* > mUserRamStart;
    static std::atomic< void* > mUserRamEnd;''')
s=s.replace('''void* operator new[](std::size_t, int);

void* operator new[](std::size_t, JKRHeap*, int);''','''void* operator new[](std::size_t, int);
void* operator new[](std::size_t, JKRHeap*);
void* operator new[](std::size_t, JKRHeap*, int);
void operator delete(void*, int) noexcept;
void operator delete(void*, JKRHeap*) noexcept;
void operator delete(void*, JKRHeap*, int) noexcept;
void operator delete[](void*, int) noexcept;
void operator delete[](void*, JKRHeap*) noexcept;
void operator delete[](void*, JKRHeap*, int) noexcept;''')
write(path,s)

path='src/JSystem/JKernel/JKRHeap.cpp'
s=Path(path).read_text()
s=s.replace('#include <aurora/exception.hpp>','#include <aurora/exception.hpp>\n#include <aurora/mem2_arena.hpp>')
s=s.replace('#include <mutex>','#include <limits>\n#include <mutex>')
s=s.replace('void *JKRHeap::mUserRamStart;\nvoid *JKRHeap::mUserRamEnd;', 'std::atomic<void *> JKRHeap::mUserRamStart;\nstd::atomic<void *> JKRHeap::mUserRamEnd;')
s=s.replace('JSUTree<JKRHeap> *nextRootHeap = sRootHeap->mChildTree.getFirstChild();', 'JSUTree<JKRHeap> *nextRootHeap = sRootHeap ? sRootHeap->mChildTree.getFirstChild() : nullptr;')
s=s.replace('''    if (sSystemHeap == this)
        sSystemHeap = !nextRootHeap ? sRootHeap : nextRootHeap->getObject();
}''','''    if (sSystemHeap == this)
        sSystemHeap = !nextRootHeap ? sRootHeap : nextRootHeap->getObject();

    if (sRootHeap == this) {
        sRootHeap = nullptr;
        sCurrentHeap = nullptr;
        sSystemHeap = nullptr;
        mUserRamStart.store(nullptr, std::memory_order_release);
        mUserRamEnd.store(nullptr, std::memory_order_relaxed);
        mMemorySize = 0;
    }
}''',1)
s=s.replace('// Wii boot-arena setup is not used by the explicit native JkrHeapRuntime.', '// The native JKRExpHeap root factory publishes its caller-owned arena bounds.')
provider=Path('src/compat/MetrowerksAlignedNew.cpp').read_text()
ops=provider[provider.index('void* operator new(std::size_t size)'):]
helpers='''namespace {
    constexpr std::size_t default_new_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
    constexpr std::size_t native_heap_size_limit = std::numeric_limits<s32>::max();

    struct CurrentHeapLock {
        CurrentHeapLock() { OSLockMutex(&JKRHeap::sCurrentHeapMutex); }
        ~CurrentHeapLock() { OSUnlockMutex(&JKRHeap::sCurrentHeapMutex); }
    };

    void* allocate_native(std::size_t size, std::size_t alignment, bool from_tail, JKRHeap* explicit_heap) {
        if (alignment == 0 || (alignment & (alignment - 1)) != 0) throw std::bad_alloc();
        if (explicit_heap != nullptr || aurora::allocation::routing_state.guest) {
            if (size > native_heap_size_limit || alignment > native_heap_size_limit) return nullptr;
            CurrentHeapLock lock;
            JKRHeap* heap = explicit_heap != nullptr ? explicit_heap : JKRHeap::sCurrentHeap;
            if (heap == nullptr) OSPanic(__FILE__, __LINE__, "Original allocation has no current JKR heap");
            // Preserve integer-alignment new while meeting the native pointer
            // alignment required by C++ objects allocated with retail values.
            alignment = alignment < alignof(void*) ? alignof(void*) : alignment;
            const int signed_alignment = from_tail ? -static_cast<int>(alignment) : static_cast<int>(alignment);
            return heap->alloc(static_cast<u32>(size), signed_alignment);
        }
        alignment = alignment < alignof(void*) ? alignof(void*) : alignment;
        void* result = nullptr;
        if (posix_memalign(&result, alignment, size == 0 ? 1 : size) != 0) return nullptr;
        return result;
    }

    void* allocate(std::size_t size, std::size_t alignment, bool from_tail = false, JKRHeap* heap = nullptr) {
        for (;;) {
            if (void* result = allocate_native(size, alignment, from_tail, heap)) return result;
            auto handler = std::get_new_handler();
            if (handler == nullptr) throw std::bad_alloc();
            handler();
        }
    }

    void* allocate_jkr(std::size_t size, JKRHeap* heap, int alignment) {
        if (alignment == std::numeric_limits<int>::min()) throw std::bad_alloc();
        auto magnitude = static_cast<std::size_t>(alignment < 0 ? -alignment : alignment);
        if (magnitude == 0) magnitude = default_new_alignment;
        return allocate(size, magnitude, alignment < 0, heap);
    }

    void release(void* memory) noexcept {
        if (memory == nullptr) return;
        if (JKRHeap* heap = JKRHeap::releaseAllocation(memory)) {
            // Provenance is consumed before the heap's own mutex is taken;
            // global delete must not acquire the current-selection lock.
            heap->free(memory);
            return;
        }
        const auto address = reinterpret_cast<std::uintptr_t>(memory);
        const auto begin = reinterpret_cast<std::uintptr_t>(JKRHeap::getUserRamStart());
        if (begin != 0 && begin <= address && address < reinterpret_cast<std::uintptr_t>(JKRHeap::getUserRamEnd())) {
            OSPanic(__FILE__, __LINE__, "Delete has no original allocation provenance: %p", memory);
        }
        if (aurora::contains_mem2_address(memory)) {
            OSPanic(__FILE__, __LINE__, "Delete has no original MEM2 allocation provenance: %p", memory);
        }
        std::free(memory);
    }
}

'''
s=s.replace('// Native global allocation routing is supplied by MetrowerksAlignedNew.cpp.', helpers+ops.rstrip())
write(path,s)

path='src/JSystem/JKernel/JKRExpHeap.hpp'
s=Path(path).read_text().replace('    static JKRExpHeap* createRoot(int, bool);', '''    static JKRExpHeap* createRoot(int, bool);
    // Construct the original root in aligned storage retained by its caller.
    // Destruction retires heap state; the caller releases the arena afterward.
    static JKRExpHeap* createRoot(void* memory, u32 size, bool errorFlag);''')
write(path,s)
path='src/JSystem/JKernel/JKRExpHeap.cpp'
s=Path(path).read_text().replace('#include <cstdint>', '#include <aurora/exception.hpp>\n#include <cstdint>\n#include <limits>')
s=s.replace('#include <new>', '#include <new>\n#include <stdexcept>')
start=s.index('JKRExpHeap *JKRExpHeap::create(u32 size,')
s=s[:start]+'''JKRExpHeap *JKRExpHeap::createRoot(void *memory, u32 size, bool errorFlag) {
    struct CurrentHeapLock {
        CurrentHeapLock() { OSLockMutex(&JKRHeap::sCurrentHeapMutex); }
        ~CurrentHeapLock() { OSUnlockMutex(&JKRHeap::sCurrentHeapMutex); }
    } lock;
    if (sRootHeap != nullptr || getUserRamStart() != nullptr) {
        aurora::throw_host_exception<std::logic_error>("An original JKR root heap already exists");
    }
    constexpr std::size_t alignment = 32;
    constexpr std::size_t headerSize = (sizeof(JKRExpHeap) + alignment - 1) & ~(alignment - 1);
    const auto begin = reinterpret_cast<std::uintptr_t>(memory);
    if (!memory || (begin & (alignment - 1)) != 0 || (size & (alignment - 1)) != 0 ||
        size < headerSize + sizeof(CMemBlock) + alignment || size > std::numeric_limits<s32>::max() ||
        size > std::numeric_limits<std::uintptr_t>::max() - begin) {
        aurora::throw_host_exception<std::invalid_argument>("JKR root requires one aligned, caller-owned arena");
    }
    auto *data = static_cast<u8 *>(memory) + headerSize;
    auto *heap = new (memory) JKRExpHeap(data, static_cast<u32>(size - headerSize), nullptr, errorFlag);
    heap->_6E = 1;
    heap->_70 = memory;
    heap->_74 = size;
    sRootHeap = heap;
    mMemorySize = size;
    mUserRamEnd.store(static_cast<u8 *>(memory) + size, std::memory_order_relaxed);
    mUserRamStart.store(memory, std::memory_order_release);
    return heap;
}

'''+s[start:]
write(path,s)

path='src/compat/JkrAllocationDomain.cpp'
s=Path(path).read_text().replace('#include "compat/JkrAllocationRouting.hpp"\n','').replace('#include <atomic>\n','')
s=s.replace('''        std::atomic<std::uintptr_t> arena_begin;
        std::atomic<std::uintptr_t> arena_end;
        std::atomic<std::uintptr_t> mem2_begin;
        std::atomic<std::uintptr_t> mem2_end;
''','')
s=s.replace('JKRHeap::sRootHeap != nullptr || arena_begin.load(std::memory_order_relaxed) != 0','JKRHeap::sRootHeap != nullptr || JKRHeap::getUserRamStart() != nullptr')
a=s.index('        auto* data = static_cast<u8*>(_storage->arena) + root_header_size;')
b=s.index('\n    }',a)
s=s[:a]+'''        _storage->root = JKRExpHeap::createRoot(_storage->arena, static_cast<u32>(budget), false);'''+s[b:]
s=s.replace('_storage->root->~JKRExpHeap();','_storage->root->destroy();')
s=s.replace('''        JKRHeap::sRootHeap = nullptr;
        JKRHeap::sCurrentHeap = nullptr;
        JKRHeap::sSystemHeap = nullptr;
        arena_begin.store(0, std::memory_order_release);
        arena_end.store(0, std::memory_order_relaxed);
''','')
s=s.replace('''            mem2_begin.store(0, std::memory_order_release);
            mem2_end.store(0, std::memory_order_relaxed);
''','')
s=s.replace('''        const auto begin = reinterpret_cast<std::uintptr_t>(memory);
        mem2_end.store(begin + budget, std::memory_order_relaxed);
        mem2_begin.store(begin, std::memory_order_release);
''','')
a=s.index('    namespace detail {\n        void* allocate_jkr_or_host')
s=s[:a]+'}\n'
write(path,s)

path='aurora/include/aurora/mem2_arena.hpp'
s=Path(path).read_text().replace('void unbind_mem2_arena(void* memory);','''void unbind_mem2_arena(void* memory);
// Query the complete retained arena, including space consumed by SDK heaps
// and ARAM. This is allocation-free and does not lock the watermark mutex.
bool contains_mem2_address(const void* memory) noexcept;''')
write(path,s)
path='aurora/lib/dolphin/os/OSArena.cpp'
s=Path(path).read_text().replace('#include <cassert>','#include <atomic>\n#include <cassert>')
s=s.replace('std::uintptr_t mem2_begin;\nstd::uintptr_t mem2_end;', 'std::atomic<std::uintptr_t> mem2_begin;\nstd::atomic<std::uintptr_t> mem2_end;')
s=s.replace('if (mem2_begin != 0 || begin == 0', 'if (mem2_begin.load(std::memory_order_relaxed) != 0 || begin == 0')
s=s.replace('''  mem2_begin = mem2_low = begin;
  mem2_end = mem2_high = begin + size;''','''  mem2_low = begin;
  mem2_high = begin + size;
  mem2_end.store(begin + size, std::memory_order_relaxed);
  mem2_begin.store(begin, std::memory_order_release);''')
s=s.replace('reinterpret_cast<std::uintptr_t>(memory) != mem2_begin', 'reinterpret_cast<std::uintptr_t>(memory) != mem2_begin.load(std::memory_order_relaxed)')
s=s.replace('size = mem2_end - mem2_begin;', 'size = mem2_end.load(std::memory_order_relaxed) - mem2_begin.load(std::memory_order_relaxed);')
s=s.replace('  mem2_begin = mem2_end = mem2_low = mem2_high = 0;', '''  mem2_begin.store(0, std::memory_order_release);
  mem2_end.store(0, std::memory_order_relaxed);
  mem2_low = mem2_high = 0;''')
pos=s.index('void* OSGetMEM2ArenaLo()')
s=s[:pos]+'''bool aurora::contains_mem2_address(const void* memory) noexcept {
  const auto address = reinterpret_cast<std::uintptr_t>(memory);
  const auto begin = mem2_begin.load(std::memory_order_acquire);
  return begin != 0 && begin <= address && address < mem2_end.load(std::memory_order_relaxed);
}

'''+s[pos:]
s=s.replace('mem2_begin == 0 || value < mem2_begin || value > mem2_high', 'mem2_begin.load(std::memory_order_relaxed) == 0 || value < mem2_begin.load(std::memory_order_relaxed) || value > mem2_high')
s=s.replace('mem2_begin == 0 || value < mem2_low || value > mem2_end', 'mem2_begin.load(std::memory_order_relaxed) == 0 || value < mem2_low || value > mem2_end.load(std::memory_order_relaxed)')
write(path,s)
write('src/compat/MetrowerksAlignedNew.cpp',None)
write('src/compat/JkrAllocationRouting.hpp',None)
