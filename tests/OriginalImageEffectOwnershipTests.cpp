#include "SceneExecutionFixture.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JutTextureAllocation.hpp"
#include "compat/StageSessionState.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/ImageEffectArea.hpp"
#include "Game/Map/WaterAreaHolder.hpp"
#include "Game/Screen/ImageEffectSystemHolder.hpp"
#include "Game/Screen/ImageEffectResource.hpp"
#include "Game/Screen/ImageEffectDirector.hpp"
#include "Game/Screen/BloomEffect.hpp"
#include "Game/Screen/WaterCameraFilter.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool value, const char* message) {
        if (!value) aurora::throw_host_exception<std::runtime_error>(message);
    }
    class Logger final : public smgpc::logging::ILogger {
        void write(std::FILE*, std::source_location, smgpc::logging::Level,
                   smgpc::logging::Category, std::string_view) override {}
    };
    struct Failure {
        SceneObjHolder* holder = nullptr;
        static NameObj* factory(int id, void* context) {
            if (id != SceneObj_SphereSelector) return nullptr;
            auto& holder = *static_cast<Failure*>(context)->holder;
            holder.create(SceneObj_BloomEffect);
            holder.create(SceneObj_ScreenBlurEffect);
            holder.create(SceneObj_DepthOfFieldBlur);
            aurora::throw_host_exception<std::runtime_error>("intentional outer failure after original effect publication");
        }
    };
}

int main() {
    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    require(disc, "SMGPC_REAL_DISC must name the actual game archive");
    smgpc::render::AuroraWindow window({.width=640, .height=456, .title="Original image effect ownership"});
    smgpc::render::AuroraRenderer renderer(window);
    require(aurora_dvd_open(disc), "cannot open the requested disc");
    struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
    DVDInit();
    smgpc::resource::GameResourceRuntime process({96U << 20, 32U << 20, 4U << 20});
    Logger logger;
    smgpc::runtime::RuntimeContext runtime(logger, window, process);
    smgpc::runtime::SceneSchedulerBinding scheduler_binding(runtime.scheduler());
    smgpc::compat::StageSessionState session("Game", "HeavensDoorGalaxy", 1, JMapIdInfo(0, 0));
    smgpc::compat::StageSessionBinding session_binding(session);
    // This independently published archive cache legitimately outlives scenes.
    require(MR::loadTexFromArc("WaterCameraFilter.arc", "WaterCameraFilter.bti"), "real water filter archive texture");
    const auto capacity = process.mem1_heap()->available_bytes();
    const auto registrations = smgpc::compat::name_obj_runtime_state_count();
    const auto scheduled = runtime.scheduler().snapshot().size();
    for (int cycle = 0; cycle < 2; ++cycle) {
        (void)renderer.begin_frame();
        Failure failure;
        {
            smgpc::test::SceneExecutionFixture execution(
                runtime.scheduler(), smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 8U << 20),
                Failure::factory, &failure);
            auto& binding = execution.objects();
            auto& holder = *smgpc::scene::current_scene_obj_holder();
            failure.holder = &holder;
            auto* areas = static_cast<AreaObjContainer*>(holder.create(SceneObj_AreaObjContainer));
            require(dynamic_cast<ImageEffectAreaMgr*>(areas->getManager("ImageEffectArea")), "exact image-effect manager exists with no areas");
            auto* system = static_cast<ImageEffectSystemHolder*>(holder.create(SceneObj_ImageEffectSystemHolder));
            bool rejected = false;
            try { holder.create(SceneObj_SphereSelector); }
            catch (const std::runtime_error&) { rejected = true; }
            require(rejected && !holder.isExist(SceneObj_BloomEffect) && !holder.isExist(SceneObj_DepthOfFieldBlur), "failed factory removes effect slots");
            require(holder.getObj(SceneObj_ImageEffectSystemHolder) == system, "preexisting original resource owner survives rollback");
            require(system->mResource->_0 && smgpc::compat::get_owned_jut_texture(system->mResource->_0->mTIMG) == system->mResource->_0,
                    "partially published shared textures remain live after outer failure");
            const auto cached_capacity = process.mem1_heap()->available_bytes();
            auto* bloom = static_cast<BloomEffect*>(holder.create(SceneObj_BloomEffect));
            require(process.mem1_heap()->available_bytes() == cached_capacity && bloom->_24 == system->mResource->_0,
                    "same-binding retry reuses the original shared textures");
            holder.create(SceneObj_BloomEffectSimple);
            holder.create(SceneObj_ScreenBlurEffect);
            holder.create(SceneObj_DepthOfFieldBlur);
            auto* water = static_cast<WaterAreaHolder*>(holder.create(SceneObj_WaterAreaHolder));
            require(water->mCameraFilter && water->mCameraFilter->mScreenTex && !water->mCamInWater,
                    "original water holder creates its actual screen filter and capture texture");
            TVec3f acceleration(1, 2, 3);
            require(!MR::calcWhirlPoolAccelInfo(TVec3f(0, 0, 0), &acceleration), "empty original accelerator registry reports no water flow");
            MR::turnOnNormalBloom();
            system->mDirector->movement();
            bloom->calcAnim();
            require(!system->mDirector->mIsAuto && bloom->isSomething(), "original manual director activates the original bloom state");
            MR::forceOffImageEffect();
            require(!bloom->isSomething(), "original force-off clears active bloom state");
            MR::setImageEffectControlAuto();
            require(system->mDirector->mIsAuto, "original auto control state is restored");
            binding.init_after_placement();
            execution.complete_initialization();
            binding.complete_initialization();
        }
        renderer.end_frame();
        require(process.mem1_heap()->available_bytes() == capacity, "scene retires all owned capture textures while keeping the independent archive cache");
        require(smgpc::compat::name_obj_runtime_state_count() == registrations && runtime.scheduler().snapshot().size() == scheduled,
                "scene retires all original effect and filter callbacks");
    }
    std::cout << "Original image effects: real archive, shared texture rollback/retry, actual water filter, control states and two scene retirements pass\n";
}
