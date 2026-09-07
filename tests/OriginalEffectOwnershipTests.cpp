#include "Game/Effect/AutoEffectInfo.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/MultiEmitter.hpp"
#include "Game/Effect/MultiEmitterCallBack.hpp"
#include "Game/Effect/ParticleEmitterHolder.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Effect/SingleEmitter.hpp"
#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/EffectSystemOwnership.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include <aurora/dvd.h>
#include <aurora/gfx.h>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <dolphin/gx/GXAurora.h>
#include <iostream>
#include <stdexcept>

namespace {
    class Logger final : public smgpc::logging::ILogger {
        void write(std::FILE *, std::source_location, smgpc::logging::Level,
                   smgpc::logging::Category, std::string_view) override {
        }
    };
    class Callback final : public MultiEmitterCallBackBase {
    public:
        unsigned initialized = 0;
        unsigned executed = 0;
        void init(JPABaseEmitter *) override {
            ++initialized;
        }
        void execute(JPABaseEmitter *) override {
            ++executed;
        }
    };
    const char *dynamic_effect_name(ParticleResourceHolder &resources) {
        for (int i = 0; i < resources.mAutoEffectList->getNumEntries(); ++i) {
            AutoEffectInfo info;
            info.init(JMapInfoIter(resources.mAutoEffectList, i));
            if (info.mUniqueName && !info.mAnimName && !info.mParentName && !info.mJointName)
                return info.mUniqueName;
        }
        throw std::runtime_error("Disc lacks a standalone auto-effect row");
    }

    class AddEffectRequest final : public NameObj {
    public:
        AddEffectRequest(LiveActor &actor, const char *name, JKRHeap &heap)
            : NameObj("Original dynamic effect registration"), _actor(actor), _name(name), _heap(heap) {
        }
        void movement() override {
            assert(!executed);
            assert(JKRHeap::getCurrentHeap() == &_heap);
            MR::addEffect(&_actor, _name);
            auto *first_info = _actor.mEffectKeeper->getEmitter(_name)->_28;
            assert(JKRHeap::findFromRoot(const_cast<AutoEffectInfo *>(first_info)) == &_heap);
            MR::addEffect(&_actor, _name);
            auto *replacement_info = _actor.mEffectKeeper->getEmitter(_name)->_28;
            assert(first_info != replacement_info);
            assert(JKRHeap::findFromRoot(const_cast<AutoEffectInfo *>(replacement_info)) == &_heap);
            assert(_actor.mEffectKeeper->_C.size() == 2);
            for (auto *emitter : _actor.mEffectKeeper->_C)
                assert(JKRHeap::findFromRoot(emitter) == &_heap);
            executed = true;
        }
        bool executed = false;

    private:
        LiveActor &_actor;
        const char *_name;
        JKRHeap &_heap;
    };

    const char *resource_name(ParticleResourceHolder &resources, bool one_shot) {
        for (int i = 0; i < resources.mParticleNames->getNumEntries(); ++i) {
            const auto *resource = resources.mResourceMgr->getResource(i);
            if (!resource || (resource->getDyn()->getMaxFrame() != 0) != one_shot)
                continue;
            // Fixed-frame callback assertions require an immediate emitter that
            // remains alive throughout both generations of the one-shot.
            if (resource->getDyn()->getStartFrame() > 0 ||
                (one_shot && resource->getDyn()->getMaxFrame() <= 4))
                continue;
            const char *name = nullptr;
            assert(resources.mParticleNames->getValue(i, "name", &name));
            // The original MultiEmitter distinguishes explicit names by a
            // two-digit suffix; catalog entries follow this convention.
            return name;
        }
        throw std::runtime_error("Disc lacks required loop or one-shot particle resource");
    }
    std::vector<std::uint8_t> read_display() {
        u32 width = 0, height = 0, stride = 0;
        assert(AuroraGetDisplayCopySize(&width, &height));
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
        assert(AuroraReadDisplayCopyRGBA8(pixels.data(), pixels.size(), &width, &height, &stride));
        assert(stride == width * 4);
        return pixels;
    }
    void advance(smgpc::runtime::SceneScheduler &scheduler) {
        scheduler.execute_movement();
        scheduler.execute_calc_anim();
    }
}  // namespace

