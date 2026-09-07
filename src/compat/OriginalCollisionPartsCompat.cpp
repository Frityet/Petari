#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Scene/SceneObjHolder.hpp"

// Original bodies from CollisionParts.cpp and CollisionDirector.cpp. The
// native static KCL provider does not manufacture CollisionParts or a Director.
void CollisionParts::calcForceMovePower(TVec3f* a1, const TVec3f& a2) const {
    TVec3f tStack88 = a2;
    TMtx34f auStack76;
    PSMTXInverse((MtxPtr)&mPrevBaseMatrix, reinterpret_cast< MtxPtr >(&auStack76));

    auStack76.mult(tStack88, tStack88);
    mBaseMatrix.mult(tStack88, tStack88);

    tStack88.sub(a2);
    *a1 = tStack88;
}

CollisionDirector* MR::getCollisionDirector() {
    return MR::getSceneObj< CollisionDirector >(SceneObj_CollisionDirector);
}
