#include "JSystem/JKernel/JKRHeap.hpp"
#include <atomic>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <aurora/guest_thread.hpp>
#include <aurora/mem2_arena.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <new>
#include <stdexcept>
#include <utility>

namespace {
    struct CurrentHeapLock {
        CurrentHeapLock() { OSLockMutex(&JKRHeap::sCurrentHeapMutex); }
        ~CurrentHeapLock() { OSUnlockMutex(&JKRHeap::sCurrentHeapMutex); }
    };

    // The original virtual methods also lock. Keep that recursive mutex held
    // until native provenance has made the same allocation transition visible.
    struct HeapLock {
        JKRHeap &heap;
        explicit HeapLock(JKRHeap &owner) : heap(owner) {
            heap.lock();
        }
        ~HeapLock() {
            heap.unlock();
        }
    };
}  // namespace

// Native allocation records and finalizers belong to the base heap. Both
// registries use malloc-backed process storage, independently of heap routing.
namespace {
    namespace AllocationRecords {
        struct Record {
            void *memory;
            JKRHeap *heap;
            Record *next;
            bool from_tail;
        };
        constexpr std::size_t bucket_count = 2048;
        Record *buckets[bucket_count];
        std::mutex &mutex() {
            // Global delete remains callable after other static destructors.
            // This tiny process synchronization owner intentionally persists;
            // constructing it never re-enters an ordinary operator new.
            static std::mutex *instance = [] {
                void *memory = std::malloc(sizeof(std::mutex));
                if (memory == nullptr)
                    std::abort();
                return new (memory) std::mutex;
            }();
            return *instance;
        }

        std::size_t bucket(void *memory) {
            auto value = reinterpret_cast<std::uintptr_t>(memory) >> 3;
            value ^= value >> 17;
            return value & (bucket_count - 1);
        }
        struct Lock {
            Lock() {
                mutex().lock();
            }
            ~Lock() {
                mutex().unlock();
            }
        };

    }  // namespace AllocationRecords
    namespace HeapFinalizers {
        struct Record {
            void *object;
            JKRHeap *heap;
            void (*finalize)(void *) noexcept;
            Record *next;
        };
        Record *records;
        std::atomic<std::size_t> record_count;
        std::mutex &mutex() {
            // Native SDK destruction remains legal during process teardown.
            // This synchronization object never uses a Game allocation.
            static auto *instance = [] {
                void *memory = std::malloc(sizeof(std::mutex));
                if (!memory)
                    std::abort();
                return new (memory) std::mutex;
            }();
            return *instance;
        }
        void finalize(JKRHeap *heap, std::uintptr_t begin, std::uintptr_t end, bool entire_heap) noexcept {
            aurora::allocation::HostAllocationScope host;
            for (;;) {
                if (record_count.load(std::memory_order_acquire) == 0)
                    return;
                Record *removed = nullptr;
                {
                    std::lock_guard lock(mutex());
                    for (auto **slot = &records; *slot; slot = &(*slot)->next) {
                        auto *record = *slot;
                        auto address = reinterpret_cast<std::uintptr_t>(record->object);
                        if (record->heap == heap && (entire_heap || (begin <= address && address < end))) {
                            removed = record;
                            *slot = record->next;
                            record_count.fetch_sub(1, std::memory_order_release);
                            break;
                        }
                    }
                }
                if (!removed)
                    return;
                auto *object = removed->object;
                auto callback = removed->finalize;
                std::free(removed);
                // Re-scan after each callback: it can destroy and unregister
                // other objects. Never hold the registry lock across GX drain.
                callback(object);
            }
        }

    }  // namespace HeapFinalizers
}  // namespace

JKRHeap::Handle JKRHeap::retainNativeLifetime() {
    const aurora::allocation::HostAllocationScope host;
    Handle parent;
    Handle result;
    {
        const CurrentHeapLock lock;
        if (mNativeRetiring) {
            aurora::throw_host_exception<std::logic_error>("Cannot retain a retiring JKR heap");
        }
        if (auto owner = mNativeLifetime.lock()) {
            return owner;
        }
        if (mNativeSharedOwner) {
            aurora::throw_host_exception<std::logic_error>("Cannot retain a JKR heap whose shared owner is releasing");
        }
        auto* actualParent = getParent();
        if (!actualParent) {
            aurora::throw_host_exception<std::logic_error>("A native JKR root requires an explicit lifetime owner");
        }
        parent = actualParent->retainNativeLifetime();
        // Each original manually owned child has its own borrow control block.
        // The deleter never touches this pointer: raw Game ownership is unchanged.
        result = Handle(this, [parent](JKRHeap*) mutable {
            const aurora::allocation::HostAllocationScope host;
            // Weak observers must not retain the parent after the final borrow.
            auto releaseParent = std::move(parent);
        });
        mNativeLifetime = result;
    }
    return result;
}

