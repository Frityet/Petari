#include "resource/TextEncoding.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/Boss/BossAccessor.hpp"
#include "Game/Boss/SkeletalFishBabyRailHolder.hpp"
#include "Game/Boss/SkeletalFishBossRailHolder.hpp"
#include "Game/Boss/TripodBossAccesser.hpp"
#include "Game/Camera/CameraContext.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Demo/DemoDirector.hpp"
#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Enemy/BegomanBase.hpp"
#include "Game/Enemy/KabokuriFireHolder.hpp"
#include "Game/Enemy/KameckBeamHolder.hpp"
#include "Game/Enemy/KarikariDirector.hpp"
#include "Game/Enemy/TakoHeiInkHolder.hpp"
#include "Game/GameAudio/AudBgmConductor.hpp"
#include "Game/GameAudio/AudCameraWatcher.hpp"
#include "Game/GameAudio/AudEffectDirector.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/LiveActorGroupArray.hpp"
#include "Game/LiveActor/MessageSensorHolder.hpp"
#include "Game/LiveActor/MirrorCamera.hpp"
#include "Game/LiveActor/SensorHitChecker.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/LiveActor/ShadowSurfaceDrawer.hpp"
#include "Game/LiveActor/ShadowVolumeDrawer.hpp"
#include "Game/LiveActor/VolumeModelDrawer.hpp"
#include "Game/Map/Air.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/LightDirector.hpp"
#include "Game/Map/NamePosHolder.hpp"
#include "Game/Map/OceanHomeMapCtrl.hpp"
#include "Game/Map/PlanetMapCreator.hpp"
#include "Game/Map/QuakeEffectGenerator.hpp"
#include "Game/Map/RaceManager.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/Map/SphereSelector.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SunshadeMapHolder.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/Map/WaterAreaHolder.hpp"
#include "Game/Map/WaterPlant.hpp"
#include "Game/MapObj/AirBubbleHolder.hpp"
#include "Game/MapObj/ArrowSwitchMultiHolder.hpp"
#include "Game/MapObj/BigBubbleHolder.hpp"
#include "Game/MapObj/BigFanHolder.hpp"
#include "Game/MapObj/ChipHolder.hpp"
#include "Game/MapObj/ClipAreaDropHolder.hpp"
#include "Game/MapObj/ClipAreaHolder.hpp"
#include "Game/MapObj/ClipFieldFillDraw.hpp"
#include "Game/MapObj/CoinHolder.hpp"
#include "Game/MapObj/CoinRotater.hpp"
#include "Game/MapObj/EarthenPipe.hpp"
#include "Game/MapObj/ElectricRailHolder.hpp"
#include "Game/MapObj/FallOutFieldDraw.hpp"
#include "Game/MapObj/FirePressureBulletHolder.hpp"
#include "Game/MapObj/GCapture.hpp"
#include "Game/MapObj/MapPartsRailGuideHolder.hpp"
#include "Game/MapObj/MiniatureGalaxyHolder.hpp"
#include "Game/MapObj/Note.hpp"
#include "Game/MapObj/PurpleCoinHolder.hpp"
#include "Game/MapObj/SpiderThread.hpp"
#include "Game/MapObj/SpinDriverPathDrawer.hpp"
#include "Game/MapObj/StarPieceDirector.hpp"
#include "Game/MapObj/WarpPod.hpp"
#include "Game/MapObj/WaterPressureBulletHolder.hpp"
#include "Game/NPC/EventDirector.hpp"
#include "Game/NPC/MiiFaceIconHolder.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"
#include "Game/NPC/NPCDirector.hpp"
#include "Game/NPC/TalkDirector.hpp"
#include "Game/NameObj/MovementOnOffGroupHolder.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Player/PlayerEvent.hpp"
#include "Game/Ride/FluffWind.hpp"
#include "Game/Ride/PlantLeaf.hpp"
#include "Game/Ride/PlantStalk.hpp"
#include "Game/Ride/SwingRope.hpp"
#include "Game/Ride/Trapeze.hpp"
#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/Scene/SceneDataInitializer.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Scene/ScenePlayingResult.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/Scene/StopSceneController.hpp"
#include "Game/Screen/BloomEffect.hpp"
#include "Game/Screen/BloomEffectSimple.hpp"
#include "Game/Screen/CaptureScreenDirector.hpp"
#include "Game/Screen/CenterScreenBlur.hpp"
#include "Game/Screen/CinemaFrame.hpp"
#include "Game/Screen/CometRetryButton.hpp"
#include "Game/Screen/DepthOfFieldBlur.hpp"
#include "Game/Screen/GalaxyMapController.hpp"
#include "Game/Screen/GalaxyNamePlateDrawer.hpp"
#include "Game/Screen/GameSceneLayoutHolder.hpp"
#include "Game/Screen/HeatHazeEffect.hpp"
#include "Game/Screen/ImageEffectSystemHolder.hpp"
#include "Game/Screen/InformationObserver.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "Game/Screen/MoviePlayerSimple.hpp"
#include "Game/Screen/MoviePlayingSequence.hpp"
#include "Game/Screen/OdhConverter.hpp"
#include "Game/Screen/PlayerActionGuidance.hpp"
#include "Game/Screen/SceneWipeHolder.hpp"
#include "Game/Screen/ScreenAlphaCapture.hpp"
#include "Game/Screen/ScreenBlurEffect.hpp"
#include "Game/Screen/StaffRoll.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "Game/Util/FurCtrl.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ShareUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"

