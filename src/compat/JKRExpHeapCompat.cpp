#include "JSystem/JKernel/JKRExpHeap.hpp"
#include "compat/JkrDiagnostics.hpp"
#include "JSystem/JSupport/JSupport.hpp"

#include <new>
#include <cstdint>

static u32 DBfoundSize;
static u32 DBfoundOffset;
static JKRExpHeap::CMemBlock* DBfoundBlock;
static JKRExpHeap::CMemBlock* DBnewFreeBlock;
static JKRExpHeap::CMemBlock* DBnewUsedBlock;

// Wii boot-arena createRoot is replaced by explicit JkrHeapRuntime ownership.

JKRExpHeap* JKRExpHeap::create(u32 size, JKRHeap* pParent, bool errorFlag) {
    if (!pParent) {
        pParent = JKRHeap::sRootHeap;
    }

    if (size == 0xFFFFFFFF) {
        size = pParent->getMaxAllocatableSize(0x10);
    }

    u32 alignedSize = ALIGN_PREV(size, 0x10);
    u32 heapSize = ALIGN_NEXT(sizeof(JKRExpHeap), 0x10);

    if (alignedSize < heapSize + sizeof(CMemBlock)) {
        return nullptr;
    }

    u8* mem = (u8*)JKRHeap::alloc(alignedSize, 16, pParent);
    if (mem == nullptr) {
        return nullptr;
    }
    u8* data = mem + heapSize;

    JKRExpHeap* heap = new (mem) JKRExpHeap(data, alignedSize - heapSize, pParent, errorFlag);

    if (heap == nullptr) {
        JKRHeap::free(mem, nullptr);
        return nullptr;
    }

    heap->_6E = 0;
    return heap;
}

JKRExpHeap* JKRExpHeap::create(void* ptr, u32 size, JKRHeap* pParent, bool errorFlag) {
    JKRHeap* parent;

    if (pParent == nullptr) {
        parent = sRootHeap->find(ptr);

        if (parent == nullptr) {
            return nullptr;
        }
    } else {
        parent = pParent;
    }

    JKRExpHeap* heap = nullptr;
    u32 heapSize = ALIGN_NEXT(sizeof(JKRExpHeap), 0x10);

    if (size < heapSize) {
        return nullptr;
    }

    void* data = (u8*)ptr + heapSize;
    u32 alignSize = ALIGN_PREV((uintptr_t)ptr + size - (uintptr_t)data, uintptr_t(0x10));
    if (ptr != nullptr) {
        heap = new (ptr) JKRExpHeap(data, alignSize, parent, errorFlag);
    }

    heap->_6E = 1;
    heap->_70 = ptr;
    heap->_74 = size;
    return heap;
}

void JKRExpHeap::do_destroy() {
    if (!_6E) {
        JKRHeap* heap = getParent();

        if (heap != nullptr) {
            this->~JKRExpHeap();
            JKRHeap::free(this, heap);
        }
    } else {
        this->~JKRExpHeap();
    }
}

void* JKRExpHeap::do_alloc(u32 size, int align) {
    void* ptr;
    OSLockMutex(&mMutex);

    if (size < 4) {
        size = 4;
    }

    if (align >= 0) {
        if (align <= 4) {
            ptr = allocFromHead(size);
        } else {
            ptr = allocFromHead(size, align);
        }
    } else {
        if (-align <= 4) {
            ptr = allocFromTail(size);
        } else {
            ptr = allocFromTail(size, -align);
        }
    }

    if (ptr == nullptr) {
        smgpc::compat::jkr_warning_f(":::cannot alloc memory (0x%x byte).\n", size);

        if (JKRHeap::mErrorFlag == true) {
            if (JKRHeap::mErrorHandler) {
                (*JKRHeap::mErrorHandler)(this, size, align);
            }
        }
    }

    OSUnlockMutex(&mMutex);
    return ptr;
}

