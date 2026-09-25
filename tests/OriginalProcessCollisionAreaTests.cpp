#include "Game/Map/HitInfo.hpp"
#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/CollisionArea.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include <aurora/exception.hpp>
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/KCollisionResource.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "scene/StageCollisionService.hpp"

#include <aurora/allocation.hpp>
#include <aurora/main.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#ifndef NDEBUG
namespace {
    void require(bool value, const char* message) {
        if (!value) aurora::throw_host_exception<std::runtime_error>(message);
    }

    void near(float actual, float expected, const char* message) {
        require(std::isfinite(actual) && std::abs(actual - expected) < 0.05F, message);
    }

    void near(const TVec3f& actual, const TVec3f& expected, const char* message) {
        near(actual.x, expected.x, message);
        near(actual.y, expected.y, message);
        near(actual.z, expected.z, message);
    }

    class PlacementZoneScope final {
    public:
        explicit PlacementZoneScope(s32 zone) : previous(MR::getCurrentPlacementZoneId()) {
            MR::setCurrentPlacementZoneId(zone);
        }
        ~PlacementZoneScope() { MR::setCurrentPlacementZoneId(previous); }
    private:
        s32 previous;
    };

    std::vector<JMapInfoIter> collision_area_placements() {
        std::vector<JMapInfoIter> result;
        auto* root = MR::getStageDataHolder();
        require(root != nullptr, "Original stage holder exists");
        for (s32 zone = 0; zone < MR::getZoneNum(); ++zone) {
            auto* holder = root->getStageDataHolderFromZoneId(zone);
            if (!holder) continue;
            for (const auto& table : holder->mPlacementObjs) {
                for (s32 row = 0; row < table.getNumEntries(); ++row) {
                    const JMapInfoIter iter(&table, row);
                    const char* name = nullptr;
                    if (MR::getObjectName(&name, iter) && std::strcmp(name, "CollisionArea") == 0)
                        result.push_back(iter);
                }
            }
        }
        return result;
    }

    class OnlyPart final : public CollisionPartsFilterBase {
    public:
        explicit OnlyPart(const CollisionParts* parts) : _parts(parts) {}
        bool isInvalidParts(const CollisionParts* parts) const override { return parts != _parts; }
    private:
        const CollisionParts* _parts;
    };

    s32 line_hits(CollisionCategorizedKeeper& keeper, const CollisionPartsFilterBase& filter,
                  const TVec3f& start, const TVec3f& offset) {
        return keeper.checkStrikeLine(start, offset, 8, &filter, nullptr);
    }

    struct Probe {
        bool expect_placements = true;
        bool exercised = false;
        std::vector<const KCLFile*> retained_files;

