#include "resource/TextEncoding.hpp"
#include "Game/Boss/PoltaRockHolder.hpp"
#include "Game/Boss/PoltaRock.hpp"

namespace {
    static const s32 sMaxPoltaRock = 16;
};  // namespace

PoltaRockHolder::PoltaRockHolder() : DeriveActorGroup< PoltaRock >(CP932("ポルタの岩管理"), ::sMaxPoltaRock) {
    PoltaRock* pRock;

    for (int i = 0; i < ::sMaxPoltaRock; i++) {
        pRock = new PoltaRock(CP932("ポルタ岩"));
        pRock->initWithoutIter();
        registerActor(pRock);
    }
}
