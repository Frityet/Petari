#include "Game/Player/MarioShadow.hpp"

#include "Game/Map/HitInfo.hpp"
#include "Game/Player/J3DModelX.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SchedulerUtil.hpp"
#include <JSystem/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JKernel/JKRSolidHeap.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JUtility/JUTVideo.hpp>
#include <cstring>
#include <revolution/gd/GDBase.h>
#include <revolution/gd/GDGeometry.h>
#include <revolution/gd/GDIndirect.h>
#include <revolution/gd/GDLight.h>
#include <revolution/gd/GDPixel.h>
#include <revolution/gd/GDTev.h>
#include <revolution/gx.h>

namespace {
    const f32 cCheckOffset0 = 5.0f;
    const f32 cDrawOffset0 = -5.0f;
};

CollisionShadow::~CollisionShadow() {
}

void CollisionShadow::setMode(u32 mode) {
    if (_C == mode) {
        return;
    }

    switch (mode) {
    case 0:
        _30C = cCheckOffset0;
        _310 = cDrawOffset0;
        break;
    case 1:
        _30C = 50.0f;
        _310 = -150.0f;
        break;
    case 2:
        _30C = 18.0f;
        _310 = 0.0f;
        break;
    }

    _C = mode;
}

CollisionShadow::CollisionShadow(f32 extent, f32 projectionLength) : NameObj("投影シャドウ") {
    _2F0.zero();
    _2FC = 0;
    _2FE = 0;
    _300 = nullptr;
    _305 = false;
    _30C = 0.0f;
    _310 = 0.0f;
    _314.zero();
    _320.zero();
    _32C = nullptr;
    _330 = nullptr;
    _334 = nullptr;
    _338 = 0;
    _33A = 0;
    _33C = 0;
    _340 = nullptr;
    _344 = nullptr;
    _348 = 0;
    _308 = 0;
    _E = 0;
    _14 = extent;
    _18 = 0.0f;
    _1C = 0.0f;
    _24.zero();
    _30.zero();
    _3C.zero();
    _48.zero();
    _10 = extent;
    _20 = projectionLength;
    _54 = 0;
    _C = 0xFF;
    setMode(0);
    initCaptureTex();

    _58 = 0x80;
    _5A = 3;
    const u32 vertexCount = _58 * _5A;
    _60 = new (0x20) TVec3f[vertexCount];
    _64 = new (0x20) TVec2f[vertexCount];
    _68 = new u16[vertexCount];
    _5C = new u8[_58];
    _6C = 0;
    _32C = new Triangle[_58];
    _330 = new Triangle[2];
    _334 = new Triangle[2];
    _338 = 0;
    _33A = 0;
    _33C = 0;
    _6E = 0;
    _306 = false;
    _304 = false;
    _307 = true;

    for (u32 i = 0; i < 0x80; i++) {
        _70[i] = 0;
    }

    for (u32 i = 0; i < 0x80; i++) {
        _F0[i] = 0;
    }

    createDL();
}