int main() {
    const auto *disc = std::getenv("SMGPC_REAL_DISC");
    if (!disc)
        throw std::runtime_error("SMGPC_REAL_DISC must name an actual game disc");
    smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original effect ownership test"});
    smgpc::render::AuroraRenderer renderer(window);
    if (!aurora_dvd_open(disc))
        throw std::runtime_error("Cannot open actual game disc");
    struct DiscGuard {
        ~DiscGuard() {
            aurora_dvd_close();
        }
    } disc_guard;
    DVDInit();
    smgpc::resource::GameResourceRuntime process({96U << 20, 32U << 20, 4U << 20});
    Logger logger;
    const auto process_free = process.host_heaps()->root_heap().getFreeSize();
    const auto process_objects = smgpc::compat::name_obj_runtime_state_count();
    (void)renderer.begin_frame();
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        {
            smgpc::runtime::RuntimeContext runtime(logger, window, process);
            runtime.initialize_particle_resources(process);
            auto resources = runtime.retain_particle_resources();
            auto &scheduler = runtime.scheduler();
            smgpc::runtime::SceneSchedulerBinding scheduler_binding(scheduler);
            const auto baseline_entries = scheduler.snapshot().size();
            const auto baseline_objects = smgpc::compat::name_obj_runtime_state_count();
            const auto baseline_free = process.host_heaps()->root_heap().getFreeSize();
            for (const auto budget : {32U, 512U, 4096U}) {
                bool failed = false;
                {
                    SceneObjHolder failed_holder;
                    smgpc::scene::SceneObjHolderBinding failed_binding(failed_holder);
                    try {
                        failed_binding.initialize_effect_system(64, 8, budget);
                    } catch (const std::invalid_argument &) {
                        assert(budget == 32U);
                        failed = true;
                    } catch (const std::bad_alloc &) {
                        assert(budget != 32U);
                        failed = true;
                    }
                }
                assert(failed);
                assert(scheduler.snapshot().size() == baseline_entries);
                assert(smgpc::compat::name_obj_runtime_state_count() == baseline_objects);
                assert(process.host_heaps()->root_heap().getFreeSize() == baseline_free);
            }
            const auto *one_shot = resource_name(resources->holder(), true);
            const auto *loop = resource_name(resources->holder(), false);
            Callback callback;
            std::weak_ptr<smgpc::compat::JkrAllocationDomain> domain;
            std::weak_ptr<smgpc::compat::JkrAllocationDomain> game_domain;
            std::unique_ptr<LiveActor> survivor;
            std::array<AuroraDepthSnapshotId, 64> depth_snapshots{};
            SceneObjHolder holder;
            {
                smgpc::scene::SceneObjHolderBinding binding(holder);
                binding.initialize_effect_system(64, 8, 256U << 10);
                auto *ownership = smgpc::scene::current_effect_system_ownership();
                domain = ownership->allocation_domain();
                game_domain = scheduler.allocation_domain();
                assert(!game_domain.expired());
                assert(scheduler.allocation_domain() != ownership->allocation_domain());
                auto *system = MR::getEffectSystem();
                assert(system && system->mEmitterManager->gidMax == 9);
                assert(system->mEmitterManager->pResMgrAry[0] == resources->holder().mResourceMgr);
                assert(scheduler.snapshot().size() == baseline_entries + 11);
                bool screen_bound = false;
                auto *resource_manager = resources->holder().mResourceMgr;
                for (u16 i = 0; i < resource_manager->mTexNum; ++i) {
                    auto *texture = resource_manager->mpTexArr[i];
                    if (std::strcmp(texture->getName(), "IndDummy") == 0) {
                        assert(texture->getJUTTexture()->getTexInfo() == MR::getScreenResTIMG());
                        screen_bound = true;
                    }
                }
                assert(screen_bound);
                {
                    LiveActor dynamic("Dynamic auto-effect owner");
                    dynamic.initEffectKeeper(2, nullptr, false);
                    AddEffectRequest request(dynamic, dynamic_effect_name(resources->holder()), scheduler.allocation_domain()->heap());
                    const auto marker = scheduler.registration_marker();
                    scheduler.connect_name_obj(request, 1, -1, -1, -1);
                    advance(scheduler);
                    assert(request.executed);
                    (void)scheduler.remove_registrations_since(marker);
                }
                auto actor = std::make_unique<LiveActor>("Emitter callback owner");
                actor->initEffectKeeper(1, nullptr, true);
                assert(actor->mEffectKeeper);
                actor->mEffectKeeper->registerEffectWithoutSRT(one_shot, "Burst");
                actor->mEffectKeeper->finalizeSort();
                auto *multi = MR::emitEffectWithEmitterCallBack(actor.get(), "Burst", &callback);
                assert(multi == actor->mEffectKeeper->getEmitter("Burst"));
                assert(multi->mEmitters.size() == 1);
                auto *first = multi->getParticleEmitter(0);
                first->mEmitter->mRate = 0;
                advance(scheduler);
                std::cout << "resource=" << one_shot << " max_frame=" << first->mEmitter->mMaxFrame
                          << " start_frame=" << first->mEmitter->mpRes->getDyn()->getStartFrame()
                          << " tick=" << first->mEmitter->mTick << " initialized=" << callback.initialized
                          << " executed=" << callback.executed << std::endl;
                assert(callback.initialized == 1 && callback.executed == 1);
                const auto token = first->mEmitter->getUserWork();
                assert(token == reinterpret_cast<uintptr_t>(&multi->mEmitters[0]));
                MR::emitEffectWithEmitterCallBack(actor.get(), "Burst", &callback);
                auto *second = multi->getParticleEmitter(0);
                assert(first != second && first->isValid());
                assert(first->mEmitter->getUserWork() == 0);
                assert(first->mEmitter->mLastNonzeroUserWork == token);
                second->mEmitter->mRate = 0;
                survivor = std::make_unique<LiveActor>("Independent emitter owner");
                survivor->initEffectKeeper(1, nullptr, false);
                survivor->mEffectKeeper->registerEffectWithoutSRT(loop, "Loop");
                auto *loop_multi = MR::emitEffectWithEmitterCallBack(survivor.get(), "Loop", &callback);
                auto *independent = loop_multi->getParticleEmitter(0);
                independent->mEmitter->mRate = 0;
                advance(scheduler);
                assert(callback.initialized == 3);
                const auto executions = callback.executed;
                scheduler.execute_calc_anim();
                assert(callback.executed == executions);
                actor.reset();
                assert(!first->isValid() && !second->isValid() && independent->isValid());
                auto *reused = system->createEmitter(resources->holder().getUserIndex(loop), 0, 0);
                assert(reused && reused->mEmitter->getUserWork() == 0);
                assert(reused->mEmitter->mLastNonzeroUserWork == 0);
                system->forceDeleteEmitter(reused);
                advance(scheduler);
                assert(callback.executed == executions + 1);
                {
                    LiveActor hierarchy("Emitter hierarchy owner");
                    hierarchy.initEffectKeeper(2, nullptr, false);
                    hierarchy.mEffectKeeper->registerEffectWithoutSRT(loop, "Parent");
                    hierarchy.mEffectKeeper->registerEffectWithoutSRT(loop, "Child");
                    auto *parent = hierarchy.mEffectKeeper->getEmitter("Parent");
                    auto *child = hierarchy.mEffectKeeper->getEmitter("Child");
                    parent->addChildEmitter(child);
                    const auto before = callback.initialized;
                    assert(MR::emitEffectWithEmitterCallBack(&hierarchy, "Parent", &callback) == parent);
                    assert(parent->isValid() && child->isValid());
                    parent->getParticleEmitter(0)->mEmitter->mRate = 0;
                    child->getParticleEmitter(0)->mEmitter->mRate = 0;
                    advance(scheduler);
                    assert(callback.initialized == before + 2);
                    parent->forceDelete(system);
                    assert(!parent->isValid() && !child->isValid());
                }
                {
                    const smgpc::render::ScopedAuroraRendererContext renderer_context(renderer);
                    renderer.set_copy_clear({.color = {0, 0, 0, 255}});
                    renderer.end_frame();
                    auto frame = renderer.begin_frame();
                    runtime.begin_frame(frame);
                    renderer.end_frame();
                    const auto baseline = read_display();
                    LiveActor visible("Visible particle owner");
                    visible.initEffectKeeper(1, nullptr, false);
                    visible.mEffectKeeper->registerEffectWithoutSRT(loop, "Visible");
                    Callback visible_callback;
                    auto *visible_multi = MR::emitEffectWithEmitterCallBack(&visible, "Visible", &visible_callback);
                    auto *emitter = visible_multi->getParticleEmitter(0)->mEmitter;
                    emitter->mRate = 1.0f;
                    unsigned particles = 0;
                    unsigned draws = 0;
                    for (unsigned i = 0; i < 12; ++i) {
                        frame = renderer.begin_frame();
                        runtime.begin_frame(frame);
                        runtime.set_scene_camera_pose({.eye = {0, 0, 300}, .watch = {0, 0, 0}});
                        GXSetProjection(MR::getCameraProjectionMtx().mMtx, GX_PERSPECTIVE);
                        advance(scheduler);
                        particles = emitter->getParticleNumber();
                        scheduler.execute_draw_type(71);
                        renderer.end_frame();
                        draws = aurora_get_stats()->drawCallCount;
                    }
                    const auto image = read_display();
                    assert(image.size() == baseline.size());
                    std::size_t changed = 0;
                    for (std::size_t i = 0; i < image.size(); ++i)
                        changed += image[i] != baseline[i];
                    std::cout << "draw_cycle=" << cycle << " resource=" << loop << " live_particles=" << particles
                              << " gpu_draws=" << draws << " changed_rgba_bytes=" << changed << std::endl;
                    if (const auto *output = std::getenv("SMGPC_EFFECT_SCREENSHOT"))
                        renderer.request_screenshot_png(std::filesystem::path(output));
                    assert(particles > 0 && draws > 0 && changed > 0);
                    (void)renderer.begin_frame();
                }
                {
                    const smgpc::compat::JkrAllocationScope game_allocations(scheduler.allocation_domain());
                    auto &game_heap = scheduler.allocation_domain()->heap();
                    const auto free_before = game_heap.getFreeSize();
                    for (auto &id : depth_snapshots) {
                        id = GXAuroraRequestDepthSnapshot();
                        assert(id != AURORA_INVALID_DEPTH_SNAPSHOT_ID);
                    }
                    // Snapshot records and FIFO storage outlive the Game heap.
                    // Their native allocation must also restore caller routing.
                    assert(game_heap.getFreeSize() == free_before);
                    auto *original_allocation = new u8[37];
                    assert(JKRHeap::findFromRoot(original_allocation) == &game_heap);
                    delete[] original_allocation;
                }
                // The binding must retire this remaining actor's callback graph
                // even when its actor storage outlives the scene.
            }
            assert(survivor->mEffectKeeper == nullptr);
            survivor.reset();
            assert(domain.expired());
            assert(game_domain.expired());
            assert(GXAuroraGetDepthSnapshotInfo(depth_snapshots.front(), nullptr) == AURORA_DEPTH_SNAPSHOT_UNKNOWN);
            for (std::size_t i = depth_snapshots.size() - 16; i < depth_snapshots.size(); ++i) {
                AuroraDepthSnapshotInfo info{};
                assert(GXAuroraGetDepthSnapshotInfo(depth_snapshots[i], &info) != AURORA_DEPTH_SNAPSHOT_UNKNOWN);
                assert(info.id == depth_snapshots[i]);
                GXAuroraReleaseDepthSnapshot(depth_snapshots[i]);
                assert(GXAuroraGetDepthSnapshotInfo(depth_snapshots[i], nullptr) == AURORA_DEPTH_SNAPSHOT_UNKNOWN);
            }
            assert(scheduler.snapshot().size() == baseline_entries);
            const auto executions = callback.executed;
            advance(scheduler);
            assert(callback.executed == executions);
            assert(smgpc::compat::name_obj_runtime_state_count() == baseline_objects);
            assert(process.host_heaps()->root_heap().getFreeSize() == baseline_free);
            std::cout << "cycle=" << cycle << " resources=" << one_shot << ',' << loop
                      << " orphan_retirement=pass callback_init=" << callback.initialized
                      << " persistent_backend_allocation=pass"
                      << " scheduler_and_heap_reclamation=pass\n";
        }
        assert(!smgpc::runtime::ParticleResourceOwnership::active());
        assert(smgpc::compat::name_obj_runtime_state_count() == process_objects);
        assert(process.host_heaps()->root_heap().getFreeSize() == process_free);
    }
    renderer.end_frame();
    std::cout << "Original effect ownership: two actual-disc scene cycles passed\n";
}
