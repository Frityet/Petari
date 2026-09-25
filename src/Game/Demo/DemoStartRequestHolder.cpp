#include "compat/Cp932Literal.hpp"
#include "Game/Demo/DemoStartRequestHolder.hpp"
#include "Game/Demo/DemoStartRequestUtil.hpp"
#include "Game/Demo/DemoExecutor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include <array>
#include <exception>
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/StringUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

DemoStartInfo::DemoStartInfo() {
    _0 = nullptr;
    _4 = nullptr;
    _8 = nullptr;
    _C = nullptr;
    _10 = nullptr;
    _14 = nullptr;
    mDemoName = nullptr;
    _1C = nullptr;
    _20 = nullptr;
    _24 = 0;
    _28 = 0;
    _2C = 0;
    _30 = 0;
    _34 = 0;
}

DemoStartInfo& DemoStartInfo::operator=(const DemoStartInfo& rOther) {
    _0 = rOther._0;
    _4 = rOther._4;
    _8 = rOther._8;
    _C = rOther._C;
    _10 = rOther._10;
    _14 = rOther._14;
    mDemoName = rOther.mDemoName;
    _1C = rOther._1C;
    _20 = rOther._20;
    _24 = rOther._24;
    _28 = rOther._28;
    _2C = rOther._2C;
    _30 = rOther._30;
    _34 = rOther._34;
    return *this;
}

void DemoStartRequestHolder::pushRequest(LiveActor* pActor, const char* pName) {
    const DemoStartInfo* pInfo = find(pActor, pName);
    if (pInfo == nullptr) {
        pInfo = findEmpty();
    }
    mRequestBuffer.push_back(pInfo);
}

void DemoStartRequestHolder::pushRequest(LayoutActor* pActor, const char* pName) {
    const DemoStartInfo* pInfo = find(pActor, pName);
    if (pInfo == nullptr) {
        pInfo = findEmpty();
    }
    mRequestBuffer.push_back(pInfo);
}

void DemoStartRequestHolder::pushRequest(NerveExecutor* pExecutor, const char* pName) {
    const DemoStartInfo* pInfo = find(pExecutor, pName);
    if (pInfo == nullptr) {
        pInfo = findEmpty();
    }
    mRequestBuffer.push_back(pInfo);
}

void DemoStartRequestHolder::pushRequest(NameObj* pObj, const char* pName) {
    const DemoStartInfo* pInfo = find(pObj, pName);
    if (pInfo == nullptr) {
        pInfo = findEmpty();
    }
    mRequestBuffer.push_back(pInfo);
}

void DemoStartRequestHolder::popRequest() {
    DemoStartInfo** pIter = mStartInfos;
    const DemoStartInfo** pCurReqPtr = mRequestBuffer.mHead.mHead;
    DemoStartInfo** pEnd = &mStartInfos[mNumInfos];

    goto check;
    do {
        pIter++;
    check:
        if (pIter == pEnd) {
            break;
        }
    } while (*pIter != *pCurReqPtr);

    if (pIter != pEnd) {
        DemoStartInfo* pFound = *pIter;
        DemoStartInfo emptyInfo;
        *pFound = emptyInfo;
    }

    if (mRequestBuffer.mCount != 0) {
        ++mRequestBuffer.mHead;
        mRequestBuffer.mCount--;
    }
}

bool DemoStartRequestHolder::isExistRequest() const {
    return mRequestBuffer.mCount != 0;
}

const DemoStartInfo* DemoStartRequestHolder::getCurrentInfo() const {
    if (mRequestBuffer.mCount == 0) {
        return nullptr;
    }

    return *mRequestBuffer.mHead.mHead;
}

void DemoStartRequestHolder::registerStartDemoInfo(const DemoStartInfo& rInfo) {
    DemoStartInfo* pEmpty = findEmpty();
    *pEmpty = rInfo;
}

DemoStartInfo* DemoStartRequestHolder::find(const LiveActor* pActor, const char* pName) const {
    for (DemoStartInfo* const* pIter = mStartInfos; pIter != &mStartInfos[mNumInfos]; pIter++) {
        if ((*pIter)->_0 == pActor && MR::isEqualString((*pIter)->mDemoName, pName)) {
            return *pIter;
        }
    }
    return nullptr;
}

DemoStartInfo* DemoStartRequestHolder::find(const LayoutActor* pActor, const char* pName) const {
    for (DemoStartInfo* const* pIter = mStartInfos; pIter != &mStartInfos[mNumInfos]; pIter++) {
        if ((*pIter)->_4 == pActor && MR::isEqualString((*pIter)->mDemoName, pName)) {
            return *pIter;
        }
    }
    return nullptr;
}

