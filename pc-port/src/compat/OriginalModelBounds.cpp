#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"
#include <cstdio>
namespace MR {
    bool isExistEffectTexMtx(J3DModelData* pModelData) {
        for (u16 i = 0; i < pModelData->getMaterialNum(); i++) {
            if (!isNormalTexMtx(pModelData->getMaterialNodePointer(i))) {
                return true;
            }
        }

        return false;
    }

    bool isExistEffectTexMtx(LiveActor* pActor) {
        return isExistEffectTexMtx(getJ3DModelData(pActor));
    }

    void calcModelBoundingBox(TBox3f* pOut, const LiveActor* pActor) {
        s32 jointNum = getJointNum(pActor);
        for (u16 i = 0; i < jointNum; i++) {
            J3DJoint* joint = getJoint(pActor, i);
            TVec3f jointMin(*joint->getMin());
            TVec3f jointMax(*joint->getMax());
            TPos3f jointMtx(getJointMtx(pActor, i));
            TVec3f yDir;
            jointMtx.getYDir(yDir);

            TVec3f sqrtVec;
            sqrtVec.x = JGeometry::TUtil< f32 >::sqrt(jointMtx.dotX());
            sqrtVec.y = JGeometry::TUtil< f32 >::sqrt(jointMtx.dotY());
            sqrtVec.z = JGeometry::TUtil< f32 >::sqrt(jointMtx.dotZ());

            jointMin.mul(sqrtVec);
            jointMax.mul(sqrtVec);

            TVec3f temp1(yDir);
            yDir.add(jointMin);
            TVec3f temp2(yDir);
            yDir.add(jointMax);
            TVec3f extend1;
            TVec3f extend2;
            extend1.set(temp1);
            extend2.set(temp2);
            if (i == 0) {
                pOut->set(extend1, extend2);
                continue;
            }

            pOut->extend(jointMin, jointMax);
        }

        TVec3f minusPos(-pActor->mPosition);
        pOut->i.add(minusPos);
        pOut->f.add(minusPos);
    }

    void calcModelBoundingRadius(f32* pOut, const LiveActor* pActor) {
        MR::getJointNum(pActor);
        TBox3f boundingBox;
        calcModelBoundingBox(&boundingBox, pActor);
        TVec3f i = boundingBox.i;
        TVec3f f = boundingBox.f;

        TVec3f max;
        max.set(MR::max(__fabsf(i.x), __fabsf(f.x)), MR::max(__fabsf(i.y), __fabsf(f.y)), MR::max(__fabsf(f.z), __fabsf(i.z)));

        *pOut = max.length();
    }

    bool isExistCollisionResource(const LiveActor* pActor, const char* pName) {
        char buff[0x80];
        snprintf(buff, sizeof(buff), "%s.kcl", pName);
        return getResourceHolder(pActor)->mFileInfoTable->findFileInfo(buff) != nullptr;
    }

    bool isExistIndirectTexture(const LiveActor* pActor) {
        const char* name = "IndDummy";
        return MR::getJ3DModelData(pActor)->mMaterialTable.mTextureName->getIndex(name) != -1;
    }
}