JKRExpHeap::JKRExpHeap(void* data, u32 size, JKRHeap* parent, bool error) : JKRHeap(data, size, parent, error) {
    CMemBlock* block = (CMemBlock*)data;

    mAllocMode = 0;
    mCurrentGroupId = 0xFF;
    mHeadFreeList = block;
    mTailFreeList = block;
    block->initiate(nullptr, nullptr, size - sizeof(CMemBlock), 0, 0);
    mHeadUsedList = nullptr;
    mTailUsedList = nullptr;
}

void JKRExpHeap::CMemBlock::initiate(CMemBlock* prev, CMemBlock* next, u32 size, u8 groupID, u8 align) {
    mMagic = 'HM';
    mFlags = align;
    mGroupId = groupID;
    mSize = size;
    mPrev = prev;
    mNext = next;
}

JKRExpHeap::CMemBlock* JKRExpHeap::CMemBlock::allocFore(u32 size, u8 group_1, u8 align_1, u8 group_2, u8 align_2) {
    CMemBlock* block = nullptr;
    mGroupId = group_1;
    mFlags = align_1;

    if (mSize >= size + sizeof(CMemBlock)) {
        block = (CMemBlock*)((uintptr_t)this + size);
        block[1].mGroupId = group_2;
        block[1].mFlags = align_2;
        block[1].mSize = mSize - (size + sizeof(CMemBlock));
        mSize = size;
        block++;
    }

    return block;
}

JKRExpHeap::CMemBlock* JKRExpHeap::CMemBlock::allocBack(u32 size, u8 group_1, u8 align_1, u8 group_2, u8 align_2) {
    CMemBlock* block = nullptr;

    if (mSize >= size + sizeof(CMemBlock)) {
        block = (CMemBlock*)((uintptr_t)this + mSize - size);
        block->mGroupId = group_2;
        block->mFlags = align_2 | 0x80;
        block->mSize = size;
        mGroupId = group_1;
        mFlags = align_1;
        mSize -= size + sizeof(CMemBlock);
    } else {
        mGroupId = group_2;
        mFlags = 0x80;
    }

    return block;
}

JKRExpHeap::CMemBlock* JKRExpHeap::CMemBlock::getHeapBlock(void* ptr) {
    if (ptr != nullptr) {
        CMemBlock* block = (CMemBlock*)ptr - 1;

        if (block->mMagic == 'HM') {
            return block;
        }
    }

    return nullptr;
}


JKRExpHeap::~JKRExpHeap() {
    dispose();
}

void* JKRExpHeap::allocFromHead(u32 size, int align) {
    u32 foundOffset;
    int foundSize;
    // Native block headers contain pointers and require their natural alignment.
    size = ALIGN_NEXT(size, alignof(CMemBlock));
    foundSize = -1;
    foundOffset = 0;
    CMemBlock* foundBlock = nullptr;
    CMemBlock* newFreeBlock = nullptr;
    CMemBlock* newUsedBlock = nullptr;
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        u32 offset =
            ALIGN_PREV(align - 1 + (uintptr_t)block->getContent(), uintptr_t(align)) - (uintptr_t)block->getContent();
        if (block->mSize < size + offset) {
            continue;
        }
        if (foundSize <= (u32)block->mSize) {
            continue;
        }
        foundSize = block->mSize;
        foundBlock = block;
        foundOffset = offset;
        if (mAllocMode != 0) {
            break;
        }
        if (foundSize == size) {
            break;
        }
    }
    DBfoundSize = foundSize;
    DBfoundOffset = foundOffset;
    DBfoundBlock = foundBlock;
    if (foundBlock) {
        if (foundOffset >= sizeof(CMemBlock)) {
            CMemBlock* prev = foundBlock->mPrev;
            CMemBlock* next = foundBlock->mNext;
            newUsedBlock = foundBlock->allocFore(foundOffset - sizeof(CMemBlock), 0, 0, 0, 0);
            if (newUsedBlock) {
                newFreeBlock = newUsedBlock->allocFore(size, mCurrentGroupId, 0, 0, 0);
            } else {
                newFreeBlock = nullptr;
            }
            if (newFreeBlock) {
                setFreeBlock(foundBlock, prev, newFreeBlock);
            } else {
                setFreeBlock(foundBlock, prev, next);
            }
            if (newFreeBlock) {
                setFreeBlock(newFreeBlock, foundBlock, next);
            }
            appendUsedList(newUsedBlock);
            DBnewFreeBlock = newFreeBlock;
            DBnewUsedBlock = newUsedBlock;
            return newUsedBlock->getContent();
        } else {
            if (foundOffset != 0) {
                CMemBlock* prev = foundBlock->mPrev;
                CMemBlock* next = foundBlock->mNext;
                removeFreeBlock(foundBlock);
                newUsedBlock = (CMemBlock*)((uintptr_t)foundBlock + foundOffset);
                newUsedBlock->mSize = foundBlock->mSize - foundOffset;
                newFreeBlock =
                    newUsedBlock->allocFore(size, mCurrentGroupId, (u8)foundOffset, 0, 0);
                if (newFreeBlock) {
                    setFreeBlock(newFreeBlock, prev, next);
                }
                appendUsedList(newUsedBlock);
                return newUsedBlock->getContent();
            } else {
                CMemBlock* prev = foundBlock->mPrev;
                CMemBlock* next = foundBlock->mNext;
                newFreeBlock = foundBlock->allocFore(size, mCurrentGroupId, 0, 0, 0);
                removeFreeBlock(foundBlock);
                if (newFreeBlock) {
                    setFreeBlock(newFreeBlock, prev, next);
                }
                appendUsedList(foundBlock);
                return foundBlock->getContent();
            }
        }
    }
    return nullptr;
}

