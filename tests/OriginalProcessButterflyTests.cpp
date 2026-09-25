#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/HitSensorInfo.hpp"
#include "Game/LiveActor/HitSensorKeeper.hpp"
#include "Game/Map/Butterfly.hpp"
#include "Game/MapObj/StarPiece.hpp"
#include "Game/MapObj/StarPieceDirector.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/Screen/StarPointerTarget.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/NameObj/NameObj.hpp"

#include <aurora/allocation.hpp>
#include <array>
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <unistd.h>
#include <vector>

#ifndef NDEBUG
namespace {
    void require(bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    }

    std::size_t original_placement_count() {
        auto* root = MR::getStageDataHolder();
        require(root != nullptr, "Actual original stage data exists");
        std::size_t count = 0;
        for (s32 zone = 0; zone < MR::getZoneNum(); ++zone) {
            auto* holder = root->getStageDataHolderFromZoneId(zone);
            if (!holder) continue;
            for (const auto& table : holder->mPlacementObjs)
                for (s32 row = 0; row < table.getNumEntries(); ++row) {
                    const JMapInfoIter iter(&table, row);
                    const char* name = nullptr;
                    if (!MR::getObjectName(&name, iter) || std::strcmp(name, "Butterfly")) continue;
                    s32 star_piece_argument = -1;
                    MR::getJMapInfoArg1NoInit(iter, &star_piece_argument);
                    require(star_piece_argument == -1, "All three real authored placements declare a Star Piece");
                    ++count;
                }
        }
        return count;
    }

    struct Probe {
        std::uint64_t ready_frame = 0;
        std::size_t samples = 0;
        std::size_t pointer_distance_samples = 0;
        std::vector<Butterfly*> actors;
        const NameObj* director_identity = nullptr;
        std::vector<const NameObj*> star_piece_identities;

        void discover() {
            require(original_placement_count() == 3, "Actual Gateway scenario has three Butterfly placements");
            for (auto* object : NameObj::snapshotNativeObjects())
                if (auto* actor = dynamic_cast<Butterfly*>(object)) actors.push_back(actor);
            require(actors.size() == 3, "Ordinary original placement phase creates all three Butterfly actors");
            auto* director = MR::getStarPieceDirector();
            require(director && director->getObjNum() == 70, "Actual original scene owns its seventy Star Piece pool actors");
            director_identity = director;
            for (s32 i = 0; i < director->getObjNum(); ++i) {
                auto* piece = dynamic_cast<StarPiece*>(director->getActor(i));
                require(piece && piece->mModelManager && piece->mSpine, "Original Star Piece pool has real initialized actors");
                star_piece_identities.push_back(piece);
            }
        }

