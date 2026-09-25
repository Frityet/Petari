#include "Game/System/DrawBuffer.hpp"
#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/System/ShapePacketUserData.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <cstring>
#include <aurora/exception.hpp>
#include <limits>
#include <memory>
#include <stdexcept>

DrawBufferShapeDrawer::DrawBufferShapeDrawer(J3DMaterial* pMaterial, J3DMatPacket* pMatPacket)
    : mMaterial(pMaterial), mMatPacket(pMatPacket), _8(1), mMaxPackets(0), mNumPackets(0), mPackets(nullptr) {
}

DrawBufferShapeDrawer::~DrawBufferShapeDrawer() {
    for (s32 idx = 0; idx < mMaxPackets; idx++) {
        delete mPackets[idx];
    }
    delete[] mPackets;
}

void DrawBufferShapeDrawer::init(s32 a1) {
    if (mPackets != nullptr || a1 <= 0 || _8 <= 0 || a1 > std::numeric_limits< s32 >::max() / _8) {
        aurora::throw_host_exception< std::logic_error >("Invalid draw shape packet allocation");
    }

    const s32 capacity = a1 * _8;
    std::unique_ptr< PacketInfo*[] > packets(new PacketInfo*[capacity]());
    s32 initialized = 0;
    try {
        for (; initialized < capacity; initialized++) {
            packets[initialized] = new PacketInfo();
        }
    } catch (...) {
        for (s32 idx = 0; idx < initialized; idx++) {
            delete packets[idx];
        }
        throw;
    }
    mPackets = packets.release();
    mMaxPackets = capacity;
}

void DrawBufferShapeDrawer::swap(DrawBufferShapeDrawer* pOther) {
    J3DMaterial* material = mMaterial;
    J3DMatPacket* matPacket = mMatPacket;
    s32 __8 = _8;

    mMaterial = pOther->mMaterial;
    mMatPacket = pOther->mMatPacket;
    _8 = pOther->_8;

    pOther->mMaterial = material;
    pOther->mMatPacket = matPacket;
    pOther->_8 = __8;
}

void DrawBufferShapeDrawer::draw() const {
    mMatPacket->mpDisplayListObj->callDL();

    J3DShapePacket* firstPacket = mPackets[0]->mShapePacket;
    firstPacket->mpShape->loadPreDrawSetting();
    if (MR::getJ3DShapePacketUserData(firstPacket) != nullptr) {
        MR::getJ3DShapePacketUserData(firstPacket)->callDL();
    }

    for (s32 idx = 0; idx < mNumPackets; idx++) {
        J3DShapePacket* packet = mPackets[idx]->mShapePacket;

        if (getLightLoader(idx) != nullptr) {
            getLightLoader(idx)->loadLight();
        }

        if ((packet->mpShape->mFlags & 1) == 0) {
            if (packet->mpDisplayListObj != nullptr) {
                packet->mpDisplayListObj->callDL();
            }
            packet->drawFast();
        }
    }
}

void DrawBufferShapeDrawer::add(const J3DShapePacket* pShapePacket, const ActorLightCtrl* pLightCtrl) {
    if (pShapePacket == nullptr || mNumPackets >= mMaxPackets) {
        aurora::throw_host_exception< std::length_error >("Draw shape packet capacity exceeded");
    }
    bool setLightLoader = false;
    s32 index = findLightSortIndex(pLightCtrl, &setLightLoader);

    PacketInfo* packet = mPackets[mNumPackets];
    packet->mShapePacket = const_cast< J3DShapePacket* >(pShapePacket);
    packet->mLightCtrl = const_cast< ActorLightCtrl* >(pLightCtrl);

    if (setLightLoader) {
        packet->mLightLoader = const_cast< ActorLightCtrl* >(pLightCtrl);
    } else {
        packet->mLightLoader = nullptr;
    }

    for (s32 idx = mNumPackets - 1; idx >= index; idx--) {
        mPackets[idx + 1] = mPackets[idx];
    }

    mPackets[index] = packet;
    mNumPackets++;
}

void DrawBufferShapeDrawer::remove(const J3DShapePacket* pShapePacket) {
    PacketInfo* packet;
    for (s32 removeIdx = 0; removeIdx < mNumPackets; removeIdx++) {
        packet = mPackets[removeIdx];
        if (packet->mShapePacket == pShapePacket) {
            for (s32 idx = removeIdx; idx < mNumPackets - 1; idx++) {
                mPackets[idx] = mPackets[idx + 1];
            }
            if (packet->mLightLoader != nullptr && removeIdx < mNumPackets - 1) {
                mPackets[removeIdx]->mLightLoader = mPackets[removeIdx]->mLightCtrl;
            }
            mNumPackets--;
            mPackets[mNumPackets] = packet;
            return;
        }
    }
}

void DrawBufferShapeDrawer::resetLightSort(const ActorLightCtrl* pLightCtrl) {
    PacketInfo* packet;
    for (s32 idx = 0; idx < mNumPackets; idx++) {
        packet = mPackets[idx];
        if (packet->mLightCtrl == pLightCtrl) {
            J3DShapePacket* shapePacket = packet->mShapePacket;
            remove(shapePacket);
            add(shapePacket, pLightCtrl);
            return;
        }
    }
}

