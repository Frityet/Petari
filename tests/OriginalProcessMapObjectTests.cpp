#include "Game/Map/CollisionParts.hpp"
#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/LiveActor/ClippingActorHolder.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/LiveActor/ViewGroupCtrl.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/MapObj/MapPartsRotator.hpp"
#include "Game/MapObj/RotateMoveObj.hpp"
#include "Game/MapObj/SimpleMapObj.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapPartsUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/NameObj/NameObj.hpp"

#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
void require(bool value, const char* message) {
    if (!value) {
        std::fprintf(stderr, "[map-object-probe] assertion failed: %s\n", message);
        std::fflush(stderr);
        aurora::throw_host_exception<std::runtime_error>(message);
    }
}

#ifndef NDEBUG
constexpr std::array names{
    "HeavensDoorMiddleRotatePartsA", "HeavensDoorMiddleRotatePartsB",
    "HeavensDoorInsideRotatePartsA", "HeavensDoorInsideRotatePartsB",
    "HeavensDoorInsideRotatePartsC", "KoopaJrNormalShipA",
};

int name_index(const char* name) {
    for (std::size_t index = 0; index < names.size(); ++index)
        if (name && std::strcmp(name, names[index]) == 0) return static_cast<int>(index);
    return -1;
}

bool near(const TVec3f& a, const TVec3f& b, float epsilon = 0.02F) {
    return std::abs(a.x - b.x) < epsilon && std::abs(a.y - b.y) < epsilon && std::abs(a.z - b.z) < epsilon;
}

struct Observation {
    MapObjActor* actor = nullptr;
    MapPartsRotator* rotator = nullptr;
    int kind = -1;
    std::uint64_t samples = 0, clipped = 0, suspended = 0, angle_changes = 0, matrix_changes = 0;
    float last_angle = 0;
    s32 last_step = 0;
    std::array<float, 12> last_matrix{};
};

struct Probe {
    bool initialized = false;
    std::uint64_t ready_frame = 0, observations = 0;
    std::vector<Observation> owners;
    std::vector<const NameObj*> identities;
    unsigned active_line_queries = 0, retained_disabled_parts = 0, move_limits = 0;

    void verify_part(CollisionParts* parts, MapObjActor* actor, bool enabled, int category) {
        require(parts && parts->mServer && parts->mServer->mFile && parts->mServer->getTriangleNum() > 0 &&
                    parts->mHitSensor && parts->mHitSensor->mHost == actor && parts->mKeeperIndex == category && parts->mZone,
                "Real KCL parts retain their original sensor host, category and zone");
        const auto* zone = parts->mZone;
        const auto membership = std::count(zone->mPartsArray, zone->mPartsArray + zone->mNumParts, parts);
        require(parts->_CC == enabled && membership == (enabled ? 1 : 0),
                "Original enabled state agrees with actual keeper membership");
        bool checked_triangle = false, checked_line = false;
        for (s32 prism = 0; prism < parts->mServer->getTriangleNum(); ++prism) {
            if (parts->mServer->getPrismData(prism)->mHeight <= 0.0F) continue;
            Triangle triangle;
            triangle.fillData(parts, prism, parts->mHitSensor);
            require(triangle.mParts == parts && triangle.mIdx == prism && triangle.getSensor() == parts->mHitSensor,
                    "Original Triangle retains its real part, prism and sensor");
            checked_triangle = true;
            if (!enabled) break;
            const auto center = triangle.mPos[0] * 0.2F + triangle.mPos[1] * 0.3F + triangle.mPos[2] * 0.5F;
            const auto start = center + triangle.mNormals[0] * 10.0F;
            const auto offset = triangle.mNormals[0] * -20.0F;
            std::array<HitInfo, 32> hits;
            const auto count = parts->checkStrikeLine(hits.data(), hits.size(), start, offset, nullptr);
            require(count <= hits.size(), "Original per-part line query respects its output capacity");
            for (u32 index = 0; index < count; ++index) {
                require(hits[index].mParentTriangle.mParts == parts,
                        "Original per-part line query retains the queried collision owner");
                checked_line |= hits[index].mParentTriangle.mIdx == static_cast<u32>(prism);
            }
            if (checked_line) break;
        }
        require(checked_triangle && (!enabled || checked_line),
                "Each real resource has an original Triangle; each active resource answers an interior face line query");
        active_line_queries += checked_line;
        retained_disabled_parts += !enabled;
    }

