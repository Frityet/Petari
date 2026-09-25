#pragma once

#include "Game/Util/Array.hpp"
#include <revolution.h>
#include <vector>

class NameObj;


typedef void (NameObj::*NameObjMethod)(void);
typedef void (NameObj::*NameObjMethodConst)(void) const;

class NameObjHolder {
public:
    NameObjHolder(int);
    ~NameObjHolder();

    void removeNativeObject(NameObj*) noexcept;
    std::vector<NameObj*> snapshotNativeObjects() const;

    void add(NameObj*);
    void suspendAllObj();
    void resumeAllObj();
    void syncWithFlags();
    void callMethodAllObj(NameObjMethod);
    void clearArray();
    NameObj* find(const char*);

private:
    /* 0x00 */ MR::Vector< MR::AssignableArray< NameObj* > > mObjArray1;
    /* 0x0C */ MR::Vector< MR::FixedArray< NameObj*, 16 > > mObjArray2;
};
