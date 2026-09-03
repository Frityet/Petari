#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/JkrDiagnostics.hpp"
#include "compat/JkrAllocationProvenance.hpp"
#include <cstdint>

namespace {
    // The original virtual methods also lock. Keep that recursive mutex held
    // until native provenance has made the same allocation transition visible.
    struct HeapLock {
        JKRHeap& heap;
        explicit HeapLock(JKRHeap& owner) : heap(owner) { heap.lock(); }
        ~HeapLock() { heap.unlock(); }
    };
}

JKRHeap* JKRHeap::sCurrentHeap;
JKRHeap* JKRHeap::sRootHeap;
JKRHeap* JKRHeap::sSystemHeap;

void* JKRHeap::mCodeStart;
void* JKRHeap::mCodeEnd;
void* JKRHeap::mUserRamStart;
void* JKRHeap::mUserRamEnd;

JKRErrorHandler JKRHeap::mErrorHandler;

static bool byte_806B26D8 = true;
static bool byte_806B70B8;

u32 JKRHeap::mMemorySize;

u32 JKRHeap::ARALT_AramStartAddr = 0x90000000;

JKRHeap::JKRHeap(void* data, u32 size, JKRHeap* parent, bool error) : JKRDisposer(), mChildTree(this), mDisposerList() {
    OSInitMutex(&mMutex);
    mSize = size;
    mStart = (u8*)data;
    mEnd = (u8*)data + size;

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
    smgpc::compat::detail::retire_jkr_heap_allocations(this);
    // Retail keeps its parentless root for process lifetime. A native runtime
    // can release its host arena after all children and resources are gone.
    if (auto* parent = mChildTree.getParent()) parent->removeChild(&mChildTree);
    JSUTree< JKRHeap >* nextRootHeap = sRootHeap->mChildTree.getFirstChild();

    if (sCurrentHeap == this)
        sCurrentHeap = !nextRootHeap ? sRootHeap : nextRootHeap->getObject();

    if (sSystemHeap == this)
        sSystemHeap = !nextRootHeap ? sRootHeap : nextRootHeap->getObject();
}

// Wii boot-arena setup is not used by the explicit native JkrHeapRuntime.

JKRHeap* JKRHeap::becomeSystemHeap() {
    JKRHeap* sys = sSystemHeap;
    sSystemHeap = this;
    return sys;
}

JKRHeap* JKRHeap::becomeCurrentHeap() {
    JKRHeap* cur = sCurrentHeap;
    sCurrentHeap = this;
    return cur;
}

void JKRHeap::destroy(JKRHeap* pHeap) {
    pHeap->do_destroy();
}

void* JKRHeap::alloc(u32 size, int align, JKRHeap* pHeap) {
    if (pHeap != nullptr) {
        return pHeap->alloc(size, align);
    }

    if (JKRHeap::sCurrentHeap != nullptr) {
        return JKRHeap::sCurrentHeap->alloc(size, align);
    }

    return nullptr;
}

void* JKRHeap::alloc(u32 size, int align) {
    HeapLock lock(*this);
    void* memory = do_alloc(size, align);
    smgpc::compat::detail::record_jkr_allocation(memory, this, align);
    return memory;
}

void JKRHeap::free(void* pData, JKRHeap* pHeap) {
    if (!pHeap) {
        pHeap = findFromRoot(pData);

        if (!pHeap) {
            return;
        }
    }

    smgpc::compat::detail::forget_jkr_allocation(pData);
    pHeap->do_free(pData);
}

void JKRHeap::free(void* pData) {
    smgpc::compat::detail::forget_jkr_allocation(pData);
    do_free(pData);
}

void JKRHeap::callAllDisposer() {
    while (mDisposerList.mHead != nullptr) {
        reinterpret_cast< JKRDisposer* >(mDisposerList.mHead->mData)->~JKRDisposer();
    }
}

void JKRHeap::freeAll() {
    HeapLock lock(*this);
    do_freeAll();
    smgpc::compat::detail::retire_jkr_heap_allocations(this);
}

void JKRHeap::freeTail() {
    HeapLock lock(*this);
    do_freeTail();
    smgpc::compat::detail::retire_jkr_heap_allocations(this, true);
}

s32 JKRHeap::resize(void* pData, u32 size) {
    return do_resize(pData, size);
}

s32 JKRHeap::getFreeSize() {
    return do_getFreeSize();
}

void* JKRHeap::getMaxFreeBlock() {
    return do_getMaxFreeBlock();
}