        void sample() {
            auto* director = MR::getStarPieceDirector();
            require(director == director_identity, "Original Star Piece owner remains the same during the scene");
            for (auto* actor : actors) {
                require(actor->mModelManager && actor->mBinder && actor->mStarPointerTarget && actor->mSpine && actor->mSensorKeeper && !actor->mFlag.mIsDead,
                        "Canonical Butterfly init owns its real model, Binder, pointer target and nerve");
                auto* body = actor->getSensor("body");
                auto* sensor_info = actor->mSensorKeeper->getSensorInfo("body");
                const auto joint = MR::getJointMtx(actor, "buttBody");
                require(body && body->mHost == actor && body->mSensorGroup && sensor_info && sensor_info->_1C == joint,
                        "Original animal sensor borrows the actual animated buttBody matrix and belongs to a real sensor group");
                auto* host = director->findHostInfo(actor);
                require(host && host->mObj == actor && host->_4 == 1 && host->_8 >= 0 && host->_C >= 0 && host->_8 + host->_C <= 1,
                        "Actual Star Piece director retains each original one-piece declaration and bounded outstanding count");
                const auto base = actor->getBaseMtx();
                require(joint && base, "Original model supplies joint and quaternion-derived base matrices");
                for (unsigned row = 0; row < 3; ++row)
                    for (unsigned column = 0; column < 4; ++column)
                        require(std::isfinite(joint[row][column]) && std::isfinite(base[row][column]),
                                "Original model matrices remain finite during normal scheduled frames");

                // Original unprojection quantizes its half-angle through the
                // 14-bit JMath table. SDK projection uses ordinary tangent, so
                // these operations do not form an exact screen round trip.
                const f32 depth = MR::calcCameraDistanceZ(actor->mPosition);
                if (depth > 1.0F) {
                    TVec3f world;
                    MR::calcStarPointerWorldPointingPos(&world, actor->mPosition, 0);
                    const auto* pointer = MR::getStarPointerScreenPosition(0);
                    const double width = MR::getScreenWidth(), height = MR::getScreenHeight();
                    const f32 half_angle = (MR::getFovy() * 0.017453292F) * 0.5F;
                    const auto index = static_cast<unsigned>(half_angle * (16384.0F / 6.2831855F)) & 16383U;
                    const double quantized_angle = index * 6.2831854820251465 / 16384.0;
                    const double tangent = std::sin(quantized_angle) / std::cos(quantized_angle);
                    const double focal = height * 0.5 / tangent;
                    const std::array<double, 4> view{
                        (pointer->x - width * 0.5) * depth / focal,
                        -(pointer->y - height * 0.5) * depth / focal, -double(depth), 1.0};
                    const auto& inverse = MR::getCameraInvViewMtx().mMtx;
                    const std::array<double, 3> actual_world{world.x, world.y, world.z};
                    for (unsigned row = 0; row < 3; ++row) {
                        double expected = 0.0, magnitude = 0.0;
                        for (unsigned column = 0; column < 4; ++column) {
                            const double term = inverse[row][column] * view[column];
                            expected += term;
                            magnitude += std::abs(term);
                        }
                        // Covers rounded table entries, float focal arithmetic
                        // and the four-term native matrix product; no world-unit
                        // tolerance changes with the actor's identity.
                        const double bound = 32.0 * std::numeric_limits<float>::epsilon() * std::max(1.0, magnitude);
                        require(std::isfinite(actual_world[row]) && std::abs(actual_world[row] - expected) <= bound,
                                "Original pointer query follows quantized pinhole geometry under the actual inverse camera matrix");
                    }
                    const auto& projection = MR::getCameraProjectionMtx().mMtx;
                    std::array<double, 4> clip{};
                    for (unsigned row = 0; row < 4; ++row)
                        for (unsigned column = 0; column < 4; ++column)
                            clip[row] += projection[row][column] * view[column];
                    const double expected_x = width * 0.5 * (1.0 + clip[0] / clip[3]);
                    const double expected_y = height * 0.5 * (1.0 - clip[1] / clip[3]);
                    TVec2f screen;
                    const bool screen_valid = MR::calcScreenPosition(&screen, world);
                    require(screen_valid && std::abs(screen.x - expected_x) < 0.05 && std::abs(screen.y - expected_y) < 0.05,
                            "Actual projection agrees with independent quantized-ray geometry in its visible screen domain");
                }
                f32 distance = -1.0F;
                if (MR::calcStarPointerScreenDistanceToTarget(actor, &distance, 0)) {
                    TVec2f target;
                    require(actor->mStarPointerTarget->calcScreenPosition(&target), "Successful target query has actual projected geometry");
                    const auto* pointer = MR::getStarPointerScreenPosition(0);
                    const double expected = std::hypot(double(target.x) - pointer->x, double(target.y) - pointer->y);
                    require(std::abs(double(distance) - expected) < 0.01,
                            "Shared distance query measures actual pointer-to-target screen geometry");
                    ++pointer_distance_samples;
                } else {
                    require(distance == -1.0F, "Rejected original target query preserves caller output");
                }
            }
            ++samples;
        }

        void after_frame(GameSystem& system, std::uint64_t frame) {
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            if (!ready_frame) ready_frame = frame;
            if (frame < ready_frame + 20) return;
            const aurora::allocation::HostAllocationScope host;
            if (actors.empty()) {
                discover();
                std::fprintf(stderr, "[butterfly-probe] all three real factory placements and actual Star Piece pool found; frame=%llu\n",
                             static_cast<unsigned long long>(frame));
            }
            sample();
        }
    };
}
#endif

int main() {
#ifdef NDEBUG
    std::fprintf(stderr, "This original-process diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
        const auto save = std::filesystem::temp_directory_path() / ("petari-original-butterfly-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic starts with a fresh native console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        setenv("SMGPC_DEBUG_WPAD_POINTER_SCRIPT", "0-119:260,180;120-239:400,250;240-:320,220", 1);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original Butterfly integration",
            .arguments = {"original-butterfly-test", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "360"},
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
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.samples >= 250,
                "OriginalProcess completes all ordinary frames and at least 250 observations");
        require(!NameObj::nativeGeneration(probe.director_identity), "Normal scene retirement removes actual Star Piece director");
        for (const auto* actor : probe.actors)
            require(!NameObj::nativeGeneration(actor), "Normal original scene teardown retires all three Butterfly actors");
        for (const auto* piece : probe.star_piece_identities)
            require(!NameObj::nativeGeneration(piece), "Normal original scene teardown retires all original pool actors");
        std::fprintf(stderr, "PASS original-process Butterfly: three ordinary placements, original sensors/models/Star Piece ownership, samples=%zu, successful pointer distances=%zu, full retirement\n",
                     probe.samples, probe.pointer_distance_samples);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process Butterfly: %s\n", error.what());
        return 1;
    }
#endif
}
