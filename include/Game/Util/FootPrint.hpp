#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "JSystem/JGeometry/TVec.hpp"

class ResTIMG;
class JUTTexture;
class FootPrintInfo;

class FootPrint : public NameObj {
public:
    FootPrint(const char*, s32);
    FootPrint(const char*, s32, s32);
    virtual ~FootPrint();

    virtual void movement();
    virtual void draw() const;

    void setTexture(ResTIMG*);
    bool addPrint(const TVec3f&, const TVec3f&, const TVec3f&, bool);
    void clear();
    void clearForce();
    const TVec3f& getPrintPos(u32) const;
    void invalidate(u32);
    bool isValid(u32) const;

private:
    void initMember(s32, s32);

public:
    JUTTexture* _C;
    FootPrintInfo* _10;
    TVec3f _14;
    s32 _20;
    s32 _24;
    s32 _28;
    f32 _2C;
    f32 _30;
    f32 _34;
    f32 _38;
    u8 _3C;
};