void* JKRExpHeap::allocFromHead(u32 size) {
    // Native block headers contain pointers and require their natural alignment.
    size = ALIGN_NEXT(size, alignof(CMemBlock));
    s32 foundSize = -1;
    CMemBlock* foundBlock = nullptr;
    CMemBlock* newblock = nullptr;
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        if (block->mSize < size) {
            continue;
        }
        if (foundSize <= block->mSize) {
            continue;
        }
        foundSize = block->mSize;
        foundBlock = block;
        if (mAllocMode != 0) {
            break;
        }
        if (foundSize == size) {
            break;
        }
    }
    if (foundBlock) {
        newblock = foundBlock->allocFore(size, mCurrentGroupId, 0, 0, 0);
        if (newblock) {
            setFreeBlock(newblock, foundBlock->mPrev, foundBlock->mNext);
        } else {
            removeFreeBlock(foundBlock);
        }
        appendUsedList(foundBlock);
        return foundBlock->getContent();
    }
    return nullptr;
}

void* JKRExpHeap::allocFromTail(u32 size, int align) {
    u32 local_2c = 0;
    u32 offset = 0;
    CMemBlock* foundBlock = nullptr;
    CMemBlock* newBlock = nullptr;
    u32 usedSize;
    uintptr_t start;
    for (CMemBlock* block = mTailFreeList; block; block = block->mPrev) {
        start = ALIGN_PREV((uintptr_t)block->getContent() + block->mSize - size, uintptr_t(align));
        usedSize = (uintptr_t)block->getContent() + block->mSize - start;
        if (block->mSize >= usedSize) {
            local_2c = usedSize;
            foundBlock = block;
            offset = block->mSize - usedSize;
            newBlock = (CMemBlock*)start - 1;
            break;
        }
    }
    if (foundBlock != nullptr) {
        if (offset >= sizeof(CMemBlock)) {
            newBlock->initiate(nullptr, nullptr, usedSize, mCurrentGroupId, -0x80);
            foundBlock->mSize = foundBlock->mSize - usedSize - sizeof(CMemBlock);
            appendUsedList(newBlock);
            return newBlock->getContent();
        } else {
            if (offset != 0) {
                removeFreeBlock(foundBlock);
                newBlock->initiate(nullptr, nullptr, usedSize, mCurrentGroupId, offset | 0x80);
                appendUsedList(newBlock);
                return newBlock->getContent();
            } else {
                removeFreeBlock(foundBlock);
                newBlock->initiate(nullptr, nullptr, usedSize, mCurrentGroupId, -0x80);
                appendUsedList(newBlock);
                return newBlock->getContent();
            }
        }
    }
    return nullptr;
}

