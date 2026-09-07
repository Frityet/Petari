#include "Game/Util/FurShader.hpp"
#include "Game/Util/MathUtil.hpp"
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3DGraphAnimator/J3DModelData.hpp>
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3DGraphBase/J3DShapeDraw.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <cstring>

CShader::CShader(const J3DModelData* pModelData, const ResTIMG* pTimg) : _4(0), mIndexArray(nullptr), mLengthMap(pTimg) {
    _1C = 0.0f;
    _8 = 0.0f;
    mIndexArray = new CIndex[pModelData->mVertexData.mVtxNum];
    _22 = 8;
    _21 = 8;
    _20 = 8;
    _23 = 0;
    _24 = 0;
    _25 = 0;

    GXVtxAttrFmtList* list = pModelData->mVertexData.mVtxAttrFmtList;

    while (list->attr != GX_VA_NULL) {
        if (list->attr == GX_VA_POS) {
            _21 = list->frac;
            _24 = list->type;
        } else if (list->attr == GX_VA_NRM) {
            _22 = list->frac;
            _25 = list->type;
        } else if (list->attr == GX_VA_TEX0) {
            _20 = list->frac;
            _23 = list->type;
        }

        list++;
    }
}

CShader::~CShader() {
    delete mIndexArray;
}

void CShader::calc(J3DModel* pModel) {
    J3DVertexBuffer* pBuffer = pModel->getVertexBuffer();
    pBuffer->swapTransformedVtxPos();
    void* pOutput = pBuffer->getTransformedVtxPos(0);
    void* pPosition = pBuffer->getCurrentVtxPos();
    void* pNormal = pBuffer->getCurrentVtxNrm();
    u32 vertexCount = pBuffer->getVertexData()->getVtxNum();
    void* pTexCoord = pBuffer->getVertexData()->getVtxTexCoordArray(0);
    f32 minS = 0.0f;
    f32 minT = 0.0f;
    f32 maxS = 0.0f;
    f32 maxT = 0.0f;
    f32 s;
    f32 t;

    for (u32 i = 0; i < vertexCount; i++) {
        u16 index = mIndexArray[i]._2;
        if (index == 0xFFFF) {
            continue;
        }

        const TVec2s* pTexFixed = static_cast< const TVec2s* >(pTexCoord) + index;
        const TVec2f* pTexFloat = static_cast< const TVec2f* >(pTexCoord) + index;
        switch (_23) {
        case GX_S16: {
            f32 fraction = 1 << _20;
            s = pTexFixed->x / fraction;
            t = pTexFixed->y / fraction;
            break;
        }
        case GX_F32:
            s = pTexFloat->x;
            t = pTexFloat->y;
            break;
        }

        if (s > maxS) {
            maxS = s;
        }
        if (s < minS) {
            minS = s;
        }
        if (t > maxT) {
            maxT = t;
        }
        if (t < minT) {
            minT = t;
        }
    }

    s32 rangeS = 0.45f + (maxS - minS);
    s32 rangeT = 0.45f + (maxT - minT);
    f32 length;
    for (u32 i = 0; i < vertexCount; i++) {
        u16 normalIndex = mIndexArray[i]._0;
        u16 texIndex = mIndexArray[i]._2;
        if (normalIndex == 0xFFFF || texIndex == 0xFFFF) {
            continue;
        }

        TVec3s* pOutputFixed = static_cast< TVec3s* >(pOutput) + i;
        const TVec3s* pPositionFixed = static_cast< const TVec3s* >(pPosition) + i;
        const TVec3s* pNormalFixed = static_cast< const TVec3s* >(pNormal) + normalIndex;
        const TVec2s* pTexFixed = static_cast< const TVec2s* >(pTexCoord) + texIndex;
        TVec3f* pOutputFloat = static_cast< TVec3f* >(pOutput) + i;
        const TVec3f* pPositionFloat = static_cast< const TVec3f* >(pPosition) + i;
        const TVec3f* pNormalFloat = static_cast< const TVec3f* >(pNormal) + normalIndex;
        const TVec2f* pTexFloat = static_cast< const TVec2f* >(pTexCoord) + texIndex;
        switch (_23) {
        case GX_S16: {
            f32 fraction = 1 << _20;
            s = (pTexFixed->x / fraction) / rangeS;
            t = (pTexFixed->y / fraction) / rangeT;
            if (s < 0.0f) {
                s += 1.0f;
            }
            if (t < 0.0f) {
                t += 1.0f;
            }
            length = mLengthMap.refer(s, t);
            break;
        }
        case GX_F32:
            length = mLengthMap.refer(pTexFloat->x, pTexFloat->y);
            break;
        }

        TVec3f position;
        TVec3f normal;
        TVec3f output;
        switch (_25) {
        case GX_S16:
            MR::fixed16ToFloat(&normal, *pNormalFixed, _22);
            break;
        case GX_F32:
            normal = *pNormalFloat;
            break;
        }
        MR::normalize(&normal);
        MR::isNan(normal);
        switch (_24) {
        case GX_S16:
            MR::fixed16ToFloat(&position, *pPositionFixed, _21);
            break;
        case GX_F32:
            position = *pPositionFloat;
            break;
        }
        MR::isNan(position);
        if (length == 0.0f) {
            output = position + normal * -1.0f;
        } else {
            output = position + normal * (_1C * length);
        }
        MR::isNan(output);
        switch (_24) {
        case GX_S16:
            MR::floatToFixed16(pOutputFixed, output, _21);
            break;
        case GX_F32:
            *pOutputFloat = output;
            break;
        }
    }

    switch (_24) {
    case GX_S16:
        DCStoreRange(pOutput, vertexCount * sizeof(TVec3s));
        break;
    case GX_F32:
        DCStoreRange(pOutput, vertexCount * sizeof(TVec3f));
        break;
    }
    pBuffer->setCurrentVtxPos(pOutput);
}