s32 JKRHeap::getTotalFreeSize() {
    return do_getTotalFreeSize();
}

JKRHeap* JKRHeap::findFromRoot(void* pData) {
    JKRHeap* root = sRootHeap;

    if (root == nullptr) {
        return nullptr;
    }

    if ((void*)root->mStart <= pData && pData < (void*)root->mEnd) {
        return root->find(pData);
    }

    return root->findAllHeap(pData);
}

/* functionally equiv but not matching */
JKRHeap* JKRHeap::find(void* pData) const {
    if (mStart <= pData && pData < mEnd) {
        const JSUTree< JKRHeap >& tree = mChildTree;

        if (tree.getNumChildren() != 0) {
            for (JSUTreeIterator< JKRHeap > iterator(mChildTree.getFirstChild()); iterator != mChildTree.getEndChild(); ++iterator) {
                JKRHeap* result = iterator->find(pData);

                if (result) {
                    return result;
                }
            }
        }

        // this is to avoid returning a const JKRHeap ptr
        return const_cast< JKRHeap* >(this);
    }

    return nullptr;
}

/* same here */
JKRHeap* JKRHeap::findAllHeap(void* ptr) const {
    if (mChildTree.getNumChildren() != 0) {
        for (JSUTreeIterator< JKRHeap > iterator(mChildTree.getFirstChild()); iterator != mChildTree.getEndChild(); ++iterator) {
            JKRHeap* heap = iterator->findAllHeap(ptr);

            if (heap != nullptr) {
                return heap;
            }
        }
    }

    if (mStart <= ptr && ptr < mEnd) {
        return const_cast< JKRHeap* >(this);
    }

    return nullptr;
}

void JKRHeap::dispose_subroutine(uintptr_t start, uintptr_t end) {
    JSUListIterator< JKRDisposer > last_it;
    JSUListIterator< JKRDisposer > next_it;
    JSUListIterator< JKRDisposer > it;

    for (it = mDisposerList.getFirst(); it != mDisposerList.getEnd(); it = next_it) {
        JKRDisposer* disp = it.getObject();

        if ((void*)start <= disp && disp < (void*)end) {
            disp->~JKRDisposer();

            if (last_it == nullptr) {
                next_it = mDisposerList.getFirst();
            } else {
                next_it = last_it;
                next_it++;
            }
        } else {
            last_it = it;
            next_it = it;
            next_it++;
        }
    }
}

bool JKRHeap::dispose(void* ptr, u32 size) {
    uintptr_t begin = (uintptr_t)ptr;
    uintptr_t end = (uintptr_t)ptr + size;
    dispose_subroutine(begin, end);
    return false;
}

void JKRHeap::dispose(void* begin, void* end) {
    dispose_subroutine((uintptr_t)begin, (uintptr_t)end);
}

void JKRHeap::dispose() {
    const JSUList< JKRDisposer >& list = mDisposerList;
    JSUListIterator< JKRDisposer > iterator;

    while (list.getFirst() != list.getEnd()) {
        iterator = list.getFirst();
        iterator->~JKRDisposer();
    }
}

void JKRHeap::copyMemory(void* pDst, void* pSrc, u32 size) {
    u32 count = (size + 3) / 4;
    u32* dst_32 = (u32*)pDst;
    u32* src_32 = (u32*)pSrc;

    while (count > 0) {
        *dst_32 = *src_32;
        dst_32++;
        src_32++;
        count--;
    }
}

void JKRDefaultMemoryErrorRoutine(void* pHeap, u32 size, int alignment) {
    smgpc::compat::jkr_panic(__FILE__, 0x355, "%s", "abort\n");
}

JKRErrorHandler JKRHeap::setErrorHandler(JKRErrorHandler errorHandler) {
    JKRErrorHandler prev = JKRHeap::mErrorHandler;

    if (!errorHandler) {
        errorHandler = JKRDefaultMemoryErrorRoutine;
    }

    JKRHeap::mErrorHandler = errorHandler;
    return prev;
}

// Native global allocation routing is supplied by MetrowerksAlignedNew.cpp.

void JKRHeap::state_register(TState*, u32) const {
    return;
}

bool JKRHeap::state_compare(const TState& lhs, const TState& rhs) const {
    return lhs.mCheckCode == rhs.mCheckCode;
}

void JKRHeap::state_dump(const TState&) const {
    return;
}

void JKRHeap::setAltAramStartAdr(u32 addr) {
    ARALT_AramStartAddr = addr;
}

u32 JKRHeap::getAltAramStartAdr() {
    return ARALT_AramStartAddr;
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