void* JKRExpHeap::allocFromTail(u32 size) {
    // Native block headers contain pointers and require their natural alignment.
    size = ALIGN_NEXT(size, alignof(CMemBlock));
    CMemBlock* foundBlock = nullptr;
    CMemBlock* freeBlock = nullptr;
    CMemBlock* usedBlock = nullptr;
    for (CMemBlock* block = mTailFreeList; block; block = block->mPrev) {
        if (block->mSize >= size) {
            foundBlock = block;
            break;
        }
    }
    if (foundBlock != nullptr) {
        usedBlock = foundBlock->allocBack(size, 0, 0, mCurrentGroupId, 0);
        if (usedBlock) {
            freeBlock = foundBlock;
        } else {
            removeFreeBlock(foundBlock);
            usedBlock = foundBlock;
            freeBlock = nullptr;
        }
        if (freeBlock) {
            setFreeBlock(freeBlock, foundBlock->mPrev, foundBlock->mNext);
        }
        appendUsedList(usedBlock);
        return usedBlock->getContent();
    }
    return nullptr;
}

void JKRExpHeap::do_free(void* ptr) {
    lock();
    if (mStart <= ptr && ptr <= mEnd) {
        CMemBlock* block = CMemBlock::getHeapBlock(ptr);
        if (block) {
            removeUsedBlock(block);
            recycleFreeBlock(block);
        }
    } else {

    }
    unlock();
}

void JKRExpHeap::do_freeAll() {
    lock();
    JKRHeap::callAllDisposer();
    mHeadFreeList = (CMemBlock*)mStart;
    mTailFreeList = mHeadFreeList;
    mHeadFreeList->initiate(nullptr, nullptr, mSize - sizeof(CMemBlock), 0, 0);
    mHeadUsedList = nullptr;
    mTailUsedList = nullptr;
    unlock();
}

void JKRExpHeap::do_freeTail() {
    lock();
    for (CMemBlock* block = mHeadUsedList; block != nullptr;) {
        if (block->isTempMemBlock()) {
            dispose(block->getContent(), block->mSize);
            CMemBlock* temp = block->mNext;
            removeUsedBlock(block);
            recycleFreeBlock(block);
            block = temp;
        } else {
            block = block->mNext;
        }
    }
    unlock();
}

void JKRExpHeap::do_fillFreeArea() {
}

s32 JKRExpHeap::do_changeGroupID(u8 param_0) {
    lock();
    u8 prev = mCurrentGroupId;
    mCurrentGroupId = param_0;
    unlock();
    return prev;
}

s32 JKRExpHeap::do_resize(void* ptr, u32 size) {
    lock();
    CMemBlock* block = CMemBlock::getHeapBlock(ptr);
    if (block == nullptr || ptr < mStart || mEnd < ptr) {
        unlock();
        return -1;
    }
    // Native block headers contain pointers and require their natural alignment.
    size = ALIGN_NEXT(size, alignof(CMemBlock));
    if (size == block->mSize) {
        unlock();
        return size;
    }
    if (size > block->mSize) {
        CMemBlock* foundBlock = nullptr;
        for (CMemBlock* freeBlock = mHeadFreeList; freeBlock; freeBlock = freeBlock->mNext) {
            if (freeBlock == (CMemBlock*)((uintptr_t)(block + 1) + block->mSize)) {
                foundBlock = freeBlock;
                break;
            }
        }
        if (foundBlock == nullptr) {
            unlock();
            return -1;
        }
        if (size > block->mSize + sizeof(CMemBlock) + foundBlock->mSize) {
            unlock();
            return -1;
        }
        u32 local_24 = block->mSize;
        removeFreeBlock(foundBlock);
        block->mSize += foundBlock->mSize + sizeof(CMemBlock);
        if (block->mSize - size > sizeof(CMemBlock)) {
            CMemBlock* newBlock = block->allocFore(size, block->mGroupId, block->mFlags, 0, 0);
            if (newBlock) {
                recycleFreeBlock(newBlock);
            }
        }
    } else {
        if (block->mSize - size > sizeof(CMemBlock)) {
            CMemBlock* freeBlock = block->allocFore(size, block->mGroupId, block->mFlags, 0, 0);
            if (freeBlock) {
                recycleFreeBlock(freeBlock);
            }
        }
    }
    unlock();
    return block->mSize;
}

