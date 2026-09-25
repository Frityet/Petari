#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/MapObj/CrystalCage.hpp"
#include "Game/MapObj/DummyDisplayModel.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
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
                if (MR::getObjectName(&name, iter) && std::strcmp(name, "CrystalCageM") == 0)
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
        require(MR::getSceneObjHolder()->nativeAllocationDomain() != nullptr, "Actual original scene allocation domain exists");
        const auto placements = original_placements();
        std::vector<CrystalCage*> actors;
        for (auto* object : smgpc::compat::snapshot_name_obj_runtime_objects())
            if (auto* actor = dynamic_cast<CrystalCage*>(object)) actors.push_back(actor);
        require(placements.size() == 3 && actors.size() == 3,
                "Actual Gateway data and ordinary factory create all three authored medium cages");
        unsigned authored_models = 0;
        for (const auto& iter : placements) {
            const s32 model_id = MR::getDummyDisplayModelId(iter, -1);
            NameObjArchiveListCollector archives;
            NameObjFactory::getMountObjectArchiveList(&archives, "CrystalCageM", iter);
            bool has_cage = false, has_break = false, has_dummy = false;
            for (s32 index = 0; index < archives.getArchiveNum(); ++index) {
                const char* name = archives.getArchive(index);
                has_cage |= std::strcmp(name, "CrystalCageM") == 0;
                has_break |= std::strcmp(name, "CrystalCageSBreak") == 0;
                has_dummy |= std::strcmp(name, "SuperSpinDriver") == 0;
            }
            require(has_cage && has_break && has_dummy == (model_id == 3),
                    "Original archive callback selects the dummy model from each real placement's argument 7");
            authored_models += model_id == 3;
        }
        require(authored_models == 1, "Exactly one authored cage requests the SuperSpinDriver display model");
        auto* scheduler = smgpc::runtime::try_active_scene_scheduler();
        require(scheduler != nullptr, "Actual scene scheduler exists");
        const auto scheduled = scheduler->snapshot();
        unsigned display_models = 0, linked_break_listeners = 0;
        for (auto* actor : actors) {
            require(actor->mCrystalCageType == 1 && actor->mModelManager && actor->mSpine &&
                        actor->mModelManager->getJ3DModel() && actor->mModelManager->mBvaPlayer,
                    "Each medium cage owns its original model, BVA and nerve");
            auto* sensor = actor->getSensor("body");
            require(sensor && sensor->mHost == actor && sensor->mSensorGroup &&
                        std::abs(sensor->mRadius - 130.0f * actor->mScale.x) < 0.001f,
                    "Each original body sensor retains its authored host, group and scaled radius");
            require(actor->mCollisionParts && actor->mCollisionParts->mServer &&
                        actor->mCollisionParts->mHitSensor == sensor,
                    "Actual resource collision and original sensor ownership exist for every cage");
            std::fprintf(stderr, "[crystal-cage-probe] cage=(%g,%g,%g) break=%p manager=%p model=%p dead=%d cageDead=%d\n",
                         actor->mPosition.x, actor->mPosition.y, actor->mPosition.z, static_cast<void*>(actor->mBreakObj),
                         actor->mBreakObj ? static_cast<void*>(actor->mBreakObj->mModelManager) : nullptr,
                         actor->mBreakObj && actor->mBreakObj->mModelManager ? static_cast<void*>(actor->mBreakObj->mModelManager->getJ3DModel()) : nullptr,
                         actor->mBreakObj ? static_cast<int>(actor->mBreakObj->mFlag.mIsDead) : -1, static_cast<int>(actor->mFlag.mIsDead));
            require(actor->mBreakObj && actor->mBreakObj->mModelManager && actor->mBreakObj->mModelManager->getJ3DModel() &&
                        actor->mBreakObj->mModelManager->mXanimePlayer &&
                        actor->mBreakObj->mModelManager->getJ3DModel() == actor->mBreakObj->mModelManager->mXanimePlayer->mModel &&
                        actor->mBreakObj->mFlag.mIsDead,
                    "Each original init creates and initially retires its real animation-owned break model");
            auto* switches = actor->mStageSwitchCtrl;
            require(switches && switches->isValidSwitchAppear() && switches->mSW_Appear->mIsGlobal &&
                        switches->mSW_Appear->getSwitchNo() == 15 && !switches->isOnSwitchAppear() && actor->mFlag.mIsDead,
                    "Authored appearance switch remains off and original cages remain dead during the opening");
            const auto name = smgpc::resource::decode_cp932(actor->getName());
            require(std::count_if(scheduled.begin(), scheduled.end(), [&](const auto& entry) {
                return entry.name == name && entry.movement_type == MR::MovementType_MapObj &&
                       entry.calc_anim_type == MR::CalcAnimType_MapObj && entry.draw_buffer_type == MR::DrawBufferType_Crystal &&
                       entry.draw_type == -1 && entry.live_actor_position[0] == actor->mPosition.x &&
                       entry.live_actor_position[1] == actor->mPosition.y && entry.live_actor_position[2] == actor->mPosition.z;
            }) == 1, "Every original cage has its exact Crystal draw-category registration");
            if (auto* display = actor->mDisplayModel) {
                ++display_models;
                require(display->mHost == actor && display->mItemType == 3 && display->_AC &&
                            display->mModelInfo && std::strcmp(display->mModelInfo->mName, "SuperSpinDriver") == 0 &&
                            std::strcmp(display->mModelInfo->mAnim, "Freeze") == 0 && display->mFixedPosition &&
                            display->mModelManager && display->mModelManager->getJ3DModel() &&
                            display->mModelManager->getPlayingBckName() &&
                            MR::isEqualStringCase(display->mModelManager->getPlayingBckName(), "Freeze") &&
                            switches->isValidSwitchDead() && switches->mSW_Dead->mIsGlobal &&
                            switches->mSW_Dead->getSwitchNo() == 17 && !switches->isValidSwitchA(),
                        "Center cage creates the real fixed SuperSpinDriver model and Freeze animation with crystal category");
                retained_identities.push_back(display);
            } else {
                require(switches->isValidSwitchA() && switches->mSW_A->mIsGlobal &&
                            switches->mSW_A->getSwitchNo() == 17 && !switches->isOnSwitchA(),
                        "The other authored cages retain their original off break-listener switches");
                ++linked_break_listeners;
            }
            for (const auto value : {actor->_F8.x, actor->_F8.y, actor->_F8.z})
                require(std::isfinite(value), "Original after-placement map query supplies a finite break position");
            retained_identities.push_back(actor);
            retained_identities.push_back(actor->mBreakObj);
        }
        require(display_models == 1 && linked_break_listeners == 2 && retained_identities.size() == 7,
                "Actual original ownership comprises three cages, three break models and one authored dummy model");
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
        std::fprintf(stderr, "[crystal-cage-probe] PASS three real factory placements, authored archive selection, actual models/collision/sensors, off switches; frame=%llu\n",
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
        const auto save = std::filesystem::temp_directory_path() / ("petari-original-crystal-cage-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic starts with a fresh native console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original CrystalCage integration",
            .arguments = {"original-crystal-cage-test", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
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
            require(!smgpc::compat::has_name_obj_runtime_state(object), "Normal original scene teardown retires every cage and child model");
        std::fprintf(stderr, "PASS original-process CrystalCage: three original placements, seven actual owned actors, authored off switches and normal scene retirement\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process CrystalCage: %s\n", error.what());
        return 1;
    }
#endif
}
