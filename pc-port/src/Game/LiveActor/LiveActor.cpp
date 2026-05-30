#include "Game/LiveActor/LiveActor.hpp"

#include <cmath>
#include <optional>
#include <utility>

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "runtime/RuntimeContext.hpp"

namespace {
    constexpr f32 cDegToRad = 3.14159265358979323846F / 180.0F;

    smgpc::render::J3dMatrix3x4 make_trs_matrix(const TVec3f& position, const TVec3f& rotation, const TVec3f& scale) {
        const auto rx = rotation.x * cDegToRad;
        const auto ry = rotation.y * cDegToRad;
        const auto rz = rotation.z * cDegToRad;
        const auto sx = std::sin(rx);
        const auto cx = std::cos(rx);
        const auto sy = std::sin(ry);
        const auto cy = std::cos(ry);
        const auto sz = std::sin(rz);
        const auto cz = std::cos(rz);

        const auto r00 = cz * cy;
        const auto r01 = (cz * sy * sx) - (sz * cx);
        const auto r02 = (cz * sy * cx) + (sz * sx);
        const auto r10 = sz * cy;
        const auto r11 = (sz * sy * sx) + (cz * cx);
        const auto r12 = (sz * sy * cx) - (cz * sx);
        const auto r20 = -sy;
        const auto r21 = cy * sx;
        const auto r22 = cy * cx;

        return smgpc::render::J3dMatrix3x4{{
            r00 * scale.x,
            r01 * scale.y,
            r02 * scale.z,
            position.x,
            r10 * scale.x,
            r11 * scale.y,
            r12 * scale.z,
            position.y,
            r20 * scale.x,
            r21 * scale.y,
            r22 * scale.z,
            position.z,
        }};
    }
}  // namespace

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

    updateHitSensors();

    if (mSpine != nullptr) {
        updateNerve();
    }

    if (mIsDead) {
        return;
    }

    control();

    if (!mIsDead) {
        updateHitSensors();
    }
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
    validateHitSensors();
    updateHitSensors();
}

void LiveActor::makeActorDead() {
    mIsDead = true;
    invalidateHitSensors();
}

bool LiveActor::receiveOtherMsg(u32, HitSensor*, HitSensor*) {
    return false;
}

bool LiveActor::receiveMessage(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    return receiveOtherMsg(msg, pSender, pReceiver);
}

void LiveActor::control() {
}

void LiveActor::attackSensor(HitSensor*, HitSensor*) {
}

void LiveActor::startClipped() {
}

void LiveActor::endClipped() {
}

void LiveActor::calcAndSetBaseMtx() {
    mBaseMatrix = make_trs_matrix(mPosition, mRotation, mScale);
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

void LiveActor::initHitSensor(s32 sensorCount) {
    mHitSensors.clear();
    if (sensorCount > 0) {
        mHitSensors.reserve(static_cast< std::size_t >(sensorCount));
    }
}

HitSensor* LiveActor::addHitSensor(const char* pName, u32 type, u16 groupSize, f32 radius, const TVec3f& offset) {
    auto entry = ActorHitSensor{};
    entry.name = pName != nullptr ? pName : "";
    entry.offset = offset;
    entry.sensor = std::make_unique< HitSensor >(type, groupSize, radius, this);
    entry.sensor->mPosition = mPosition + offset;
    entry.sensor->validateBySystem();
    if (mIsDead) {
        entry.sensor->invalidate();
    } else {
        entry.sensor->validate();
    }

    mHitSensors.push_back(std::move(entry));
    return mHitSensors.back().sensor.get();
}

HitSensor* LiveActor::getSensor(const char* pName) {
    if (pName == nullptr) {
        return nullptr;
    }

    for (auto& entry : mHitSensors) {
        if (entry.name == pName) {
            return entry.sensor.get();
        }
    }

    return nullptr;
}

const HitSensor* LiveActor::getSensor(const char* pName) const {
    if (pName == nullptr) {
        return nullptr;
    }

    for (const auto& entry : mHitSensors) {
        if (entry.name == pName) {
            return entry.sensor.get();
        }
    }

    return nullptr;
}

const char* LiveActor::getSensorName(const HitSensor* pSensor) const {
    if (pSensor == nullptr) {
        return "";
    }

    for (const auto& entry : mHitSensors) {
        if (entry.sensor.get() == pSensor) {
            return entry.name.c_str();
        }
    }

    return "";
}

void LiveActor::collectHitSensors(std::vector< HitSensor* >& sensors) {
    for (auto& entry : mHitSensors) {
        if (entry.sensor != nullptr) {
            sensors.push_back(entry.sensor.get());
        }
    }
}

void LiveActor::validateHitSensors() {
    for (auto& entry : mHitSensors) {
        if (entry.sensor != nullptr) {
            entry.sensor->validate();
        }
    }
}

void LiveActor::invalidateHitSensors() {
    for (auto& entry : mHitSensors) {
        if (entry.sensor != nullptr) {
            entry.sensor->invalidate();
        }
    }
}

void LiveActor::updateHitSensors() {
    for (auto& entry : mHitSensors) {
        if (entry.sensor != nullptr) {
            entry.sensor->mPosition = mPosition + entry.offset;
        }
    }
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
