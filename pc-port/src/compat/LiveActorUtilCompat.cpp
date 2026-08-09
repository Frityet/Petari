#include "Game/Util/LiveActorUtil.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/LiveActorMatrixCompat.hpp"
#include "render/J3dMatrix.hpp"
#include "render/live_actor/LiveActorModel.hpp"
#include "runtime/RuntimeContext.hpp"

namespace MR {

    void validateClipping(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsInvalidClipping = false;
        }
    }

    void invalidateClipping(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }
        if (pActor->mFlag.mIsClipped) {
            pActor->endClipped();
        }
        pActor->mFlag.mIsInvalidClipping = true;
    }

    void setClippingFarMax(LiveActor* pActor) {
        smgpc::compat::configure_actor_clipping_far_level(pActor, 0);
    }

    void startBck(const LiveActor* pActor, const char* pName, const char* pFileName) {
        if (pActor != nullptr) {
            smgpc::compat::start_actor_bck(const_cast<LiveActor*>(pActor), pName, pFileName);
        }
    }

    void startBckWithInterpole(const LiveActor* pActor, const char* pName, s32 interpolation) {
        if (interpolation != 0) {
            throw std::logic_error(
                "Nonzero BCK interpolation is unavailable without the generalized J3D blend controller.");
        }
        startBck(pActor, pName, nullptr);
    }

    void startBrk(const LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            smgpc::compat::start_actor_brk(const_cast<LiveActor*>(pActor), pName);
        }
    }

    void startBtk(const LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            smgpc::compat::start_actor_btk(const_cast<LiveActor*>(pActor), pName);
        }
    }

    void newDifferedDLBuffer(LiveActor* pActor) {
        if (pActor == nullptr || smgpc::compat::actor_model(pActor) == nullptr) {
            throw std::logic_error(
                "A deferred display-list request requires a real LiveActor model renderer.");
        }

        // LiveActorModel already retains its renderer packets for both scene
        // draw-buffer passes. The retail request therefore needs no second
        // host buffer, but it must not silently accept an actor without the
        // model resource that the original DisplayListMaker would own.
    }

    bool tryStartAllAnim(const LiveActor* pActor, const char* pName) {
        if (pActor == nullptr || pName == nullptr) {
            return false;
        }

        auto* actor = const_cast<LiveActor*>(pActor);
        if (smgpc::compat::actor_model(actor) == nullptr) {
            throw std::logic_error("Starting all animations requires a real LiveActor model renderer.");
        }

        auto started = false;
        started |= smgpc::compat::try_start_actor_bck(actor, pName, nullptr);
        started |= smgpc::compat::try_start_actor_btk(actor, pName);
        started |= smgpc::compat::try_start_actor_brk(actor, pName);
        started |= smgpc::compat::try_start_actor_btp(actor, pName);
        return started;
    }

    void startAllAnim(const LiveActor* pActor, const char* pName) {
        (void)tryStartAllAnim(pActor, pName);
    }

    void setAllAnimFrameAtEnd(const LiveActor* pActor, const char* pName) {
        if (pActor == nullptr || pName == nullptr) {
            return;
        }

        auto* actor = const_cast<LiveActor*>(pActor);
        auto* model = smgpc::compat::actor_model(actor);
        if (model == nullptr) {
            throw std::logic_error("Setting all animation frames requires a real LiveActor model renderer.");
        }

        if (model->hasBck(pName, {})) {
            throw std::logic_error("BCK end-frame control is unavailable without a host J3D frame controller.");
        }
        if (model->hasBtk(pName)) {
            throw std::logic_error("BTK end-frame control is unavailable without a host J3D frame controller.");
        }
        if (model->hasBrk(pName)) {
            auto* ctrl = smgpc::compat::actor_brk_ctrl(actor);
            if (ctrl == nullptr) {
                throw std::logic_error("BRK animation data is unavailable.");
            }
            smgpc::compat::set_actor_brk_frame(actor, static_cast<f32>(ctrl->mEnd));
        }
        if (model->hasBtp(pName)) {
            throw std::logic_error("BTP end-frame control is unavailable without a host J3D frame controller.");
        }
    }

    bool isAnyAnimStopped(const LiveActor* pActor, const char* pName) {
        if (pActor == nullptr || pName == nullptr) {
            return false;
        }

        auto* model = smgpc::compat::actor_model(pActor);
        if (model == nullptr) {
            throw std::logic_error("Animation stop state requires a real LiveActor model renderer.");
        }

        if (model->hasBck(pName, {}) && MR::isBckStopped(pActor)) {
            return true;
        }
        if (model->hasBtk(pName)) {
            throw std::logic_error("BTK stop state is unavailable without a host J3D frame controller.");
        }
        if (model->hasBrk(pName) && smgpc::compat::is_actor_brk_one_time_and_stopped(pActor)) {
            return true;
        }
        if (model->hasBtp(pName) && MR::isBtpStopped(pActor)) {
            return true;
        }
        return false;
    }

    void startAction(const LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            auto* actor = const_cast<LiveActor*>(pActor);
            smgpc::compat::start_actor_bck(actor, pName, nullptr);
            smgpc::compat::start_actor_brk(actor, pName);
            smgpc::compat::start_actor_btk(actor, pName);
            if (auto* model = smgpc::compat::actor_model(pActor); model != nullptr) {
                model->startActionBtp(pName != nullptr ? pName : "");
            }
        }
    }

    void setBrkFrame(const LiveActor* pActor, f32 frame) {
        if (pActor != nullptr) {
            smgpc::compat::set_actor_brk_frame(const_cast<LiveActor*>(pActor), frame);
        }
    }

    void setBckFrameAndStop(const LiveActor* pActor, f32 frame) {
        if (pActor != nullptr) {
            smgpc::compat::set_actor_bck_frame_and_stop(const_cast<LiveActor*>(pActor), frame);
        }
    }

    void setBckFrame(const LiveActor* pActor, f32 frame) {
        if (pActor == nullptr) {
            return;
        }

        auto* controller = smgpc::compat::actor_bck_ctrl(pActor);
        auto* model = smgpc::compat::actor_model(pActor);
        if (controller == nullptr || model == nullptr) {
            throw std::logic_error("BCK animation data is unavailable.");
        }
        controller->setFrame(frame);
        model->syncBckFrameController(
            controller->mFrame, controller->mRate, controller->mState);
    }

    void setBrkRate(const LiveActor* pActor, f32 rate) {
        if (pActor != nullptr) {
            smgpc::compat::set_actor_brk_rate(const_cast<LiveActor*>(pActor), rate);
        }
    }

    void setBrkFrameAndStop(const LiveActor* pActor, f32 frame) {
        if (pActor != nullptr) {
            smgpc::compat::set_actor_brk_frame_and_stop(const_cast<LiveActor*>(pActor), frame);
        }
    }

    void setBrkFrameEndAndStop(const LiveActor* pActor) {
        if (pActor != nullptr) {
            smgpc::compat::set_actor_brk_frame_end_and_stop(const_cast<LiveActor*>(pActor));
        }
    }

    J3DFrameCtrl* getBrkCtrl(const LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::logic_error("BRK animation state is unavailable.");
        }
        auto* ctrl = smgpc::compat::actor_brk_ctrl(pActor);
        if (ctrl == nullptr) {
            throw std::logic_error("BRK animation data is unavailable.");
        }
        return ctrl;
    }

    J3DFrameCtrl* getBckCtrl(const LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::logic_error("BCK animation state is unavailable.");
        }
        auto* ctrl = smgpc::compat::actor_bck_ctrl(pActor);
        if (ctrl == nullptr) {
            throw std::logic_error("BCK animation data is unavailable.");
        }
        return ctrl;
    }

    void setBaseTRMtx(LiveActor* pActor, MtxPtr pMtx) {
        if (pMtx != nullptr) {
            setBaseTRMtx(pActor, smgpc::render::j3d_matrix_from_mtx(pMtx));
        }
    }

    void setBaseTRMtx(LiveActor* pActor, const TPos3f& matrix) {
        setBaseTRMtx(pActor, smgpc::render::J3dMatrix3x4{{
                                 matrix.mMtx[0][0], matrix.mMtx[0][1], matrix.mMtx[0][2], matrix.mMtx[0][3],
                                 matrix.mMtx[1][0], matrix.mMtx[1][1], matrix.mMtx[1][2], matrix.mMtx[1][3],
                                 matrix.mMtx[2][0], matrix.mMtx[2][1], matrix.mMtx[2][2], matrix.mMtx[2][3],
                             }});
    }

    void setBaseTRMtx(LiveActor* pActor, const smgpc::render::J3dMatrix3x4& matrix) {
        if (pActor != nullptr) {
            smgpc::compat::set_actor_base_matrix(
                pActor, smgpc::render::j3d_apply_matrix_scale(matrix, pActor->mScale.x,
                                                              pActor->mScale.y, pActor->mScale.z));
        }
    }

    PartsModel* createPartsModelMapObj(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx) {
        PartsModel* pModel = new PartsModel(pHost, pName, pModelName, pMtx, MR::DrawBufferType_MapObj, false);
        pModel->initWithoutIter();
        return pModel;
    }

    PartsModel* createPartsModelNoSilhouettedMapObj(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx) {
        PartsModel* pModel = new PartsModel(pHost, pName, pModelName, pMtx, MR::DrawBufferType_NoSilhouettedMapObj, false);
        pModel->initWithoutIter();
        return pModel;
    }

    void emitEffect(LiveActor* pActor, const char* pEffectName) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pActor != nullptr && pEffectName != nullptr) {
            runtime->emit_effect(pActor->getName(), pEffectName, pActor);
        }
    }

    void deleteEffect(LiveActor* pActor, const char* pEffectName) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pActor != nullptr && pEffectName != nullptr) {
            runtime->delete_effect(pActor->getName(), pEffectName, pActor);
        }
    }

    void initLightCtrl(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }

        pActor->initActorLightCtrl();
        pActor->mActorLightCtrl->init(-1, false);
    }

    void initLightCtrlForPlayer(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }

        pActor->initActorLightCtrl();
        pActor->mActorLightCtrl->init(-1, false);
        LightFunction::registerPlayerLightCtrl(pActor->mActorLightCtrl);
    }

    void initLightCtrlNoDrawEnemy(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }

        pActor->initActorLightCtrl();
        pActor->mActorLightCtrl->init(1, true);
    }

    void initLightCtrlNoDrawMapObj(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }

        pActor->initActorLightCtrl();
        pActor->mActorLightCtrl->init(3, true);
    }

    void updateLightCtrl(LiveActor* pActor) {
        if (pActor != nullptr && pActor->mActorLightCtrl != nullptr) {
            pActor->mActorLightCtrl->update(false);
        }
    }

    void updateLightCtrlDirect(LiveActor* pActor) {
        if (pActor != nullptr && pActor->mActorLightCtrl != nullptr) {
            pActor->mActorLightCtrl->update(true);
        }
    }

    void loadActorLight(const LiveActor* pActor) {
        if (pActor != nullptr) {
            if (pActor->mActorLightCtrl != nullptr) {
                pActor->mActorLightCtrl->loadLight();
            }
        }
    }

    ActorLightCtrl* getLightCtrl(const LiveActor* pActor) {
        return pActor == nullptr ? nullptr : pActor->mActorLightCtrl;
    }

    void initDefaultPos(LiveActor* pActor, const JMapInfoIter& rIter) {
        if (pActor == nullptr || !rIter.isValid()) {
            return;
        }

        (void)MR::getJMapInfoTrans(rIter, &pActor->mPosition);
        (void)MR::getJMapInfoRotate(rIter, &pActor->mRotation);
        (void)MR::getJMapInfoScale(rIter, &pActor->mScale);
        pActor->mRotation.x = MR::repeat(pActor->mRotation.x, 0.0F, 360.0F);
        pActor->mRotation.y = MR::repeat(pActor->mRotation.y, 0.0F, 360.0F);
        pActor->mRotation.z = MR::repeat(pActor->mRotation.z, 0.0F, 360.0F);
    }

    void initDefaultPosNoRepeat(LiveActor* pActor, const JMapInfoIter& rIter) {
        if (pActor == nullptr || !rIter.isValid()) {
            return;
        }

        (void)MR::getJMapInfoTrans(rIter, &pActor->mPosition);
        (void)MR::getJMapInfoRotate(rIter, &pActor->mRotation);
        (void)MR::getJMapInfoScale(rIter, &pActor->mScale);
    }

    bool isHiddenModel(const LiveActor* pActor) {
        return pActor == nullptr || pActor->mFlag.mIsHiddenModel;
    }

    bool isClipped(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mFlag.mIsClipped;
    }

    bool isNoEntryDrawBuffer(const LiveActor* pActor) {
        return pActor == nullptr || pActor->mFlag.mIsHiddenModel;
    }

    void onEntryDrawBuffer(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsHiddenModel = false;
        }
    }

    void offEntryDrawBuffer(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsHiddenModel = true;
        }
    }

    void showModel(LiveActor* pActor) {
        onEntryDrawBuffer(pActor);
    }

    void hideModel(LiveActor* pActor) {
        offEntryDrawBuffer(pActor);
    }

    bool isNoCalcAnim(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mFlag.mIsNoCalcAnim;
    }

    void offCalcAnim(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsNoCalcAnim = true;
        }
    }

    bool isDead(const LiveActor* pActor) {
        return pActor == nullptr || pActor->mFlag.mIsDead;
    }

    bool isStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() == step;
    }

    bool isFirstStep(const LiveActor* pActor) {
        return isStep(pActor, 0);
    }

    bool isLessStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() < step;
    }

    bool isLessEqualStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() <= step;
    }

    bool isGreaterStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() > step;
    }

    bool isGreaterEqualStep(const LiveActor* pActor, s32 step) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve comparison requires a real actor.");
        }
        return pActor->getNerveStep() >= step;
    }

    bool isNewNerve(const LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve query requires a real actor.");
        }
        return pActor->getNerveStep() < 0;
    }

    f32 calcNerveRate(const LiveActor* pActor, s32 stepMax) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve rate requires a real actor.");
        }
        return stepMax <= 0
                   ? 1.0F
                   : std::clamp(static_cast<f32>(pActor->getNerveStep()) / static_cast<f32>(stepMax),
                                0.0F, 1.0F);
    }

    f32 calcNerveEaseInRate(const LiveActor *pActor, s32 stepMax) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A LiveActor nerve rate requires a real actor.");
        }
        const auto rate = stepMax <= 0
                              ? 1.0F
                              : std::clamp(static_cast<f32>(pActor->getNerveStep()) /
                                               static_cast<f32>(stepMax),
                                           0.0F, 1.0F);
        constexpr auto cHalfPi = 1.57079632679489661923F;
        return 1.0F - std::cos(rate * cHalfPi);
    }

    void setNerveAtStep(LiveActor* pActor, const Nerve* pNerve, s32 step) {
        if (pActor != nullptr && pActor->getNerveStep() == step) {
            pActor->setNerve(pNerve);
        }
    }

    bool isBtpStopped(const LiveActor* pActor) {
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        const auto* model = smgpc::compat::actor_model(pActor);
        if (runtime == nullptr || model == nullptr) {
            throw std::logic_error("BTP animation state is unavailable.");
        }

        const auto stopped = model->is_btp_stopped(runtime->frame_index());
        if (!stopped.has_value()) {
            throw std::logic_error("BTP animation data is unavailable.");
        }
        return *stopped;
    }

    bool isBckStopped(const LiveActor* pActor) {
        auto* controller = pActor != nullptr
                               ? smgpc::compat::actor_bck_ctrl(pActor)
                               : nullptr;
        if (controller == nullptr) {
            throw std::logic_error("BCK animation state is unavailable.");
        }
        return controller->checkState(1U);
    }

    bool checkPassBckFrame(const LiveActor* pActor, f32 frame) {
        auto* controller = pActor != nullptr
                               ? smgpc::compat::actor_bck_ctrl(pActor)
                               : nullptr;
        if (controller == nullptr) {
            throw std::logic_error("BCK animation state is unavailable.");
        }
        return controller->checkPass(frame) == TRUE;
    }

    f32 getBckFrameMax(const LiveActor* pActor) {
        const auto* model = smgpc::compat::actor_model(pActor);
        if (pActor == nullptr || model == nullptr) {
            throw std::logic_error("BCK animation state is unavailable.");
        }
        const auto bck_name = smgpc::compat::actor_current_bck_name(pActor);
        if (bck_name.empty()) {
            throw std::logic_error("BCK animation state is unavailable.");
        }

        const auto frame_max = model->bck_frame_max(bck_name);
        if (!frame_max.has_value()) {
            throw std::logic_error("BCK animation data is unavailable.");
        }
        return static_cast<f32>(*frame_max);
    }

    MtxPtr getJointMtx(const LiveActor* pActor, const char* pJointName) {
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        auto* model = smgpc::compat::actor_model(pActor);
        if (pActor == nullptr || pJointName == nullptr || runtime == nullptr || model == nullptr) {
            return nullptr;
        }

        const auto* matrix = model->joint_world_matrix(
            pJointName, smgpc::compat::actor_base_matrix(pActor), runtime->frame_index());
        if (matrix == nullptr) {
            return nullptr;
        }
        static_assert(sizeof(*matrix) == sizeof(Mtx));
        return reinterpret_cast<MtxPtr>(const_cast<f32*>(matrix->m.data()));
    }

    s32 getJointNum(const LiveActor* pActor) {
        const auto count = smgpc::compat::actor_model_joint_count(pActor);
        if (count > static_cast<std::size_t>(std::numeric_limits<s32>::max())) {
            throw std::overflow_error("J3D joint count exceeds the retail s32 surface.");
        }
        return static_cast<s32>(count);
    }

    bool isBrkOneTimeAndStopped(const LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::logic_error("BRK animation state is unavailable.");
        }
        return smgpc::compat::is_actor_brk_one_time_and_stopped(pActor);
    }

}  // namespace MR
