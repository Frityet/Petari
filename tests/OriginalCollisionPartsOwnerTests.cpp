#include "compat/ActorRuntimeRegistry.hpp"
#include "SceneExecutionFixture.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/CollisionDirectorOwnership.hpp"
#include "compat/HitInfoCompat.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/StageResourceBinding.hpp"
#include "compat/StageSessionState.hpp"
#include "compat/StageZoneMatrixRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "scene/PlacementZoneNameScope.hpp"
#include "scene/StageCollisionService.hpp"
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
#include "Game/Scene/SceneObjHolder.hpp"
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
    class Logger final : public smgpc::logging::ILogger {
        void write(std::FILE*, std::source_location, smgpc::logging::Level,
                   smgpc::logging::Category, std::string_view) override {}
    };
    class MatrixActor final : public LiveActor {
    public:
        MatrixActor() : LiveActor("original collision ownership test") { matrix.identity(); }
        MtxPtr getBaseMtx() const override { return const_cast<MatrixActor*>(this)->matrix.toMtxPtr(); }
        TPos3f matrix;
    };
    struct Rollback {
        SceneObjHolder* holder;
        static NameObj* factory(int id, void* context) {
            if (id != SceneObj_SphereSelector) return nullptr;
            auto& self = *static_cast<Rollback*>(context);
            require(self.holder->create(SceneObj_CollisionDirector) != nullptr, "nested original collision owner is created");
            aurora::throw_host_exception<std::runtime_error>("intentional outer factory failure after collision construction");
        }
    };
}

