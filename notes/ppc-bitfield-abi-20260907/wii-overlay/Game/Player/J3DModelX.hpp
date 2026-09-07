#pragma once
#include <aurora/ppc_bitfield.hpp>

#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <revolution/gd/GDBase.h>

class J3DShape;
class J3DShapeX;
class J3DShapePacketX;

class J3DModelX : public J3DModel {
public:
    J3DModelX(J3DModelData*, u32, u32);

    virtual ~J3DModelX();

    void viewCalc2();
    void viewCalc3(u32, MtxPtr);
    bool simpleDrawSetup(J3DMaterial*);
    void simpleDrawShape(J3DMaterial*);
    void storeDisplayList(_GDLObj*, u32);
    void shapePacketDrawFast(J3DShapePacketX*);
    void shapeDrawFast(J3DShapeX*) const;
    void copyExtraMtxBuffer(const J3DModelX*);
    void copyAnmMtxBuffer(const J3DModelX*);
    void swapDrawBuffer(u32);
    void setDynamicDL(u8*, u32);
    void setDrawViewBuffer(MtxPtr);
    void setDrawView(u32);
    void directDraw(J3DModel*);

    struct Flags {
        inline void clear() {
            *(u32*)this = 0;
        }

        AURORA_PPC_BITFIELD_GROUP(unsigned,
            (_0, 1),
            (_1, 1),
            (_2, 1),
            (_3, 1),
            (_4, 1),
            (_5, 1),
            (_6, 1),
            (_7, 1),
            (_8, 1),
            (_9, 1),
            (_A, 1),
            (_B, 1),
            (_C, 1),
            (_D, 1),
            (_E, 1),
            (_F, 1),
            (_10, 1),
            (_11, 1),
            (_12, 1),
            (_13, 1),
            (_14, 1),
            (_15, 1),
            (_16, 1),
            (_17, 1),
            (_18, 1),
            (_19, 1),
            (_1A, 1),
            (_1B, 1),
            (_1C, 1),
            (, 3)
        )
    };

    u8 _DC;
    u8 _DD;
    union {
        struct {
            Mtx* _E0;
            Mtx* _E4;
            Mtx* _E8;
            Mtx* _EC;
            Mtx* _F0;
            Mtx* _F4;
            Mtx* _F8;
            Mtx* _FC;
            Mtx* _100;
            Mtx* _104;
            Mtx* _108;
            Mtx* _10C;
            Mtx* _110;
            Mtx* _114;
            Mtx* _118;
            Mtx* _11C;
            Mtx* _120;
        };
        Mtx* mExtraMtxBuffer[17];
    };
    void (*mShapeCallback)(J3DShape*);
    u32 _128;
    u32 _12C;
    u8 _130[0x1B0 - 0x130];
    Flags mFlags;
    u32 _1B4;
    u8* _1B8;
    u32 _1BC;
    u32 _1C0;
    u32* _1C4;
    u8** _1C8;
    u16* _1CC;
    u8 _1D0;
    f32 _1D4;
    u32 _1D8;
    u32 _1DC;
    s32 _1E0;
    u8 _1E4;
    u8 _1E5;
};