JKRHeap::Handle JKRHeap::retainCurrentNativeLifetime() {
    if (!aurora::allocation::routing_state.callbackGuest) {
        return {};
    }
    const CurrentHeapLock lock;
    return sCurrentHeap ? sCurrentHeap->retainNativeLifetime() : Handle{};
}

JKRHeap::Handle JKRHeap::adoptNativeOwnership(std::shared_ptr<void> backing) {
    const aurora::allocation::HostAllocationScope host;
    Handle parent;
    {
        const CurrentHeapLock lock;
        if (mNativeRetiring || mNativeSharedOwner || !mNativeLifetime.expired()) {
            aurora::throw_host_exception<std::logic_error>("A native JKR heap can transfer ownership only before publication");
        }
        if (auto* actualParent = getParent()) {
            parent = actualParent->retainNativeLifetime();
        } else if (!backing) {
            aurora::throw_host_exception<std::invalid_argument>("A shared JKR root requires its backing storage owner");
        }
        // An observer in the allocation/publication gap must not create a new
        // manual borrow owner. Control-block allocation occurs outside the lock;
        // its failure invokes the same complete heap-destruction rollback.
        mNativeSharedOwner = true;
    }
    Handle result(this, [parent, backing](JKRHeap* heap) mutable {
        const aurora::os::GuestThreadExecutionScope execution;
        const aurora::allocation::HostAllocationScope host;
        auto releaseParent = std::move(parent);
        auto releaseBacking = std::move(backing);
        {
            const aurora::allocation::ClientAllocationScope game({true, true});
            heap->destroy();
        }
        // Both captures are empty before weak control-block observers remain.
        // Their local releases, and any parent/callback teardown, are unlocked.
    });
    {
        const CurrentHeapLock lock;
        mNativeLifetime = result;
    }
    return result;
}

void JKRHeap::bindNativeBackingStorage(std::shared_ptr<void> backing) {
    if (!backing) {
        aurora::throw_host_exception<std::invalid_argument>("A JKR backing binding requires actual storage ownership");
    }
    const CurrentHeapLock lock;
    if (mNativeRetiring || mNativeBacking) {
        aurora::throw_host_exception<std::logic_error>("A JKR heap backing owner can be bound only once before retirement");
    }
    mNativeBacking = std::move(backing);
}

void JKRHeap::validateNativeRetirement() const {
    const CurrentHeapLock lock;
    if (const auto count = mNativeLifetime.use_count(); count != 0) {
        char message[128];
        std::snprintf(message, sizeof(message), "Cannot destroy JKR heap %p with %ld retained native handles",
                      static_cast<const void*>(this), count);
        aurora::throw_host_exception<std::logic_error>(message);
    }
    // A host root may own physically separate arenas through its child tree.
    // Its process owner must retire those real children before freeing the root.
    if (sRootHeap == this && mChildTree.getNumChildren() != 0) {
        aurora::throw_host_exception<std::logic_error>("Cannot destroy a JKR root with live child heaps");
    }
    for (auto* child = mChildTree.getFirstChild(); child; child = child->getNextChild()) {
        child->getObject()->validateNativeRetirement();
    }
}

std::shared_ptr<void> JKRHeap::beginNativeRetirement() {
    const CurrentHeapLock lock;
    if (mNativeRetiring) {
        aurora::throw_host_exception<std::logic_error>("A JKR heap cannot recursively retire itself");
    }
    validateNativeRetirement();
    mNativeRetiring = true;
    return std::move(mNativeBacking);
}

void JKRHeap::validateNativeDestructor() noexcept {
    const CurrentHeapLock lock;
    if (mNativeRetiring) {
        return;
    }
    // Direct/stack destruction cannot postpone storage release beyond the base
    // destructor. Bound external storage must use the complete destroy() path.
    if (mNativeBacking) {
        OSPanic(__FILE__, __LINE__, "A JKR heap with retained external storage requires destroy()");
    }
    try {
        validateNativeRetirement();
    } catch (const std::exception& error) {
        OSPanic(__FILE__, __LINE__, "%s", error.what());
    }
    mNativeRetiring = true;
}

