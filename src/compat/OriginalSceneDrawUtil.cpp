#include "Game/MapObj/SpinDriverPathDrawer.hpp"
#include "Game/MapObj/SpiderThread.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"

namespace {
    SpiderThread* getSpiderThread() {
        return MR::getSceneObj< SpiderThread >(SceneObj_SpiderThread);
    }
}

namespace MR {
    bool isDrawSpinDriverPathAtOpa() {
        if (!MR::isExistSceneObj(SceneObj_SpinDriverPathDrawInit)) {
            return false;
        }

        return MR::getSceneObj< SpinDriverPathDrawInit >(SceneObj_SpinDriverPathDrawInit)->mIsPathAtOpa;
    }

    void drawSpiderThreadBloom() {
        if (!MR::isExistSceneObj(SceneObj_SpiderThread)) {
            return;
        }

        if (::getSpiderThread()->mIsBloomOn) {
            ::getSpiderThread()->draw();
            CategoryList::drawOpa(MR::DrawBufferType_Ride);
        }
    }
}