void CollisionShadow::create(const TVec3f& rPosition, const TVec3f& rNormal, const TVec3f& rDirection) {
    TVec3f direction(rDirection);
    TVec3f normal;
    const bool blended = MR::vecBlendSphere(_24, rNormal, &normal, 0.1f);

    if (MR::isNearZero(_24, 0.001f) || !blended || _305) {
        normal = rNormal;
    }

    _305 = false;
    if (!MR::isNearZero(normal, 0.001f)) {
        _24 = normal;
        MR::vecKillElement(direction, normal, &direction);
        MR::normalizeOrZero(&direction);
        if (!MR::isNearZero(direction, 0.001f)) {
            _30 = direction;
        }
    }

    _2F0 = rPosition;
    if (_C == 3) {
        return;
    }

    _E++;
    TVec3f side;
    side.cross(direction, normal);
    if (MR::isNearZero(side, 0.001f)) {
        return;
    }
    MR::normalize(&side);

    TVec3f corners[8];
    for (u32 i = 0; i < 8; i++) {
        corners[i] = rPosition;
        if (i & 1) {
            corners[i] += direction * _10;
        } else {
            corners[i] -= direction * _10;
        }
        if (i & 2) {
            corners[i] += side * _10;
        } else {
            corners[i] -= side * _10;
        }
        if (i & 4) {
            corners[i] -= normal * _30C;
        } else {
            corners[i] += normal * _20;
        }
    }

    TVec3f boundsMin;
    TVec3f boundsMax;
    MR::createBoundingBox(corners, 8, &boundsMin, &boundsMax);
    _314 = boundsMin;
    _320 = boundsMax;

    if (MR::isInitializeStatePlacementSomething()) {
        return;
    }

    u32 triangleCount = MR::createAreaPolygonListArray(_32C, _58, corners, 8);
    for (u32 i = 0; i < _338; i++) {
        _32C[triangleCount++] = _330[i];
    }
    _338 = 0;
    for (u32 i = 0; i < _33A; i++) {
        _32C[triangleCount++] = _334[i];
    }
    _33A = 0;
    _33C = triangleCount;
    _54 = 0;

    if (triangleCount == 0) {
        _10 = _14;
        return;
    }

    _6E = 0;
    u32 acceptedCount = 0;
    for (u32 i = 0; i < triangleCount; i++) {
        TVec3f polygonNormal(*_32C[i].getNormal(0));
        const f32 normalDot = polygonNormal.dot(_24);
        if (normalDot > 0.0f || polygonNormal.dot(MR::getCamZdir()) > 0.707f) {
            continue;
        }

        if (MR::isNearZero(normalDot, 0.15f)) {
            _70[_6E++] = i;
            continue;
        }

        const char* floorCode = MR::getFloorCodeString(&_32C[i]);
        if (floorCode != nullptr && (strcmp(floorCode, "PullBack") == 0 || strcmp(floorCode, "Glass") == 0)) {
            continue;
        }

        _5C[acceptedCount++] = i;
    }
    _54 = acceptedCount;

    const f32 tolerance = 2.0f + _310;
    _6C = 0;
    u32 outputVertex = 0;
    for (u32 polygon = 0; polygon < _54; polygon++) {
        Triangle& triangle = _32C[_5C[polygon]];
        for (u32 vertex = 0; vertex < _5A; vertex++) {
            const TVec3f* position = triangle.getPos(vertex);
            u32 unique = 0;
            for (; unique < _6C; unique++) {
                const TVec3f& candidate = _60[unique];
                if (MR::abs(candidate.x - position->x) < tolerance && MR::abs(candidate.y - position->y) < tolerance &&
                    MR::abs(candidate.z - position->z) < tolerance) {
                    break;
                }
            }

            if (unique == _6C) {
                _60[_6C] = *position + *triangle.getNormal(0) * _310;
                _6C++;
            }

            _68[outputVertex++] = unique;
        }
    }

    for (u32 i = 0; i < _6C; i++) {
        TVec3f relative = _60[i] - rPosition;
        _64[i].x = 0.5f + relative.dot(side) * 0.0033333334f;
        _64[i].y = 0.5f + relative.dot(_30) * 0.0033333334f;
    }

    if (_33C > 40) {
        _10 -= 10.0f;
    } else if (_33C > 10) {
        _10 -= 1.0f;
    } else if (_33C > 20) {
        _10 -= 4.0f;
    } else {
        _10 += 1.0f;
    }

    if (_10 < 1.0f) {
        _10 = 1.0f;
    } else if (_10 > _14) {
        _10 = _14;
    }
}

