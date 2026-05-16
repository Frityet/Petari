#include "Game/LiveActor/LiveActor.hpp"

#include "Game/LiveActor/Spine.hpp"
#include "Game/compat/RuntimeContext.hpp"

LiveActor::LiveActor(const char* pName) : NameObj(pName) {
}

LiveActor::~LiveActor() {
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->unregister_sky_actor(*this);
    }
    delete mSpine;
}

void LiveActor::init(const JMapInfoIter&) {
}

void LiveActor::movement() {
    if (mIsDead) {
        return;
    }

    if (mSpine != nullptr) {
        updateNerve();
    }

    if (mIsDead) {
        return;
    }

    control();
}

void LiveActor::calcAnim() {
}

void LiveActor::calcViewAndEntry() {
    if (!mIsDead) {
        calcAndSetBaseMtx();
    }
}

void LiveActor::appear() {
    makeActorAppeared();
}

void LiveActor::kill() {
    makeActorDead();
}

void LiveActor::makeActorAppeared() {
    mIsDead = false;
}

void LiveActor::makeActorDead() {
    mIsDead = true;
}

bool LiveActor::receiveOtherMsg(u32, HitSensor*, HitSensor*) {
    return false;
}

void LiveActor::control() {
}

void LiveActor::calcAndSetBaseMtx() {
}

void LiveActor::initNerve(const Nerve* pNerve) {
    delete mSpine;
    mSpine = new Spine(this, pNerve);
}

void LiveActor::updateNerve() {
    if (mSpine != nullptr) {
        mSpine->update();
    }
}

void LiveActor::setNerve(const Nerve* pNerve) {
    if (mSpine != nullptr) {
        mSpine->setNerve(pNerve);
    }
}

bool LiveActor::isNerve(const Nerve* pNerve) const {
    return mSpine != nullptr && mSpine->getCurrentNerve() == pNerve;
}

s32 LiveActor::getNerveStep() const {
    if (mSpine == nullptr) {
        return 0;
    }

    return mSpine->mStep;
}

void LiveActor::initModelManagerWithAnm(const char* pModelArcName, const char* pAnimArcName, bool) {
    mModel = std::make_unique< LiveActorModelCompat >(pModelArcName != nullptr ? pModelArcName : "", pAnimArcName != nullptr ? pAnimArcName : "");
}

void LiveActor::initEffectKeeper(int, const char*, bool) {
}

void LiveActor::setBaseMatrix(const smgpc::game::J3dMatrix3x4& matrix) {
    mBaseMatrix = matrix;
}

void LiveActor::drawModel(smgpc::render::IRendererEngine& renderer, const smgpc::game::CameraPoseCompat& camera_pose, std::uint64_t frame,
                          LiveActorModelCompat::DrawPass pass) {
    if (mIsDead || mModel == nullptr) {
        return;
    }

    mModel->draw(renderer, camera_pose, mBaseMatrix, frame, pass);
}

void LiveActor::startBck(const char* pName, const char* pFileName) {
    if (mModel != nullptr) {
        mModel->startBck(pName != nullptr ? pName : "", pFileName != nullptr ? pFileName : "");
    }
}

void LiveActor::startBtk(const char* pName) {
    if (mModel != nullptr) {
        mModel->startBtk(pName != nullptr ? pName : "");
    }
}

bool LiveActor::isDead() const {
    return mIsDead;
}

const smgpc::game::J3dMatrix3x4& LiveActor::getBaseMatrix() const {
    return mBaseMatrix;
}
