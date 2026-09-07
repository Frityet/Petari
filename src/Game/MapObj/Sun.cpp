#include "Game/MapObj/Sun.hpp"

#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

// The exact source helper is not yet part of pc-port's compiled ObjUtil
// subset.  Keep the actor call source-shaped; its generalized provider is a
// separate scheduler integration step.
namespace MR {
    void connectToSceneSun(LiveActor* actor);
}

Sun::Sun(const char* name) : LiveActor(name) {
}

Sun::~Sun() = default;

void Sun::init(const JMapInfoIter&) {
    initModelManagerWithAnm("Sun", nullptr, false);
    MR::connectToSceneSun(this);
    MR::invalidateClipping(this);
    makeActorAppeared();
}
