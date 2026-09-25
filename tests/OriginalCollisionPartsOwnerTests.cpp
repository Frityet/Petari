#include "Game/Map/HitInfo.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "NativeHeapFixture.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/MapObj/ClipAreaHolder.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool value, const char* message) {
        if (!value) aurora::throw_host_exception<std::runtime_error>(message);
    }
    void near(float actual, float expected, const char* message) {
        require(std::isfinite(actual) && std::abs(actual - expected) < 0.01F, message);
    }
    class MatrixActor final : public LiveActor {
    public:
        MatrixActor() : LiveActor("original collision ownership test") { matrix.identity(); }
        MtxPtr getBaseMtx() const override { return const_cast<MatrixActor*>(this)->matrix.toMtxPtr(); }
        TPos3f matrix;
    };

}

int main() {
    return smgpc::test::run_stage_resource_process("original-collision-parts-owner", [] {
        const auto domain = MR::getSceneObjHolder()->nativeAllocationHeap();
        auto& holder = *MR::getSceneObjHolder();
        auto* director = static_cast<CollisionDirector*>(holder.getObj(SceneObj_CollisionDirector));
        struct RestorePlacementZone {
            s32 previous = MR::getCurrentPlacementZoneId();
            ~RestorePlacementZone() { MR::setCurrentPlacementZoneId(previous); }
        } placement;
        MR::setCurrentPlacementZoneId(0);
        ResourceHolder* resource = nullptr;
        smgpc::test::on_resource_worker([&] {
            resource = MR::createAndAddResourceHolder("HeavensDoorSmallPlanet.arc");
        });
        MatrixActor actor;
        Triangle retained;
        {
            const JKRHeap::CurrentHeapScope game(*(domain));
            const aurora::allocation::ClientAllocationScope gameRouting({true, true});
            actor.initHitSensor(1);
            auto* sensor = MR::addHitSensorMapObj(&actor, "body", 8, 0, TVec3f(0, 0, 0));
            actor.initActorCollisionParts("HeavensDoorSmallPlanet", sensor, resource, actor.matrix.toMtxPtr(), false, false);
            auto* parts = actor.mCollisionParts;
            require(parts && !parts->_CC && parts->_CD && !parts->_CE && parts->_0 == nullptr,
                    "original resource-holder init creates a real initially invalid unbound part");
            require(JKRHeap::findFromRoot(parts) == &(*domain) &&
                    JKRHeap::findFromRoot(parts->mServer) == &(*domain) &&
                    JKRHeap::findFromRoot(parts->mServer->mapInfo) == &(*domain),
                    "part, server and original map iterator belong to Game allocations");
            const auto zone_members = parts->mZone->mNumParts;
            actor.makeActorAppeared();
            require(parts->_CC && parts->mZone->mNumParts == zone_members + 1,
                    "original appearance inserts exactly one real zone member");

            Triangle surface;
            for (s32 i = 0; i < parts->mServer->getTriangleNum() && !surface.isValid(); ++i)
                if (parts->mServer->getPrismData(i)->mHeight > 0.0F)
                    surface.fillData(parts, i, sensor);
            require(surface.isValid(), "Real archived part contains an active original prism");
            retained = surface;
            require(retained.mParts == parts && retained.mIdx == surface.mIdx &&
                    retained.getBaseMtx() == &parts->mBaseMatrix,
                    "native query retains exact original owner, local prism and matrix identity");
            {
                MatrixActor bound_actor;
                bound_actor.initBinder(10.0F, 0.0F, 8U);
                MR::setBinderExceptSensorType(&bound_actor, &bound_actor.mPosition, 10.0F);
                auto* filter = dynamic_cast<ClipAreaCollisionFilter*>(bound_actor.mBinder->mCollisionPartsFilter);
                require(filter && filter->_04 == &bound_actor.mPosition && filter->_08 == 10.0F &&
                        JKRHeap::findFromRoot(filter) == &(*domain),
                        "original ClipArea filter retains the live center and scene allocation domain");
                const auto sensor_type = sensor->mType;
                sensor->mType = ATYPE_CLIP_FIELD_MAP_PARTS;
                MR::createClipAreaHolder();
                auto* clip_holder = dynamic_cast<ClipAreaHolder*>(holder.getObj(SceneObj_ClipAreaHolder));
                require(clip_holder && clip_holder->mIsActive && clip_holder->getObjNum() == 0 &&
                        JKRHeap::findFromRoot(clip_holder) == &(*domain),
                        "scene factory creates the actual empty active ClipArea holder on the Game heap");
                require(filter->isInvalidParts(parts),
                        "clip-field collision is excluded outside all live ClipAreas");
                const auto center = (surface.mPos[0] + surface.mPos[1] + surface.mPos[2]) * (1.0F / 3.0F);
                const auto start = center + surface.mNormals[0];
                const auto offset = surface.mNormals[0] * -2.0F;
                Triangle found;
                TVec3f position;
                require(!MR::getFirstPolyOnLineToMap(&position, &found, start, offset, filter, nullptr),
                        "original map queries dispatch the ClipArea filter against real archived collision parts");
                sensor->mType = sensor_type;
                require(!filter->isInvalidParts(parts) &&
                        MR::getFirstPolyOnLineToMap(&position, &found, start, offset, filter, nullptr) && found.mParts == parts,
                        "ordinary collision remains queryable through the same installed filter");
                MR::deactivateClipArea();
                require(!MR::isActiveClipArea() && !MR::isInClipArea(center, 10.0F),
                        "holder activity is owned by the original scene object");
                MR::activateClipArea();
                require(MR::isActiveClipArea(), "original ClipArea holder can reactivate");
            }
            {
                MatrixActor caster;
                MR::initShadowVolumeSphere(&caster, 10);
                auto* shadow = caster.mShadowControllerList->getController(0U);
                const auto center = (surface.mPos[0] + surface.mPos[1] + surface.mPos[2]) * (1.0F / 3.0F);
                const auto normal = surface.mNormals[0];
                shadow->setDropPosFix(center + normal);
                shadow->setDropDirFix(normal * -1.0F);
                shadow->setDropStartOffset(0.25F);
                shadow->setDropLength(2.0F);
                shadow->onCalcCollisionOneTime();
                shadow->updateProjection();
                TVec3f projected, projected_normal;
                shadow->getProjectionPos(&projected);
                shadow->getProjectionNormal(&projected_normal);
                require(shadow->isProjected() && shadow->mProjectedSensor == sensor,
                        "original shadow projects against the actual resource CollisionParts sensor");
                near(projected.x, center.x, "original projection reaches the archived KCL face X");
                near(projected.y, center.y, "original projection reaches the archived KCL face Y");
                near(projected.z, center.z, "original projection reaches the archived KCL face Z");
                near(shadow->getProjectionLength(), 1.0F, "original projection length starts at the drop position");
                require(projected_normal.epsilonEquals(normal, 0.01F) && shadow->_65 == 1 && !shadow->isCalcCollision(),
                        "original projection retains the KCL normal and consumes one-shot collision");
                shadow->setDropPosFix(center + normal * 10.0F);
                shadow->updateProjection();
                shadow->getProjectionPos(&projected);
                require(projected.epsilonEquals(center, 0.01F), "consumed one-shot projection stays cached");
            }
            auto* base = retained.getBaseMtx();
            actor.matrix.mMtx[0][3] = 30;
            actor.calcAnim();
            near(parts->mMatrix.mMtx[0][3], 30, "original LiveActor calcAnim submits pending matrix");
            near(base->mMtx[0][3], 0, "submission does not prematurely commit collision movement");
            director->movement();
            near(base->mMtx[0][3], 30, "original collision movement commits pending matrix");
            near(parts->mPrevBaseMatrix.mMtx[0][3], 0, "first moved frame keeps preceding transform");
            require(parts->_D4 == 0 && retained.isHostMoved(), "normal original movement retains moving-host flag");
            TVec3f velocity;
            MR::calcVelocityMovingPoint(&retained, TVec3f(35, 0, 0), &velocity);
            near(velocity.x, 30, "moving-point velocity follows actual previous/current transforms");
            const auto* position = retained.calcAndGetPos(0);
            near(position->x, surface.mPos[0].x + 30, "retained original triangle recalculates translated vertex");
            MR::offUpdateCollisionParts(&actor);
            actor.matrix.mMtx[0][3] = 90;
            actor.calcAnim(); director->movement();
            near(base->mMtx[0][3], 30, "original disabled update policy preserves committed matrix");
            actor.mPosition.x = 90;
            MR::onUpdateCollisionPartsOnetimeImmediately(&actor);
            near(base->mMtx[0][3], 90, "one-shot immediate policy commits real current actor matrix");
            require(!parts->_CD && !parts->_CE && parts->_D4 == 1,
                    "one-shot update clears only its original request flag and preserves continuous-off policy");
            MR::invalidateCollisionParts(&actor);
            require(!parts->_CC && retained.isValid() && retained.getBaseMtx() == base,
                    "zone invalidation hides queries while existing original triangle retains its live part");
            MR::validateCollisionParts(&actor);
            MR::validateCollisionParts(&actor);
            require(parts->mZone->mNumParts == zone_members + 1, "revalidation preserves one original zone entry");
            actor.makeActorDead();
            require(!parts->_CC && parts->mZone->mNumParts == zone_members, "original death removes zone membership");
            actor.makeActorAppeared();
            auto* zone = parts->mZone;
            actor.releaseNativeCollisionParts();
            require(actor.mCollisionParts == nullptr && !retained.isValid() && zone->mNumParts == zone_members,
                    "actor retirement removes native part identity");
        }
    });
}