s32 JKRExpHeap::do_getSize(void* ptr) {
    lock();
    CMemBlock* block = CMemBlock::getHeapBlock(ptr);
    if (!block || ptr < mStart || mEnd < ptr) {
        unlock();
        return -1;
    }
    unlock();
    return block->mSize;
}

s32 JKRExpHeap::do_getFreeSize() {
    lock();
    s32 size = 0;
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        if (size < (s32)block->mSize) {
            size = block->mSize;
        }
    }
    unlock();
    return size;
}

void* JKRExpHeap::do_getMaxFreeBlock() {
    lock();
    s32 size = 0;
    CMemBlock* res = nullptr;
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        if (size < (s32)block->mSize) {
            size = block->mSize;
            res = block;
        }
    }
    unlock();
    return res;
}

s32 JKRExpHeap::do_getTotalFreeSize() {
    u32 size = 0;
    lock();
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        size += block->mSize;
    }
    unlock();
    return size;
}

s32 JKRExpHeap::getUsedSize(u8 groupId) const {
    lock();
    u32 size = 0;
    for (CMemBlock* block = mHeadUsedList; block; block = block->mNext) {
        if (block->mGroupId == groupId) {
            size += block->mSize + sizeof(CMemBlock);
        }
    }
    unlock();
    return size;
}

bool JKRExpHeap::isEmpty() {
    lock();
    bool result = !mHeadUsedList ? true : false;
    unlock();
    return result;
}

void JKRExpHeap::appendUsedList(JKRExpHeap::CMemBlock* newblock) {
    if (!newblock) {
        smgpc::compat::jkr_panic("JKRExpHeap.cpp", 1568, "%s", "bad appendUsedList\n");
    }
    CMemBlock* block = mTailUsedList;
    newblock->mMagic = 'HM';
    if (block) {
        block->mNext = newblock;
        newblock->mPrev = block;
    } else {
        newblock->mPrev = nullptr;
    }
    mTailUsedList = newblock;
    if (!mHeadUsedList) {
        mHeadUsedList = newblock;
    }
    newblock->mNext = nullptr;
}

void JKRExpHeap::setFreeBlock(CMemBlock* block, CMemBlock* prev, CMemBlock* next) {
    if (prev == nullptr) {
        mHeadFreeList = block;
        block->mPrev = nullptr;
    } else {
        prev->mNext = block;
        block->mPrev = prev;
    }
    if (next == nullptr) {
        mTailFreeList = block;
        block->mNext = nullptr;
    } else {
        next->mPrev = block;
        block->mNext = next;
    }
    block->mMagic = 0;
}

void JKRExpHeap::removeFreeBlock(CMemBlock* block) {
    CMemBlock* prev = block->mPrev;
    CMemBlock* next = block->mNext;
    if (prev == nullptr) {
        mHeadFreeList = next;
    } else {
        prev->mNext = next;
    }
    if (next == nullptr) {
        mTailFreeList = prev;
    } else {
        next->mPrev = prev;
    }
}

void JKRExpHeap::removeUsedBlock(JKRExpHeap::CMemBlock* block) {
    CMemBlock* prev = block->mPrev;
    CMemBlock* next = block->mNext;
    if (prev == nullptr) {
        mHeadUsedList = next;
    } else {
        prev->mNext = next;
    }
    if (next == nullptr) {
        mTailUsedList = prev;
    } else {
        next->mPrev = prev;
    }
}