    void initialize() {
        auto* stage = MR::getStageDataHolder();
        auto* collision = MR::getCollisionDirector();
        require(stage && collision, "Actual original stage and collision owners exist");
        std::vector<MapObjActor*> actors;
        for (auto* object : NameObj::snapshotNativeObjects())
            if (auto* actor = dynamic_cast<MapObjActor*>(object); actor && name_index(actor->mObjectName) >= 0) actors.push_back(actor);
        require(actors.size() == 8, "Ordinary factory constructs all eight newly supported authored map objects");
        std::array<unsigned, names.size()> counts{};
        for (s32 zone_id = 0; zone_id < MR::getZoneNum(); ++zone_id) {
            auto* holder = stage->getStageDataHolderFromZoneId(zone_id);
            if (!holder) continue;
            for (const auto& table : holder->mPlacementObjs) {
                for (s32 row = 0; row < table.getNumEntries(); ++row) {
                    const JMapInfoIter iter(&table, row);
                    const char* name = nullptr;
                    if (!MR::getObjectName(&name, iter)) continue;
                    const auto kind = name_index(name);
                    if (kind < 0) continue;
                    TVec3f position;
                    require(MR::getJMapInfoTrans(iter, &position), "Authored world placement is readable");
                    const auto matches = [&](const MapObjActor* actor) {
                        return std::strcmp(actor->mObjectName, name) == 0 && near(actor->mPosition, position);
                    };
                    require(std::count_if(actors.begin(), actors.end(), matches) == 1,
                            "Each authored row corresponds to one actual class/model owner at its transformed world position");
                    auto* actor = *std::find_if(actors.begin(), actors.end(), matches);
                    require(std::none_of(owners.begin(), owners.end(), [=](const auto& owner) { return owner.actor == actor; }),
                            "Two placement rows cannot share one constructed actor");
                    ++counts[kind];
                    const bool ship = kind == 5;
                    require(actor->mModelManager && actor->mModelManager->getJ3DModel() &&
                                actor->mModelManager->getModelResourceHolder() && actor->mCollisionParts &&
                                MR::isEqualStringCase(MR::getModelResName(actor), name),
                            "Each actor owns its named real model resource and main collision");
                    require(actor->mCollisionParts->mZone && actor->mCollisionParts->mZone->mZoneID == zone_id &&
                                actor->getSensor("body") == actor->mCollisionParts->mHitSensor,
                            "Main collision belongs to the authored zone and original body sensor");
                    auto* switches = actor->mStageSwitchCtrl;
                    s32 authored_appear = -1;
                    require(iter.getValue("SW_APPEAR", &authored_appear), "Authored appearance field is readable");
                    const bool has_appear = switches && switches->isValidSwitchAppear();
                    require(has_appear == (authored_appear >= 0),
                            "Actual appearance controller presence matches the authored row");
                    if (has_appear) {
                        require(switches->mSW_Appear->mIsGlobal == (authored_appear >= 1000) &&
                                    switches->mSW_Appear->getSwitchNo() == authored_appear % 1000,
                                "Original appearance controller retains its authored switch number");
                    }
                    std::fprintf(stderr, "[map-object-probe] appearance=%s authored=%d valid=%d on=%d dead=%d\n",
                                 name, authored_appear, has_appear, has_appear ? switches->isOnSwitchAppear() : -1, actor->mFlag.mIsDead);
                    // The switch watcher and original demos own appearance. Observe
                    // their result; do not force the Inside geometry alive or dead.
                    verify_part(actor->mCollisionParts, actor, !actor->mFlag.mIsDead, 0);
                    const auto& resources = actor->nativeCollisionParts();
                    require(resources.size() == (ship ? 2U : 1U) &&
                                std::count_if(resources.begin(), resources.end(), [&](const auto& resource) {
                                    return resource->nativeResourceName() == name && resource->nativeKclSize() > 0 && resource->nativeAttributesSize() > 0;
                                }) == 1,
                            "Each actor owns the authored main KCL and attributes without substitute geometry");
                    Observation observation{.actor = actor, .kind = kind};
                    if (!ship) {
                        auto* rotator = dynamic_cast<MapPartsRotator*>(actor->mRotator);
                        require(dynamic_cast<RotateMoveObj*>(actor) && rotator && rotator->mHost == actor && rotator->mSpine,
                                "Each rotating placement uses the original RotateMoveObj and actor-owned MapPartsRotator");
                        f32 raw_speed = 0, angle = 0;
                        s32 axis = -1, accel = -1, condition = -1;
                        require(MR::getMapPartsArgRotateSpeed(&raw_speed, iter) && MR::getMapPartsArgRotateAngle(&angle, iter) &&
                                    MR::getMapPartsArgRotateAxis(&axis, iter) && MR::getMapPartsArgRotateAccelType(&accel, iter) &&
                                    MR::getMapPartsArgMoveConditionType(&condition, iter), "Actual authored rotation arguments are readable");
                        require(angle == 0 && accel == 0 && condition == 0 && rotator->mRotateAxis == axis &&
                                    std::abs(rotator->_18 - raw_speed * 0.01F) < 0.00001F && rotator->mRotateAngle == angle,
                                "Original continuous rotation retains authored axis and hundredths-of-degree speed conversion");
                        observation.rotator = rotator;
                        observation.last_angle = rotator->mAngle;
                        observation.last_step = rotator->getStep();
                        identities.push_back(rotator);
                    } else {
                        require(dynamic_cast<SimpleMapObj*>(actor) && !actor->mRotator,
                                "Ships use the ordinary SimpleMapObj creator, without a rotating-actor substitute");
                        auto* lod = actor->mPlanetLodCtrl;
                        require(lod && lod->mActor == actor && !lod->_10 && lod->_14 && lod->_0 == 5000.0F && lod->_4 == 10000.0F &&
                                    lod->_1C && lod->_20 && lod->_24 && lod->_28,
                                "Each ship owns a real original LodCtrl with canonical thresholds and view controls");
                        auto* low = lod->_14;
                        require(low->mModelManager && low->mModelManager->getJ3DModel() && low->mModelManager->mXanimePlayer &&
                                    low->mModelManager->getPlayingBckName() && low->mMtx == actor->getBaseMtx() &&
                                    MR::isEqualStringCase(MR::getModelResName(low), "KoopaJrNormalShipALow") && !low->mCollisionParts,
                                "Each Low child owns its actual animated Low resource and follows the original high-model matrix");
                        auto* clipping = MR::getClippingDirector();
                        require(clipping && clipping->mActorHolder && clipping->mActorHolder->mViewGroupCtrl,
                                "Original clipping and view-group owners exist");
                        const auto* view = clipping->mActorHolder->mViewGroupCtrl;
                        require(std::count(view->mLodCtrls, view->mLodCtrls + view->mViewCtrlCount, lod) == 1,
                                "Each real LodCtrl has one original view-group registration");
                        auto* keeper = MR::getCollisionDirector()->getCategoryKeeper(3);
                        CollisionParts* limit = nullptr;
                        for (s32 zone = 0; zone < keeper->mZoneNum; ++zone)
                            for (s32 index = 0; index < keeper->mZones[zone]->mNumParts; ++index) {
                                auto* part = keeper->mZones[zone]->mPartsArray[index];
                                if (part->mHitSensor && part->mHitSensor->mHost == actor) {
                                    require(!limit, "Each ship has exactly one original MoveLimit part");
                                    limit = part;
                                }
                            }
                        require(limit && limit != actor->mCollisionParts && limit->mZone->mZoneID == zone_id &&
                                    std::count_if(resources.begin(), resources.end(), [](const auto& resource) {
                                        return resource.resource_name == "MoveLimit" && resource.kcl_size > 0;
                                    }) == 1, "Real MoveLimit KCL retains a separate category-3 part under its authored actor");
                        verify_part(limit, actor, true, 3);
                        ++move_limits;
                        identities.push_back(low);
                    }
                    std::memcpy(observation.last_matrix.data(), actor->mCollisionParts->mBaseMatrix.mMtx,
                                sizeof(float) * observation.last_matrix.size());
                    owners.push_back(observation);
                    identities.push_back(actor);
                    std::fprintf(stderr, "[map-object-probe] owner=%s zone=%d dead=%d clipped=%d main_prisms=%d resources=%zu\n",
                                 name, zone_id, actor->mFlag.mIsDead, actor->mFlag.mIsClipped,
                                 actor->mCollisionParts->mServer->getTriangleNum(), resources.size());
                }
            }
        }
        require(counts == std::array<unsigned, names.size()>{1, 1, 1, 1, 1, 3} && owners.size() == 8 &&
                    move_limits == 3 && retained_disabled_parts + active_line_queries == 11,
                "Eight real placements own eleven retained collision resources with original active membership and three active MoveLimits");
    }