JKRHeap::CurrentHeapScope::CurrentHeapScope(JKRHeap &heap) {
    const aurora::allocation::HostAllocationScope host;
    OSLockMutex(&sCurrentHeapMutex);
    try {
        mSelectedOwner = heap.retainNativeLifetime();
        mPrevious = sCurrentHeap;
        if (mPrevious) {
            mPreviousOwner = mPrevious->retainNativeLifetime();
        }
        heap.becomeCurrentHeap();
    } catch (...) {
        OSUnlockMutex(&sCurrentHeapMutex);
        mPreviousOwner.reset();
        mSelectedOwner.reset();
        throw;
    }
}

JKRHeap::CurrentHeapScope::~CurrentHeapScope() {
    sCurrentHeap = mPrevious;
    OSUnlockMutex(&sCurrentHeapMutex);
    const aurora::allocation::HostAllocationScope host;
    mSelectedOwner.reset();
    mPreviousOwner.reset();
}

OSMutex JKRHeap::sCurrentHeapMutex;
JKRHeap *JKRHeap::sCurrentHeap;
JKRHeap *JKRHeap::sRootHeap;
JKRHeap *JKRHeap::sSystemHeap;

void *JKRHeap::mCodeStart;
void *JKRHeap::mCodeEnd;
std::atomic<void *> JKRHeap::mUserRamStart;
std::atomic<void *> JKRHeap::mUserRamEnd;

JKRErrorHandler JKRHeap::mErrorHandler;

static bool byte_806B26D8 = true;
static bool byte_806B70B8;

u32 JKRHeap::mMemorySize;

uintptr_t ARALT_AramStartAdr = 0x90000000;

JKRHeap::JKRHeap(void *data, u32 size, JKRHeap *parent, bool error) : JKRDisposer(), mChildTree(this), mDisposerList() {
    OSInitMutex(&mMutex);
    mSize = size;
    mStart = (u8 *)data;
    mEnd = (u8 *)data + size;

    if (parent == nullptr) {
        JKRHeap::sSystemHeap = this;
        JKRHeap::sCurrentHeap = this;
    } else {
        parent->mChildTree.appendChild(&mChildTree);

        if (JKRHeap::sSystemHeap == JKRHeap::sRootHeap) {
            JKRHeap::sSystemHeap = this;
        }

        if (JKRHeap::sCurrentHeap == JKRHeap::sRootHeap) {
            JKRHeap::sCurrentHeap = this;
        }
    }

    mErrorFlag = error;

    if (mErrorFlag == true && mErrorHandler == nullptr) {
        mErrorHandler = JKRDefaultMemoryErrorRoutine;
    }

    _3C = byte_806B26D8;
    _3D = byte_806B70B8;
    _69 = false;
}

JKRHeap::~JKRHeap() {
    validateNativeDestructor();
    const CurrentHeapLock lock;
    retireAllocations(this);
    // Retail keeps its parentless root for process lifetime. A native runtime
    // can release its host arena after all children and resources are gone.
    if (auto *parent = mChildTree.getParent())
        parent->removeChild(&mChildTree);
    JSUTree<JKRHeap> *nextRootHeap = sRootHeap ? sRootHeap->mChildTree.getFirstChild() : nullptr;

    if (sCurrentHeap == this)
        sCurrentHeap = !nextRootHeap ? sRootHeap : nextRootHeap->getObject();

    if (sSystemHeap == this)
        sSystemHeap = !nextRootHeap ? sRootHeap : nextRootHeap->getObject();

    if (sRootHeap == this) {
        sRootHeap = nullptr;
        sCurrentHeap = nullptr;
        sSystemHeap = nullptr;
        mUserRamStart.store(nullptr, std::memory_order_release);
        mUserRamEnd.store(nullptr, std::memory_order_relaxed);
        mMemorySize = 0;
    }
}

// The native JKRExpHeap root factory publishes its caller-owned arena bounds.

JKRHeap *JKRHeap::becomeSystemHeap() {
    JKRHeap *sys = sSystemHeap;
    sSystemHeap = this;
    return sys;
}

