#include "Game/Util/FixedPosition.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/LiveActorUtil.hpp"

#include <cmath>
#include <type_traits>

namespace {
    constexpr f32 cDegToRad = 3.14159265358979323846F / 180.0F;
    constexpr f32 cRadToDeg = 180.0F / 3.14159265358979323846F;
    constexpr f32 cMinAxisLength = 0.000001F;

    static_assert(sizeof(smgpc::render::J3dMatrix3x4) == sizeof(Mtx));
    static_assert(std::is_standard_layout_v< smgpc::render::J3dMatrix3x4 >);

    [[nodiscard]] MtxPtr actor_base_mtx(const LiveActor* pActor) {
        if (pActor == nullptr) {
            return nullptr;
        }

        const auto& matrix = pActor->getBaseMatrix();
        return reinterpret_cast< MtxPtr >(const_cast< f32* >(matrix.m.data()));
    }

    void set_local_tr(TPos3f* pMtx, const TVec3f& rTrans, const TVec3f& rRotDegrees) {
        const auto rx = rRotDegrees.x * cDegToRad;
        const auto ry = rRotDegrees.y * cDegToRad;
        const auto rz = rRotDegrees.z * cDegToRad;
        const auto sinX = std::sin(rx);
        const auto cosX = std::cos(rx);
        const auto sinY = std::sin(ry);
        const auto cosY = std::cos(ry);
        const auto sinZ = std::sin(rz);
        const auto cosZ = std::cos(rz);

        pMtx->mMtx[0][0] = cosZ * cosY;
        pMtx->mMtx[1][0] = sinZ * cosY;
        pMtx->mMtx[2][0] = -sinY;

        pMtx->mMtx[0][1] = cosZ * sinY * sinX - sinZ * cosX;
        pMtx->mMtx[1][1] = sinZ * sinY * sinX + cosZ * cosX;
        pMtx->mMtx[2][1] = cosY * sinX;

        pMtx->mMtx[0][2] = cosZ * sinY * cosX + sinZ * sinX;
        pMtx->mMtx[1][2] = sinZ * sinY * cosX - cosZ * sinX;
        pMtx->mMtx[2][2] = cosY * cosX;
        pMtx->setTrans(rTrans);
    }

    void normalize_axes(TPos3f* pMtx) {
        for (auto column = 0; column < 3; ++column) {
            const auto x = pMtx->mMtx[0][column];
            const auto y = pMtx->mMtx[1][column];
            const auto z = pMtx->mMtx[2][column];
            const auto length = std::sqrt(x * x + y * y + z * z);
            if (length <= cMinAxisLength) {
                continue;
            }

            const auto inverse = 1.0F / length;
            pMtx->mMtx[0][column] *= inverse;
            pMtx->mMtx[1][column] *= inverse;
            pMtx->mMtx[2][column] *= inverse;
        }
    }
}  // namespace

FixedPosition::FixedPosition(const LiveActor* pActor, const char* pJointName, const TVec3f& rLocalTrans, const TVec3f& rRotAxes) {
    init(MR::getJointMtx(pActor, pJointName), rLocalTrans, rRotAxes);
}

FixedPosition::FixedPosition(const LiveActor* pActor, const TVec3f& rLocalTrans, const TVec3f& rRotAxes) {
    init(actor_base_mtx(pActor), rLocalTrans, rRotAxes);
}

FixedPosition::FixedPosition(MtxPtr mtx, const TVec3f& rLocalTrans, const TVec3f& rRotAxes) {
    init(mtx, rLocalTrans, rRotAxes);
}

FixedPosition::FixedPosition(const LiveActor* pActor, const char* pBcsvName, const LiveActor* pResourceActor) {
    // The PC resource layer does not yet expose ResourceHolder BCSV lookup. Retain
    // the host-relative behavior so callers remain deterministic until it does.
    (void)pBcsvName;
    (void)pResourceActor;
    init(actor_base_mtx(pActor), TVec3f(0.0F, 0.0F, 0.0F), TVec3f(0.0F, 0.0F, 0.0F));
}

void FixedPosition::init(MtxPtr mtx, const TVec3f& rLocalTrans, const TVec3f& rRotAxes) {
    setBaseMtx(mtx);
    mLocalTrans.set(rLocalTrans);
    mRotDegrees.set(rRotAxes);
    mMtx.identity();
    mNormalizeScale = true;
}

void FixedPosition::calc() {
    set_local_tr(&mMtx, mLocalTrans, mRotDegrees);

    if (mBaseMtx != nullptr) {
        TMtx34f baseMtx;
        baseMtx.set((const MtxPtr)mBaseMtx);
        mMtx.concat(baseMtx, mMtx);
    }

    if (mNormalizeScale) {
        normalize_axes(&mMtx);
    }
}

void FixedPosition::setBaseMtx(MtxPtr mtx) {
    mBaseMtx = (TMtx34f*)mtx;
}

void FixedPosition::setLocalTrans(const TVec3f& rLocalTrans) {
    mLocalTrans.set(rLocalTrans);
}

void FixedPosition::copyRotate(TVec3f* pRotate) const {
    if (pRotate == nullptr) {
        return;
    }

    mMtx.getEulerXYZ(*pRotate);
    pRotate->scale(cRadToDeg);
}
