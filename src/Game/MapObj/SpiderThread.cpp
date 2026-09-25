#include "Game/MapObj/SpiderThread.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"

namespace {
    SpiderThread* getSpiderThread() {
        return MR::getSceneObj< SpiderThread >(SceneObj_SpiderThread);
    }
};  // namespace

void MR::drawSpiderThreadBloom() {
    if (!MR::isExistSceneObj(SceneObj_SpiderThread)) {
        return;
    }

    if (::getSpiderThread()->mIsBloomOn) {
        ::getSpiderThread()->draw();
        CategoryList::drawOpa(MR::DrawBufferType_Ride);
    }
}
