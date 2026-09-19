#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/RestartCube.hpp"
#include "Game/LiveActor/LiveActorGroup.hpp"
#include "Game/MapObj/WarpPod.hpp"
#include "Game/NameObj/NameObjFinder.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/System/GameSequenceDirector.hpp"
#include "Game/System/GameDataTemporaryInGalaxy.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/Cp932Literal.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "scene/OriginalPlacementCoverage.hpp"
#include "scene/StagePlacementResolver.hpp"
#include <aurora/allocation.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <unistd.h>
#include <vector>

#ifndef NDEBUG
namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

using Matrix = std::array<std::array<double, 3>, 3>;

Matrix rotation(const TVec3f& degrees) {
    constexpr double rad = 3.14159265358979323846 / 180.0;
    const double sx = std::sin(degrees.x * rad), cx = std::cos(degrees.x * rad);
    const double sy = std::sin(degrees.y * rad), cy = std::cos(degrees.y * rad);
    const double sz = std::sin(degrees.z * rad), cz = std::cos(degrees.z * rad);
    return {{{cy * cz, sx * sy * cz - cx * sz, cx * sy * cz + sx * sz},
             {cy * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz},
             {-sy, sx * cy, cx * cy}}};
}

TVec3f transformed(const Mtx& matrix, const TVec3f& local) {
    TVec3f result;
    const std::array<float*, 3> components{&result.x, &result.y, &result.z};
    for (unsigned row = 0; row < 3; ++row)
        *components[row] = static_cast<float>(matrix[row][0] * double(local.x) +
            matrix[row][1] * double(local.y) + matrix[row][2] * double(local.z) + matrix[row][3]);
    return result;
}

void require_position(const TVec3f& actual, const TVec3f& expected, const char* message) {
    if (std::abs(actual.x - expected.x) > 0.015f || std::abs(actual.y - expected.y) > 0.015f ||
        std::abs(actual.z - expected.z) > 0.015f) {
        std::fprintf(stderr, "[placement-transform] actual=(%.9g,%.9g,%.9g) expected=(%.9g,%.9g,%.9g)\n",
                     actual.x, actual.y, actual.z, expected.x, expected.y, expected.z);
        throw std::runtime_error(message);
    }
}

void require_rotation(const TVec3f& actual, const TVec3f& local, const Mtx& zone) {
    const auto got = rotation(actual);
    const auto source = rotation(local);
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned column = 0; column < 3; ++column) {
            double expected = 0;
            for (unsigned k = 0; k < 3; ++k) expected += zone[row][k] * source[k][column];
            require(std::abs(got[row][column] - expected) < 0.004,
                    "Original rotation getter composes the owning zone and local orientations");
        }
}

struct Probe {
    bool exercised = false;
    unsigned positions = 0, rotations = 0, rail_points = 0, areas = 0;
    std::vector<NameObj*> retained;