#include "Game/System/DrawSyncManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/allocation.hpp>
#include "camera/CameraDirectorRuntime.hpp"
#include <aurora/exception.hpp>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

struct SceneObjHolder::NativeResources {
    JKRHeap::Handle domain;
    std::unique_ptr<smgpc::camera::CameraDirectorRuntime> camera;
    std::vector<std::unique_ptr<NameObj>> objects;
    std::vector<NameObj*> registrations;
    struct ProvisionalSlot { int id; NameObj* object; };
    std::vector<ProvisionalSlot> provisional;
    std::size_t constructionDepth = 0;
    bool retiring = false;
};

namespace {
    bool isUnclaimedSceneObject(const NameObj* object, const void*) noexcept {
        return !NameObj::isNativeOwnershipClaimed(object);
    }

    void rollbackSceneObjects(NameObj::NativeRegistrationMarker marker) noexcept {
        while (auto* object = NameObj::newestNativeObjectSince(marker, isUnclaimedSceneObject, nullptr)) {
            delete object;
        }
    }
}

SceneObjHolder::SceneObjHolder() {
    for (auto& object : mObj) {
        object = nullptr;
    }
}

SceneObjHolder::~SceneObjHolder() {
    retireNativeResources();
}

void SceneObjHolder::initializeNative(JKRHeap::Handle domain) {
    const aurora::allocation::HostAllocationScope host;
    if (!domain || mNativeResources || MR::getSceneObjHolder() != this) {
        aurora::throw_host_exception<std::logic_error>("SceneObjHolder initialization requires the actual controller's scene and heap");
    }
    mNativeResources = std::make_unique<NativeResources>();
    mNativeResources->domain = std::move(domain);
    try {
        const JKRHeap::CurrentHeapScope game(*mNativeResources->domain);
        const aurora::allocation::ClientAllocationScope game_routing({true, true});
        // Every original LiveActor joins this group, including actors without execution entries.
        if (!dynamic_cast<AllLiveActorGroup*>(create(SceneObj_AllLiveActorGroup))) {
            aurora::throw_host_exception<std::logic_error>("Scene initialization requires the original AllLiveActorGroup");
        }
    } catch (...) {
        retireNativeResources();
        throw;
    }
}

JKRHeap::Handle SceneObjHolder::nativeAllocationHeap() const {
    return mNativeResources ? mNativeResources->domain : nullptr;
}

bool SceneObjHolder::isNativeRetiring() const {
    return mNativeResources && mNativeResources->retiring;
}