void JKRExpHeap::recycleFreeBlock(JKRExpHeap::CMemBlock* block) {
    JKRExpHeap::CMemBlock* newBlock = block;
    int size = block->mSize;
    void* blockEnd = (u8*)newBlock + size;
    block->mMagic = 0;
    if ((block->mFlags & 0x7f) != 0) {
        newBlock = (CMemBlock*)((u8*)newBlock - (block->mFlags & 0x7f));
        size += (block->mFlags & 0x7f);
        blockEnd = (u8*)newBlock + size;
        newBlock->mGroupId = 0;
        newBlock->mFlags = 0;
        newBlock->mSize = size;
    }
    if (!mHeadFreeList) {
        newBlock->initiate(nullptr, nullptr, size, 0, 0);
        mHeadFreeList = newBlock;
        mTailFreeList = newBlock;
        setFreeBlock(newBlock, nullptr, nullptr);
        return;
    }
    if (mHeadFreeList >= blockEnd) {
        newBlock->initiate(nullptr, nullptr, size, 0, 0);
        setFreeBlock(newBlock, nullptr, mHeadFreeList);
        joinTwoBlocks(newBlock);
        return;
    }
    if (mTailFreeList <= newBlock) {
        newBlock->initiate(nullptr, nullptr, size, 0, 0);
        setFreeBlock(newBlock, mTailFreeList, nullptr);
        joinTwoBlocks(newBlock->mPrev);
        return;
    }
    for (CMemBlock* freeBlock = mHeadFreeList; freeBlock; freeBlock = freeBlock->mNext) {
        if (freeBlock >= newBlock || newBlock >= freeBlock->mNext) {
            continue;
        }
        newBlock->mNext = freeBlock->mNext;
        newBlock->mPrev = freeBlock;
        freeBlock->mNext = newBlock;
        newBlock->mNext->mPrev = newBlock;
        newBlock->mGroupId = 0;
        joinTwoBlocks(newBlock);
        joinTwoBlocks(freeBlock);
        return;
    }
}

void JKRExpHeap::joinTwoBlocks(CMemBlock* block) {
    uintptr_t endAddr = (uintptr_t)(block + 1) + block->mSize;
    CMemBlock* next = block->mNext;
    uintptr_t nextAddr = (uintptr_t)next - (next->mFlags & 0x7f);
    if (endAddr > nextAddr) {
        smgpc::compat::jkr_warning_f(":::Heap may be broken. (block = %p)", (void*)block);
        JKRHeap* heap = JKRHeap::getCurrentHeap();
        heap->dump();
        smgpc::compat::jkr_panic("JKRExpHeap.cpp", 1820, "%s", "Bad Block\n");
    }
    if (endAddr == nextAddr) {
        block->mSize = next->mSize + sizeof(CMemBlock) + (next->mFlags & 0x7f) + block->mSize;
        CMemBlock* local_30 = next->mNext;
        setFreeBlock(block, block->mPrev, local_30);
    }
}

bool JKRExpHeap::check() {
    lock();
    int totalBytes = 0;
    bool ok = true;
    for (CMemBlock* block = mHeadUsedList; block; block = block->mNext) {
        if (block->mMagic != 'HM') {
            ok = false;
            smgpc::compat::jkr_warning_f(":::addr %p: bad heap signature. (%c%c)\n", (void*)block,
                                JSUHiByte(block->mMagic), JSULoByte(block->mMagic));
        }
        if (block->mNext) {
            if (block->mNext->mMagic != 'HM') {
                ok = false;
                smgpc::compat::jkr_warning_f(":::addr %p: bad next pointer (%p)\nabort\n", (void*)block,
                                    (void*)block->mNext);
                break;
            }
            if (block->mNext->mPrev != block) {
                ok = false;
                smgpc::compat::jkr_warning_f(":::addr %p: bad previous pointer (%p)\n", (void*)block->mNext,
                                    (void*)block->mNext->mPrev);
            }
        } else {
            if (mTailUsedList != block) {
                ok = false;
                smgpc::compat::jkr_warning_f(":::addr %p: bad used list(REV) (%p)\n", (void*)block,
                                    (void*)mTailUsedList);
            }
        }
        totalBytes += sizeof(CMemBlock) + block->mSize + block->getAlignment();
    }
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        totalBytes += block->mSize + sizeof(CMemBlock);
        if (block->mNext) {
            if (block->mNext->mPrev != block) {
                ok = false;
                smgpc::compat::jkr_warning_f(":::addr %p: bad previous pointer (%p)\n", (void*)block->mNext,
                                    (void*)block->mNext->mPrev);
            }
            if ((uintptr_t)block + block->mSize + sizeof(CMemBlock) > (uintptr_t)block->mNext) {
                ok = false;
                smgpc::compat::jkr_warning_f(":::addr %p: bad block size (%08x)\n", (void*)block, block->mSize);
            }
        } else {
            if (mTailFreeList != block) {
                ok = false;
                smgpc::compat::jkr_warning_f(":::addr %p: bad used list(REV) (%p)\n", (void*)block,
                                    (void*)mTailFreeList);
            }
        }
    }
    if (totalBytes != mSize) {
        ok = false;
        smgpc::compat::jkr_warning_f(":::bad total memory block size (%08X, %08X)\n", mSize, static_cast<u32>(totalBytes));
    }
    if (!ok) {
        smgpc::compat::jkr_warning(":::there is some error in this heap!\n");
    }
    unlock();
    return ok;
}