    void after_frame(GameSystem& system, std::uint64_t frame) {
        auto* controller = system.mSceneController;
        if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
            controller->getCurrentSceneForExecute() != controller->mScene || !dynamic_cast<GameScene*>(controller->mScene)) return;
        const aurora::allocation::HostAllocationScope host;
        if (!ready_frame) ready_frame = frame;
        if (!initialized) {
            if (frame < ready_frame + 20) return;
            initialize();
            initialized = true;
            std::fprintf(stderr, "[map-object-probe] PASS authored classes/models/KCL/Low ownership and original active line queries; frame=%llu\n",
                         static_cast<unsigned long long>(frame));
            return;
        }
        ++observations;
        for (auto& owner : owners) {
            auto* actor = owner.actor;
            ++owner.samples;
            owner.clipped += actor->mFlag.mIsClipped;
            owner.suspended += (actor->getFlag() & 1) != 0;
            require(actor->mCollisionParts->_CC == !actor->mFlag.mIsDead,
                    "Natural original actor appearance remains synchronized with main collision membership");
            if (auto* rotator = owner.rotator) {
                const auto step = rotator->getStep();
                const auto delta = std::remainder(rotator->mAngle - owner.last_angle, 360.0F);
                require(std::isfinite(rotator->mAngle) && step >= owner.last_step && step <= owner.last_step + 1,
                        "Original continuous MapParts nerve advances at most once per observed game frame");
                require(std::abs(delta - rotator->_18 * (step - owner.last_step)) < 0.0001F,
                        "Naturally observed rotation follows authored speed per actual nerve update, including wrap and suspension");
                owner.angle_changes += std::abs(delta) > 0.0001F;
                owner.last_angle = rotator->mAngle;
                owner.last_step = step;
            }
            std::array<float, 12> matrix;
            std::memcpy(matrix.data(), actor->mCollisionParts->mBaseMatrix.mMtx, sizeof(float) * matrix.size());
            require(std::all_of(matrix.begin(), matrix.end(), [](float value) { return std::isfinite(value); }),
                    "Actual collision matrices remain finite through ordinary updates");
            owner.matrix_changes += matrix != owner.last_matrix;
            owner.last_matrix = matrix;
        }
    }
};
#endif
}

