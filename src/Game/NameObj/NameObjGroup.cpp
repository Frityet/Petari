#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Util/ObjUtil.hpp"
#include <algorithm>

NameObjGroup::NameObjGroup(const char* pName, int numMax) : NameObj(pName), mObjNumMax(), mObjNum(), mObjArray() {
    initObjArray(numMax);
}

NameObjGroup::~NameObjGroup() {
    retireNativeLifetime();
    delete[] mObjArray;
}

void NameObjGroup::registerObj(NameObj* pObj) {
    mObjArray[mObjNum] = pObj;
    mObjNum++;
}

void NameObjGroup::pauseOffAll() const {
    for (s32 i = 0; i < mObjNum; i++) {
        MR::requestMovementOn(mObjArray[i]);
    }
}

void NameObjGroup::initObjArray(int numMax) {
    mObjNumMax = numMax;
    mObjArray = new NameObj*[numMax];

    for (s32 i = 0; i < mObjNumMax; i++) {
        mObjArray[i] = nullptr;
    }
}

void NameObjGroup::releaseNativeReference(const NameObj* object) noexcept {
    if (!mObjNum) {
        return;
    }
    auto* oldEnd = mObjArray + mObjNum;
    auto* newEnd = std::remove(mObjArray, oldEnd, object);
    std::fill(newEnd, oldEnd, nullptr);
    mObjNum = static_cast< s32 >(newEnd - mObjArray);
}
