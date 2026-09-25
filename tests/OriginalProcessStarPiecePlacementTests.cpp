#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "NativeHeapFixture.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/SceneScheduler.hpp"
#include "Game/Scene/SceneObjHolder.hpp"

#include <aurora/allocation.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <unistd.h>
#include <vector>

#include "Game/LiveActor/DisplayListMaker.hpp"
#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/MapObj/StarPiece.hpp"
#include "Game/MapObj/StarPieceDirector.hpp"
#include "Game/MapObj/StarPieceGroup.hpp"

#ifndef NDEBUG
namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

std::vector<JMapInfoIter> original_placements() {
    auto* root = MR::getStageDataHolder();
    require(root != nullptr, "Actual original stage data exists");
    std::vector<JMapInfoIter> result;
    for (s32 zone = 0; zone < MR::getZoneNum(); ++zone) {
        auto* holder = root->getStageDataHolderFromZoneId(zone);
        if (!holder) continue;
        for (const auto& table : holder->mPlacementObjs)
            for (s32 row = 0; row < table.getNumEntries(); ++row) {
                const JMapInfoIter iter(&table, row);
                const char* name = nullptr;
                if (MR::getObjectName(&name, iter) && std::strcmp(name, "StarPiece") == 0)
                    result.push_back(iter);
            }
    }
    return result;
}

struct Probe {
    bool exercised = false;
    std::uint64_t ready_frame = 0;
    std::vector<const NameObj*> retained_identities;

    void exercise() {
        require(MR::getSceneObjHolder()->nativeAllocationHeap() != nullptr, "Actual scene allocation domain exists");
        const auto placements = original_placements();
        require(placements.size() == 7, "Actual active Gateway stage data has seven standalone StarPiece placements");
        auto* director = MR::getStarPieceDirector();
        require(director && director->getObjNum() == 70, "Original director retains exactly its seventy reusable pool members");
        std::vector<StarPiece*> pooled, grouped, placed, all;
        for (s32 i = 0; i < director->getObjNum(); ++i) {
            auto* piece = dynamic_cast<StarPiece*>(director->getActor(i));
            require(piece != nullptr, "Every original pool member is a real StarPiece");
            pooled.push_back(piece);
        }
        const auto objects = NameObj::snapshotNativeObjects();
        for (auto* object : objects)
            if (auto* group = dynamic_cast<StarPieceGroup*>(object))
                for (s32 i = 0; i < group->mNumPieces; ++i) grouped.push_back(group->mPieces[i]);
        for (auto* object : objects) {
            auto* piece = dynamic_cast<StarPiece*>(object);
            if (!piece) continue;
            all.push_back(piece);
            if (std::ranges::find(pooled, piece) == pooled.end() && std::ranges::find(grouped, piece) == grouped.end())
                placed.push_back(piece);
        }
        require(placed.size() == 7 && director->mNumStarPieceNewed == all.size(),
                "Seven ordinary placements are separate from pool and group children; original total-newed count covers all actors");
        std::vector<TVec3f> remaining_positions;
        for (const auto& iter : placements) {
            TVec3f position;
            require(MR::getJMapInfoTrans(iter, &position), "Original placement exposes its authored translation");
            remaining_positions.push_back(position);
        }
        for (auto* piece : placed) {
            const auto position = std::ranges::find_if(remaining_positions, [&](const auto& value) {
                return value.x == piece->mPosition.x && value.y == piece->mPosition.y && value.z == piece->mPosition.z;
            });
            require(position != remaining_positions.end(), "Each ordinary actor retains a distinct actual placement translation");
            remaining_positions.erase(position);
            require(piece->mGroupType == StarPiece::groupType_noGroup && !piece->mFlags.isGroup &&
                        !piece->mFlag.mIsDead && piece->isFloat() && !piece->mHostInfo && !piece->mReceiverInfo &&
                        piece->mStageSwitchCtrl == nullptr,
                    "Valid-iterator initialization preserves appeared standalone Floating ownership without invented pool/switch bindings");
            require(piece->mModelManager && piece->mModelManager->getJ3DModel() && piece->mModelManager->mBtkPlayer &&
                        piece->mBinder && piece->mSpine && piece->mStarPointerTarget && piece->mDelegator,
                    "Each placed StarPiece owns real model/Gift animation, Binder, nerve, pointer target and triangle filter");
            for (const auto* name : {"attack", "body"}) {
                auto* sensor = piece->getSensor(name);
                require(sensor && sensor->mHost == piece && sensor->mSensorGroup && sensor->mRadius == 30.0f,
                        "Original placed-piece sensors retain their actual host/group/radius");
            }
            require(piece->mShadowControllerList && piece->mShadowControllerList->getControllerCount() == 1,
                    "Each placed actor owns its original shadow controller");
            auto* maker = piece->mModelManager->mDisplayListMaker;
            require(maker != nullptr, "Original actor owns its differential material controller");
            unsigned bindings = 0;
            for (auto* controller : maker->mMaterialCtrl)
                if (auto* color = dynamic_cast<MatColorCtrl*>(controller))
                    bindings += color->mColorChoice == 0 && color->mColor == &piece->mColor;
            require(bindings == 1, "Real material controller borrows exactly this placed actor's color storage");
            require(piece->mScale.x == 1.0f && piece->mScale.y == 1.0f && piece->mScale.z == 1.0f &&
                        !piece->mFlags._2 && !piece->mFlags._3,
                    "The seven default argument rows use original unit scale and default shadow flags");
            retained_identities.push_back(piece);
        }
        require(remaining_positions.empty(), "All authored positions are represented exactly once");
        retained_identities.push_back(director);
    }

    void after_frame(GameSystem& system, std::uint64_t frame) {
        if (exercised) return;
        auto* controller = system.mSceneController;
        if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
            controller->getCurrentSceneForExecute() != controller->mScene || !dynamic_cast<GameScene*>(controller->mScene)) return;
        if (!ready_frame) ready_frame = frame;
        if (frame < ready_frame + 20) return;
        const aurora::allocation::HostAllocationScope host;
        exercise();
        exercised = true;
        std::fprintf(stderr, "[star-piece-placement-probe] PASS seven distinct real factory placements, separate from original seventy-piece pool and group children; frame=%llu\n",
                     static_cast<unsigned long long>(frame));
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
        const auto save = std::filesystem::temp_directory_path() / ("petari-original-star-piece-placement-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic starts with a fresh native console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original placed StarPiece integration",
            .arguments = {"original-star-piece-placement-test", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
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
                "OriginalProcess completes the read-only placement diagnostic and normal bounded frame loop");
        for (const auto* object : probe.retained_identities)
            require(!NameObj::nativeGeneration(object), "Normal original scene teardown retires every standalone piece and the director");
        std::fprintf(stderr, "PASS original-process StarPiece placement: seven original non-pool placements, actual resource owners and normal scene retirement\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process StarPiece placement: %s\n", error.what());
        return 1;
    }
#endif
}
