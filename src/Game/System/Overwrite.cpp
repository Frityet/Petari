#include "Game/System/WPad.hpp"
#include "JSystem/JUtility/JUTException.hpp"
#include "JSystem/JAudio2/JASHeapCtrl.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include "JSystem/JKernel/JKRThread.hpp"
#include "JSystem/JKernel/JKRAram.hpp"
#include "JSystem/JKernel/JKRAramPiece.hpp"
#include <revolution/aralt.h>
#include <JSystem/JAudio2/JASAudioThread.hpp>

JASAudioThread::JASAudioThread(int stackSize, int msgCount, u32 threadPriority)
    : JKRThread(JASDram, threadPriority, msgCount, stackSize), JASGlobalInstance< JASAudioThread >(true) {
    sbPauseFlag = false;
    OSInitThreadQueue(&sThreadQueue);
}

bool JUTException::readPad(u32* pTrigger, u32* pHold) {
    OSTime startTime = OSGetTime();
    OSTime elapsed;

    do {
        elapsed = OSTicksToMilliseconds(OSGetTime() - startTime);
    } while (elapsed < 50);

    *pHold = 0;
    *pTrigger = 0;

    MR::getPadDataForExceptionNoInit(WPAD_CHAN0, pHold, pTrigger);
    MR::getPadDataForExceptionNoInit(WPAD_CHAN1, pHold, pTrigger);
    MR::getPadDataForExceptionNoInit(WPAD_CHAN2, pHold, pTrigger);
    MR::getPadDataForExceptionNoInit(WPAD_CHAN3, pHold, pTrigger);

    return true;
}

namespace {
    u32 sAramThreadStackSize = 0xC000;
    s32 sAramThreadMsgSize = 0x20;
}

void JKRAramPiece::startDMA(JKRAMCommand* pCommand) {
    if (pCommand->mSrc < 0x80000000) {
        doneDMA(reinterpret_cast< u32 >(pCommand));
    } else if (pCommand->mDst >= 0x4000000) {
        doneDMA(reinterpret_cast< u32 >(pCommand));
    } else if (pCommand->mDataLength > 0xE00000) {
        doneDMA(reinterpret_cast< u32 >(pCommand));
    } else {
        ARStartDMA(pCommand->mTransferDirection, pCommand->mSrc, pCommand->mDst, pCommand->mDataLength);
        doneDMA(reinterpret_cast< u32 >(pCommand));
    }
}

JKRAram::JKRAram(u32 audioSize, u32 graphSize, s32 priority) : JKRThread(sAramThreadStackSize, sAramThreadMsgSize, priority) {
    u32 baseAddress = ARInit(mStackArray, 3);
    ARQInit();
    u32 aramSize = ARGetSize();
    mAudioMemorySize = audioSize;
    if (graphSize == 0xFFFFFFFF) {
        mGraphMemorySize = aramSize - audioSize - baseAddress;
        mAramMemorySize = 0;
    } else {
        mGraphMemorySize = graphSize;
        mAramMemorySize = aramSize - (audioSize + graphSize) - baseAddress;
    }
    mAudioMemoryPtr = ARAlloc(mAudioMemorySize);
    mGraphMemoryPtr = ARAlloc(mGraphMemorySize);
    if (mAramMemorySize != 0) {
        mAramMemoryPtr = ARAlloc(mAramMemorySize);
    } else {
        mAramMemoryPtr = 0;
    }
    mAramHeap = new (JKRGetSystemHeap(), 0) JKRAramHeap(mGraphMemoryPtr, mGraphMemorySize);
}
