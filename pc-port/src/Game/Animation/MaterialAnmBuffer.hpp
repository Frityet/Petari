#pragma once

#include <revolution/types.h>

class J3DMaterialAnm;
class J3DModelData;
class J3DAnmColorKey;
class J3DAnmTexPattern;
class J3DAnmTextureSRTKey;
class J3DAnmTevRegKey;
class ResourceHolder;

class MaterialAnmBuffer {
public:
    MaterialAnmBuffer(const ResourceHolder*, J3DModelData*, bool);

    u32 getDiffFlag(s32) const;
    u16 getAllocMaterialAnmNum(J3DModelData*, bool) const;
    void searchUpdateMaterialID(const ResourceHolder*, J3DModelData*);
    void setDiffFlag(const ResourceHolder*);
    u16 getDifferedMaterialNum(const J3DModelData*) const;
    void attachMaterialAnmBuffer(J3DModelData*, bool);

    /* 0x00 */ J3DMaterialAnm* _0;
    /* 0x04 */ u32* _4;
};

namespace MR {
    void onDiffFlagBpk(u32*, const J3DAnmColorKey*, const char*);
    void offDiffFlagBpk(u32*, const J3DAnmColorKey*, const char*);
    void onDiffFlagBtp(u32*, const J3DAnmTexPattern*, const char*);
    void offDiffFlagBtp(u32*, const J3DAnmTexPattern*, const char*);
    void onDiffFlagBtk(u32*, const J3DAnmTextureSRTKey*, const char*);
    void offDiffFlagBtk(u32*, const J3DAnmTextureSRTKey*, const char*);
    void onDiffFlagBrk(u32*, const J3DAnmTevRegKey*, const char*);
    void offDiffFlagBrk(u32*, const J3DAnmTevRegKey*, const char*);
}  // namespace MR
