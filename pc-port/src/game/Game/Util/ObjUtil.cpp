#include "Game/Util/ObjUtil.hpp"

#include "Logger.hpp"
#include "compat/DecompIntegration.hpp"
#include "compat/LayoutSceneCompat.hpp"
#include "compat/RuntimeContext.hpp"

namespace {

// SMGPC_INTEGRATION_BEGIN
SMGPC_STUB(src/Game/Effect/EffectSystemUtil.cpp);
// SMGPC_INTEGRATION_END

}  // namespace

namespace MR {

void connectToSceneLayout(void *pActor) {
    smgpc::game::compat::connect_layout_scene_actor(static_cast< LayoutActor * >(pActor), smgpc::game::compat::LayoutSceneLayer::Layout);
}

void connectToSceneLayoutDecoration(void *pActor) {
    smgpc::game::compat::connect_layout_scene_actor(static_cast< LayoutActor * >(pActor), smgpc::game::compat::LayoutSceneLayer::LayoutDecoration);
}

void connectToSceneTalkLayout(void *pActor) {
    smgpc::game::compat::connect_layout_scene_actor(static_cast< LayoutActor * >(pActor), smgpc::game::compat::LayoutSceneLayer::TalkLayout);
}

void connectToSceneLayoutOnPause(void *pActor) {
    smgpc::game::compat::connect_layout_scene_actor(static_cast< LayoutActor * >(pActor), smgpc::game::compat::LayoutSceneLayer::LayoutOnPause);
}

void requestMovementOn(void *pActor) {
    (void)pActor;
}

void tryRumblePadMiddle(void *pActor, int intensity) {
    (void)pActor;

    const auto &logger = smgpc::game::compat::runtime_context().logger;
    if (logger) {
        logger->debug(__FILE__, __LINE__, smgpc::logging::Category::GAME, "tryRumblePadMiddle intensity={}", intensity);
    }
}

}  // namespace MR
