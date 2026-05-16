#pragma once

#include <string>

#include <revolution.h>

#include "Game/Util/JMapInfo.hpp"

class NameObj {
public:
    explicit NameObj(const char* pName);
    virtual ~NameObj();

    virtual void init(const JMapInfoIter& rIter);
    virtual void initAfterPlacement();
    virtual void movement();
    virtual void draw() const;
    virtual void calcAnim();
    virtual void calcViewAndEntry();

    void initWithoutIter();
    void setName(const char* pName);
    void executeMovement();
    void requestSuspend();
    void requestResume();
    void syncWithFlags();

    [[nodiscard]] bool isSuspended() const;
    [[nodiscard]] const char* getName() const;

    const char* mName = "";
    volatile u16 mFlag = 0U;
    s16 mExecutorIdx = -1;

private:
    std::string mNameStorage;
};

class NameObjFunction {
public:
    static void requestMovementOn(NameObj* pObj);
    static void requestMovementOff(NameObj* pObj);
};
