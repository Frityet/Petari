#include "Game/System/DrawSyncManager.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <stdint.h>

DrawSyncManager* DrawSyncManager::sInstance = nullptr;

DrawSyncManager* DrawSyncManager::start(u32 a1, s32 a2) {
    if (!DrawSyncManager::sInstance) {
        DrawSyncManager::sInstance = new DrawSyncManager(a1, a2);
    }

    return DrawSyncManager::sInstance;
}

void DrawSyncManager::prepareReset() {
    if (!DrawSyncManager::sInstance) {
        return;
    }

    DrawSyncManager::sInstance->reset(true);
}

void DrawSyncManager::resetIfAborted() {
    if (!DrawSyncManager::sInstance) {
        return;
    }

    DrawSyncManager::sInstance->reset(false);
}

void DrawSyncManager::clearFifo() {
    while (mFifo->getCount() != 0) {
        mFifo->pop();
    }
}

void* Fifo::pop() {
    void* cur = mArray[mLoopIdx];
    mLoopIdx = getLoopIdx(mLoopIdx + 1);
    return cur;
}

u32 Fifo::getLoopIdx(u32 idx) {
    if (idx >= mCount + 1) {
        idx = 0;
    }

    return idx;
}

u32 Fifo::getCount() {
    if (mLoopIdx <= _C) {
        return _C - mLoopIdx;
    }

    return _C + mCount + 1 - mLoopIdx;
}

void DrawSyncManager::end() {
    if (!DrawSyncManager::sInstance) {
        return;
    }

    delete DrawSyncManager::sInstance;
    DrawSyncManager::sInstance = 0;
}

void DrawSyncManager::drawSyncCallback(u16 token) {
    if (sInstance != nullptr) {
        sInstance->drawSyncCallbackSub(token);
    }
}

void* DrawSyncManager::threadFunc(void* pArg) {
    DrawSyncManager* pManager = static_cast< DrawSyncManager* >(pArg);

    while (true) {
        OSMessage message;
        OSReceiveMessage(&pManager->mQueue, &message, OS_MESSAGE_BLOCK);

        if (reinterpret_cast< uintptr_t >(message) >= 0x80000000) {
            Fifo* pFifo = pManager->mFifo;
            pFifo->mArray[pFifo->_C] = message;
            pFifo->_C = pFifo->getLoopIdx(pFifo->_C + 1);
            (void)pManager->_373;

            if (pManager->mFifo->getCount() == 2) {
                GXEnableBreakPt(message);
            }
        } else if (reinterpret_cast< uintptr_t >(message) < 0x10000) {
            pManager->mFifo->pop();
            (void)pManager->_373;
            u32 count = pManager->mFifo->getCount();

            if (count == 0) {
                continue;
            }

            if (count == 1) {
                GXDisableBreakPt();
            } else if (count >= 2) {
                Fifo* pFifo = pManager->mFifo;
                GXEnableBreakPt(pFifo->mArray[pFifo->getLoopIdx(pFifo->mLoopIdx + 1)]);
            }
        } else {
            return nullptr;
        }
    }
}

DrawSyncManager::DrawSyncManager(u32 count, s32 priority)
    : _372(0), mStack(nullptr), mMessages(nullptr), _36C(0), _36E(0), _370(0), _373(0) {
    mStack = new u8[0x8000];
    OSCreateThread(&mThread, threadFunc, this, mStack + 0x8000, 0x8000, priority, 0);
    mMessages = new OSMessage[20];
    OSInitMessageQueue(&mQueue, mMessages, 20);
    mFifo = new Fifo(count);
    OSResumeThread(&mThread);
    GXSetDrawSyncCallback(drawSyncCallback);
}

DrawSyncManager::~DrawSyncManager() {
    GXSetDrawSyncCallback(0);
    GXDisableBreakPt();
    OSSendMessage(&mQueue, (void*)0x10000, OS_MESSAGE_BLOCK);
    OSJoinThread(&mThread, 0);
}

u16 DrawSyncManager::setCallback(u32 index, u16 count, DrawSyncCallback* pCallback) {
    if (index < 3) {
        u16 start = _36E + 1;
        mTokenRanges[index] = TDrawSyncTokenRange(start, start + count - 1, pCallback);
        _36E += count;
        return start;
    } else {
        u16 start = _370 + 0xA000;
        mTokenRanges[index] = TDrawSyncTokenRange(start, start + count - 1, pCallback);
        _370 += count;
        return start;
    }
}

void DrawSyncManager::reset(bool arg) {
    if (arg) {
        _372 = 1;
    } else if (_372) {
        _372 = 0;
        _373 = 1;
        GXSetDrawSyncCallback(&DrawSyncManager::drawSyncCallback);
        SingletonHolder< GameSystem >::get()->initGX();
    }
}

void DrawSyncManager::drawSyncCallbackSub(u16 token) {
    if (token == 0) {
        if (!(_36C & 2)) {
            OSSendMessage(&mQueue, nullptr, OS_MESSAGE_BLOCK);
        }
    } else {
        for (u16 i = 0; i < 5; i++) {
            TDrawSyncTokenRange& range = mTokenRanges[i];

            if (range.mCallback != nullptr && range.mStart <= token && token <= range.mEnd) {
                range.mCallback->drawSyncCallback(token);

                if (!(_36C & 2)) {
                    OSSendMessage(&mQueue, reinterpret_cast< OSMessage >(static_cast< uintptr_t >(token)), OS_MESSAGE_BLOCK);
                }

                break;
            }
        }
    }
}

void DrawSyncManager::pushBreakPoint() {
    if (!(_36C & 0x3)) {
        GXFlush();
        GXFifoObj fifoObj;
        GXGetCPUFifo(&fifoObj);

        void *readPtr, *writePtr;

        GXGetFifoPtrs(&fifoObj, &readPtr, &writePtr);
        OSSendMessage(&mQueue, writePtr, OS_MESSAGE_BLOCK);
    }
}
