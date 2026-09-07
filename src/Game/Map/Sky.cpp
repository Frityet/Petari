#include "Game/Map/Sky.hpp"

#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/StringUtil.hpp"

#include <stdexcept>

namespace {
    const char* cChangeAnimName = "Change";
};  // namespace

namespace NrvSky {
    NEW_NERVE(HostTypeWait, Sky, Wait);
    NEW_NERVE(HostTypeChange, Sky, Change);
};  // namespace NrvSky

Sky::Sky(const char* pSkyName) : LiveActor(pSkyName) {
    mSpaceInner = 0;
    mReflectionModel = 0;
}

void Sky::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    const char* objectName = 0;
    MR::getObjectName(&objectName, rIter);
    initModel(objectName);
    MR::connectToSceneSky(this);
    MR::useStageSwitchReadA(this, rIter);
    MR::useStageSwitchReadB(this, rIter);
    MR::useStageSwitchReadAppear(this, rIter);

    // SummerSky's exact SpaceInner child and switch listeners are outside the
    // compiled PC actor subset. Reject only the authored row that requests
    // that subfeature; every ordinary Sky keeps the original path below.
    if (MR::isEqualString(objectName, "SummerSky")) {
        throw std::logic_error("SummerSky requires the unavailable exact SpaceInner actor.");
    }

    s32 arg = -1;
    MR::getJMapInfoArg0NoInit(rIter, &arg);

    // Obj_arg0 == 0 is the retail request for a MirrorReflectionModel child.
    // Keep the request explicit instead of silently substituting a renderer.
    if (!arg) {
        throw std::logic_error("Sky mirror mode requires the unavailable exact MirrorReflectionModel actor.");
    }

    MR::tryStartAllAnim(this, objectName);

    MR::invalidateClipping(this);
    MR::registerDemoSimpleCastAll(this);
    initNerve(&NrvSky::HostTypeWait::sInstance);

    if (MR::isValidSwitchAppear(this)) {
        MR::syncStageSwitchAppear(this);
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

void Sky::calcAnim() {
    TVec3f pos = MR::getCamPos();
    mPosition.x = pos.x;
    mPosition.y = pos.y;
    mPosition.z = pos.z;
    LiveActor::calcAnim();
}

void Sky::initModel(const char* pModelName) {
    initModelManagerWithAnm(pModelName, 0, false);
}

void Sky::control() {
    if (mSpaceInner != 0 || mReflectionModel != 0) {
        throw std::logic_error("Sky child actors are unavailable in the compiled PC subset.");
    }
}

void Sky::appearSpaceInner() {
    throw std::logic_error("Sky SpaceInner is unavailable in the compiled PC subset.");
}

void Sky::disappearSpaceInner() {
    throw std::logic_error("Sky SpaceInner is unavailable in the compiled PC subset.");
}

void Sky::exeWait() {
    if (MR::isValidSwitchA(this) && MR::isOnSwitchA(this)) {
        setNerve(&NrvSky::HostTypeChange::sInstance);
    }
}

void Sky::exeChange() {
    if (MR::isFirstStep(this)) {
        MR::startAllAnim(this, ::cChangeAnimName);
    }
}

ProjectionMapSky::ProjectionMapSky(const char* pSkyName) : Sky(pSkyName) {
    mMtxSetter = 0;
}

Sky::~Sky() {
}

void ProjectionMapSky::calcAndSetBaseMtx() {
    LiveActor::calcAndSetBaseMtx();

    if (mMtxSetter) {
        mMtxSetter->updateMtxUseBaseMtx();
    }
}

void ProjectionMapSky::initModel(const char* pName) {
    initModelManagerWithAnm(pName, 0, true);
    mMtxSetter = MR::initDLMakerProjmapEffectMtxSetter(this);
    MR::newDifferedDLBuffer(this);
    mMtxSetter->updateMtxUseBaseMtx();
}

ProjectionMapSky::~ProjectionMapSky() {
}
