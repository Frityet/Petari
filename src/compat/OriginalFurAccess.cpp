#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/FurMulti.hpp"

// Original LiveActorUtil entry points; the complete Fur subsystem retains
// material selection, resource initialization and real scene draw ownership.
namespace MR {
    void initFur(LiveActor* pActor) {
        initMultiFur(pActor, -1);
    }

    void initFurPlanet(LiveActor* pActor) {
        initMultiFur(pActor, 3);
    }

    FurMulti* initFurPlayer(LiveActor* pActor) {
        return initMultiFur(pActor, 0);
    }
}