int main(int argc, char** argv) {
#ifdef NDEBUG
    std::fprintf(stderr, "This original-process diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const char* frames = "360";
        for (int index = 1; index < argc; ++index) {
            if (std::strcmp(argv[index], "--120") == 0) {
                frames = "120";
            } else if (std::strcmp(argv[index], "-ApplePersistenceIgnoreState") == 0 && index + 1 < argc) {
                ++index;  // AppKit process option supplied by the bounded test runner.
            } else {
                require(false, "Only --120 and the AppKit persistence process option are accepted");
            }
        }
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the actual disc image");
        const auto save = std::filesystem::temp_directory_path() / ("petari-original-map-object-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic starts with fresh console settings");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE", "SMGPC_STRICT_PLACEMENT"}) unsetenv(name);
        const smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original MapObj ownership integration",
            .arguments = {"original-map-object-test", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", frames},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct DiscLifetime { ~DiscLifetime() { smgpc::app::close_disc_image(); } } disc_lifetime;
        Probe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<Probe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.initialized && probe.observations >= 30,
                "Original process completes ownership checks and subsequent normal opening frames");
        for (const auto* object : probe.identities)
            require(!NameObj::nativeGeneration(object), "Normal scene retirement removes every observed actor/child/rotator identity");
        for (const auto& owner : probe.owners) {
            require(!NameObj::nativeGeneration(owner.actor),
                    "Normal scene retirement releases actual model and all collision resource owners");
            std::fprintf(stderr, "[map-object-probe] observation=%s samples=%llu clipped=%llu suspended=%llu angle_changes=%llu collision_matrix_changes=%llu\n",
                         names[owner.kind], static_cast<unsigned long long>(owner.samples), static_cast<unsigned long long>(owner.clipped),
                         static_cast<unsigned long long>(owner.suspended), static_cast<unsigned long long>(owner.angle_changes),
                         static_cast<unsigned long long>(owner.matrix_changes));
        }
        std::fprintf(stderr, "PASS original-process MapObj: eight authored owners, three actual Low models, eleven real collision parts (%u active face queries, %u retained inactive), observed appearance, %s frames and retirement; no visibility or gameplay-parity claim\n",
                     probe.active_line_queries, probe.retained_disabled_parts, frames);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process MapObj: %s\n", error.what());
        return 1;
    }
#endif
}
