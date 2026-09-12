#include "Game/NWC24/NWC24System.hpp"
#include "Game/NWC24/NWC24SendThread.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <revolution/nwc24.h>
// #include <revolution/vf.h>

#define NWC24_WORK_MEM_SIZE 16384
#define VF_DRIVE_WORKSIZE (16 * 1024)

extern "C" {
NWC24Err NWC24OpenLib(void*);
NWC24Err NWC24CloseLib(void);
s32 NWC24GetErrorCode(void);
void VFInitEx(void*, u32);
};

namespace {
    static u32 sWorkSize = VF_DRIVE_WORKSIZE;
};  // namespace

NWC24System::NWC24System(JKRHeap* pHeap, s32 threadPriority) : _0(false), mWorkBuffer(nullptr), mVFWorkBuffer(nullptr) {
#if defined(TARGET_PC)
    // The native platform has no WiiConnect24/message-board service. Keep
    // this optional owner closed; VF and its sending worker are not started.
    mSendThread = nullptr;
#else
    mVFWorkBuffer = new (pHeap, 32) u8[::sWorkSize];
    VFInitEx(mVFWorkBuffer, ::sWorkSize);
    mWorkBuffer = new (pHeap, 32) u8[NWC24_WORK_MEM_SIZE];
    mSendThread = new NWC24SendThread(threadPriority, pHeap);
#endif
}

bool NWC24System::open(NWC24Err* pErr, s32* pErrCode) {
#if defined(TARGET_PC)
    NWC24Err err = NWC24_ERR_DISABLED;
#else
    NWC24Err err = NWC24OpenLib(mWorkBuffer);
#endif

    if (pErr != nullptr) {
        *pErr = err;
    }

    if (err != NWC24_OK && pErrCode != nullptr) {
#if defined(TARGET_PC)
        *pErrCode = NWC24_ERR_DISABLED;
#else
        *pErrCode = NWC24GetErrorCode();
#endif
    }

    if (err != NWC24_OK) {
        _0 = false;

        return false;
    } else {
        _0 = true;

        return true;
    }
}

bool NWC24System::close(NWC24Err* pErr) {
    _0 = false;

#if defined(TARGET_PC)
    NWC24Err err = NWC24_ERR_LIB_NOT_OPENED;
#else
    NWC24Err err = NWC24CloseLib();
#endif

    if (pErr != nullptr) {
        *pErr = err;
    }

    if (err != NWC24_OK) {
        return false;
    } else {
        return true;
    }
}

bool NWC24System::send(const u16* pText, const u16* pAltName, const u8* pLetter, u32 letterSize, const u8* pPicture, u32 pictureSize, u16 tag,
                       bool isMsgLedPattern, u8 delayHours) {
#if defined(TARGET_PC)
    return false;
#else
    return mSendThread->requestSend(pText, pAltName, pLetter, letterSize, pPicture, pictureSize, tag, isMsgLedPattern, delayHours);
#endif
}

bool NWC24System::isSent(NWC24Err* pErr, u32* pSize) {
#if defined(TARGET_PC)
    if (pErr != nullptr) {
        *pErr = NWC24_ERR_DISABLED;
    }
    return false;
#else
    return mSendThread->isDone(pErr, pSize);
#endif
}
