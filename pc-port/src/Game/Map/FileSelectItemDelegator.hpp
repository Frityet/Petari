#pragma once

#include <revolution/types.h>

class FileSelectItem;

class FileSelectItemDelegatorBase {
public:
    virtual ~FileSelectItemDelegatorBase() = default;
    virtual void notify(FileSelectItem*, s32) = 0;
};

template < typename T >
class FileSelectItemDelegator : public FileSelectItemDelegatorBase {
public:
    typedef void (T::*Func)(FileSelectItem*, s32);

    inline FileSelectItemDelegator(T* pObject, Func pFunc) : mObject(pObject), mFunc(pFunc) {
    }

    void notify(FileSelectItem* pItem, s32 action) override {
        (mObject->*mFunc)(pItem, action);
    }

    T* mObject;
    Func mFunc;
};
