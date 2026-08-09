#include "Game/Util/FixedPosition.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/FixedPositionCompat.hpp"
#include "render/live_actor/LiveActorModel.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

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

        const auto& matrix = smgpc::compat::actor_base_matrix(pActor);
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

    [[nodiscard]] TVec3f read_vector(const smgpc::resource::BcsvTable& table, std::string_view prefix) {
        auto result = TVec3f{};
        if (const auto x = table.get_float(0U, std::string(prefix) + "X"); x.has_value()) {
            result.x = *x;
        }
        if (const auto y = table.get_float(0U, std::string(prefix) + "Y"); y.has_value()) {
            result.y = *y;
        }
        if (const auto z = table.get_float(0U, std::string(prefix) + "Z"); z.has_value()) {
            result.z = *z;
        }
        return result;
    }
}  // namespace

namespace smgpc::compat {
    FixedPositionResourceData load_fixed_position_resource(const smgpc::resource::RarcArchive& archive, std::string_view resource_name) {
        if (resource_name.empty()) {
            throw std::invalid_argument("FixedPosition resource name is empty");
        }

        const auto file_name = std::string(resource_name) + ".bcsv";
        const auto* entry = archive.find_resource(file_name);
        if (entry == nullptr) {
            throw std::runtime_error("FixedPosition resource does not exist: " + file_name);
        }

        const auto table = smgpc::resource::BcsvTable::from_bytes(archive.file_data(*entry));
        if (table.entry_count() == 0U) {
            throw std::runtime_error("FixedPosition resource has no rows: " + file_name);
        }

        auto joint_name = table.get_string(0U, "JointName");
        if (joint_name.has_value() && joint_name->empty()) {
            joint_name.reset();
        }

        return FixedPositionResourceData{
            .joint_name = std::move(joint_name),
            .translation = read_vector(table, "Trans"),
            .rotation = read_vector(table, "Rotate"),
        };
    }
}  // namespace smgpc::compat

FixedPosition::FixedPosition(const LiveActor* pActor, const char* pJointName, const TVec3f& rLocalTrans, const TVec3f& rRotAxes) {
    if (pActor == nullptr || pJointName == nullptr || *pJointName == '\0') {
        throw std::invalid_argument("FixedPosition named-joint construction requires an actor and joint name");
    }

    throw std::runtime_error("FixedPosition named-joint matrices are not available on the host: " + std::string(pJointName));
}

FixedPosition::FixedPosition(const LiveActor* pActor, const TVec3f& rLocalTrans, const TVec3f& rRotAxes) {
    if (pActor == nullptr) {
        throw std::invalid_argument("FixedPosition actor-relative construction requires an actor");
    }

    init(actor_base_mtx(pActor), rLocalTrans, rRotAxes);
}

FixedPosition::FixedPosition(MtxPtr mtx, const TVec3f& rLocalTrans, const TVec3f& rRotAxes) {
    init(mtx, rLocalTrans, rRotAxes);
}

FixedPosition::FixedPosition(const LiveActor* pActor, const char* pBcsvName, const LiveActor* pResourceActor) {
    if (pActor == nullptr || pBcsvName == nullptr || *pBcsvName == '\0') {
        throw std::invalid_argument("FixedPosition resource construction requires an actor and resource name");
    }

    const auto* resource_actor = pResourceActor != nullptr ? pResourceActor : pActor;
    const auto* model = smgpc::compat::actor_model(resource_actor);
    if (model == nullptr || model->model_arc_name().empty()) {
        throw std::runtime_error("FixedPosition resource actor has no real model archive");
    }

    auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr) {
        throw std::runtime_error("FixedPosition resource lookup requires an active runtime");
    }

    const auto archive_path = runtime->find_object_archive(model->model_arc_name());
    if (!archive_path.has_value()) {
        throw std::runtime_error("FixedPosition model archive does not exist: " + std::string(model->model_arc_name()));
    }

    const auto& archive = runtime->dvd().archive_for_path(*archive_path);
    const auto resource = smgpc::compat::load_fixed_position_resource(archive, pBcsvName);
    if (resource.joint_name.has_value()) {
        throw std::runtime_error("FixedPosition resource requests an unavailable host joint matrix: " + *resource.joint_name);
    }

    init(actor_base_mtx(pActor), resource.translation, resource.rotation);
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