s32 DrawBufferShapeDrawer::findLightSortIndex(const ActorLightCtrl* pLightCtrl, bool* pSetLightLoader) const {
    *pSetLightLoader = false;

    if (pLightCtrl == nullptr) {
        return mNumPackets;
    }

    if (pLightCtrl->_1C != 0) {
        *pSetLightLoader = true;
        return mNumPackets;
    }

    bool foundLight = false;
    for (s32 idx = 0; idx < mNumPackets; idx++) {
        if (mPackets[idx]->mLightLoader != nullptr) {
            if (foundLight) {
                return idx;
            }

            if (pLightCtrl->isSameLight(mPackets[idx]->mLightLoader)) {
                foundLight = true;
            }
        }
    }

    if (!foundLight) {
        *pSetLightLoader = true;
    }
    return mNumPackets;
}

DrawBuffer::DrawBuffer(J3DModel* pModel)
    : mModelData(pModel != nullptr ? pModel->mModelData : nullptr), mModel(pModel), _8(0), mNumActors(0), _10(0), mNumMaterials(0),
      mMaterialNos(nullptr), mNumShapeDrawers(0), mNumOpaShapeDrawers(0), mShapeDrawers(nullptr) {
    if (mModelData == nullptr) {
        aurora::throw_host_exception< std::invalid_argument >("Draw buffer requires an original model");
    }
}

DrawBuffer::~DrawBuffer() {
    clearNativeStorage();
}

void DrawBuffer::clearNativeStorage() noexcept {
    if (mShapeDrawers != nullptr) {
        for (s32 idx = 0; idx < mNumShapeDrawers; idx++) {
            delete mShapeDrawers[idx];
        }
    }
    delete[] mShapeDrawers;
    delete[] mMaterialNos;
    mShapeDrawers = nullptr;
    mMaterialNos = nullptr;
    mNumShapeDrawers = 0;
    mNumOpaShapeDrawers = 0;
    mNumMaterials = 0;
    mNumActors = 0;
}

void DrawBuffer::init(int a1) {
    if (_8 != 0 || a1 <= 0) {
        aurora::throw_host_exception< std::logic_error >("Draw buffer actor storage can only be initialized once");
    }
    _8 = a1;
    try {
        initTable();
    } catch (...) {
        clearNativeStorage();
        _8 = 0;
        throw;
    }
}

void DrawBuffer::add(const LiveActor* pActor) {
    if (pActor == nullptr || mNumActors >= _8) {
        aurora::throw_host_exception< std::length_error >("Draw buffer actor capacity exceeded");
    }
    for (s32 idx = 0; idx < mNumShapeDrawers; idx++) {
        const DrawBufferShapeDrawer* drawer = mShapeDrawers[idx];
        if (drawer->mMaxPackets - drawer->mNumPackets < drawer->_8) {
            aurora::throw_host_exception< std::length_error >("Draw buffer material packet capacity exceeded");
        }
    }
    J3DModel* model = MR::getJ3DModel(pActor);
    ActorLightCtrl* lightCtrl = MR::getLightCtrl(pActor);

    for (s32 idx = 0; idx < mNumMaterials; idx++) {
        getShapeDrawerByMatNo(idx)->add(model->getShapePacket(mModelData->mMaterialTable.getMaterialNodePointer(idx)->getShape()->mIndex), lightCtrl);
    }
    mNumActors++;
}

void DrawBuffer::remove(const LiveActor* pActor) {
    J3DModel* model = MR::getJ3DModel(pActor);

    for (s32 idx = 0; idx < mNumMaterials; idx++) {
        getShapeDrawerByMatNo(idx)->remove(model->getShapePacket(mModelData->mMaterialTable.getMaterialNodePointer(idx)->getShape()->mIndex));
    }
    mNumActors--;
}

void DrawBuffer::resetLightSort(const ActorLightCtrl* pLightCtrl) {
    J3DModel* model = MR::getJ3DModel(pLightCtrl->mActor);

    for (s32 idx = 0; idx < mNumMaterials; idx++) {
        getShapeDrawerByMatNo(idx)->resetLightSort(pLightCtrl);
    }
}

void DrawBuffer::drawOpa() const {
    if (mNumActors > 0) {
        J3DShape::sOldVcdVatCmd = nullptr;
        for (s32 idx = 0; idx < mNumOpaShapeDrawers; idx++) {
            getShapeDrawerByIndex(idx)->draw();
        }
    }
}

void DrawBuffer::drawXlu() const {
    if (mNumActors > 0) {
        J3DShape::sOldVcdVatCmd = nullptr;
        for (s32 idx = mNumOpaShapeDrawers; idx < mNumShapeDrawers; idx++) {
            getShapeDrawerByIndex(idx)->draw();
        }
    }
}

