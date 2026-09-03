#include "Game/Animation/MaterialAnmBuffer.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"

template < typename T >
void modifyDiffFlag(u32* pFlags, const T* pAnimation, J3DDiffFlag flag, bool enable, const char*, const char*) {
    for (u16 i = 0; i < pAnimation->getUpdateMaterialNum(); i++) {
        u16 materialID = pAnimation->getUpdateMaterialID(i);
        if (materialID != 0xFFFF) {
            if (enable) {
                pFlags[materialID] |= flag;
            } else {
                pFlags[materialID] &= ~flag;
            }
        }
    }
}

void modifyDiffFlagBrk(u32* pFlags, const J3DAnmTevRegKey* pAnimation, bool enable, const char*) {
    for (u16 i = 0; i < pAnimation->getCRegUpdateMaterialNum(); i++) {
        u16 materialID = pAnimation->getCRegUpdateMaterialID(i);
        if (materialID != 0xFFFF) {
            if (enable) {
                pFlags[materialID] |= J3DDiffFlag_TevReg;
            } else {
                pFlags[materialID] &= ~J3DDiffFlag_TevReg;
            }
        }
    }

    for (u16 i = 0; i < pAnimation->getKRegUpdateMaterialNum(); i++) {
        u16 materialID = pAnimation->getKRegUpdateMaterialID(i);
        if (materialID != 0xFFFF) {
            if (enable) {
                pFlags[materialID] |= J3DDiffFlag_TevReg;
            } else {
                pFlags[materialID] &= ~J3DDiffFlag_TevReg;
            }
        }
    }
}

MaterialAnmBuffer::MaterialAnmBuffer(const ResourceHolder* pResourceHolder, J3DModelData* pModelData, bool differedOnly)
    : _0(nullptr), _4(nullptr) {
    searchUpdateMaterialID(pResourceHolder, pModelData);
    u16 materialNum = pModelData->getMaterialNum();
    if (differedOnly) {
        _4 = new u32[materialNum];
        MR::zeroMemory(_4, materialNum * sizeof(u32));
        setDiffFlag(pResourceHolder);
    }

    u16 allocNum = getAllocMaterialAnmNum(pModelData, differedOnly);
    _0 = new J3DMaterialAnm[allocNum];
    attachMaterialAnmBuffer(pModelData, differedOnly);
}

u32 MaterialAnmBuffer::getDiffFlag(s32 index) const {
    return _4[index];
}

u16 MaterialAnmBuffer::getAllocMaterialAnmNum(J3DModelData* pModelData, bool differedOnly) const {
    if (differedOnly) {
        return getDifferedMaterialNum(pModelData);
    }

    return pModelData->getMaterialNum();
}

void MaterialAnmBuffer::searchUpdateMaterialID(const ResourceHolder* pResourceHolder, J3DModelData* pModelData) {
    ResTable* table = pResourceHolder->mBpkResTable;
    for (u32 i = 0; i < table->mCount; i++) {
        static_cast< J3DAnmColorKey* >(table->getRes(i))->searchUpdateMaterialID(pModelData);
    }

    table = pResourceHolder->mBtpResTable;
    for (u32 i = 0; i < table->mCount; i++) {
        static_cast< J3DAnmTexPattern* >(table->getRes(i))->searchUpdateMaterialID(pModelData);
    }

    table = pResourceHolder->mBtkResTable;
    for (u32 i = 0; i < table->mCount; i++) {
        static_cast< J3DAnmTextureSRTKey* >(table->getRes(i))->searchUpdateMaterialID(pModelData);
    }

    table = pResourceHolder->mBrkResTable;
    for (u32 i = 0; i < table->mCount; i++) {
        static_cast< J3DAnmTevRegKey* >(table->getRes(i))->searchUpdateMaterialID(pModelData);
    }
}

