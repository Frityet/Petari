#pragma once

#include <revolution/types.h>

class MiiSelect {
public:
    MiiSelect(const char* pName);

    void initWithoutIter();
    void collectValidMiiIndex();

    [[nodiscard]] s32 getCollectValidMiiIndexCount() const;

private:
    s32 mCollectValidMiiIndexCount = 0;
};
