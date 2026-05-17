#pragma once

#include "Game/Screen/LayoutActor.hpp"

class SaveIcon : public LayoutActor {
public:
    explicit SaveIcon(const LayoutActor* pActor);

    void calcAnim() override;
    void appear() override;

private:
    /* 0x20 */ const LayoutActor* mActor;
};
