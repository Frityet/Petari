#pragma once

#include "JSystem/JAudio2/JASHeapCtrl.hpp"
#include <revolution/types.h>
#include <aurora/ppc_bitfield.hpp>

struct JAUAudibleParam {
    JAUAudibleParam() {
    }

    JAUAudibleParam(u16 a1, u16 a2) {
        halves.mAudibleSw = a1;
        halves._f1 = a2;
    }

    JAUAudibleParam(u32 id) {
        raw = id;
    }

    operator u32() const {
        return raw;
    }

    u16 getAudibleSw() const {
        return halves.mAudibleSw;
    }

    u8 get_BIT11() const {
        return halves.mAudibleSw >> 11 & 1;
    }

    bool calcVolume() const {
        return halves.mAudibleSw >> 10 & 1;
    }

    bool calcFxMix() const {
        return halves.mAudibleSw >> 9 & 1;
    }

    u8 get_BIT8() const {
        return halves.mAudibleSw >> 8 & 1;
    }

    bool calcPan() const {
        return halves.mAudibleSw >> 7 & 1;
    }

    bool calcDolby() const {
        return halves.mAudibleSw >> 6 & 1;
    }

    bool calcDoppler() const {
        return (raw >> 28 & 0xF) != 0;
    }

    u32 getDoppler() const {
        return (raw >> 28) & 0xf;
        // return raw >> 28 & 0xF;
    }

    f32 getDopplerPower() const {
        JAUAudibleParam param(*this);
        return param.getDoppler() * (1.0f / 15.0f);
    }

    f32 getDopplerPower2() const {
        JAUAudibleParam param(*this);
        return param.getDoppler() * (1.0f / 15.0f);
    }

    u32 getVolDistBit() const {
        return 1 << ((raw >> 20) & 3);
    }

    union {
        u32 raw;
        struct { AURORA_PPC_BITFIELD_GROUP(u32, (mAudibleSw, 16), (_f1, 16)) } halves;
        struct {
            AURORA_PPC_BITFIELD_GROUP(u32, (b0_0, 4), (b0_4, 1), (b0_5, 1), (b0_6, 1), (b0_7, 1),
                                     (b1_0, 1), (b1_1, 1), (mVolDistSetting, 2), (b1_4_7, 4), (b2, 8), (b3, 8))
        } bits;
    };
};
// TODO: these are probably what AudAudible inherits from in AudioLib
template < int SIZE >
class JAUAudible : public JASPoolAllocObject< JAUAudible< SIZE > > {
public:
    JAUAudible() {};
};

template < int SIZE >
class JAUDopplerAudible : public JASPoolAllocObject< JAUDopplerAudible< SIZE > > {
public:
    JAUDopplerAudible() {};
};
