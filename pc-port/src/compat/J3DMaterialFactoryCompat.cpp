#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

J3DColorBlock* J3DMaterial::createColorBlock(u32 flags) {
    J3DColorBlock* rv = NULL;
    switch (flags) {
    case 0:
        rv = new J3DColorBlockLightOff();
        break;
    case 0x40000000:
        rv = new J3DColorBlockLightOn();
        break;
    case 0x80000000:
        rv = new J3DColorBlockAmbientOn();
        break;
    }

    return rv;
}

J3DTexGenBlock* J3DMaterial::createTexGenBlock(u32 flags) {
    J3DTexGenBlock* rv = NULL;
    switch (flags) {
    case 0x8000000:
        rv = new J3DTexGenBlock4();
        break;
    case 0:
    default:
        rv = new J3DTexGenBlockBasic();
    }

    return rv;
}

J3DTevBlock* J3DMaterial::createTevBlock(int tevStageNum) {
    J3DTevBlock* rv = NULL;
    if (tevStageNum <= 1) {
        rv = new J3DTevBlock1();
    } else if (tevStageNum == 2) {
        rv = new J3DTevBlock2();
    } else if (tevStageNum <= 4) {
        rv = new J3DTevBlock4();
    } else if (tevStageNum <= 16) {
        rv = new J3DTevBlock16();
    }

    return rv;
}

J3DIndBlock* J3DMaterial::createIndBlock(int flags) {
    J3DIndBlock* rv = NULL;
    if (flags != 0) {
        rv = new J3DIndBlockFull();
    } else {
        rv = new J3DIndBlockNull();
    }

    return rv;
}

J3DPEBlock* J3DMaterial::createPEBlock(u32 flags, u32 materialMode) {
    J3DPEBlock* rv = NULL;
    if (flags == 0) {
        if (materialMode & 1) {
            rv = new J3DPEBlockOpa();
            return rv;
        } else if (materialMode & 2) {
            rv = new J3DPEBlockTexEdge();
            return rv;
        } else if (materialMode & 4) {
            rv = new J3DPEBlockXlu();
            return rv;
        }
    }

    if (flags == 0x10000000) {
        rv = new J3DPEBlockFull();
    } else if (flags == 0x20000000) {
        rv = new J3DPEBlockFogOff();
    }

    return rv;
}

u32 J3DMaterial::calcSizeColorBlock(u32 flags) {
    u32 rv = 0;
    switch (flags) {
    case 0:
        rv += sizeof(J3DColorBlockLightOff);
        break;
    case 0x40000000:
        rv += sizeof(J3DColorBlockLightOn);
        break;
    case 0x80000000:
        rv += sizeof(J3DColorBlockAmbientOn);
        break;
    }

    return rv;
}

u32 J3DMaterial::calcSizeTexGenBlock(u32 flags) {
    u32 rv = 0;
    switch (flags) {
    case 0x8000000:
        rv += sizeof(J3DTexGenBlock4);
        break;
    case 0:
    default:
        rv += sizeof(J3DTexGenBlockBasic);
    }

    return rv;
}

u32 J3DMaterial::calcSizeTevBlock(int tevStageNum) {
    u32 rv = 0;
    if (tevStageNum <= 1) {
        rv += sizeof(J3DTevBlock1);
    } else if (tevStageNum == 2) {
        rv += sizeof(J3DTevBlock2);
    } else if (tevStageNum <= 4) {
        rv += sizeof(J3DTevBlock4);
    } else if (tevStageNum <= 16) {
        rv += sizeof(J3DTevBlock16);
    }

    return rv;
}

u32 J3DMaterial::calcSizeIndBlock(int flags) {
    u32 rv = 0;
    if (flags != 0) {
        rv += sizeof(J3DIndBlockFull);
    } else {
        rv += sizeof(J3DIndBlockNull);
    }

    return rv;
}

u32 J3DMaterial::calcSizePEBlock(u32 flags, u32 materialMode) {
    u32 rv = 0;
    if (flags == 0) {
        if (materialMode & 1) {
            rv += sizeof(J3DPEBlockOpa);
        } else if (materialMode & 2) {
            rv += sizeof(J3DPEBlockTexEdge);
        } else if (materialMode & 4) {
            rv += sizeof(J3DPEBlockXlu);
        }
    } else if (flags == 0x10000000) {
        rv += sizeof(J3DPEBlockFull);
    } else if (flags == 0x20000000) {
        rv += sizeof(J3DPEBlockFogOff);
    }

    return rv;
}

