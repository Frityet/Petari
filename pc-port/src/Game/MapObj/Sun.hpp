#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class Sun : public LiveActor {
public:
    explicit Sun(const char* name);
    ~Sun() override;

    void init(const JMapInfoIter& iter) override;
};