void CollisionShadow::draw1() const {
    if (_308 & 8) {
        return;
    }

    TDDraw::setup(1, 1, 0);
    GXSetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
    _300->load(GX_TEXMAP0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetAlphaCompare(GX_GREATER, 1, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetDstAlpha(GX_TRUE, 0x40);
    sendVtx();
    GXSetDstAlpha(GX_FALSE, 0);
}

void CollisionShadow::sendVtx() const {
    if (_54 == 0) {
        return;
    }

    GXBegin(_5A == 4 ? GX_QUADS : GX_TRIANGLES, GX_VTXFMT0, static_cast<u16>(_54 * _5A));
    u32 index = 0;
    for (u32 polygon = 0; polygon < _54; polygon++) {
        for (u32 vertex = 0; vertex < _5A; vertex++) {
            const TVec3f& position = getDrawPos(index);
            const TVec2f& texCoord = _64[_68[index]];
            index++;
            GXPosition3f32(position.x, position.y, position.z);
            GXTexCoord2f32(texCoord.x, texCoord.y);
        }
    }
}

void CollisionShadow::sendZsortedVtx(bool addOffset) const {
    GXBegin(_5A == 4 ? GX_QUADS : GX_TRIANGLES, GX_VTXFMT0, static_cast<u16>(_54 * _5A));
    TVec3f offset = -_24;
    offset.scale(5.0f);
    for (u32 polygon = 0; polygon < _54; polygon++) {
        for (u32 vertex = 0; vertex < _5A; vertex++) {
            const TVec3f& sourcePosition = getDrawPos(polygon, vertex);
            const TVec2f& texCoord = getDrawTx(polygon, vertex);
            if (addOffset) {
                TVec3f position = sourcePosition + offset;
                GXPosition3f32(position.x, position.y, position.z);
            } else {
                GXPosition3f32(sourcePosition.x, sourcePosition.y, sourcePosition.z);
            }
            GXTexCoord2f32(texCoord.x, texCoord.y);
        }
    }
}

void CollisionShadow::initCaptureTex() {
    _2FC = 160;
    _2FE = 160;
    {
        MR::CurrentHeapRestorer heapRestorer(MR::getSceneHeapGDDR3());
        _300 = new JUTTexture(_2FC, _2FE, GX_TF_I8);
        MR::zeroMemory(_300->mImage, _2FC * _2FE);
        MR::setMarioShadowTex(_300);
    }

    _300->mWrapS = GX_CLAMP;
    _300->mWrapT = GX_CLAMP;
    _300->mMinType = GX_LINEAR;
    _300->mMagType = GX_LINEAR;
    _300->init();
}

void CollisionShadow::setViewMtx(const TVec3f& rDirection) {
    TVec3f direction(rDirection);
    MR::normalize(&direction);
    TVec3f offset(direction);
    offset.scale(10000.0f);
    TVec3f eye = _2F0 - offset;

    TPos3f view;
    view.identity();
    view.setPositionFromLookAt(eye, _30, _2F0);

    PSMTXCopy(view.mMtx, j3dSys.mViewMtx);
    TDDraw::setViewMtx(view.mMtx);
    MR::setMarioShadowVec(rDirection);
}

void CollisionShadow::setUpdateFlag() {
    if (!MR::isNearZero(_24, 0.001f)) {
        _304 = true;
    }
}

void CollisionShadow::calcView(J3DModelX* model, u32 view, J3DModelX* reference) {
    if (!_304) {
        return;
    }

    _304 = false;
    _306 = true;
    if (_C == 3) {
        Mtx referenceMtx;
        PSMTXCopy(reference->mBaseTransformMtx, referenceMtx);
        if (reference != nullptr) {
            model->viewCalcRefPos(view, reference, _48, _24);
        }
    } else {
        setViewMtx(_24);
        if (reference != nullptr) {
            model->viewCalcRef(view, reference);
        } else {
            model->viewCalc3(view, nullptr);
        }
    }

    if (_C == 2) {
        doSortPolygons();
    }
}

void CollisionShadow::drawAndCaptureTex(J3DModelX* model, const TVec3f&) {
    if (!_306) {
        return;
    }

    if (_C == 3) {
        _340 = model;
        return;
    }

    if (!(_308 & 2)) {
        setViewMtx(_24);
        TDDraw::setup(0, 0, 1);
        Mtx44 projection;
        C_MTXOrtho(projection, -150.0f, 150.0f, 150.0f, -150.0f, 10.0f, 100000.0f);
        GXSetProjection(projection, GX_ORTHOGRAPHIC);

        if (_307) {
            GXSetViewport(608.0f - (_2FC + 32), -32.0f, _2FC + 64.0f, _2FE + 64.0f, 0.0f, 1.0f);
            GXSetScissor(608 - _2FC + 1, 1, _2FC - 2, _2FE - 2);
            model->setDrawView(2);
            *reinterpret_cast<u32*>(&model->mFlags) = 0x4002;
            model->directDraw(nullptr);
        }

        GXSetViewport(608.0f - _2FC, 0.0f, _2FC, _2FE, 0.0f, 1.0f);
        GXSetScissor(608 - _2FC + 1, 1, _2FC - 2, _2FE - 2);
        model->setDrawView(2);
        *reinterpret_cast<u32*>(&model->mFlags) = 2;
        model->directDraw(nullptr);
        *reinterpret_cast<u32*>(&model->mFlags) &= ~2;
    }

    if (!(_308 & 4)) {
        TDDraw::setup(0, 1, 2);
        GXSetColorUpdate(GX_FALSE);
        GXSetAlphaUpdate(GX_TRUE);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTALPHA, GX_BL_ZERO, GX_LO_NOOP);
        GXSetDstAlpha(GX_FALSE, 0);

        TVec3f circlePosition(608.0f - (_2FC >> 1), static_cast<f32>(_2FE >> 1), 0.0f);
        TDDraw::fix2Dpos(&circlePosition);
        TDDraw::drawFillCircle(circlePosition, static_cast<f32>(_2FC >> 1), 0xFFFFFFC0, 0, 16);

        GXRenderModeObj* renderMode = JUTVideo::getManager()->getRenderMode();
        GXSetCopyFilter(GX_FALSE, renderMode->sample_pattern, GX_FALSE, renderMode->vfilter);
        void* image = reinterpret_cast<u8*>(const_cast<ResTIMG*>(_300->mTIMG)) + _300->mTIMG->mImageDataOffset;
        JUTTexture::captureDolTexture(image, _2FC, _2FE, 608 - _2FC, 0, false, GX_CTF_A8);
        GXSetCopyFilter(GX_FALSE, renderMode->sample_pattern, GX_TRUE, renderMode->vfilter);
        GXInvalidateTexAll();
    }

    GXSetAlphaUpdate(GX_FALSE);
    GXSetColorUpdate(GX_TRUE);
    J3DShape::resetVcdVatCache();
    MR::setDefaultViewportAndScissor();
    TDDraw::cameraInit3D();
    GXSetDstAlpha(GX_FALSE, 0);
}

void CollisionShadow::clearAlphaBuffer() {
    TVec2f size(static_cast<f32>(_2FC), static_cast<f32>(_2FE));
    TVec2f position(608.0f - _2FC, 0.0f);
    MR::clearAlphaBuffer(0, position, size);
}

void CollisionShadow::drawVolumeBox(const TVec3f& rPosition, const TVec3f& rSize) const {
    TVec3f volumeAxis(_3C);
    TVec3f side;
    side.cross(_24, volumeAxis);
    MR::normalizeOrZero(&side);

    TDDraw::setup(0, 1, 0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetColorUpdate(GX_FALSE);
    GXSetDstAlpha(GX_FALSE, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXColorS10 color = {255, 255, 255, 255};
    GXSetTevColorS10(GX_TEVREG0, color);

    TVec3f doubledSize = rSize * 2.0f;
    TVec3f sideExtent = side * _1C;
    TVec3f axisExtent = volumeAxis * _18;
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_NOOP);
    GXSetCullMode(GX_CULL_BACK);
    TDDraw::drawFillBox3D(rPosition, doubledSize, sideExtent, axisExtent, 0xFFFFFF01);
    GXSetBlendMode(GX_BM_SUBTRACT, GX_BL_ONE, GX_BL_ONE, GX_LO_NOOP);
    GXSetCullMode(GX_CULL_FRONT);
    TDDraw::drawFillBox3D(rPosition, doubledSize, sideExtent, axisExtent, 0xFFFFFF01);
}

void CollisionShadow::draw() const {
    MR::loadViewMtx();
    MR::loadProjectionMtx();
    switch (_C) {
    case 0:
        draw1();
        break;
    case 1:
        draw2();
        break;
    case 2:
        draw3();
        break;
    case 3:
        drawVolume();
        break;
    }
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_FALSE);
}

void CollisionShadow::draw2() const {
    if (_308 & 8) {
        return;
    }

    TVec3f position = _2F0 - _24 * _30C;
    TVec3f size = _24 * _20;
    drawVolumeBox(position, size);
    TDDraw::setup(1, 1, 0);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetColorUpdate(GX_TRUE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTALPHA, GX_BL_INVDSTALPHA, GX_LO_NOOP);
    GXSetAlphaCompare(GX_GREATER, 1, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetDstAlpha(GX_TRUE, 0);
    _300->load(GX_TEXMAP0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    sendVtx();
    GXSetDstAlpha(GX_FALSE, 0);
}

void CollisionShadow::draw3() const {
    if (_308 & 8) {
        return;
    }

    TVec3f position = _2F0 - _24 * _30C;
    TVec3f size = _24 * _20;
    drawVolumeBox(position, size);
    TDDraw::setup(1, 1, 0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetColorUpdate(GX_FALSE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, GX_LO_NOOP);
    GXSetAlphaCompare(GX_EQUAL, 0, GX_AOP_AND, GX_EQUAL, 0);
    GXSetDstAlpha(GX_TRUE, 0);
    _300->load(GX_TEXMAP0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    sendZsortedVtx(true);

    if (_6E != 0) {
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXBegin(GX_TRIANGLES, GX_VTXFMT0, static_cast<u16>(_6E * 3));
        for (u32 polygon = 0; polygon < _6E; polygon++) {
            for (u32 vertex = 0; vertex < 3; vertex++) {
                const TVec3f* vertexPosition = _32C[_70[polygon]].getPos(vertex);
                GXPosition3f32(vertexPosition->x, vertexPosition->y, vertexPosition->z);
                GXTexCoord2f32(-1.0f, -1.0f);
            }
        }
    }

    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetColorUpdate(GX_TRUE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTALPHA, GX_BL_INVDSTALPHA, GX_LO_NOOP);
    GXSetAlphaCompare(GX_NEQUAL, 0, GX_AOP_AND, GX_NEQUAL, 0);
    GXSetMisc(GX_MT_XF_FLUSH, GX_XF_FLUSH_SAFE);

    for (u32 polygon = 0; polygon < _54; polygon++) {
        GXSetColorUpdate(GX_FALSE);
        GXSetDstAlpha(GX_FALSE, 0);
        GXBegin(_5A == 4 ? GX_QUADS : GX_TRIANGLES, GX_VTXFMT0, _5A);
        for (u32 vertex = 0; vertex < _5A; vertex++) {
            const TVec3f& vertexPosition = getDrawPos(polygon, vertex);
            const TVec2f& texCoord = getDrawTx(polygon, vertex);
            GXPosition3f32(vertexPosition.x, vertexPosition.y, vertexPosition.z);
            GXTexCoord2f32(texCoord.x, texCoord.y);
        }

        GXSetColorUpdate(GX_TRUE);
        GXSetDstAlpha(GX_TRUE, 0);
        GXBegin(_5A == 4 ? GX_QUADS : GX_TRIANGLES, GX_VTXFMT0, _5A);
        for (u32 vertex = 0; vertex < _5A; vertex++) {
            const TVec3f& vertexPosition = getDrawPos(polygon, vertex);
            const TVec2f& texCoord = getDrawTx(polygon, vertex);
            GXPosition3f32(vertexPosition.x, vertexPosition.y, vertexPosition.z);
            GXTexCoord2f32(texCoord.x, texCoord.y);
        }
    }

    GXSetMisc(GX_MT_XF_FLUSH, GX_XF_FLUSH_NONE);
    GXSetDstAlpha(GX_FALSE, 0);
}

void CollisionShadow::createDL() {
    MR::ProhibitSchedulerAndInterrupts prohibit(false);
    u8 buffer[0x200] ATTRIBUTE_ALIGN(32);
    GDLObj displayList;
    GDInitGDLObj(&displayList, buffer, sizeof(buffer));
    __GDCurrentDL = &displayList;

    GDSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
    GDSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GDSetTevDirect(GX_TEVSTAGE0);
    GDSetGenMode(0, 1, 1);
    GDSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GDSetTevAlphaCalcAndSwap(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE,
                           GX_TEVPREV, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GXColor color = {0, 0, 0, 4};
    GDSetTevColor(GX_TEVREG0, color);
    GDPadCurr32();

    const u32 usedSize = displayList.ptr - displayList.start;
    _348 = usedSize;
    const u32 alignedSize = (usedSize + 31) & ~31;
    _344 = new (0x20) u8[alignedSize];
    MR::copyMemory(_344, buffer, alignedSize);
    DCStoreRange(_344, alignedSize);
}

void CollisionShadow::drawVolume() const {
    if (_308 & 8) {
        return;
    }

    _340->setDrawView(2);
    TVec3f position = _2F0 - _24 * _30C;
    TVec3f size = _24 * _20;
    drawVolumeBox(position, size);
    *reinterpret_cast<u32*>(&_340->mFlags) = 0x400;
    _340->directDraw(nullptr);
    GXSetDstAlpha(GX_FALSE, 0);
}

void CollisionShadow::doSortPolygons() {
    f32 distances[0x80];
    for (u32 i = 0; i < _54; i++) {
        TVec3f center(getDrawPos(0));
        center += getDrawPos(1);
        center += getDrawPos(2);
        center.scale(0.33333334f);
        center -= _2F0;
        distances[i] = center.dot(_24);
    }
    MR::sortSmall(_54, distances, _F0);
}

const TVec3f& CollisionShadow::getDrawPos(u32 index) const {
    return _60[_68[index]];
}

const TVec3f& CollisionShadow::getDrawPos(u32 polygon, u32 vertex) const {
    return _60[_68[vertex + _5A * _F0[polygon]]];
}

const TVec2f& CollisionShadow::getDrawTx(u32 polygon, u32 vertex) const {
    return _64[_68[vertex + _5A * _F0[polygon]]];
}
