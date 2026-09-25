#include "OriginalStageResourceProcessFixture.hpp"
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
#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) aurora::throw_host_exception<std::runtime_error>(message);
}
}

int main() {
    return smgpc::test::run_stage_resource_process("original-image-effect-ownership", [] {
        smgpc::test::on_resource_worker([] {
            require(MR::loadTexFromArc("WaterCameraFilter.arc", "WaterCameraFilter.bti"),
                    "real water filter archive texture");
        });
        const aurora::allocation::ClientAllocationScope game;
        auto& holder = *MR::getSceneObjHolder();
        auto* areas = MR::getAreaObjContainer();
        require(areas && dynamic_cast<ImageEffectAreaMgr*>(areas->getManager("ImageEffectArea")),
                "the actual scene owns its original image-effect area manager");
        auto* system = static_cast<ImageEffectSystemHolder*>(holder.create(SceneObj_ImageEffectSystemHolder));
        auto* bloom = static_cast<BloomEffect*>(holder.create(SceneObj_BloomEffect));
        require(system->mResource->_0 && bloom->_24 == system->mResource->_0 &&
                    system->mResource->_0->getCaptureFlag() && system->mResource->_0->mTIMG == system->mResource->_0->_3C,
                "original bloom borrows the actual image-effect owner's live shared texture");
        require(holder.create(SceneObj_BloomEffect) == bloom &&
                    holder.getObj(SceneObj_ImageEffectSystemHolder) == system,
                "repeated original scene-object queries preserve shared effect ownership");
        holder.create(SceneObj_BloomEffectSimple);
        holder.create(SceneObj_ScreenBlurEffect);
        holder.create(SceneObj_DepthOfFieldBlur);
        auto* water = static_cast<WaterAreaHolder*>(holder.create(SceneObj_WaterAreaHolder));
        require(water->mCameraFilter && water->mCameraFilter->mScreenTex,
                "original water holder owns its actual screen filter and capture texture");
        MR::turnOnNormalBloom();
        system->mDirector->movement();
        bloom->calcAnim();
        require(!system->mDirector->mIsAuto && bloom->isSomething(),
                "original manual director activates the original bloom state");
        MR::forceOffImageEffect();
        require(!bloom->isSomething(), "original force-off clears active bloom state");
        MR::setImageEffectControlAuto();
        require(system->mDirector->mIsAuto, "original auto control state is restored");
    });
}