int main() {
    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    require(disc, "SMGPC_REAL_DISC must name the actual game archive");
    smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original CollisionParts ownership"});
    smgpc::render::AuroraRenderer renderer(window);
    require(aurora_dvd_open(disc), "cannot open requested disc");
    struct Disc { ~Disc() { aurora_dvd_close(); } } disc_guard;
    DVDInit();
    smgpc::resource::GameResourceRuntime process({96U << 20, 32U << 20, 4U << 20});
    Logger logger;
    smgpc::runtime::RuntimeContext runtime(logger, window, process);
    runtime.initialize_scenario_catalog(process);
    auto& scheduler = runtime.scheduler();
    smgpc::runtime::SceneSchedulerBinding scheduler_binding(scheduler);
    std::vector<smgpc::scene::StageHolderOccurrence> holders;
    const auto tables = smgpc::scene::resolve_stage_placement_tables(runtime.dvd(), "HeavensDoorGalaxy", 1, &holders);
    smgpc::compat::StageResourceBinding stage_resources(runtime.dvd(), holders, tables);
    smgpc::compat::StageZoneMatrixBinding zones(holders, tables);
    smgpc::compat::StageSessionState session("Game", "HeavensDoorGalaxy", 1, JMapIdInfo(0, 0));
    smgpc::compat::StageSessionBinding session_binding(session);
    (void)renderer.begin_frame();
    // StageSessionBinding owns its pointer layout graph across both cycles.
    const auto baseline_objects = smgpc::compat::name_obj_runtime_state_count();
    const auto baseline_entries = scheduler.snapshot().size();
    for (int cycle = 0; cycle < 2; ++cycle) {
        smgpc::scene::StageCollisionService collision;
        collision.activate();
        Rollback rollback{nullptr};
        std::weak_ptr<smgpc::compat::JkrAllocationDomain> weak_domain;
        {
            const auto domain = smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 8U << 20);
            smgpc::test::SceneExecutionFixture scene(scheduler, domain, Rollback::factory, &rollback);
            auto& holder = scene.holder();
            auto& binding = scene.objects();
            rollback.holder = &holder;
            weak_domain = domain;
            const auto empty_objects = smgpc::compat::name_obj_runtime_state_count();
            try { holder.create(SceneObj_SphereSelector); require(false, "factory must fail"); }
            catch (const std::runtime_error&) {}
            // This original JKRSolidHeap reclaims its arena at scene retirement.
            // Rollback must immediately retire owners, registrations and slots.
            require(!holder.isExist(SceneObj_CollisionDirector) &&
                    smgpc::compat::name_obj_runtime_state_count() == empty_objects,
                    "failed outer factory retires original raw collision children and registrations");
            auto* director = static_cast<CollisionDirector*>(holder.create(SceneObj_CollisionDirector));
            require(director && director->mCategoryKeeper[0], "same binding can retry its original owner after rollback");
            holder.create(SceneObj_NameObjGroup);
            holder.create(SceneObj_AreaObjContainer);
            holder.create(SceneObj_PlanetGravityManager);
            holder.create(SceneObj_DemoDirector);
            binding.initialize_camera_system();
            holder.create(SceneObj_PlacementStateChecker);
            smgpc::scene::PlacementZoneNameScope placement(0, "HeavensDoorGalaxy");
            auto* resource = MR::createAndAddResourceHolder("HeavensDoorSmallPlanet.arc");
            MatrixActor actor;
            Triangle retained;
            {
                smgpc::compat::JkrAllocationScope game(domain);
                actor.initHitSensor(1);
                auto* sensor = MR::addHitSensorMapObj(&actor, "body", 8, 0, TVec3f(0, 0, 0));
                actor.initActorCollisionParts("HeavensDoorSmallPlanet", sensor, resource, actor.matrix.toMtxPtr(), false, false);
                auto* parts = actor.mCollisionParts;
                require(parts && !parts->_CC && parts->_CD && !parts->_CE && parts->_0 == nullptr,
                        "original resource-holder init creates a real initially invalid unbound part");
                require(JKRHeap::findFromRoot(parts) == &domain->heap() &&
                        JKRHeap::findFromRoot(parts->mServer) == &domain->heap() &&
                        JKRHeap::findFromRoot(parts->mServer->mapInfo) == &domain->heap(),
                        "part, server and original map iterator belong to Game allocations");
                actor.makeActorAppeared();
                require(parts->_CC && parts->mZone->mNumParts == 1 && director->mCategoryKeeper[0]->mZoneCount == 1,
                        "original appearance inserts exactly one real zone member");
                collision.build();
                std::optional<smgpc::scene::StageCollisionSurface> surface;
                for (s32 i = 0; i < parts->mServer->getTriangleNum() && !surface; ++i)
                    surface = collision.surface(parts, i);
                require(surface.has_value(), "real archived part publishes its prisms");
                retained = smgpc::compat::make_collision_triangle(collision, surface->triangle_index);
                require(retained.mParts == parts && retained.mIdx == surface->prism_index &&
                        retained.getBaseMtx() == &parts->mBaseMatrix,
                        "native query retains exact original owner, local prism and matrix identity");
                {
                    MatrixActor bound_actor;
                    bound_actor.initBinder(10.0F, 0.0F, 8U);
                    MR::setBinderExceptSensorType(&bound_actor, &bound_actor.mPosition, 10.0F);
                    auto* filter = dynamic_cast<ClipAreaCollisionFilter*>(bound_actor.mBinder->mCollisionPartsFilter);
                    require(filter && filter->_04 == &bound_actor.mPosition && filter->_08 == 10.0F &&
                            JKRHeap::findFromRoot(filter) == &domain->heap(),
                            "original ClipArea filter retains the live center and scene allocation domain");
                    const auto sensor_type = sensor->mType;
                    sensor->mType = ATYPE_CLIP_FIELD_MAP_PARTS;
                    require(!MR::isExistClipAreaHolder() && !filter->isInvalidParts(parts),
                            "retail filtering preserves collision when no ClipArea holder exists");
                    MR::createClipAreaHolder();
                    auto* clip_holder = dynamic_cast<ClipAreaHolder*>(holder.getObj(SceneObj_ClipAreaHolder));
                    require(clip_holder && clip_holder->mIsActive && clip_holder->getObjNum() == 0 &&
                            JKRHeap::findFromRoot(clip_holder) == &domain->heap(),
                            "scene factory creates the actual empty active ClipArea holder on the Game heap");
                    require(filter->isInvalidParts(parts),
                            "clip-field collision is excluded outside all live ClipAreas");
                    const auto center = (surface->vertices[0] + surface->vertices[1] + surface->vertices[2]) * (1.0F / 3.0F);
                    const auto start = center + surface->normals[0];
                    const auto offset = surface->normals[0] * -2.0F;
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
                    const auto center = (surface->vertices[0] + surface->vertices[1] + surface->vertices[2]) * (1.0F / 3.0F);
                    const auto normal = surface->normals[0];
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
                near(position->x, surface->vertices[0].x + 30, "retained original triangle recalculates translated vertex");
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
                require(!collision.surface(surface->triangle_index) && retained.isValid() && retained.getBaseMtx() == base,
                        "zone invalidation hides queries while existing original triangle retains its live part");
                MR::validateCollisionParts(&actor);
                MR::validateCollisionParts(&actor);
                require(parts->mZone->mNumParts == 1, "revalidation preserves one original zone entry");
                actor.makeActorDead();
                require(!parts->_CC && parts->mZone->mNumParts == 0, "original death removes zone membership");
                actor.makeActorAppeared();
                auto* zone = parts->mZone;
                smgpc::compat::release_actor_collision_parts(&actor);
                require(actor.mCollisionParts == nullptr && !retained.isValid() && zone->mNumParts == 0,
                        "actor retirement removes native part identity");
            }
        }
        require(weak_domain.expired() && smgpc::compat::name_obj_runtime_state_count() == baseline_objects &&
                scheduler.snapshot().size() == baseline_entries,
                "all scene collision owners, registrations and Game storage retire between cycles");
        collision.deactivate();
    }
    renderer.end_frame();
    std::cout << "Original CollisionParts ownership, rollback, matrices, flags, query identity and retirement passed\n";
}
