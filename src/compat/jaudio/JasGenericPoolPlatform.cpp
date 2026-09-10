#include <JSystem/JAudio2/JASHeapCtrl.hpp>
#include <JSystem/JKernel/JKRSolidHeap.hpp>

JKRSolidHeap* JASDram;

JASGenericMemPool::JASGenericMemPool() {
    _0 = nullptr;
    mFreeMemCount = 0;
    mTotalMemCount = 0;
    mUsedMemCount = 0;
}

JASGenericMemPool::~JASGenericMemPool() {
    void* chunk = _0;
    while (chunk != nullptr) {
        void* next_chunk = *(void**)chunk;
        delete[] static_cast<u8*>(chunk);
        chunk = next_chunk;
    }
}

void JASGenericMemPool::newMemPool(u32 size, int n) {
    void* runner;
    for (int i = 0; i < n; i++) {
        runner = new (JASDram, 0) u8[size];
        *(void**)runner = _0;
        _0 = runner;
    }
    mFreeMemCount += n;
    mTotalMemCount += n;
}

void* JASGenericMemPool::alloc(u32 size) {
    if (_0 == nullptr) {
        return nullptr;
    }
    void* chunk = _0;
    _0 = *(void**)chunk;
    mFreeMemCount--;
    if (mUsedMemCount < mTotalMemCount - mFreeMemCount) {
        mUsedMemCount = mTotalMemCount - mFreeMemCount;
    }
    return chunk;
}

void JASGenericMemPool::free(void* ptr, u32 param_1) {
    if (!ptr) {
        return;
    }
    void* chunk = ptr;
    *(void**)chunk = _0;
    _0 = chunk;
    mFreeMemCount++;
}

#include <aurora/exception.hpp>
#include <stdexcept>
// Native process owners may end before exit; detach the fully returned original pool.
void retire_jas_pool(JASGenericMemPool& pool) {
    if (pool.mFreeMemCount != pool.mTotalMemCount) {
        aurora::throw_host_exception<std::logic_error>("Cannot retire a JAS pool with live original owners");
    }
    void* chunk = pool._0;
    pool._0 = nullptr;
    pool.mFreeMemCount = pool.mTotalMemCount = pool.mUsedMemCount = 0;
    while (chunk != nullptr) {
        void* next = *static_cast<void**>(chunk);
        delete[] static_cast<u8*>(chunk);
        chunk = next;
    }
}

#include <cstdarg>
#include <cstdio>
void JASReport(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
}
