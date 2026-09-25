#pragma once

#include <revolution.h>
#include <array>

class JKRHeap;

class DrawSyncCallback {
public:
    DrawSyncCallback() {
    }

    virtual void drawSyncCallback(u16) = 0;
};

class Fifo {
public:
    Fifo(u32 count) : mCount(count), mLoopIdx(0), _C(0) {
        mArray = new void*[mCount + 1];
    }

    ~Fifo() { delete[] mArray; }

    void* pop();
    u32 getLoopIdx(u32);
    u32 getCount();

    void** mArray;  // 0x0
    u32 mCount;     // 0x4
    u32 mLoopIdx;   // 0x8
    u32 _C;
};

class DrawSyncManager {
public:
    struct TDrawSyncTokenRange {
        TDrawSyncTokenRange() : mStart(0), mEnd(0), mCallback(nullptr) {
        }

        TDrawSyncTokenRange(u16 start, u16 end, DrawSyncCallback* pCallback) : mStart(start), mEnd(end), mCallback(pCallback) {
        }

        u16 mStart;
        u16 mEnd;
        DrawSyncCallback* mCallback;
    };

    // Failed native construction must restore the actual callback ranges before
    // destroying objects that a submitted token may still reference.
    class CallbackRegistration {
    public:
        CallbackRegistration();
        ~CallbackRegistration();
        CallbackRegistration(const CallbackRegistration&) = delete;
        CallbackRegistration& operator=(const CallbackRegistration&) = delete;
        void commit() noexcept;
        void rollback();

    private:
        DrawSyncManager* mManager = nullptr;
        std::array<TDrawSyncTokenRange, 5> mRanges;
        u16 mLow = 0;
        u16 mHigh = 0;
    };

    static void quiesceNativeCallbacks();
    static void retireNativeCallbacks(const JKRHeap&);

    DrawSyncManager(u32, s32);

    ~DrawSyncManager();

    void reset(bool);

    static DrawSyncManager* start(u32, s32);
    static void prepareReset();
    static void resetIfAborted();

    void clearFifo();
    static void end();

    void pushBreakPoint();

    static void drawSyncCallback(u16);
    static void* threadFunc(void*);
    void drawSyncCallbackSub(u16);

    u16 setCallback(u32, u16, DrawSyncCallback*);

    static DrawSyncManager* sInstance;

    TDrawSyncTokenRange mTokenRanges[5];
    OSThread mThread;       // 0x28
    OSMessageQueue mQueue;  // 0x340
    u8* mStack;  // 0x360
    OSMessage* mMessages;  // 0x364
    Fifo* mFifo;  // 0x368
    u16 _36C;
    u16 _36E;
    u16 _370;
    u8 _372;
    volatile u8 _373;
    u32 _374;
};