        void check_placements(const std::vector<JMapInfoIter>& placements) {
            auto* manager = MR::getAreaObjManager("CollisionArea");
            require(manager != nullptr, "Original CollisionArea manager exists");
            require(manager->getNumAreaObj() == (expect_placements ? 2 : 0),
                    "Factory support and authored CollisionArea instance count agree");
            for (s32 index = 0; index < manager->getNumAreaObj(); ++index) {
                auto* area = dynamic_cast<CollisionArea*>(manager->getAreaObj(index));
                require(area && area->mFormType == AreaForm::Type_Cube1 && area->mPolygon,
                        "Factory creates the exact center-origin cube and original polygon");
                require(area->mPolygon->mForm == area->mForm && area->_5C == 0 && area->_60 == -1,
                        "Original form ownership and NoInit argument defaults survive placement");
                auto* parts = area->mPolygon->mParts;
                require(parts && parts->mZone->mZoneID == 5 && parts->mServer->getTriangleNum() == 2 &&
                            parts->mHitSensor == area->mPolygon->getSensor("body") &&
                            parts->mHitSensor->mHost == area->mPolygon && parts->mHitSensor->mSensorGroup,
                        "Authored area uses its real zone, sensor group and two-prism parts");
                require(!area->mSwitchCtrl->isOnSwitchAppear() && !area->isValid() && !parts->_CC,
                        "Fresh authored SW_APPEAR remains off and collision membership is disabled");
                auto& collision = *smgpc::scene::StageCollisionService::active();
                const auto first = collision.surface(parts, 0);
                const auto second = collision.surface(parts, 1);
                require(first && second && !collision.surface(first->triangle_index) && !collision.surface(second->triangle_index),
                        "Switch-off placement retains diagnostic geometry but no enabled native surface");
                const auto point = first->vertices[0] * 0.2F + first->vertices[1] * 0.3F + first->vertices[2] * 0.5F;
                const auto start = point + first->normals[0] * 100.0F;
                const auto offset = first->normals[0] * -200.0F;
                const OnlyPart only_area(parts);
                auto* keeper = MR::getCollisionDirector()->getCategoryKeeper(parts->mKeeperIndex);
                require(line_hits(*keeper, only_area, start, offset) == 0,
                        "Authored off switch excludes its actual parts from the original keeper query");
                retained_files.push_back(area->mPolygon->mKCLFile);
            }
            for (const auto& iter : placements) {
                s32 appear = -1;
                require(iter.getValue("SW_APPEAR", &appear) && appear == 1015 && MR::getPlacedZoneId(iter) == 5,
                        "Real-disc placement retains its authored appearance switch and zone");
            }
        }