JKRHeap *JKRHeap::becomeCurrentHeap() {
    JKRHeap *cur = sCurrentHeap;
    sCurrentHeap = this;
    return cur;
}

void JKRHeap::destroy(JKRHeap *pHeap) {
    pHeap->do_destroy();
}

void *JKRHeap::alloc(u32 size, int align, JKRHeap *pHeap) {
    if (pHeap != nullptr) {
        return pHeap->alloc(size, align);
    }

    if (JKRHeap::sCurrentHeap != nullptr) {
        return JKRHeap::sCurrentHeap->alloc(size, align);
    }

    return nullptr;
}

void *JKRHeap::alloc(u32 size, int align) {
    HeapLock lock(*this);
    void *memory = do_alloc(size, align);
    recordAllocation(memory, this, align);
    return memory;
}

void JKRHeap::free(void *pData, JKRHeap *pHeap) {
    if (!pHeap) {
        pHeap = findFromRoot(pData);

        if (!pHeap) {
            return;
        }
    }

    releaseAllocation(pData);
    pHeap->do_free(pData);
}

void JKRHeap::free(void *pData) {
    releaseAllocation(pData);
    do_free(pData);
}

void JKRHeap::callAllDisposer() {
    while (mDisposerList.mHead != nullptr) {
        reinterpret_cast<JKRDisposer *>(mDisposerList.mHead->mData)->~JKRDisposer();
    }
    finalizeObjects();
}

void JKRHeap::freeAll() {
    HeapLock lock(*this);
    do_freeAll();
    retireAllocations(this);
}

void JKRHeap::freeTail() {
    HeapLock lock(*this);
    do_freeTail();
    retireAllocations(this, true);
}

s32 JKRHeap::resize(void *pData, u32 size) {
    return do_resize(pData, size);
}

s32 JKRHeap::getSize(void *pointer, JKRHeap *heap) {
    if (heap == nullptr)
        heap = findFromRoot(pointer);
    return heap != nullptr ? heap->do_getSize(pointer) : -1;
}

s32 JKRHeap::getFreeSize() {
    return do_getFreeSize();
}

void *JKRHeap::getMaxFreeBlock() {
    return do_getMaxFreeBlock();
}

s32 JKRHeap::getTotalFreeSize() {
    return do_getTotalFreeSize();
}

JKRHeap *JKRHeap::findFromRoot(void *pData) {
    JKRHeap *root = sRootHeap;

    if (root == nullptr) {
        return nullptr;
    }

    if ((void *)root->mStart <= pData && pData < (void *)root->mEnd) {
        return root->find(pData);
    }

    return root->findAllHeap(pData);
}

/* functionally equiv but not matching */
JKRHeap *JKRHeap::find(void *pData) const {
    if (mStart <= pData && pData < mEnd) {
        const JSUTree<JKRHeap> &tree = mChildTree;

        if (tree.getNumChildren() != 0) {
            for (JSUTreeIterator<JKRHeap> iterator(mChildTree.getFirstChild()); iterator != mChildTree.getEndChild(); ++iterator) {
                JKRHeap *result = iterator->find(pData);

                if (result) {
                    return result;
                }
            }
        }

        // this is to avoid returning a const JKRHeap ptr
        return const_cast<JKRHeap *>(this);
    }

    return nullptr;
}

/* same here */
JKRHeap *JKRHeap::findAllHeap(void *ptr) const {
    if (mChildTree.getNumChildren() != 0) {
        for (JSUTreeIterator<JKRHeap> iterator(mChildTree.getFirstChild()); iterator != mChildTree.getEndChild(); ++iterator) {
            JKRHeap *heap = iterator->findAllHeap(ptr);

            if (heap != nullptr) {
                return heap;
            }
        }
    }

    if (mStart <= ptr && ptr < mEnd) {
        return const_cast<JKRHeap *>(this);
    }

    return nullptr;
}

void JKRHeap::dispose_subroutine(uintptr_t start, uintptr_t end) {
    JSUListIterator<JKRDisposer> it(mDisposerList.getFirst());
    JSUListIterator<JKRDisposer> last_it;

    JSULink<JKRDisposer> *link;
    while ((link = it.mLink) != nullptr) {
        JKRDisposer *disp = link->getObject();

        if (reinterpret_cast<void *>(start) <= disp && disp < reinterpret_cast<void *>(end)) {
            disp->~JKRDisposer();

            if (last_it == nullptr) {
                it = mDisposerList.getFirst();
            } else {
                it = last_it;
                it++;
            }
        } else {
            last_it = it;
            it++;
        }
    }
    finalizeObjects(start, end);
}