DemoStartInfo* DemoStartRequestHolder::find(const NerveExecutor* pExecutor, const char* pName) const {
    for (DemoStartInfo* const* pIter = mStartInfos; pIter != &mStartInfos[mNumInfos]; pIter++) {
        if ((*pIter)->_8 == pExecutor && MR::isEqualString((*pIter)->mDemoName, pName)) {
            return *pIter;
        }
    }
    return nullptr;
}

DemoStartInfo* DemoStartRequestHolder::find(const NameObj* pObj, const char* pName) const {
    for (DemoStartInfo* const* pIter = mStartInfos; pIter != &mStartInfos[mNumInfos]; pIter++) {
        if ((*pIter)->_C == pObj && MR::isEqualString((*pIter)->mDemoName, pName)) {
            return *pIter;
        }
    }
    return nullptr;
}

DemoStartInfo* DemoStartRequestHolder::findEmpty() const {
    for (DemoStartInfo* const* pIter = mStartInfos; pIter != &mStartInfos[mNumInfos]; pIter++) {
        if (DemoStartRequestUtil::isEmpty(*pIter)) {
            DemoStartInfo* pFound = *pIter;
            DemoStartInfo emptyInfo;
            *pFound = emptyInfo;
            return *pIter;
        }
    }
    return nullptr;
}

DemoStartRequestHolder::DemoStartRequestHolder()
    : mNumInfos(0), mRequestBuffer(mRequestBuffer.mBuffer, mRequestBuffer.mBuffer), mProxyObj(nullptr) {
    try {
        mProxyObj = new NameObj(CP932("代理人"));
        for (u32 i = 0; i < ARRAY_SIZE(mStartInfos); i++) {
            DemoStartInfo* pInfo = new DemoStartInfo();
            s32 idx = mNumInfos;
            mNumInfos = idx + 1;
            mStartInfos[idx] = pInfo;
        }
        smgpc::compat::claim_name_obj_runtime_ownership(mProxyObj, this);
    } catch (...) {
        delete mProxyObj;
        for (s32 i = 0; i < mNumInfos; i++) {
            delete mStartInfos[i];
        }
        throw;
    }
}

DemoStartRequestHolder::~DemoStartRequestHolder() {
    delete mProxyObj;
    for (s32 i = 0; i < mNumInfos; i++) {
        delete mStartInfos[i];
    }
}

template <>
MR::FixedRingBuffer< const DemoStartInfo*, 16 >::iterator::iterator(const DemoStartInfo** head, const DemoStartInfo** tail) {
    mHead = head;
    mTail = tail;
    mEnd = tail + 16;
}

template <>
void MR::FixedRingBuffer< const DemoStartInfo*, 16 >::push_back(const DemoStartInfo* const& val) {
    if ((u32)mCount >= 16) {
        return;
    }

    *mEnd.mHead = val;
    ++mEnd;
    mCount++;
}

template <>
void MR::FixedRingBuffer< const DemoStartInfo*, 16 >::iterator::operator++() {
    const DemoStartInfo** pEnd = mEnd;
    mHead++;

    if (pEnd <= mHead) {
        mHead = mTail;
    }
}

void DemoStartRequestHolder::releaseNativeReference(const NameObj* pObject) noexcept {
    const auto borrows = [pObject](const DemoStartInfo& info) {
        return info._0 == pObject || info._4 == pObject || info._C == pObject ||
               info._10 == pObject || info._14 == pObject;
    };
    std::array<const DemoStartInfo*, 16> retained{};
    s32 count = 0;
    if (mRequestBuffer.mCount < 0 || mRequestBuffer.mCount > 16) {
        std::terminate();
    }
    auto cursor = mRequestBuffer.mHead;
    for (s32 i = 0; i < mRequestBuffer.mCount; i++, ++cursor) {
        const auto* info = *cursor.mHead;
        if (info != nullptr && !borrows(*info)) {
            retained[count++] = info;
        }
    }
    for (s32 i = 0; i < mNumInfos; i++) {
        if (borrows(*mStartInfos[i])) {
            *mStartInfos[i] = DemoStartInfo{};
        }
    }
    mRequestBuffer.mHead = decltype(mRequestBuffer.mHead)(mRequestBuffer.mBuffer, mRequestBuffer.mBuffer);
    mRequestBuffer.mEnd = mRequestBuffer.mHead;
    mRequestBuffer.mCount = 0;
    for (auto& info : mRequestBuffer.mBuffer) {
        info = nullptr;
    }
    for (s32 i = 0; i < count; i++) {
        mRequestBuffer.push_back(retained[i]);
    }
}