bool JKRExpHeap::dump() {
    lock();
    bool result = check();
    u32 usedBytes = 0;
    u32 usedCount = 0;
    u32 freeCount = 0;
    smgpc::compat::jkr_report(" attr  address:   size    gid aln   prev_ptr next_ptr\n");
    smgpc::compat::jkr_report("(Used Blocks)\n");
    if (!mHeadUsedList) {
        smgpc::compat::jkr_report(" NONE\n");
    }
    for (CMemBlock* block = mHeadUsedList; block; block = block->mNext) {
        if (block->mMagic != 'HM') {
            smgpc::compat::jkr_report_f("xxxxx %p: --------  --- ---  (-------- --------)\nabort\n",
                               (void*)block);
            break;
        }
        smgpc::compat::jkr_report_f("%s %p: %08x  %3d %3d  (%p %p)\n",
                           block->isTempMemBlock() ? " temp" : "alloc", block->getContent(),
                           block->mSize, block->mGroupId, block->getAlignment(), (void*)block->mPrev,
                           (void*)block->mNext);
        usedBytes += sizeof(CMemBlock) + block->mSize + block->getAlignment();
        usedCount++;
    }
    smgpc::compat::jkr_report("(Free Blocks)\n");
    if (!mHeadFreeList) {
        smgpc::compat::jkr_report(" NONE\n");
    }
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        smgpc::compat::jkr_report_f("%s %p: %08x  %3d %3d  (%p %p)\n", " free", block->getContent(),
                           block->mSize, block->mGroupId, block->getAlignment(), (void*)block->mPrev,
                           (void*)block->mNext);
        freeCount++;
    }
    smgpc::compat::jkr_report_f("%u / %u bytes (%6.2f%%) used (U:%u F:%u)\n", usedBytes, mSize, (f32(usedBytes) / f32(mSize)) * 100.0f,
                       usedCount, freeCount);
    unlock();
    return result;
}

