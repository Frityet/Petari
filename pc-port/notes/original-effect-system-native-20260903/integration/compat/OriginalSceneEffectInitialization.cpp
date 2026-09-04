#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Util/SystemUtil.hpp"

// Literal complete root SceneFunction.cpp body, until that full TU is selected.
void SceneFunction::initEffectSystem(u32 a1, u32 a2) {
    MR::createSceneObj(SceneObj_EffectSystem);
    MR::getEffectSystem()->entry(MR::getParticleResourceHolder(), a1, a2);
}