bool SceneObjHolder::ownsNativeObject(const NameObj* object) const {
    return object && mNativeResources &&
           std::ranges::find(mNativeResources->registrations, object) != mNativeResources->registrations.end();
}

void SceneObjHolder::adoptNativeObject(NameObj* object) {
    const aurora::allocation::HostAllocationScope host;
    if (!mNativeResources || mNativeResources->retiring || !object) {
        aurora::throw_host_exception<std::logic_error>("SceneObjHolder adoption requires its live original scene");
    }
    if (ownsNativeObject(object)) {
        return;
    }
    if (!NameObj::nativeGeneration(object) || NameObj::isNativeOwnershipClaimed(object)) {
        aurora::throw_host_exception<std::logic_error>("SceneObjHolder children must be registered and unclaimed");
    }
    if (mNativeResources->constructionDepth) {
        return; // The outer construction transaction adopts the complete registration suffix.
    }
    mNativeResources->objects.reserve(mNativeResources->objects.size() + 1);
    mNativeResources->registrations.reserve(mNativeResources->registrations.size() + 1);
    object->claimNativeOwnership(this);
    mNativeResources->objects.emplace_back(object);
    mNativeResources->registrations.push_back(object);
}

void SceneObjHolder::prepareNativeRetirement() noexcept {
    if (!mNativeResources) {
        return;
    }
    DrawSyncManager::retireNativeCallbacks(*mNativeResources->domain);
    if (auto* talk = static_cast<TalkDirector*>(getObj(SceneObj_TalkDirector))) {
        talk->beginNativeRetirement();
    }
}

void SceneObjHolder::retireNativeResources() noexcept {
    if (!mNativeResources || mNativeResources->retiring) {
        return;
    }
    const aurora::allocation::HostAllocationScope host;
    prepareNativeRetirement();
    mNativeResources->retiring = true;
    if (mNativeResources->camera) {
        mNativeResources->camera->unpublish();
    }
    if (auto* effects = static_cast<EffectSystem*>(getObj(SceneObj_EffectSystem))) {
        effects->retireNativeResources();
    }
    // Release actor-owned collision and sensor resources while every SceneObj dependency still lives.
    for (const auto& object : mNativeResources->objects) {
        if (auto* actor = dynamic_cast<LiveActor*>(object.get())) {
            actor->releaseNativeResources();
        }
    }
    while (!mNativeResources->objects.empty()) {
        mNativeResources->objects.pop_back();
    }
    for (auto& object : mObj) {
        object = nullptr;
    }
    mNativeResources.reset();
}

