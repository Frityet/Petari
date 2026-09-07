#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.hpp"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DShape.hpp"
#include "JSystem/J3DGraphBase/J3DPacket.hpp"

void J3DJoint::entryIn() {
    MtxPtr anmMtx = j3dSys.getModel()->getAnmMtx(mJntNo);
    j3dSys.getDrawBuffer(0)->setZMtx(anmMtx);
    j3dSys.getDrawBuffer(1)->setZMtx(anmMtx);
    for (J3DMaterial* mesh = mMesh; mesh != NULL;) {
        if (mesh->getShape()->checkFlag(J3DShpFlag_Visible)) {
            mesh = mesh->getNext();
        } else {
            J3DMatPacket* matPacket = j3dSys.getModel()->getMatPacket(mesh->getIndex());
            J3DShapePacket* shapePacket = j3dSys.getModel()->getShapePacket(mesh->getShape()->getIndex());
            if (!matPacket->isLocked()) {
                if (mesh->getMaterialAnm()) {
                    J3DMaterialAnm* piVar8 = mesh->getMaterialAnm();
                    piVar8->calc(mesh);
                }
                mesh->calc(anmMtx);
            }
            mesh->setCurrentMtx();
            matPacket->setMaterialAnmID(mesh->getMaterialAnm());
            matPacket->setShapePacket(shapePacket);
            J3DDrawBuffer* drawBuffer = j3dSys.getDrawBuffer(mesh->isDrawModeOpaTexEdge());
            if ((u8)matPacket->entry(drawBuffer)) {
                j3dSys.setMatPacket(matPacket);
                J3DDrawBuffer::entryNum++;
                mesh->makeDisplayList();
            }
            mesh = mesh->getNext();
        }
    }
}

