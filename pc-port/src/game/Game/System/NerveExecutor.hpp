#pragma once

#include "compat/Types.hpp"

class Nerve;
class Spine;

class NerveExecutor {
public:
    explicit NerveExecutor(const char *pName);
    virtual ~NerveExecutor();

    void initNerve(const Nerve *pNerve);
    void updateNerve();
    void setNerve(const Nerve *pNerve);
    [[nodiscard]] bool isNerve(const Nerve *pNerve) const;
    [[nodiscard]] s32 getNerveStep() const;

    /* 0x08 */ Spine *mSpine;
};
