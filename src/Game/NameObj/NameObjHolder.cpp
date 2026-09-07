#include "Game/NameObj/NameObjHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/HashUtil.hpp"
#include <algorithm>
#include <cstring>

namespace {
    struct equal_fullname {
        equal_fullname(const char* pName) : mHash(MR::getHashCode(pName)), mName(pName) {
        }

        bool operator()(NameObj* pObj) const {
            return strcmp(pObj->mName, mName) == 0;
        }

        u32 mHash;
        const char* mName;
    };
}  // namespace

NameObjHolder::NameObjHolder(int num) {
    mObjArray1.init(num);
}

void NameObjHolder::add(NameObj* pObj) {
    mObjArray1.push_back(pObj);
}

void NameObjHolder::suspendAllObj() {
    for (int i = 0; i < mObjArray1.size(); i++) {
        MR::requestMovementOff(mObjArray1[i]);
    }
}

void NameObjHolder::resumeAllObj() {
    for (int i = 0; i < mObjArray1.size(); i++) {
        MR::requestMovementOn(mObjArray1[i]);
    }
}

void NameObjHolder::syncWithFlags() {
    callMethodAllObj(&NameObj::syncWithFlags);
}

// Missing stack variables?
void NameObjHolder::callMethodAllObj(NameObjMethod pMethod) {
    NameObjMethod method = pMethod;
    NameObj** begin = mObjArray1.begin();
    NameObj** end = mObjArray1.end();

    for (NameObj** p = begin; p != end; p++) {
        (*p->*method)();
    }
}

void NameObjHolder::clearArray() {
    mObjArray1.clear();
    mObjArray2.clear();
}

NameObj* NameObjHolder::find(const char* pName) {
    NameObj** cached = std::find_if(mObjArray2.begin(), mObjArray2.end(), equal_fullname(pName));

    if (cached != mObjArray2.end()) {
        NameObj* pObj = *cached;
        mObjArray2.erase(cached);
        mObjArray2.insert(mObjArray2.begin(), pObj);
        return pObj;
    }

    NameObj** found = std::find_if(mObjArray1.begin(), mObjArray1.end(), equal_fullname(pName));

    if (found == mObjArray1.end()) {
        return nullptr;
    }

    NameObj* pObj = *found;

    if (mObjArray2.size() >= 16) {
        mObjArray2.mCount--;
    }

    mObjArray2.insert(mObjArray2.begin(), pObj);
    return pObj;
}