NameObj *SceneObjHolder::create(int id) {
    if (this != MR::getSceneObjHolder() || !mNativeResources || mNativeResources->retiring ||
        id < 0 || id >= SceneObj_NumMax) {
        return nullptr;
    }

    if (mObj[id] != nullptr) {
        return mObj[id];
    }

    auto* resources = mNativeResources.get();
    const auto owned_checkpoint = resources->objects.size();
    const auto registrations_checkpoint = resources->registrations.size();
    const auto marker = NameObj::markNativeRegistrations();
    DrawSyncManager::CallbackRegistration callbacks;
    const auto slot_checkpoint = resources->provisional.size();
    const auto outermost = resources->constructionDepth == 0U;
    ++resources->constructionDepth;
    auto object = std::unique_ptr<NameObj>{};
    try {
        // Original factories may await resource creation on the main thread.
        // Keep their caller-selected heap without holding the heap mutex over
        // that wait; each original allocation serializes its own heap access.
        const aurora::allocation::ClientAllocationScope game({true, true});
        object.reset(newEachObj(id));
        if (object == nullptr) {
            if (resources->provisional.size() != slot_checkpoint ||
                NameObj::newestNativeObjectSince(
                    marker, nullptr, nullptr) != nullptr) {
                aurora::throw_host_exception<std::logic_error>(
                    "SceneObj factory returned null after creating nested scene objects");
            }
            --resources->constructionDepth;
            return nullptr;
        }

        object->initWithoutIter();
        aurora::allocation::HostAllocationScope host_metadata;
        auto registrations =
            NameObj::snapshotNativeObjectsSince(
                marker);
        if (std::ranges::count(registrations, object.get()) != 1 ||
            registrations.empty() || registrations.front() != object.get() ||
            NameObj::isNativeOwnershipClaimed(
                object.get())) {
            aurora::throw_host_exception<std::logic_error>(
                "SceneObj construction did not register one leading, unclaimed root");
        }
        auto *result = object.get();
        resources->provisional.push_back({id, result});
        mObj[id] = result;
        (void)object.release();

        if (id == SceneObj_CameraDirector) {
            auto *context = dynamic_cast<CameraContext *>(mObj[SceneObj_CameraContext]);
            auto *director = dynamic_cast<CameraDirector *>(result);
            if (!context || !director)
                aurora::throw_host_exception<std::logic_error>("Camera publication requires the actual CameraContext and CameraDirector");
            resources->camera = std::make_unique<smgpc::camera::CameraDirectorRuntime>(*context, *director);
        }

        if (outermost) {
            registrations =
                NameObj::snapshotNativeObjectsSince(
                    marker);
            const auto unclaimed_count = static_cast<std::size_t>(
                std::ranges::count_if(
                    registrations, [](const NameObj *registered) {
                        return !NameObj::isNativeOwnershipClaimed(
                                       registered);
                    }));
            resources->objects.reserve(
                resources->objects.size() + unclaimed_count);
            resources->registrations.reserve(resources->registrations.size() + registrations.size());

            for (auto *registered : registrations) {
                if (!NameObj::isNativeOwnershipClaimed(registered)) {
                    registered->claimNativeOwnership(this);
                    resources->objects.emplace_back(registered);
                }
            }
            for (auto* registered : registrations) {
                resources->registrations.push_back(registered);
            }
            resources->provisional.clear();
        }
        --resources->constructionDepth;
        callbacks.commit();
        return result;
    } catch (...) {
        callbacks.rollback();
        if (resources->camera && NameObj::wasNativeRegisteredSince(
                &resources->camera->director(), marker))
            resources->camera.reset();
        if (object != nullptr &&
            NameObj::wasNativeRegisteredSince(
                object.get(), marker)) {
            (void)object.release();
        }
        while (resources->provisional.size() > slot_checkpoint) {
            const auto slot = resources->provisional.back();
            resources->provisional.pop_back();
            if (mObj[slot.id] == slot.object) {
                mObj[slot.id] = nullptr;
            }
        }
        while (resources->objects.size() > owned_checkpoint) {
            resources->objects.pop_back();
        }
        resources->registrations.resize(registrations_checkpoint);
        rollbackSceneObjects(marker);
        --resources->constructionDepth;
        if (outermost) {
            resources->provisional.clear();
        }
        throw;
    }
}

NameObj *SceneObjHolder::getObj(int id) const {
    if (id < 0 || id >= SceneObj_NumMax) {
        return nullptr;
    }
    return mObj[id];
}

bool SceneObjHolder::isExist(int id) const {
    return id >= 0 && id < SceneObj_NumMax && mObj[id] != nullptr;
}

