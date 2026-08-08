#pragma once

#include "JSystem/J3DGraphBase/J3DStruct.hpp"
#include <revolution.h>

class J3DMaterial;
class J3DModel;
class J3DModelData;
class J3DTexMtx;
class ResourceHolder;

class MaterialCtrl {
public:
    MaterialCtrl(J3DModelData*, const char*);

    virtual void update();
    virtual void updateMaterial(J3DMaterial*);

    J3DModelData* mModelData;  // 0x4
    J3DMaterial* mMaterial;    // 0x8
};

class FogCtrl : public MaterialCtrl {
public:
    FogCtrl(J3DModelData*, bool);

    virtual void update() override;

    J3DFogInfo mFogInfo;       // 0xC
    s32 mNumMaterials;         // 0x38
    J3DMaterial** mMaterials;  // 0x3C
};

class MatColorCtrl : public MaterialCtrl {
public:
    MatColorCtrl(J3DModelData*, const char*, u32, const J3DGXColor*);

    virtual void updateMaterial(J3DMaterial*) override;

    u32 mColorChoice;          // 0xC
    const J3DGXColor* mColor;  // 0x10
};

class ViewProjmapEffectMtxSetter : public MaterialCtrl {
public:
    ViewProjmapEffectMtxSetter(J3DModelData*);

    virtual void update() override;

    J3DTexMtxInfo** mMatricies;  // 0xC
    s32 mNumMatricies;           // 0x10
};

class TexMtxCtrl : public MaterialCtrl {
public:
    TexMtxCtrl(J3DModelData*, const char*);

    void setTexMtx(u32, J3DTexMtx*);

    J3DTexMtx* mMatricies[8];  // 0xC
};

class ProjmapEffectMtxSetter : public MaterialCtrl {
public:
    ProjmapEffectMtxSetter(J3DModel*, const ResourceHolder*);

    virtual void update() override;

    void getBaseTrans(TVec3f*) const;
    void updateMtxUseBaseMtx();
    void updateMtxUseBaseMtxWithLocalOffset(const TVec3f&);

    struct UpdateEffectMtxInfo {
        UpdateEffectMtxInfo();

        /* 0x00 */ J3DTexMtxInfo* mTexMtxInfo;
        /* 0x04 */ TPos3f mEffectMtx;
    };

    /* 0x0C */ UpdateEffectMtxInfo* mUpdateEffectMtxInfo;
    /* 0x10 */ s32 mNumUpdateEffectMtxInfo;
    /* 0x14 */ TPos3f mBaseMtx;
    /* 0x44 */ J3DModel* mModel;
};