namespace {
    s32 getSortedMaterialIndex(int materialIndex, J3DModel* pModel) {
        J3DMaterial* material;
        material = MR::getMaterial(pModel, materialIndex);
        for (s32 idx = 0; idx < materialIndex; idx++) {
            if (MR::getMaterial(pModel, idx)->getDiffFlag() == material->getDiffFlag()) {
                return idx;
            }
        }
        return -1;
    }
};  // namespace

void DrawBuffer::initTable() {
    mNumMaterials = MR::getMaterialNum(mModel);
    mNumShapeDrawers = mModelData->mMaterialTable.mUniqueMatNum;
    if (mNumShapeDrawers < 0 || mNumShapeDrawers > mNumMaterials) {
        aurora::throw_host_exception< std::logic_error >("Original model has an invalid unique-material count");
    }

    J3DModel* model = mModel;
    s32 idx, numOpaMaterials;
    numOpaMaterials = 0;
    for (idx = 0; idx < MR::getMaterialNum(model); idx++) {
        if (!MR::getMaterial(model, idx)->isDrawModeOpaTexEdge()) {
            if (::getSortedMaterialIndex(idx, model) < 0) {
                numOpaMaterials++;
            }
        }
    }
    mNumOpaShapeDrawers = numOpaMaterials;
    if (mNumOpaShapeDrawers > mNumShapeDrawers) {
        aurora::throw_host_exception< std::logic_error >("Original opaque material count exceeds its drawer table");
    }

    mMaterialNos = new s32[mNumMaterials];
    for (s32 idx = 0; idx < mNumMaterials; idx++) {
        mMaterialNos[idx] = -1;
    }

    mShapeDrawers = new DrawBufferShapeDrawer*[mNumShapeDrawers]();

    for (s32 idx = 0; idx < mNumShapeDrawers; idx++) {
        mShapeDrawers[idx] = nullptr;
    }

    s32 opaIndex = 0;
    s32 xluIndex = 0;
    for (s32 idx = 0; idx < mNumMaterials; idx++) {
        J3DMaterial* material = MR::getMaterial(mModel, idx);
        s32 index = ::getSortedMaterialIndex(idx, mModel);

        if (index >= 0) {
            s32 matNo = mMaterialNos[index];
            if (matNo < 0 || matNo >= mNumShapeDrawers || mShapeDrawers[matNo] == nullptr) {
                aurora::throw_host_exception< std::logic_error >("Original material alias has no draw owner");
            }
            mMaterialNos[idx] = matNo;
            mShapeDrawers[matNo]->_8++;
            continue;
        }

        if (!material->isDrawModeOpaTexEdge()) {
            if (opaIndex >= mNumOpaShapeDrawers) {
                aurora::throw_host_exception< std::length_error >("Original opaque material drawer capacity exceeded");
            }
            mMaterialNos[idx] = opaIndex;
            mShapeDrawers[opaIndex] = new DrawBufferShapeDrawer(material, mModel->getMatPacket(idx));
            opaIndex++;
        } else {
            if (xluIndex + mNumOpaShapeDrawers >= mNumShapeDrawers) {
                aurora::throw_host_exception< std::length_error >("Original translucent material drawer capacity exceeded");
            }
            mMaterialNos[idx] = xluIndex + mNumOpaShapeDrawers;
            mShapeDrawers[xluIndex + mNumOpaShapeDrawers] = new DrawBufferShapeDrawer(material, mModel->getMatPacket(idx));
            xluIndex++;
        }
    }

    if (opaIndex + xluIndex != mNumShapeDrawers) {
        aurora::throw_host_exception< std::logic_error >("Original unique material table does not match its drawers");
    }

    // sort Opa
    for (s32 idx = 0; idx < mNumOpaShapeDrawers - 1; idx++) {
        for (s32 idx2 = idx + 1; idx2 < mNumOpaShapeDrawers; idx2++) {
            sortShapeDrawer(idx, idx2);
        }
    }

    // sort Xlu
    for (s32 idx = mNumOpaShapeDrawers; idx < mNumShapeDrawers - 1; idx++) {
        for (s32 idx2 = idx + 1; idx2 < mNumShapeDrawers; idx2++) {
            sortShapeDrawer(idx, idx2);
        }
    }

    for (s32 idx = 0; idx < mNumShapeDrawers; idx++) {
        mShapeDrawers[idx]->init(_8);
    }
}

void DrawBuffer::sortShapeDrawer(s32 matNo1, s32 matNo2) {
    DrawBufferShapeDrawer* drawerA = getShapeDrawerByIndex(matNo1);
    DrawBufferShapeDrawer* drawerB = getShapeDrawerByIndex(matNo2);
    const char* nameA = MR::getMaterialName(mModelData, drawerA->mMaterial->mIndex);
    const char* nameB = MR::getMaterialName(mModelData, drawerB->mMaterial->mIndex);

    if (strcasecmp(nameA, nameB) > 0) {
        drawerA->swap(drawerB);

        for (s32 idx = 0; idx < mNumMaterials; idx++) {
            if (matNo1 == mMaterialNos[idx]) {
                mMaterialNos[idx] = matNo2;
            } else if (matNo2 == mMaterialNos[idx]) {
                mMaterialNos[idx] = matNo1;
            }
        }
    }
}