        void exercise_polygon(const JMapInfoIter& iter) {
            auto& collision = *smgpc::scene::StageCollisionService::active();
            const auto domain = MR::getSceneObjHolder()->nativeAllocationDomain();
            require(domain != nullptr, "Actual original GameScene allocation domain exists");
            const PlacementZoneScope zone(MR::getPlacedZoneId(iter));
            const auto actors_before = smgpc::compat::actor_runtime_state_count();
            smgpc::scene::NameObjChildOwner objects;
            std::unique_ptr<AreaFormCube> form;
            AreaPolygon* polygon = nullptr;
            objects.capture_construction_children([&] {
                const smgpc::compat::JkrAllocationScope game(domain);
                form = std::make_unique<AreaFormCube>(0);
                form->init(iter);
                polygon = new AreaPolygon;
                polygon->mForm = form.get();
                polygon->init(iter);
            });
            auto* parts = polygon->mParts;
            auto* server = parts->mServer;
            auto* file = polygon->mKCLFile;
            auto* sensor = polygon->getSensor("body");
            require(parts->_CC && parts->mKeeperIndex == 0 && parts->mZone->mZoneID == MR::getPlacedZoneId(iter) &&
                        server->mFile == file && server->getTriangleNum() == 2 &&
                        sensor == parts->mHitSensor && sensor->mHost == polygon && sensor->mSensorGroup,
                    "AreaPolygon::init creates real sensor/group, CollisionParts and typed generated KCL");
            require(polygon->nativeCollisionParts().size() == 1,
                    "Actual actor owns exactly one generated collision part");
            const auto initial0 = collision.surface(parts, 0);
            const auto initial1 = collision.surface(parts, 1);
            require(initial0 && initial1, "Generated initialization publishes both original prisms");
            const auto id0 = initial0->triangle_index;
            const auto id1 = initial1->triangle_index;
            const auto zone_count = parts->mZone->mNumParts;
            auto* original_zone = parts->mZone;
            const OnlyPart only_polygon(parts);
            auto* keeper = MR::getCollisionDirector()->getCategoryKeeper(parts->mKeeperIndex);

            TPos3f world;
            form->calcWorldMtx(&world);
            TVec3f center;
            form->calcWorldPos(&center);
            std::array<TVec3f, 3> axes;
            world.getXDir(axes[0]);
            world.getYDir(axes[1]);
            world.getZDir(axes[2]);
            const std::array<float, 3> scales{form->mScale.x, form->mScale.y, form->mScale.z};
            std::array<float, 3> extents;
            for (std::size_t axis = 0; axis < axes.size(); ++axis) {
                const auto length = axes[axis].length();
                require(length > 0.0F && scales[axis] > 0.0F, "Authored cube has nondegenerate positive dimensions");
                axes[axis] *= 1.0F / length;
                extents[axis] = length * 0.5F * scales[axis] * form->getBaseSize();
                if (!MR::isPlayerElementModeTeresa()) extents[axis] += 10.0F;
            }

            TVec3f last_start, last_offset;
            for (s32 face = 0; face < 6; ++face) {
                {
                    const smgpc::compat::JkrAllocationScope game(domain);
                    polygon->setSurfaceAndSync(face);
                }
                require(polygon->mParts == parts && polygon->mKCLFile == file && parts->mServer == server,
                        "Face mutation preserves actual file, part and server identity");
                const auto first = collision.surface(parts, 0);
                const auto second = collision.surface(parts, 1);
                require(first && second && first->triangle_index == id0 && second->triangle_index == id1 &&
                            first->prism_index == 0 && second->prism_index == 1 && first->sensor == sensor,
                        "Six face mutations preserve local prisms, global IDs and the real sensor");
                const auto axis = static_cast<std::size_t>(face / 2);
                const auto sign = face % 2 == 0 ? 1.0F : -1.0F;
                const auto normal = axes[axis] * sign;
                near(first->normals[0], normal, "Regenerated face normal matches the authored world axis");
                for (const auto* surface : {&*first, &*second}) {
                    for (const auto& vertex : surface->vertices) {
                        const auto local = vertex - center;
                        near(local.dot(axes[axis]), sign * extents[axis], "All face vertices lie on the authored expanded plane");
                        for (std::size_t other = 0; other < axes.size(); ++other) {
                            if (other != axis)
                                near(std::abs(local.dot(axes[other])), extents[other], "Face corners retain both authored tangent extents");
                        }
                    }
                }
                // A strictly interior point avoids shared-diagonal and edge tolerances.
                const auto point = first->vertices[0] * 0.2F + first->vertices[1] * 0.3F + first->vertices[2] * 0.5F;
                last_start = point + normal * 100.0F;
                last_offset = normal * -200.0F;
                const auto hit_count = line_hits(*keeper, only_polygon, last_start, last_offset);
                require(hit_count > 0, "Actual keeper/parts/KCollision traversal reaches the generated face");
                for (s32 entry = 0; entry < hit_count; ++entry) {
                    const auto* hit = keeper->getStrikeInfo(entry);
                    require(hit->mParentTriangle.mParts == parts && hit->mParentTriangle.mIdx == 0 &&
                                hit->mParentTriangle.mSensor == sensor,
                            "Keeper results retain the actual original part, prism and sensor");
                    near(hit->_60, 100.0F, "Original regenerated arrow world distance");
                    near(hit->mHitPos, point, "Original regenerated arrow position");
                }
                // Retail preserves repeated encounters of the same leaf. Match
                // that ordered sequence instead of deduplicating its hits.
                TVec3f local_start, local_end;
                const auto world_end = last_start + last_offset;
                PSMTXMultVec(parts->mInvBaseMatrix, &last_start, &local_start);
                PSMTXMultVec(parts->mInvBaseMatrix, &world_end, &local_end);
                std::array<float, 8> fractions{};
                std::array<u8, 8> flags{};
                std::array<KC_PrismData*, 8> prisms{};
                u32 count = 0;
                server->checkArrow(local_start, local_end - local_start, fractions.data(), flags.data(),
                                   &count, prisms.data(), prisms.size());
                require(count == static_cast<u32>(hit_count), "Original keeper retains the server leaf encounter count");
                for (u32 encounter = 0; encounter < count; ++encounter) {
                    require(server->toIndex(prisms[encounter]) == 0, "Original encounter sequence retains selected local prism identity");
                    near(keeper->getStrikeInfo(encounter)->_60 / last_offset.length(), fractions[encounter],
                         "Original server fraction and keeper world distance agree");
                }
                std::fprintf(stderr, "[collision-area-probe] face=%d zone=%d prisms=2 ids=%u,%u encounters=%u fraction=%g\n",
                             face, parts->mZone->mZoneID, id0, id1, count, double(fractions[0]));
            }
            polygon->invalidate();
            require(!parts->_CC && original_zone->mNumParts + 1 == zone_count &&
                        !collision.surface(id0) && line_hits(*keeper, only_polygon, last_start, last_offset) == 0,
                    "Original invalidation removes zone membership and original query results");
            polygon->validate();
            require(parts->_CC && original_zone->mNumParts == zone_count && collision.surface(id0),
                    "Original validation restores the same membership and surface identity");
            objects.clear();
            require(!smgpc::compat::has_actor_runtime_state(polygon) &&
                        smgpc::compat::actor_runtime_state_count() == actors_before &&
                        original_zone->mNumParts + 1 == zone_count && !collision.surface(id0) &&
                        line_hits(*keeper, only_polygon, last_start, last_offset) == 0,
                    "Actor retirement removes real sensor and collision children, zone membership and original query results");
            require(smgpc::resource::is_native_kcollision_file(file),
                    "Scene collision cache retains generated arrays after the actor is retired");
            retained_files.push_back(file);
        }

