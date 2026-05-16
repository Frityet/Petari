#include "Game/Map/FileSelectSky.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/compat/FileSelectSkyRuntime.hpp"

namespace NrvFileSelectSky {
    NEW_NERVE(FileSelectSkyNrvWait, FileSelectSky, Wait);
};  // namespace NrvFileSelectSky

FileSelectSky::FileSelectSky(const char* pName) : LiveActor(pName) {
}

FileSelectSky::~FileSelectSky() = default;

void FileSelectSky::init(const JMapInfoIter&) {
    initModelManagerWithAnm("CometNearOrbitSky", nullptr, false);
    mScale.x = 0.8F;
    mScale.y = 0.8F;
    mScale.z = 0.8F;
    mProjmapEffectMtxSetter.reset(MR::initDLMakerProjmapEffectMtxSetter(this));
    MR::connectToSceneSky(this);
    initEffectKeeper(0, nullptr, false);
    MR::invalidateClipping(this);
    initNerve(&NrvFileSelectSky::FileSelectSkyNrvWait::sInstance);
    makeActorDead();
}

void FileSelectSky::calcAnim() {
    LiveActor::calcAnim();
    if (mProjmapEffectMtxSetter != nullptr) {
        mProjmapEffectMtxSetter->updateMtxUseBaseMtx();
    }
}

void FileSelectSky::calcAndSetBaseMtx() {
    MR::setBaseTRMtx(this, _94);
}

bool FileSelectSky::receiveOtherMsg(u32, HitSensor*, HitSensor*) {
    return false;
}

void FileSelectSky::exeWait() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "CometNearOrbitSky", nullptr);
        MR::startBtk(this, "CometNearOrbitSky");
    }

    if (MR::isFirstStep(this)) {
        _8C = 0.0F;
        _90 = 0.0F;
    }

    const auto step = static_cast< std::uint64_t >(getNerveStep());
    _94 = smgpc::game::file_select_sky_actor_matrix(step);
    _90 = smgpc::game::file_select_sky_yaw(step + 1U);
    _8C = smgpc::game::file_select_sky_pitch(step);
}

void FileSelectSky::draw(smgpc::render::IRendererEngine& renderer, const smgpc::game::CameraPoseCompat& camera_pose) {
    if (MR::isDead(this)) {
        return;
    }

    calcAndSetBaseMtx();
    drawModel(renderer, camera_pose, static_cast< std::uint64_t >(getNerveStep()));
}