void CShader::setup(J3DModelData* pData) {
}

void CShader::makeIndexData(J3DShape* pShape) const {
    s32 attrSize[] = {0, 1, 1, 2};
    s32 stride = 0;
    s32 posOffset = -1;
    s32 normalOffset = -1;
    s32 texOffset = -1;
    GXVtxDescList* pDesc = pShape->getVtxDesc();
    while (pDesc->attr != GX_VA_NULL) {
        switch (pDesc->attr) {
        case GX_VA_POS:
            posOffset = stride;
            break;
        case GX_VA_NRM:
            normalOffset = stride;
            break;
        case GX_VA_TEX0:
            texOffset = stride;
            break;
        }
        stride += attrSize[pDesc->type];
        pDesc++;
    }
    if (posOffset == -1 || normalOffset == -1 || texOffset == -1) {
        return;
    }

    for (u16 i = 0; i < pShape->getMtxGroupNum(); i++) {
        const u8* pBegin = pShape->getShapeDraw(i)->getDisplayList();
        const u8* pCommand = pBegin;
        while (pCommand - pBegin < pShape->getShapeDraw(i)->getDisplayListSize()) {
            if (*pCommand == 0) {
                break;
            }
            u16 count = *reinterpret_cast< const u16* >(pCommand + 1);
            for (s32 j = 0; j < count; j++) {
                const u8* pVertex = pCommand + stride * j + 3;
                u16 posIndex = *reinterpret_cast< const u16* >(pVertex + posOffset);
                mIndexArray[posIndex]._0 = *reinterpret_cast< const u16* >(pVertex + normalOffset);
                mIndexArray[posIndex]._2 = *reinterpret_cast< const u16* >(pVertex + texOffset);
            }
            pCommand += stride * count + 3;
        }
    }
}