NameObj* SceneObjHolder::newEachObj(int id) {
    switch (id) {
    case SceneObj_SensorHitChecker:
        return new SensorHitChecker(CP932("センサー当たり"));
    case SceneObj_CollisionDirector:
        return new CollisionDirector();
    case SceneObj_ClippingDirector:
        return new ClippingDirector();
    case SceneObj_DemoDirector:
        return new DemoDirector(CP932("デモ指揮"));
    case SceneObj_EventDirector:
        return new EventDirector();
    case SceneObj_EffectSystem:
        return new EffectSystem(CP932("エフェクトシステム"), true);
    case SceneObj_LightDirector:
        return new LightDirector();
    case SceneObj_SceneDataInitializer:
        return new SceneDataInitializer();
    case SceneObj_StageDataHolder:
        return new StageDataHolder(MR::getCurrentStageName(), 0, true);
    case SceneObj_MessageSensorHolder:
        return new MessageSensorHolder(CP932("システム汎用センサー"));
    case SceneObj_StageSwitchContainer:
        return new StageSwitchContainer();
    case SceneObj_SwitchWatcherHolder:
        return new SwitchWatcherHolder();
    case SceneObj_SleepControllerHolder:
        return new SleepControllerHolder();
    case SceneObj_AreaObjContainer:
        return new AreaObjContainer(CP932("エリアオブジェクトコンテナ管理"));
    case SceneObj_LiveActorGroupArray:
        return new LiveActorGroupArray(CP932("オブジェクトグループ"));
    case SceneObj_MovementOnOffGroupHolder:
        return new MovementOnOffGroupHolder(CP932("Movementグループ管理"));
    case SceneObj_CaptureScreenActor:
        return new CaptureScreenActor(MR::DrawType_CaptureScreenIndirect, "Indirect");
    case SceneObj_AudCameraWatcher:
        return new AudCameraWatcher();
    case SceneObj_AudEffectDirector:
        return new AudEffectDirector();
    case SceneObj_AudBgmConductor:
        return new AudBgmConductor();
    case SceneObj_MarioHolder:
        return new MarioHolder();
    case SceneObj_MirrorCamera:
        return new MirrorCamera(CP932("鏡用カメラ"));
    case SceneObj_CameraContext:
        return new CameraContext();
    case SceneObj_NameObjGroup:
        return new NameObjGroup("IgnorePauseNameObj", 16);
    case SceneObj_TalkDirector:
        return new TalkDirector(CP932("会話ディレクター"));
    case SceneObj_EventSequencer:
        return new EventSequencer();
    case SceneObj_StopSceneController:
        return new StopSceneController();
    case SceneObj_SceneNameObjMovementController:
        return new SceneNameObjMovementController();
    case SceneObj_ImageEffectSystemHolder:
        return new ImageEffectSystemHolder();
    case SceneObj_BloomEffect:
        return new BloomEffect(CP932("ブルーム"));
    case SceneObj_BloomEffectSimple:
        return new BloomEffectSimple();
    case SceneObj_ScreenBlurEffect:
        return new ScreenBlurEffect(CP932("画面ブラー"));
    case SceneObj_DepthOfFieldBlur:
        return new DepthOfFieldBlur(CP932("被写界深度ブラー"));
    case SceneObj_SceneWipeHolder:
        return new SceneWipeHolder();
    case SceneObj_PlayerActionGuidance:
        return new PlayerActionGuidance();
    case SceneObj_ScenePlayingResult:
        return new ScenePlayingResult();
    case SceneObj_LensFlareDirector:
        return new LensFlareDirector();
    case SceneObj_FurDrawManager:
        return new FurDrawManager(64);
    case SceneObj_PlacementStateChecker:
        return new PlacementStateChecker(CP932("オブジェクト配置状態の監視"));
    case SceneObj_NamePosHolder:
        return new NamePosHolder();
    case SceneObj_NPCDirector:
        return new NPCDirector();
    case SceneObj_ResourceShare:
        return new ResourceShare();
    case SceneObj_MoviePlayerSimple:
        return new MoviePlayerSimple();
    case SceneObj_InformationObserver:
        return new InformationObserver();
    case SceneObj_CenterScreenBlur:
        return new CenterScreenBlur();
    case SceneObj_OdhConverter:
        return new OdhConverter();
    case SceneObj_CometRetryButton:
        return new CometRetryButton(CP932("コメットリトライボタン"));
    case SceneObj_AllLiveActorGroup:
        return new AllLiveActorGroup();
    case SceneObj_CameraDirector:
        return new CameraDirector(CP932("カメラ管理"));
    case SceneObj_PlanetGravityManager:
        return new PlanetGravityManager(CP932("重力"));
    case SceneObj_BaseMatrixFollowTargetHolder:
        return new BaseMatrixFollowTargetHolder(CP932("行列追随先リスト"), 256, 256);
    case SceneObj_GameSceneLayoutHolder:
        return new GameSceneLayoutHolder();
    case SceneObj_TripodBossAccesser:
        return new TripodBossAccesser(CP932("三脚ボスアクセサ"));
    case SceneObj_KameckBeamHolder:
        return new KameckBeamHolder();
    case SceneObj_KameckFireBallHolder:
        return new KameckFireBallHolder();
    case SceneObj_KameckBeamTurtleHolder:
        return new KameckBeamTurtleHolder();
    case SceneObj_KabokuriFireHolder:
        return new KabokuriFireHolder();
    case SceneObj_TakoHeiInkHolder:
        return new TakoHeiInkHolder();
    case SceneObj_SwingRopeGroup:
        return new SwingRopeGroup(CP932("スイングロープ描画"));
    case SceneObj_CoinHolder:
        return new CoinHolder(CP932("コイン管理"));
    case SceneObj_PurpleCoinHolder:
        return new PurpleCoinHolder();
    case SceneObj_CoinRotater:
        return new CoinRotater(CP932("コイン回転管理"));
    case SceneObj_AirBubbleHolder:
        return new AirBubbleHolder(CP932("空気アワ管理"));
    case SceneObj_StarPieceDirector:
        return new StarPieceDirector(CP932("スターピース指揮"));
    case SceneObj_BegomanAttackPermitter:
        return new BegomanAttackPermitter(CP932("ベーゴマン攻撃許可者"));
    case SceneObj_BigFanHolder:
        return new BigFanHolder();
    case SceneObj_KarikariDirector:
        return new KarikariDirector(CP932("カリカリディレクター"));
    case SceneObj_ShadowControllerHolder:
        return new ShadowControllerHolder();
    case SceneObj_ShadowVolumeDrawInit:
        return new ShadowVolumeDrawInit();
    case SceneObj_ShadowSurfaceDrawInit:
        return new ShadowSurfaceDrawInit(CP932("水面影描画初期化"));
    case SceneObj_PlantStalkDrawInit:
        return new PlantStalkDrawInit(CP932("植物の茎描画初期化"));
    case SceneObj_PlantLeafDrawInit:
        return new PlantLeafDrawInit(CP932("描画初期化[植物の葉]"));
    case SceneObj_TrapezeRopeDrawInit:
        return new TrapezeRopeDrawInit(CP932("空中ブランコロープ描画"));
    case SceneObj_VolumeModelDrawInit:
        return new VolumeModelDrawInit();
    case SceneObj_SpinDriverPathDrawInit:
        return new SpinDriverPathDrawInit();
    case SceneObj_NoteGroup:
        return new NoteGroup();
    case SceneObj_ClipAreaHolder:
        return new ClipAreaHolder(CP932("クリップエリアホルダー"));
    case SceneObj_ArrowSwitchMultiHolder:
        return new ArrowSwitchMultiHolder();
    case SceneObj_ClipAreaDropHolder:
        return new ClipAreaDropHolder();
    case SceneObj_FallOutFieldDraw:
        return new FallOutFieldDraw(CP932("クリップエリア描画[抜き]"));
    case SceneObj_ClipFieldFillDraw:
        return new ClipFieldFillDraw(CP932("クリップエリア描画[塗りつぶし]"));
    case SceneObj_ScreenAlphaCapture:
        return new ScreenAlphaCapture(CP932("アルファテクスチャ取り込み"));
    case SceneObj_MapPartsRailGuideHolder:
        return new MapPartsRailGuideHolder();
    case SceneObj_GCapture:
        return new GCapture(CP932("Gキャプチャー"));
    case SceneObj_NameObjExecuteHolder:
        return new NameObjExecuteHolder(4096);
    case SceneObj_ElectricRailHolder:
        return new ElectricRailHolder(CP932("電撃レール保持"));
    case SceneObj_SpiderThread:
        return new SpiderThread(CP932("クモの巣"));
    case SceneObj_QuakeEffectGenerator:
        return new QuakeEffectGenerator();
    case SceneObj_HeatHazeDirector:
        return new HeatHazeDirector(CP932("陽炎制御"));
    case SceneObj_BlueChipHolder:
        return new ChipHolder(CP932("ブルーチップホルダー"), 0);
    case SceneObj_YellowChipHolder:
        return new ChipHolder(CP932("イエローーチップホルダー"), 1);
    case SceneObj_BigBubbleHolder:
        return new BigBubbleHolder(CP932("オオアワホルダー"));
    case SceneObj_EarthenPipeMediator:
        return new EarthenPipeMediator();
    case SceneObj_WaterAreaHolder:
        return new WaterAreaHolder();
    case SceneObj_WaterPlantDrawInit:
        return new WaterPlantDrawInit();
    case SceneObj_OceanHomeMapCtrl:
        return new OceanHomeMapCtrl();
    case SceneObj_RaceManager:
        return new RaceManager();
    case SceneObj_GroupCheckManager:
        return new GroupCheckManager(CP932("属性グループマネージャー"));
    case SceneObj_SkeletalFishBabyRailHolder:
        return new SkeletalFishBabyRailHolder(CP932("スカルシャークベビーレール管理"));
    case SceneObj_SkeletalFishBossRailHolder:
        return new SkeletalFishBossRailHolder(CP932("スカルシャークボスレール管理"));
    case SceneObj_WaterPressureBulletHolder:
        return new WaterPressureBulletHolder(CP932("ウォータープレッシャー玉ホルダ−"));
    case SceneObj_FirePressureBulletHolder:
        return new FirePressureBulletHolder(CP932("ファイアプレッシャー玉ホルダ−"));
    case SceneObj_SunshadeMapHolder:
        return new SunshadeMapHolder();
    case SceneObj_MiiFacePartsHolder:
        return new MiiFacePartsHolder(128);
    case SceneObj_MiiFaceIconHolder:
        return new MiiFaceIconHolder(16, CP932("Miiアイコン保持管理"));
    case SceneObj_FluffWindHolder:
        return new FluffWindHolder();
    case SceneObj_SphereSelector:
        return new SphereSelector();
    case SceneObj_GalaxyNamePlateDrawer:
        return new GalaxyNamePlateDrawer();
    case SceneObj_CinemaFrame:
        return new CinemaFrame(true);
    case SceneObj_BossAccessor:
        return new BossAccessor();
    case SceneObj_MiniatureGalaxyHolder:
        return new MiniatureGalaxyHolder();
    case SceneObj_PlanetMapCreator:
        return new PlanetMapCreator(CP932("惑星クリエイタ"));
    case SceneObj_WarpPodMgr:
        return new WarpPodMgr(CP932("ワープポッド管理局"));
    case SceneObj_PriorDrawAirHolder:
        return new PriorDrawAirHolder();
    case SceneObj_GalaxyMapController:
        return new GalaxyMapController();
    case SceneObj_MoviePlayingSequenceHolder:
        return new MoviePlayingSequenceHolder(CP932("ムービー管理保持"));
    case SceneObj_PrologueHolder:
        return new PrologueHolder(CP932("プロローグ保持"));
    case SceneObj_StaffRoll:
        return new StaffRoll(CP932("スタッフロール"));
    default:
        return nullptr;
    }
}
namespace MR {
    NameObj* createSceneObj(int id) {
        auto* holder = getSceneObjHolder();
        return holder ? holder->create(id) : nullptr;
    }

    SceneObjHolder* getSceneObjHolder() {
        auto* system = SingletonHolder<GameSystem>::get();
        if (!system || !system->mSceneController || !system->mSceneController->isExistSceneObjHolder()) {
            return nullptr;
        }
        auto* holder = system->mSceneController->getSceneObjHolder();
        return holder && !holder->isNativeRetiring() ? holder : nullptr;
    }

    bool isExistSceneObj(int id) {
        auto* holder = getSceneObjHolder();
        return holder && holder->isExist(id);
    }
}