    void verify(StageDataHolder& root, GameSystem& system) {
        require(MR::isPlacementLocalStage(), "Actual original stage contains child holders");
        auto* child = root.getStageDataHolderFromZoneId(5);
        require(child && std::abs(child->mPlacementMtx[0][3]) > 10000,
                "Actual authored MysteriousZone placement has a nonidentity transform");
        std::fprintf(stderr, "[placement-transform] zone=5 translation=(%.9g,%.9g,%.9g)\n",
                     child->mPlacementMtx[0][3], child->mPlacementMtx[1][3], child->mPlacementMtx[2][3]);
        // This report must identify original retained rows even though their
        // JMapInfo native-copy zone metadata is deliberately unpopulated.
        const auto queues = std::array{
            smgpc::scene::OriginalPlacementQueue{"common_priority", root._FC},
            smgpc::scene::OriginalPlacementQueue{"scenario_priority", root._104},
            smgpc::scene::OriginalPlacementQueue{"common", root._100},
            smgpc::scene::OriginalPlacementQueue{"scenario", root._108},
            smgpc::scene::OriginalPlacementQueue{"deferred", root._10C},
        };
        const auto start = root.makeCurrentMarioJMapInfoIter();
        require(start.isValid() && start.mInfo->getPlacedZoneId() == -1,
                "Original start has no synthetic-copy zone metadata");
        const auto coverage = smgpc::scene::inspect_original_placement_queues(queues, start, &root);
        std::set<std::string> identities;
        std::set<s32> zones;
        bool player_seen = false, rosetta_seen = false;
        for (const auto& entry : coverage) {
            const auto* owner = &root;
            for (const auto index : entry.holder_path) {
                require(index >= 0 && index < owner->mStageDataHolderCount,
                        "Coverage reports an attached holder path");
                owner = owner->mStageDataArray[index];
            }
            require(owner->mZoneID == entry.zone && owner->_A8 == entry.zone_name,
                    "Coverage provenance agrees with the actual attached owner");
            require(entry.table_path.starts_with("/jmp/") && entry.table_path.ends_with('/' + entry.table) &&
                        (entry.layer == "common" || entry.layer == "layera"),
                    "Coverage keeps full archive path and actual scenario-one layer");
            for (unsigned row = 0; row < 3; ++row)
                for (unsigned column = 0; column < 4; ++column)
                    require(entry.zone_placement_matrix[row * 4 + column] == owner->mPlacementMtx[row][column],
                            "Coverage matrix is copied from the original owner");
            const auto identity = std::to_string(entry.zone) + ':' + entry.table_path + ':' + std::to_string(entry.row);
            require(identities.insert(identity).second, "Coverage includes each retained placement row once");
            zones.insert(entry.zone);
            if (entry.phase == "player") {
                require(entry.zone == 0 && entry.holder_path.empty() &&
                            entry.table_path == "/jmp/start/layera/startinfo",
                        "Default Mario start resolves to original root-zone LayerA");
                player_seen = true;
            }
            if (entry.object == "Rosetta") {
                require(entry.zone == 5 && entry.layer == "layera" && entry.link_id == 28,
                        "Rosetta retains her authored zone, layer and link ID");
                rosetta_seen = true;
            }
        }
        require(coverage.size() == 243 && zones == std::set<s32>{0, 1, 2, 4, 5, 6} && player_seen && rosetta_seen,
                "Actual queues contain all attached scenario-one zones and exclude unattached LargeZone");
        std::fprintf(stderr, "[placement-transform] coverage provenance PASS rows=%zu attached_zones=%zu\n",
                     coverage.size(), zones.size());
        auto* restarts = MR::getAreaObjContainer()->getManager("RestartCube");
        require(restarts && restarts->getNumAreaObj() == 4,
                "All four authored RestartCube placements use the original manager");
        auto* temporary = system.mSequenceDirector->mGameDataTemporaryInGalaxy;
        require(temporary && temporary->mPlayerRestartIdInfo &&
                    MR::getPlayerRestartIdInfo() == temporary->mPlayerRestartIdInfo,
                "Restart dispatch uses the actual original GameSequenceDirector owner");
        const JMapIdInfo saved_restart(*temporary->mPlayerRestartIdInfo);
        unsigned restart_dispatches = 0;
        {
            struct RestoreRestart {
                GameDataTemporaryInGalaxy& owner;
                JMapIdInfo saved;
                ~RestoreRestart() { owner.setPlayerRestartIdInfo(saved); }
            } restore{*temporary, saved_restart};
            for (const auto& table : root.mPlacementObjs) {
                for (s32 row = 0; row < table.getNumEntries(); ++row) {
                    const JMapInfoIter iter(&table, row);
                    const char* name = nullptr;
                    if (!MR::getObjectName(&name, iter) || std::strcmp(name, "RestartCube") != 0) continue;
                    s32 start_id = -1;
                    require(MR::getJMapInfoArg0NoInit(iter, &start_id), "Authored restart target is readable");
                    const JMapIdInfo authored_id(start_id, iter);
                    RestartCube* cube = nullptr;
                    for (s32 i = 0; i < restarts->getNumAreaObj(); ++i) {
                        auto* candidate = dynamic_cast<RestartCube*>(restarts->getAreaObj(i));
                        require(candidate && candidate->mIdInfo, "Manager retains actual original RestartCube controllers");
                        if (*candidate->mIdInfo == authored_id) {
                            require(!cube, "Authored restart ID identifies one original area");
                            cube = candidate;
                        }
                    }
                    require(cube && cube->mFormType == AreaForm::Type_Cube2 && cube->mObjArg3 == -1,
                            "Restart placement keeps its original base-origin form and authored ground policy");
                    auto* form = cube->getForm<AreaFormCube>();
                    TVec3f authored_position;
                    require(MR::getJMapInfoTrans(iter, &authored_position), "Authored restart position is readable");
                    require_position(form->mTranslation, authored_position, "Restart controller keeps original placement coordinates");
                    TPos3f world;
                    form->calcWorldMtx(&world);
                    TVec3f interior(0, form->mScale.y * 500.0f, 0);
                    world.mult(interior, interior);
                    require(MR::getAreaObj("RestartCube", interior) == cube,
                            "Original area query selects the actual authored restart controller");
                    const bool old_bgm_latch = cube->_48;
                    MR::tryToUpdatePlayerRestartIdInfo(interior);
                    require(*temporary->mPlayerRestartIdInfo == authored_id,
                            "Original area dispatch writes the target ID and zone to real restart state");
                    cube->_48 = old_bgm_latch;
                    const JMapIdInfo after_dispatch(*temporary->mPlayerRestartIdInfo);
                    MR::tryToUpdatePlayerRestartIdInfo(TVec3f(1.0e8f, 1.0e8f, 1.0e8f));
                    require(*temporary->mPlayerRestartIdInfo == after_dispatch,
                            "A real empty-volume query leaves restart state unchanged");
                    retained.push_back(cube);
                    ++restart_dispatches;
                }
            }
        }
        require(restart_dispatches == 4 && *temporary->mPlayerRestartIdInfo == saved_restart,
                "All four original restart dispatches were verified and prior scene state restored");
        std::fprintf(stderr, "[placement-transform] restart dispatch PASS authored_areas=%u owner=GameSequenceDirector restored=true\n",
                     restart_dispatches);
        std::vector<TVec3f> warp_positions;
        auto* area_manager = MR::getAreaObjContainer()->getManager("SwitchArea");
        require(area_manager, "Actual switch-area manager exists");
        for (const auto& table : child->mPlacementObjs) {
            for (s32 row = 0; row < table.getNumEntries(); ++row) {
                const JMapInfoIter iter(&table, row);
                TVec3f local, local_rotation, actual;
                if (!MR::getJMapInfoTransLocal(iter, &local)) continue;
                require(root.findPlacedStageDataHolder(iter) == child &&
                            MR::getZonePlacementMtx(iter) == reinterpret_cast<TPos3f*>(child->mPlacementMtx),
                        "Raw archive row resolves to its actual StageDataHolder matrix");
                const auto expected = transformed(child->mPlacementMtx, local);
                require(MR::getJMapInfoTrans(iter, &actual), "Authored translation is readable");
                require_position(actual, expected, "Original placement getter applies the owning zone exactly once");
                ++positions;
                if (MR::getJMapInfoRotateLocal(iter, &local_rotation)) {
                    require(MR::getJMapInfoRotate(iter, &actual), "Authored rotation is readable");
                    require_rotation(actual, local_rotation, child->mPlacementMtx);
                    ++rotations;
                }
                const char* name = nullptr;
                if (!MR::getObjectName(&name, iter)) continue;
                if (std::strcmp(name, "WarpPod") == 0) warp_positions.push_back(expected);
                if (std::strcmp(name, "SwitchCube") != 0) continue;
                AreaFormCube* found = nullptr;
                for (s32 i = 0; i < area_manager->getNumAreaObj(); ++i) {
                    auto* area = area_manager->getAreaObj(i);
                    if (area->mFormType != AreaForm::Type_Cube2) continue;
                    auto* form = area->getForm<AreaFormCube>();
                    if (std::abs(form->mTranslation.x - expected.x) < 0.015f &&
                        std::abs(form->mTranslation.y - expected.y) < 0.015f &&
                        std::abs(form->mTranslation.z - expected.z) < 0.015f) {
                        require(!found, "Authored switch cube identifies one original form");
                        found = form;
                        retained.push_back(area);
                    }
                }
                require(found, "Factory-created switch cube consumes the world-space placement");
                require_rotation(found->mRotation, local_rotation, child->mPlacementMtx);
                TPos3f world;
                found->calcWorldMtx(&world);
                TVec3f inside(0.0f, found->mScale.y * 500.0f, 0.0f);
                world.mult(inside, inside);
                require(found->isInVolume(inside), "Actual oriented base-origin cube contains its geometric center");
                TVec3f outside(found->mScale.x * 600.0f, found->mScale.y * 500.0f, 0.0f);
                world.mult(outside, outside);
                require(!found->isInVolume(outside), "Actual oriented cube rejects a point past its side");
                ++areas;
            }
        }
        using Getter = void (*)(const JMapInfoIter&, TVec3f*);
        const std::array<Getter, 3> getters{MR::getRailPointPos0, MR::getRailPointPos1, MR::getRailPointPos2};
        for (const auto& table : child->mPathObjs) {
            for (s32 row = 0; row < table.getNumEntries(); ++row) {
                const JMapInfoIter iter(&table, row);
                for (unsigned point = 0; point < getters.size(); ++point) {
                    TVec3f local;
                    const std::array<float*, 3> components{&local.x, &local.y, &local.z};
                    bool present = true;
                    for (unsigned component = 0; component < 3; ++component) {
                        char key[16];
                        std::snprintf(key, sizeof(key), "pnt%u_%c", point, "xyz"[component]);
                        present &= iter.getValue(key, components[component]);
                    }
                    if (!present) continue;
                    require(root.findPlacedStageDataHolder(iter) == child,
                            "Authored rail point retains the same original zone owner");
                    TVec3f actual;
                    getters[point](iter, &actual);
                    require_position(actual, transformed(child->mPlacementMtx, local),
                                     "Original rail control point uses the same world space as actors");
                    ++rail_points;
                }
            }
        }
        auto* group = dynamic_cast<LiveActorGroup*>(NameObjFinder::find(CP932("ワープポッド群")));
        require(group && group->getObjNum() == 2 && warp_positions.size() == 2,
                "Both actual stationary WarpPods exist in their original group");
        for (s32 i = 0; i < group->getObjNum(); ++i) {
            auto* actor = dynamic_cast<WarpPod*>(group->getActor(i));
            require(actor, "Actual group member has the original actor type");
            unsigned matches = 0;
            for (const auto& expected : warp_positions)
                matches += std::abs(actor->mPosition.x - expected.x) < 0.015f &&
                           std::abs(actor->mPosition.y - expected.y) < 0.015f &&
                           std::abs(actor->mPosition.z - expected.z) < 0.015f;
            require(matches == 1, "Actual actor placement agrees with its transformed archive row");
            retained.push_back(actor);
        }
        require(positions > 20 && rotations > 20 && rail_points > 9 && areas == 2,
                "Actual nonidentity placement, rotation, all rail controls and both switch cubes were exercised");

        // Inventory tooling retains raw JMap rows too. Its separate world-space
        // metadata must stay useful without pretransforming inputs to Game.
        auto* archives = smgpc::runtime::ArchiveMountService::active();
        require(archives, "Actual process retains its archive filesystem");
        const auto tables = smgpc::scene::resolve_stage_placement_tables(archives->dvd(), "HeavensDoorGalaxy", 1);
        const auto objects = smgpc::scene::resolve_stage_placement_objects(archives->dvd(), tables);
        unsigned checked_inventory = 0, checked_starts = 0;
        for (const auto& object : objects) {
            if (object.zone_id != 5 || !object.has_translation) continue;
            TVec3f raw;
            require(MR::getJMapInfoTransLocal(JMapInfoIter(&object.jmap_info, object.jmap_entry_index), &raw),
                    "Inventory keeps raw placement fields");
            const auto world = transformed(child->mPlacementMtx, raw);
            require_position(TVec3f(object.translation[0], object.translation[1], object.translation[2]), world,
                             "Inventory world metadata transforms its retained local JMap row once");
            ++checked_inventory;
        }
        for (const auto& table : tables) {
            if (table.category != "start" || table.zone_id != 5) continue;
            for (s32 row = 0; row < table.jmap_info.getNumEntries(); ++row) {
                const JMapInfoIter iter(&table.jmap_info, row);
                s32 start_id = -1;
                if (!iter.getValue("MarioNo", &start_id)) continue;
                const auto start = smgpc::scene::select_stage_start_info(tables, start_id, 5);
                require(start.has_value(), "Authored child-zone start remains selectable");
                TVec3f raw;
                require(MR::getJMapInfoTransLocal(start->iter(), &raw), "Start descriptor retains raw local JMap fields");
                require_position(raw, TVec3f(start->local_position[0], start->local_position[1], start->local_position[2]),
                                 "Start JMap does not contain a baked world position");
                require_position(TVec3f(start->world_position[0], start->world_position[1], start->world_position[2]),
                                 transformed(child->mPlacementMtx, raw), "Start world metadata remains transformed once");
                ++checked_starts;
            }
        }
        require(checked_inventory > 20 && checked_starts > 0,
                "Native inventory and start copies were checked for double-transform regressions");
    }