void MaterialAnmBuffer::setDiffFlag(const ResourceHolder* pResourceHolder) {
    ResTable* table = pResourceHolder->mBpkResTable;
    for (u32 i = 0; i < table->mCount; i++) {
        const char* name = table->getResName(i);
        MR::onDiffFlagBpk(_4, static_cast< J3DAnmColorKey* >(table->getRes(i)), name);
    }

    table = pResourceHolder->mBtpResTable;
    for (u32 i = 0; i < table->mCount; i++) {
        const char* name = table->getResName(i);
        MR::onDiffFlagBtp(_4, static_cast< J3DAnmTexPattern* >(table->getRes(i)), name);
    }

    table = pResourceHolder->mBtkResTable;
    for (u32 i = 0; i < table->mCount; i++) {
        const char* name = table->getResName(i);
        MR::onDiffFlagBtk(_4, static_cast< J3DAnmTextureSRTKey* >(table->getRes(i)), name);
    }

    table = pResourceHolder->mBrkResTable;
    for (u32 i = 0; i < table->mCount; i++) {
        const char* name = table->getResName(i);
        MR::onDiffFlagBrk(_4, static_cast< J3DAnmTevRegKey* >(table->getRes(i)), name);
    }
}

u16 MaterialAnmBuffer::getDifferedMaterialNum(const J3DModelData* pModelData) const {
    u16 materialNum = pModelData->getMaterialNum();
    u16 count = 0;
    for (u16 i = 0; i < materialNum; i++) {
        if (_4[i] != 0) {
            count++;
        }
    }
    return count;
}

void MaterialAnmBuffer::attachMaterialAnmBuffer(J3DModelData* pModelData, bool differedOnly) {
    u32 animationIndex = 0;
    for (u16 i = 0; i < pModelData->getMaterialNum(); i++) {
        if (!differedOnly || _4[i] != 0) {
            pModelData->getMaterialNodePointer(i)->mMaterialAnm = &_0[animationIndex++];
        }
    }
}

namespace MR {
    void onDiffFlagBpk(u32* pFlags, const J3DAnmColorKey* pAnimation, const char* pName) {
        modifyDiffFlag(pFlags, pAnimation, J3DDiffFlag_MatColor, true, pName, "bpk");
    }

    void offDiffFlagBpk(u32* pFlags, const J3DAnmColorKey* pAnimation, const char* pName) {
        modifyDiffFlag(pFlags, pAnimation, J3DDiffFlag_MatColor, false, pName, "bpk");
    }

    void onDiffFlagBtp(u32* pFlags, const J3DAnmTexPattern* pAnimation, const char* pName) {
        modifyDiffFlag(pFlags, pAnimation, static_cast< J3DDiffFlag >(0x20000), true, pName, "btp");
    }

    void offDiffFlagBtp(u32* pFlags, const J3DAnmTexPattern* pAnimation, const char* pName) {
        modifyDiffFlag(pFlags, pAnimation, static_cast< J3DDiffFlag >(0x20000), false, pName, "btp");
    }

    void onDiffFlagBtk(u32* pFlags, const J3DAnmTextureSRTKey* pAnimation, const char* pName) {
        modifyDiffFlag(pFlags, pAnimation, static_cast< J3DDiffFlag >(0x200), true, pName, "btk");
    }

    void offDiffFlagBtk(u32* pFlags, const J3DAnmTextureSRTKey* pAnimation, const char* pName) {
        modifyDiffFlag(pFlags, pAnimation, static_cast< J3DDiffFlag >(0x200), false, pName, "btk");
    }

    void onDiffFlagBrk(u32* pFlags, const J3DAnmTevRegKey* pAnimation, const char* pName) {
        modifyDiffFlagBrk(pFlags, pAnimation, true, pName);
    }

    void offDiffFlagBrk(u32* pFlags, const J3DAnmTevRegKey* pAnimation, const char* pName) {
        modifyDiffFlagBrk(pFlags, pAnimation, false, pName);
    }
}  // namespace MR