void CShader::checkBorderVtx(J3DModelData* pModelData, u32 shapeIndex) {
    for (u32 i = 0; i < pModelData->getShapeNum(); i++) {
        if (i == shapeIndex) {
            continue;
        }
        J3DShape* pShape = pModelData->getShapeNodePointer(i);
        if (strstr(pModelData->getMaterialName()->getName(pShape->getMaterial()->getIndex()), "Fur") != nullptr) {
            continue;
        }

        s32 attrSize[] = {0, 1, 1, 2};
        s32 stride = 0;
        s32 posOffset = -1;
        GXVtxDescList* pDesc = pShape->getVtxDesc();
        while (pDesc->attr != GX_VA_NULL) {
            switch (pDesc->attr) {
            case GX_VA_POS:
                posOffset = stride;
                break;
            case GX_VA_NRM:
            case GX_VA_TEX0:
                break;
            }
            stride += attrSize[pDesc->type];
            pDesc++;
        }
        if (posOffset == -1) {
            continue;
        }

        for (u16 j = 0; j < pShape->getMtxGroupNum(); j++) {
            const u8* pBegin = pShape->getShapeDraw(j)->getDisplayList();
            const u8* pCommand = pBegin;
            while (pCommand - pBegin < pShape->getShapeDraw(j)->getDisplayListSize()) {
                if (*pCommand == 0) {
                    break;
                }
                u16 count = *reinterpret_cast< const u16* >(pCommand + 1);
                for (s32 k = 0; k < count; k++) {
                    u16 posIndex = *reinterpret_cast< const u16* >(pCommand + stride * k + posOffset + 3);
                    mIndexArray[posIndex]._0 = 0xFFFF;
                    mIndexArray[posIndex]._2 = 0xFFFF;
                }
                pCommand += stride * count + 3;
            }
        }
    }
}

CShader::CLengthMap::CLengthMap(const ResTIMG* pTimg) {
    _0 = pTimg;
    _4 = reinterpret_cast< const u8* >(pTimg) + 0x20;
    setLengthMap(pTimg);
}

void CShader::CLengthMap::setLengthMap(const ResTIMG* pTimg) {
    if (pTimg == nullptr) {
        _8 = true;
        return;
    }
    if (pTimg->mFormat != GX_TF_I8) {
        _8 = true;
        return;
    }
    _0 = pTimg;
    _4 = reinterpret_cast< const u8* >(pTimg) + 0x20;
    _8 = false;
}

f32 CShader::CLengthMap::refer(f32 s, f32 t) const {
    if (_8) {
        return 1.0f;
    }
    if (_0->mFormat != GX_TF_I8) {
        return 1.0f;
    }
    s32 texelS = getTexelOrder(_0->mWidth, s, static_cast< GXTexWrapMode >(_0->mWrapS));
    s32 texelT = getTexelOrder(_0->mHeight, t, static_cast< GXTexWrapMode >(_0->mWrapT));
    u16 x = texelS;
    u16 y = texelT;
    u32 tile = (static_cast< u32 >(x) / 8) + (_0->mWidth / 8) * (static_cast< u32 >(y) / 4);
    const u8* pTile = _4 + tile * 32;
    s32 offset = (x % 8) + (y % 4) * 8;
    return pTile[offset] / 255.0f;
}

s32 CShader::CLengthMap::getTexelOrder(u16 a1, f32 a2, _GXTexWrapMode mode) const {
    if (a2 > 1.0f) {
        switch (mode) {
        case GX_CLAMP:
            a2 = 1.0f;
            break;
        case GX_REPEAT:
            while (a2 > 1.0f) {
                a2 -= 1.0f;
            }

            while (a2 < 0.0f) {
                a2 += 1.0f;
            }

            break;
        case GX_MIRROR:
            while (a2 > 1.0f) {
                a2 -= 1.0f;
            }

            while (a2 < 0.0f) {
                a2 += 1.0f;
            }
        }
    }

    return (a1 - 1) * a2;
}

CShader::CIndex::CIndex() {
    _0 = -1;
    _2 = -1;
}
