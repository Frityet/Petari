#include "Game/Util/LiveActorUtil.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/J3dMatrix.hpp"
#include "render/live_actor/LiveActorModel.hpp"
#include "runtime/RuntimeContext.hpp"

ProjmapEffectMtxSetter::ProjmapEffectMtxSetter(LiveActor* pActor) : mActor(pActor) {
}

MtxPtr LiveActor::getBaseMtx() const {
    if (smgpc::compat::actor_model(this) == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<MtxPtr>(const_cast<f32*>(getBaseMatrix().m.data()));
}

namespace {
    smgpc::render::J3dMatrix3x4 projmap_base_transform(const LiveActor& actor) {
        return smgpc::render::j3d_remove_matrix_scale(actor.getBaseMatrix(), actor.mScale.x, actor.mScale.y, actor.mScale.z);
    }
}  // namespace

void ProjmapEffectMtxSetter::updateMtxUseBaseMtx() {
    if (mActor == nullptr) {
        return;
    }

    mActor->setProjmapEffectMatrix(smgpc::render::j3d_invert_affine_matrix(projmap_base_transform(*mActor)));
}

void ProjmapEffectMtxSetter::updateMtxUseBaseMtxWithLocalOffset(const TVec3f& offset) {
    if (mActor == nullptr) {
        return;
    }

    const auto local_offset = smgpc::render::J3dMatrix3x4{{
        1.0F,
        0.0F,
        0.0F,
        offset.x,
        0.0F,
        1.0F,
        0.0F,
        offset.y,
        0.0F,
        0.0F,
        1.0F,
        offset.z,
    }};
    mActor->setProjmapEffectMatrix(
        smgpc::render::j3d_invert_affine_matrix(smgpc::render::j3d_concat_matrix(projmap_base_transform(*mActor), local_offset)));
}

namespace MR {

    ProjmapEffectMtxSetter* initDLMakerProjmapEffectMtxSetter(LiveActor* pActor) {
        return new ProjmapEffectMtxSetter(pActor);
    }

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

    void startBck(LiveActor* pActor, const char* pName, const char* pFileName) {
        if (pActor != nullptr) {
            pActor->startBck(pName, pFileName);
        }
    }

    void startBrk(LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            pActor->startBrk(pName);
        }
    }

    void startBtk(LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            pActor->startBtk(pName);
        }
    }

    void startAction(LiveActor* pActor, const char* pName) {
        if (pActor != nullptr) {
            pActor->startBck(pName, nullptr);
            pActor->startBrk(pName);
            pActor->startBtk(pName);
            if (auto* model = smgpc::compat::actor_model(pActor); model != nullptr) {
                model->startActionBtp(pName != nullptr ? pName : "");
            }
        }
    }

    void setBrkFrame(LiveActor* pActor, f32 frame) {
        if (pActor != nullptr) {
            pActor->setBrkFrame(frame);
        }
    }

    void setBrkFrameAndStop(LiveActor* pActor, f32 frame) {
        if (pActor != nullptr) {
            pActor->setBrkFrameAndStop(frame);
        }
    }

    void setBrkFrameEndAndStop(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->setBrkFrameEndAndStop();
        }
    }

    J3DFrameCtrl* getBrkCtrl(LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::logic_error("BRK animation state is unavailable.");
        }
        auto* ctrl = pActor->getBrkCtrl();
        if (ctrl == nullptr) {
            throw std::logic_error("BRK animation data is unavailable.");
        }
        return ctrl;
    }

    void setBaseTRMtx(LiveActor* pActor, MtxPtr pMtx) {
        if (pMtx != nullptr) {
            setBaseTRMtx(pActor, smgpc::render::j3d_matrix_from_mtx(pMtx));
        }
    }

    void setBaseTRMtx(LiveActor* pActor, const smgpc::render::J3dMatrix3x4& matrix) {
        if (pActor != nullptr) {
            pActor->setBaseMatrix(smgpc::render::j3d_apply_matrix_scale(matrix, pActor->mScale.x, pActor->mScale.y, pActor->mScale.z));
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

    void connectToDrawTemporarily(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsClipped = false;
        }
    }

    void disconnectToDrawTemporarily(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsClipped = true;
        }
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
            pActor->loadActorLight();
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

    bool isDead(const LiveActor* pActor) {
        return pActor == nullptr || pActor->isDead();
    }

    bool isStep(const LiveActor* pActor, s32 step) {
        return pActor != nullptr && pActor->getNerveStep() == step;
    }

    bool isFirstStep(const LiveActor* pActor) {
        return isStep(pActor, 0);
    }

    bool isGreaterEqualStep(const LiveActor* pActor, s32 step) {
        return pActor != nullptr && pActor->getNerveStep() >= step;
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
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        const auto* model = smgpc::compat::actor_model(pActor);
        if (runtime == nullptr || model == nullptr) {
            throw std::logic_error("BCK animation state is unavailable.");
        }

        const auto stopped = model->is_bck_stopped(runtime->frame_index());
        if (!stopped.has_value()) {
            throw std::logic_error("BCK animation data is unavailable.");
        }
        return *stopped;
    }

    bool checkPassBckFrame(const LiveActor* pActor, f32 frame) {
        const auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        const auto* model = smgpc::compat::actor_model(pActor);
        if (runtime == nullptr || model == nullptr) {
            throw std::logic_error("BCK animation state is unavailable.");
        }

        const auto passed = model->check_pass_bck_frame(runtime->frame_index(), frame);
        if (!passed.has_value()) {
            throw std::logic_error("BCK animation data is unavailable.");
        }
        return *passed;
    }

    f32 getBckFrameMax(const LiveActor* pActor) {
        const auto* model = smgpc::compat::actor_model(pActor);
        if (pActor == nullptr || model == nullptr || pActor->currentBckName().empty()) {
            throw std::logic_error("BCK animation state is unavailable.");
        }

        const auto frame_max = model->bck_frame_max(pActor->currentBckName());
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

        const auto* matrix = model->joint_world_matrix(pJointName, pActor->getBaseMatrix(), runtime->frame_index());
        if (matrix == nullptr) {
            return nullptr;
        }
        static_assert(sizeof(*matrix) == sizeof(Mtx));
        return reinterpret_cast<MtxPtr>(const_cast<f32*>(matrix->m.data()));
    }

    bool isBrkOneTimeAndStopped(const LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::logic_error("BRK animation state is unavailable.");
        }
        return pActor->isBrkOneTimeAndStopped();
    }

}  // namespace MR