        void after_frame(GameSystem& system, std::uint64_t frame) {
            if (exercised) return;
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            require(MR::isEqualStageName("HeavensDoorGalaxy") && MR::getMarioHolder()->getMarioActor(),
                    "Probe executes under the requested original GameScene and actual initialized Mario");
            const aurora::allocation::HostAllocationScope host;
            const auto placements = collision_area_placements();
            require(placements.size() == 2, "Actual stage holder contains exactly two authored CollisionArea placements");
            check_placements(placements);
            for (const auto& iter : placements) exercise_polygon(iter);
            exercised = true;
            std::fprintf(stderr, "[collision-area-probe] PASS two real placement forms, twelve face mutations, actor retirement; frame=%llu; factory_placements=%d\n",
                         static_cast<unsigned long long>(frame), expect_placements ? 2 : 0);
            std::fflush(stderr);
        }
    };
}
#endif

int main(int argc, char* argv[]) {
#ifdef NDEBUG
    std::fprintf(stderr, "This original-process diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
        const auto save = std::filesystem::temp_directory_path() /
                          ("petari-original-collision-area-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic must start with a fresh native console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        unsetenv("SMGPC_NAND_DIR");
        unsetenv("SMGPC_DEBUG_WPAD_BUTTON_SCRIPT");
        unsetenv("SMGPC_DEBUG_WPAD_POINTER_SCRIPT");
        unsetenv("SMGPC_DEBUG_WPAD_STICK_SCRIPT");
        std::fprintf(stderr, "[collision-area-probe] Fresh native console: %s\n", save.c_str());
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640,
            .window_height = 456,
            .window_title = "Original CollisionArea integration",
            .arguments = {"original-collision-area-test", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "360"},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct Disc { ~Disc() { smgpc::app::close_disc_image(); } } disc_lifetime;
        Probe probe;
        for (int index = 1; index < argc; ++index)
            if (std::strcmp(argv[index], "--probe-only") == 0) probe.expect_placements = false;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<Probe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.exercised,
                "Actual OriginalProcess completed the diagnostic and bounded normal frame loop");
        require(smgpc::scene::StageCollisionService::active() == nullptr,
                "Normal OriginalProcess teardown retires the actual scene collision owner");
        for (const auto* file : probe.retained_files)
            require(!smgpc::resource::is_native_kcollision_file(file),
                    "Normal scene-cache retirement releases every generated typed-resource identity");
        std::fprintf(stderr, "PASS original-process CollisionArea integration: twelve face mutations, real sensors/parts, original keeper/parts queries, actor and scene retirement, factory placements=%d\n",
                     probe.expect_placements ? 2 : 0);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process CollisionArea integration: %s\n", error.what());
        return 1;
    }
#endif
}