bool JKRHeap::dispose(void *ptr, u32 size) {
    uintptr_t begin = (uintptr_t)ptr;
    uintptr_t end = (uintptr_t)ptr + size;
    dispose_subroutine(begin, end);
    return false;
}

void JKRHeap::dispose(void *begin, void *end) {
    dispose_subroutine((uintptr_t)begin, (uintptr_t)end);
}

void JKRHeap::dispose() {
    const JSUList<JKRDisposer> &list = mDisposerList;
    JSUListIterator<JKRDisposer> iterator;

    while (list.getFirst() != list.getEnd()) {
        iterator = list.getFirst();
        iterator->~JKRDisposer();
    }
    finalizeObjects();
}

void JKRHeap::copyMemory(void *pDst, void *pSrc, u32 size) {
    u32 count = (size + 3) / 4;
    u32 *dst_32 = (u32 *)pDst;
    u32 *src_32 = (u32 *)pSrc;

    while (count > 0) {
        *dst_32 = *src_32;
        dst_32++;
        src_32++;
        count--;
    }
}

void JKRDefaultMemoryErrorRoutine(void *pHeap, u32 size, int alignment) {
    OSPanic(__FILE__, 0x355, "%s", "abort\n");
}

JKRErrorHandler JKRHeap::setErrorHandler(JKRErrorHandler errorHandler) {
    JKRErrorHandler prev = JKRHeap::mErrorHandler;

    if (!errorHandler) {
        errorHandler = JKRDefaultMemoryErrorRoutine;
    }

    JKRHeap::mErrorHandler = errorHandler;
    return prev;
}

