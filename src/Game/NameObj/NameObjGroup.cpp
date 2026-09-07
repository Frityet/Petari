#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Util/ObjUtil.hpp"

NameObjGroup::NameObjGroup(const char* pName, int numMax) : NameObj(pName), mObjectNumMax(), mObjectCount(), mObjects() {
    initObjArray(numMax);
}

NameObjGroup::~NameObjGroup() {
    delete[] mObjects;
}

void NameObjGroup::registerObj(NameObj* pObj) {
    mObjects[mObjectCount] = pObj;
    mObjectCount++;
}

void NameObjGroup::pauseOffAll() const {
    for (s32 i = 0; i < mObjectCount; i++) {
        MR::requestMovementOn(mObjects[i]);
    }
}

void NameObjGroup::initObjArray(int numMax) {
    mObjectNumMax = numMax;
    mObjects = new NameObj*[numMax];

    for (s32 i = 0; i < mObjectNumMax; i++) {
        mObjects[i] = nullptr;
    }
}
