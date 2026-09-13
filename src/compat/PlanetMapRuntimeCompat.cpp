#include <aurora/exception.hpp>
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/Map/OceanHomeMapCtrl.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "resource/TextEncoding.hpp"

#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {
    // Original generated-name helper; Game names and punctuation retain CP932
    // identity when this compatibility TU is compiled with the host charset.
    const char* createSubModelObjName(const LiveActor* pActor, const char* pSubName) {
        constexpr char brackets[] = "\x81\x69\x81\x6a";
        constexpr char format[] = "%s\x81\x69%s\x81\x6a";
        const auto length = std::strlen(pActor->mName) + std::strlen(pSubName) + std::strlen(brackets) + 1U;
        auto* name = new char[length];
        std::snprintf(name, length, format, pActor->mName, pSubName);
        return name;
    }

    // Original LiveActorUtil owner construction, shared by all submodel kinds.
    PartsModel* createSubModel(LiveActor* pActor, const char* pSubModelName, MtxPtr pMtx, int drawBufferType) {
        const char* modelResName = MR::getModelResourceHolder(pActor)->mModelResTable->getResName(static_cast<u32>(0));
        if (!MR::isExistSubModel(modelResName, pSubModelName)) {
            return nullptr;
        }
        char subModelName[0x100];
        snprintf(subModelName, sizeof(subModelName), "%s%s", modelResName, pSubModelName);
        const char* objName = createSubModelObjName(pActor, pSubModelName);
        PartsModel* parts = new PartsModel(pActor, objName, subModelName, pMtx, drawBufferType, false);
        parts->initWithoutIter();
        MR::tryStartAllAnim(parts, subModelName);
        return parts;
    }
}  // namespace

namespace MR {
    const char* createLowModelObjName(const LiveActor* pActor) {
        return createSubModelObjName(pActor, "Low");
    }

    const char* createMiddleModelObjName(const LiveActor* pActor) {
        return createSubModelObjName(pActor, "Middle");
    }

    PartsModel* createWaterModel(LiveActor* pActor, MtxPtr pMtx) {
        return ::createSubModel(pActor, "Water", pMtx, 8);
    }

    PartsModel* createIndirectPlanetModel(LiveActor* pActor, MtxPtr pMtx) {
        return ::createSubModel(pActor, "Indirect", pMtx, 0x1D);
    }

    ModelObj* createModelObjBloomModel(const char* pName, const char* pModelName, MtxPtr pMtx) {
        ModelObj* pObj = new ModelObj(pName, pModelName, pMtx, MR::DrawBufferType_BloomModel, -2, -2, false);
        pObj->initWithoutIter();
        registerDemoSimpleCastAll(pObj);
        return pObj;
    }

    LodCtrl* createLodCtrlPlanet(LiveActor* pActor, const JMapInfoIter& rIter, f32 farClip, s32 lowModelType) {
        auto owner = std::make_unique<LodCtrl>(pActor, rIter);
        LodCtrl* pLod = owner.get();
        pLod->createLodModel(MR::DrawBufferType_PlanetLow, lowModelType, MR::DrawBufferType_Sky);
        pLod->setDistanceToMiddleAndLow(5000.0f, 10000.0f);
        pLod->setFarClipping(farClip);
        if (pLod->_10 != nullptr) {
            const char* pResName = getModelResourceHolder(pLod->_10)->mModelResTable->getResName(static_cast<u32>(0));
            tryStartAllAnim(pLod->_10, pResName);
        }
        if (pLod->_14 != nullptr) {
            const char* pResName = getModelResourceHolder(pLod->_14)->mModelResTable->getResName(static_cast<u32>(0));
            tryStartAllAnim(pLod->_14, pResName);
        }
        smgpc::compat::adopt_actor_lod_ctrl(pActor, pLod);
        return owner.release();
    }
}  // namespace MR

namespace OceanHomeMapFunction {

    void tryEntryOceanHomeMap(PlanetMap *planet) {
        if (planet == nullptr || planet->mName == nullptr) {
            return;
        }
        const auto name = std::string_view(planet->mName);
        if (name == smgpc::resource::encode_cp932("海洋ホーム惑星") ||
            name == smgpc::resource::encode_cp932("オーシャンリング惑星")) {
            aurora::throw_host_exception<std::logic_error>(
                "OceanHome PlanetMap control is unavailable in the ordinary planet tranche.");
        }
    }

}  // namespace OceanHomeMapFunction
