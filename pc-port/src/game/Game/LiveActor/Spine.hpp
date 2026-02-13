#pragma once

#include "compat/Types.hpp"

class Nerve;

class Spine {
public:
    Spine(void *pExecutor, const Nerve *pNerve);

    void update();
    void setNerve(const Nerve *pNerve);
    [[nodiscard]] const Nerve *getCurrentNerve() const;
    void changeNerve();

    /* 0x00 */ void *mExecutor;
    /* 0x08 */ const Nerve *mCurrNerve;
    /* 0x10 */ const Nerve *mNextNerve;
    /* 0x18 */ s32 mStep;
};
