#include "Game/Map/FileSelectSky.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/compat/J3dMatrix.hpp"
#include "Game/compat/JMathTrig.hpp"

namespace NrvFileSelectSky {
    NEW_NERVE(FileSelectSkyNrvWait, FileSelectSky, Wait);
};  // namespace NrvFileSelectSky

FileSelectSky::FileSelectSky(const char* pName) : LiveActor(pName) {
}

FileSelectSky::~FileSelectSky() {
    delete mProjmapEffectMtxSetter;
}

void FileSelectSky::init(const JMapInfoIter&) {
    initModelManagerWithAnm("CometNearOrbitSky", nullptr, false);
    mScale.x = 0.8F;
    mScale.y = 0.8F;
    mScale.z = 0.8F;
    mProjmapEffectMtxSetter = MR::initDLMakerProjmapEffectMtxSetter(this);
    MR::connectToSceneSky(this);
    initEffectKeeper(0, nullptr, false);
    MR::invalidateClipping(this);
    initNerve(&NrvFileSelectSky::FileSelectSkyNrvWait::sInstance);
    makeActorDead();
}

void FileSelectSky::calcAnim() {
    LiveActor::calcAnim();
    mProjmapEffectMtxSetter->updateMtxUseBaseMtx();
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

    const auto yaw = smgpc::game::j3d_rotation_matrix(0.0F, 1.0F, 0.0F, _90);
    const auto pitch = smgpc::game::j3d_rotation_matrix(1.0F, 0.0F, 0.0F, _8C);
    _94 = smgpc::game::j3d_invert_orthonormal_matrix(smgpc::game::j3d_concat_matrix(yaw, pitch));

    auto steps = (3.1415927F * static_cast< f32 >(getNerveStep())) / 3000.0F;
    if (steps < 0.0F) {
        steps = -steps;
    }

    const auto value = _90 + 0.001F;
    const auto temp = 1.0F - smgpc::game::jmath_cos_lap_rad(steps * 0.25F);
    _90 = value;
    _8C = (3.0F * ((temp * 0.5F) * 3.1415927F)) * 0.25F;
}

void FileSelectSky::draw(smgpc::render::IRendererEngine& renderer, const smgpc::game::CameraPoseCompat& camera_pose) {
    if (MR::isDead(this)) {
        return;
    }

    calcAndSetBaseMtx();
    drawModel(renderer, camera_pose, static_cast< std::uint64_t >(getNerveStep()));
}
