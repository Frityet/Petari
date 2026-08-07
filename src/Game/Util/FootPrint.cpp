#include "Game/Util/FootPrint.hpp"
#include "Game/Util/Color.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <revolution/gx/GXPixel.h>
#include <revolution/gx/GXTev.h>

class FootPrintInfo {
public:
    FootPrintInfo();

    TVec3f mPosition;
    TVec3f mFront;
    TVec3f mNormal;
    s32 mTime;
    u8 mIsValid;
    u8 mIsReverse;
};

FootPrint::FootPrint(const char* pName, s32 printNum, s32 drawType) : NameObj(pName) {
    initMember(printNum, drawType);
}

FootPrint::FootPrint(const char* pName, s32 printNum) : NameObj(pName) {
    initMember(printNum, 0x17);
}

void FootPrint::initMember(s32 printNum, s32 drawType) {
    _C = nullptr;
    _10 = nullptr;
    _20 = 0;
    _24 = 0;
    _28 = 0;
    _2C = 20.0f;
    _30 = 20.0f;
    _34 = 20.0f;
    _38 = 80.0f;
    _14.x = _14.y = _14.z = 0.0f;

    _10 = new FootPrintInfo[printNum];
    _20 = printNum;

    for (s32 i = 0; i < _20; i++) {
        _10[i].mIsValid = false;
    }

    MR::connectToScene(this, 0x22, -1, -1, drawType);
}

FootPrintInfo::FootPrintInfo() {
}

void FootPrint::setTexture(ResTIMG* pTexture) {
    MR::CurrentHeapRestorer heapRestorer(MR::getSceneHeapGDDR3());
    _C = new JUTTexture(pTexture, 0);
}

void FootPrint::movement() {
    if (_3C) {
        for (s32 i = 0; i < _24; i++) {
            if (_10[i].mIsValid) {
                _10[i].mTime--;

                if (_10[i].mTime <= 0) {
                    _10[i].mIsValid = false;
                }
            }
        }

        return;
    }

    for (s32 i = _28; i < _28 + 10; i++) {
        s32 index = i;

        if (index >= _20) {
            index -= _20;
        }

        if (_10[index].mIsValid) {
            _10[index].mTime--;

            if (_10[index].mTime <= 0) {
                _10[index].mIsValid = false;
            }
        }
    }
}

bool FootPrint::addPrint(const TVec3f& rPosition, const TVec3f& rFront, const TVec3f& rNormal, bool isReverse) {
    _3C = false;

    if (_24 > 0 && rPosition.distance(_14) < _38) {
        return false;
    }

    _14 = rPosition;

    _10[_28].mPosition = rPosition;
    _10[_28].mFront = rFront;
    _10[_28].mNormal = rNormal;
    _10[_28].mTime = 60;
    _10[_28].mIsValid = true;
    _10[_28].mIsReverse = isReverse;

    _28++;
    _24++;

    if (_28 >= _20) {
        _28 -= _20;
    }

    if (_24 > _20) {
        _24 = _20;
    }

    return true;
}

void FootPrint::draw() const {
    if (_24 <= 0) {
        return;
    }

    TDDraw::setup(2, 1, 0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    _C->load(GX_TEXMAP0);

    Color8 color(0, 0, 0, 0xFF);

    for (s32 i = 0; i < _24; i++) {
        if (!_10[i].mIsValid) {
            continue;
        }

        f32 alpha = MR::normalize(_10[i].mTime, 0.0f, 60.0f);
        color.a = alpha * 255.0f;
        GXSetTevColor(GX_TEVREG0, color);

        TVec3f side;
        side.cross(_10[i].mFront, _10[i].mNormal);

        TVec3f position;

        if (i & 1) {
            position = _10[i].mPosition + side * _2C;
        } else {
            position = _10[i].mPosition - side * _2C;
        }

        position += _10[i].mNormal * 5.0f;

        TDDraw::drawTexture3D(position, _10[i].mNormal, _10[i].mFront, _34, _30, nullptr, _10[i].mIsReverse, false);
    }

    TDDraw::close();
}

void FootPrint::clear() {
    _3C = true;
}

void FootPrint::clearForce() {
    _24 = 0;
    _28 = 0;
}

const TVec3f& FootPrint::getPrintPos(u32 index) const {
    return _10[index % _20].mPosition;
}

void FootPrint::invalidate(u32 index) {
    _10[index % _20].mIsValid = false;
}

bool FootPrint::isValid(u32 index) const {
    return _10[index % _20].mIsValid;
}

FootPrint::~FootPrint() {
}