bool JKRExpHeap::dump_sort() {
    lock();
    bool result = check();
    u32 usedBytes = 0;
    u32 usedCount = 0;
    u32 freeCount = 0;
    smgpc::compat::jkr_report(" attr  address:   size    gid aln   prev_ptr next_ptr\n");
    smgpc::compat::jkr_report("(Used Blocks)\n");
    if (mHeadUsedList == nullptr) {
        smgpc::compat::jkr_report(" NONE\n");
    } else {
        CMemBlock* var1 = nullptr;
        while (true) {
            CMemBlock* block = (CMemBlock*)UINTPTR_MAX;
            for (CMemBlock* iterBlock = mHeadUsedList; iterBlock; iterBlock = iterBlock->mNext) {
                if (var1 < iterBlock && iterBlock < block) {
                    block = iterBlock;
                }
            }
            if (uintptr_t(block) == UINTPTR_MAX) {
                break;
            }
            if (block->mMagic != 'HM') {
                smgpc::compat::jkr_report_f("xxxxx %p: --------  --- ---  (-------- --------)\nabort\n",
                                   (void*)var1);
                break;
            }
            smgpc::compat::jkr_report_f("%s %p: %08x  %3d %3d  (%p %p)\n", block->isTempMemBlock() ? " temp" : "alloc", block->getContent(), block->mSize,
                               block->mGroupId, block->getAlignment(), (void*)block->mPrev, (void*)block->mNext);
            usedBytes += sizeof(CMemBlock) + block->mSize + block->getAlignment();
            usedCount++;
            var1 = block;
        }
    }
    smgpc::compat::jkr_report("(Free Blocks)\n");
    if (mHeadFreeList == nullptr) {
        smgpc::compat::jkr_report(" NONE\n");
    }
    for (CMemBlock* block = mHeadFreeList; block; block = block->mNext) {
        smgpc::compat::jkr_report_f("%s %p: %08x  %3d %3d  (%p %p)\n", " free", block->getContent(),
                           block->mSize, block->mGroupId, block->getAlignment(), (void*)block->mPrev,
                           (void*)block->mNext);
        freeCount++;
    }
    smgpc::compat::jkr_report_f("%u / %u bytes (%6.2f%%) used (U:%u F:%u)\n", usedBytes, mSize, (f32(usedBytes) / f32(mSize)) * 100.0f,
                       usedCount, freeCount);
    unlock();
    return result;
}

void JKRExpHeap::state_register(JKRHeap::TState* p, u32 param_1) const {


    void* r24 = getState_buf_(p);
    u32 r25 = param_1;
    setState_u32ID_(p, param_1);
    if (param_1 <= 0xff) {
        setState_uUsedSize_(p, getUsedSize(r25));
    } else {
        setState_uUsedSize_(p, mSize - const_cast<JKRExpHeap*>(this)->getTotalFreeSize());
    }
    u32 checkCode = 0;
    for (CMemBlock* block = mHeadUsedList; block; block = block->mNext) {
        if (param_1 <= 0xff) {
            if (block->mGroupId == param_1) {
                checkCode += (uintptr_t)block * 3;
            }
        } else {
            checkCode += (uintptr_t)block * 3;
        }
    }
    setState_u32CheckCode_(p, checkCode);
}

bool JKRExpHeap::state_compare(JKRHeap::TState const& r1, JKRHeap::TState const& r2) const {

    bool result = true;
    if (r1.getCheckCode() != r2.getCheckCode()) {
        result = false;
    }
    if (r1.getUsedSize() != r2.getUsedSize()) {
        result = false;
    }
    return result;
}

u32 JKRExpHeap::getHeapType() {
    return 'EXPH';
}

u8 JKRExpHeap::do_getCurrentGroupId() {
    return mCurrentGroupId;
}

s32 JKRExpHeap::adjustSize() {
    JKRHeap* parent = getParent();
    if (parent == nullptr) {
        return -1;
    }

    lock();
    u8* end = mStart;
    for (CMemBlock* block = mHeadUsedList; block != nullptr; block = block->mNext) {
        u8* blockEnd = (u8*)block + block->mSize + sizeof(CMemBlock);
        if (blockEnd > end) {
            end = blockEnd;
        }
    }

    if (end == mEnd) {
        unlock();
        return -1;
    }
    if (parent->getHeapType() != 'EXPH') {
        unlock();
        return -1;
    }

    CMemBlock* block = mHeadFreeList;
    while (block != nullptr) {
        CMemBlock* next = block->mNext;
        if ((u8*)block >= end) {
            if (next != nullptr) {
                next = next->mNext;
            }
            removeFreeBlock(block);
        }
        block = next;
    }

    if (mHeadFreeList == nullptr) {
        CMemBlock* freeBlock = (CMemBlock*)end;
        freeBlock->initiate(nullptr, nullptr, 0, 0, 0);
        mHeadFreeList = freeBlock;
        mTailFreeList = freeBlock;
        end += sizeof(CMemBlock);
    }

    u32 size = end - (u8*)this;
    parent->resize(this, size);
    mEnd = end;
    mSize = end - mStart;
    unlock();
    return size;
}