    void after_frame(GameSystem& system, std::uint64_t frame) {
        if (exercised) return;
        auto* controller = system.mSceneController;
        if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
            controller->getCurrentSceneForExecute() != controller->mScene ||
            !dynamic_cast<GameScene*>(controller->mScene)) return;
        const aurora::allocation::HostAllocationScope host;
        verify(*MR::getStageDataHolder(), system);
        exercised = true;
        std::fprintf(stderr, "[placement-transform] PASS frame=%llu positions=%u rotations=%u rail_points=%u areas=%u\n",
                     static_cast<unsigned long long>(frame), positions, rotations, rail_points, areas);
    }
};
}
#endif

int main() {
#ifdef NDEBUG
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the actual disc image");
        const auto save = std::filesystem::temp_directory_path() / ("petari-placement-transform-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic starts with fresh console settings");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original zone placement integration",
            .arguments = {"placement-transform-test", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct Disc { ~Disc() { smgpc::app::close_disc_image(); } } disc_lifetime;
        Probe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<Probe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.exercised,
                "Actual process completes the placement probe and normal frame loop");
        for (auto* object : probe.retained)
            require(!smgpc::compat::has_name_obj_runtime_state(object),
                    "Normal original scene retirement releases checked actor and area identities");
        std::fprintf(stderr, "PASS original-process zone transforms: actual raw rows, matrices, actors, switch volumes, rail controls and retirement\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process zone transforms: %s\n", error.what());
        return 1;
    }
#endif
}
