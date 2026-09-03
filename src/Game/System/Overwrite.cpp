#include "Game/System/ShapePacketUserData.hpp"
#include "JSystem/J3DGraphBase/J3DFifo.hpp"
#include "JSystem/J3DGraphBase/J3DPacket.hpp"
#include "JSystem/J3DGraphBase/J3DShape.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/JAudio2/JASHeapCtrl.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include "JSystem/JKernel/JKRThread.hpp"
#include <JSystem/JAudio2/JASAudioThread.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>

void JUTTexture::captureDolTexture(void* image, int width, int height, int x, int y, bool mipmap, GXTexFmt format) {
    if (mipmap) {
        GXSetTexCopySrc(x, y, width * 2, height * 2);
    } else {
        GXSetTexCopySrc(x, y, width, height);
    }
    GXSetTexCopyDst(width, height, format, mipmap);
    GXCopyTex(image, GX_FALSE);
    GXPixModeSync();
}

JASAudioThread::JASAudioThread(int stackSize, int msgCount, u32 threadPriority)
    : JKRThread(JASDram, threadPriority, msgCount, stackSize), JASGlobalInstance< JASAudioThread >(true) {
    sbPauseFlag = false;
    OSInitThreadQueue(&sThreadQueue);
}

void J3DShapeMtx::loadMtxIndx_PNGP(int slot, u16 index) const {
    J3DFifoLoadIndx(0x20, index, 0xB000 | static_cast<u16>(slot * 12));
    J3DFifoLoadNrmMtxIndx3x3(index, slot * 3);
    ShapePacketUserData* userData = MR::getJ3DShapePacketUserData(j3dSys.getShapePacket());
    if (userData != nullptr) {
        userData->loadTexMtx(j3dSys.getShapePacket()->getShape()->getMaterial(), slot, index);
    }
}
