#include "Game/System/ShapePacketUserData.hpp"
#include "JSystem/J3DGraphBase/J3DFifo.hpp"
#include "JSystem/J3DGraphBase/J3DPacket.hpp"
#include "JSystem/J3DGraphBase/J3DShape.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

void J3DShapeMtx::loadMtxIndx_PNGP(int slot, u16 index) const {
    J3DFifoLoadIndx(0x20, index, 0xB000 | static_cast<u16>(slot * 12));
    J3DFifoLoadNrmMtxIndx3x3(index, slot * 3);
    ShapePacketUserData* userData = MR::getJ3DShapePacketUserData(j3dSys.getShapePacket());
    if (userData != nullptr) {
        userData->loadTexMtx(j3dSys.getShapePacket()->getShape()->getMaterial(), slot, index);
    }
}