namespace {
    constexpr std::size_t default_new_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
    constexpr std::size_t native_heap_size_limit = std::numeric_limits<s32>::max();


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

void* operator new(std::size_t size) { return allocate(size, default_new_alignment); }
void* operator new[](std::size_t size) { return allocate(size, default_new_alignment); }
void* operator new(std::size_t size, std::align_val_t alignment) { return allocate(size, static_cast<std::size_t>(alignment)); }
void* operator new[](std::size_t size, std::align_val_t alignment) { return allocate(size, static_cast<std::size_t>(alignment)); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    try { return ::operator new(size); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    try { return ::operator new[](size); } catch (...) { return nullptr; }
}
void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try { return ::operator new(size, alignment); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try { return ::operator new[](size, alignment); } catch (...) { return nullptr; }
}

void operator delete(void* memory) noexcept { release(memory); }
void operator delete[](void* memory) noexcept { release(memory); }
void operator delete(void* memory, std::size_t) noexcept { release(memory); }
void operator delete[](void* memory, std::size_t) noexcept { release(memory); }
void operator delete(void* memory, std::align_val_t) noexcept { release(memory); }
void operator delete[](void* memory, std::align_val_t) noexcept { release(memory); }
void operator delete(void* memory, std::size_t, std::align_val_t) noexcept { release(memory); }
void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept { release(memory); }
void operator delete(void* memory, const std::nothrow_t&) noexcept { release(memory); }
void operator delete[](void* memory, const std::nothrow_t&) noexcept { release(memory); }
void operator delete(void* memory, std::align_val_t, const std::nothrow_t&) noexcept { release(memory); }
void operator delete[](void* memory, std::align_val_t, const std::nothrow_t&) noexcept { release(memory); }

void* operator new(std::size_t size, int alignment) { return allocate_jkr(size, nullptr, alignment); }
void* operator new[](std::size_t size, int alignment) { return allocate_jkr(size, nullptr, alignment); }
void* operator new(std::size_t size, JKRHeap* heap) { return allocate_jkr(size, heap, 4); }
void* operator new[](std::size_t size, JKRHeap* heap) { return allocate_jkr(size, heap, 4); }
void* operator new(std::size_t size, JKRHeap* heap, int alignment) { return allocate_jkr(size, heap, alignment); }
void* operator new[](std::size_t size, JKRHeap* heap, int alignment) { return allocate_jkr(size, heap, alignment); }
void operator delete(void* memory, int) noexcept { release(memory); }
void operator delete[](void* memory, int) noexcept { release(memory); }
void operator delete(void* memory, JKRHeap*) noexcept { release(memory); }
void operator delete[](void* memory, JKRHeap*) noexcept { release(memory); }
void operator delete(void* memory, JKRHeap*, int) noexcept { release(memory); }
void operator delete[](void* memory, JKRHeap*, int) noexcept { release(memory); }

void JKRHeap::state_register(TState *, u32) const {
    return;
}

bool JKRHeap::state_compare(const TState &lhs, const TState &rhs) const {
    return lhs.mCheckCode == rhs.mCheckCode;
}

void JKRHeap::state_dump(const TState &) const {
    return;
}

void JKRHeap::setAltAramStartAdr(uintptr_t addr) {
    ARALT_AramStartAdr = addr;
}

uintptr_t JKRHeap::getAltAramStartAdr() {
    return ARALT_AramStartAdr;
}

s32 JKRHeap::do_changeGroupID(u8) {
    return 0;
}

u8 JKRHeap::do_getCurrentGroupId() {
    return 0;
}

bool JKRHeap::dump_sort() {
    return true;
}

void JKRHeap::recordAllocation(void *memory, JKRHeap *heap, int alignment) {
    using namespace AllocationRecords;
    if (memory == nullptr)
        return;
    auto *fresh = static_cast<Record *>(std::malloc(sizeof(Record)));
    if (fresh == nullptr) {
        heap->free(memory);
        throw std::bad_alloc();
    }
    *fresh = {memory, heap, nullptr, alignment < 0};
    Lock lock;
    auto **slot = &buckets[bucket(memory)];
    for (auto *record = *slot; record != nullptr; record = record->next) {
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

JKRHeap *JKRHeap::releaseAllocation(void *memory) noexcept {
    using namespace AllocationRecords;
    if (memory == nullptr)
        return nullptr;
    Lock lock;
    for (auto **slot = &buckets[bucket(memory)]; *slot != nullptr; slot = &(*slot)->next) {
        auto *record = *slot;
        if (record->memory == memory) {
            auto *heap = record->heap;
            *slot = record->next;
            std::free(record);
            return heap;
        }
    }
    return nullptr;
}

void JKRHeap::retireAllocations(JKRHeap *heap, bool tail_only) noexcept {
    using namespace AllocationRecords;
    Lock lock;
    for (auto &head : buckets) {
        auto **slot = &head;
        while (*slot != nullptr) {
            auto *record = *slot;
            if (record->heap == heap && (!tail_only || record->from_tail)) {
                *slot = record->next;
                std::free(record);
            } else {
                slot = &record->next;
            }
        }
    }
}

void JKRHeap::registerFinalizer(void *object, void (*callback)(void *) noexcept) {
    using namespace HeapFinalizers;
    aurora::allocation::HostAllocationScope host;
    if (!object || !callback)
        aurora::throw_host_exception<std::invalid_argument>("Heap finalizer requires an object and callback");
    auto *heap = JKRHeap::findFromRoot(object);
    if (!heap)
        return;
    HeapLock heap_lock(*heap);
    auto *record = static_cast<Record *>(std::malloc(sizeof(Record)));
    if (!record)
        throw std::bad_alloc();
    *record = {object, heap, callback, nullptr};
    std::lock_guard lock(mutex());
    for (auto *current = records; current; current = current->next) {
        if (current->object == object) {
            std::free(record);
            aurora::throw_host_exception<std::logic_error>("Object already has a JKR heap finalizer");
        }
    }
    record->next = records;
    records = record;
    record_count.fetch_add(1, std::memory_order_release);
}

void JKRHeap::unregisterFinalizer(void *object) noexcept {
    using namespace HeapFinalizers;
    if (record_count.load(std::memory_order_acquire) == 0)
        return;
    std::lock_guard lock(mutex());
    for (auto **slot = &records; *slot; slot = &(*slot)->next) {
        if ((*slot)->object == object) {
            auto *removed = *slot;
            *slot = removed->next;
            record_count.fetch_sub(1, std::memory_order_release);
            std::free(removed);
            return;
        }
    }
}

void JKRHeap::finalizeObjects() noexcept {
    HeapFinalizers::finalize(this, 0, 0, true);
}

void JKRHeap::finalizeObjects(std::uintptr_t begin, std::uintptr_t end) noexcept {
    HeapFinalizers::finalize(this, begin, end, false);
}
