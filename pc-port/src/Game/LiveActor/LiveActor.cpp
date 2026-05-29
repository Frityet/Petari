#include "Game/LiveActor/LiveActor.hpp"

#include <optional>

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "runtime/RuntimeContext.hpp"

LiveActor::LiveActor(const char* pName) : NameObj(pName) {
}

LiveActor::~LiveActor() {
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->star_pointer().unregister_target(*this);
        runtime->unregister_effect_keeper(getName());
        runtime->unregister_live_actor_model(*this);
    }
    delete mActorLightCtrl;
    delete mSpine;
}

void LiveActor::init(const JMapInfoIter&) {
}

void LiveActor::initAfterPlacement() {
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
    if (mModel != nullptr) {
        calcAndSetBaseMtx();
    }

    if (!mBrkActive || mBrkCtrl.mRate == 0.0F) {
        return;
    }

    mBrkCtrl.mFrame += mBrkCtrl.mRate;
    if (mBrkCtrl.mRate > 0.0F && mBrkCtrl.mFrame >= static_cast< f32 >(mBrkCtrl.mEnd)) {
        mBrkCtrl.mFrame = static_cast< f32 >(mBrkCtrl.mEnd);
        mBrkCtrl.mRate = 0.0F;
    } else if (mBrkCtrl.mRate < 0.0F && mBrkCtrl.mFrame <= static_cast< f32 >(mBrkCtrl.mStart)) {
        mBrkCtrl.mFrame = static_cast< f32 >(mBrkCtrl.mStart);
        mBrkCtrl.mRate = 0.0F;
    }
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

bool LiveActor::receiveMessage(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    return receiveOtherMsg(msg, pSender, pReceiver);
}

void LiveActor::control() {
}

void LiveActor::calcAndSetBaseMtx() {
    mBaseMatrix = smgpc::render::J3dMatrix3x4{{
        mScale.x,
        0.0F,
        0.0F,
        mPosition.x,
        0.0F,
        mScale.y,
        0.0F,
        mPosition.y,
        0.0F,
        0.0F,
        mScale.z,
        mPosition.z,
    }};
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

void LiveActor::initSound(s32, bool) {
}

void LiveActor::initModelManagerWithAnm(const char* pModelArcName, const char* pAnimArcName, bool) {
    mModel = std::make_unique< smgpc::render::live_actor::LiveActorModel >(pModelArcName != nullptr ? pModelArcName : "", pAnimArcName != nullptr ? pAnimArcName : "");
}

void LiveActor::initEffectKeeper(int effectNum, const char* pEffectName, bool sort) {
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        const auto group_name =
            pEffectName != nullptr ? std::string_view(pEffectName) : (mModel != nullptr ? mModel->model_arc_name() : std::string_view{});
        runtime->register_effect_keeper(smgpc::runtime::EffectKeeperHostKind::LiveActor, getName(), effectNum, group_name, sort);
    }
}

void LiveActor::initActorLightCtrl() {
    delete mActorLightCtrl;
    mActorLightCtrl = new ActorLightCtrl(this);
}

void LiveActor::loadActorLight() const {
    if (mActorLightCtrl != nullptr) {
        mActorLightCtrl->loadLight();
    }
}

void LiveActor::setBaseMatrix(const smgpc::render::J3dMatrix3x4& matrix) {
    mBaseMatrix = matrix;
}

void LiveActor::setProjmapEffectMatrix(const smgpc::render::J3dMatrix3x4& matrix) {
    if (mModel != nullptr) {
        mModel->setProjmapEffectMatrix(matrix);
    }
}

void LiveActor::drawModel(const smgpc::camera::CameraPose& camera_pose, std::uint64_t frame,
                          smgpc::render::live_actor::LiveActorModel::DrawPass pass) {
    if (mIsDead || mModel == nullptr) {
        return;
    }

    mModel->draw(camera_pose, mBaseMatrix, frame, pass);
}

void LiveActor::startBck(const char* pName, const char* pFileName) {
    mCurrentBckName = pName != nullptr ? pName : "";
    if (mModel != nullptr) {
        mModel->startBck(mCurrentBckName, pFileName != nullptr ? pFileName : "");
    }
}

void LiveActor::startBrk(const char* pName) {
    mCurrentBrkName = pName != nullptr ? pName : "";
    auto frame_max = std::optional< std::int16_t >{};
    if (mModel != nullptr) {
        frame_max = mModel->startBrk(mCurrentBrkName);
    }

    mBrkActive = true;
    mBrkCtrl.mStart = 0;
    mBrkCtrl.mEnd = frame_max.value_or(0);
    mBrkCtrl.mFrame = 0.0F;
    mBrkCtrl.mRate = 1.0F;
    if (mBrkCtrl.mEnd <= mBrkCtrl.mStart) {
        mBrkCtrl.mFrame = static_cast< f32 >(mBrkCtrl.mEnd);
        mBrkCtrl.mRate = 0.0F;
    }
}

void LiveActor::startBtk(const char* pName) {
    mCurrentBtkName = pName != nullptr ? pName : "";
    if (mModel != nullptr) {
        mModel->startBtk(mCurrentBtkName);
    }
}

void LiveActor::setBrkFrame(f32 frame) {
    mBrkActive = true;
    mBrkCtrl.mFrame = frame;
}

void LiveActor::setBrkFrameAndStop(f32 frame) {
    setBrkFrame(frame);
    mBrkCtrl.mRate = 0.0F;
}

void LiveActor::setBrkFrameEndAndStop() {
    setBrkFrameAndStop(static_cast< f32 >(mBrkCtrl.mEnd));
}

J3DFrameCtrl* LiveActor::getBrkCtrl() {
    return &mBrkCtrl;
}

const J3DFrameCtrl* LiveActor::getBrkCtrl() const {
    return &mBrkCtrl;
}

bool LiveActor::isBrkOneTimeAndStopped() const {
    return !mBrkActive || mBrkCtrl.mRate == 0.0F || mBrkCtrl.mFrame >= static_cast< f32 >(mBrkCtrl.mEnd);
}

std::string_view LiveActor::currentBckName() const {
    return mCurrentBckName;
}

std::string_view LiveActor::currentBrkName() const {
    return mCurrentBrkName;
}

std::string_view LiveActor::currentBtkName() const {
    return mCurrentBtkName;
}

bool LiveActor::isDead() const {
    return mIsDead;
}

const smgpc::render::J3dMatrix3x4& LiveActor::getBaseMatrix() const {
    return mBaseMatrix;
}
