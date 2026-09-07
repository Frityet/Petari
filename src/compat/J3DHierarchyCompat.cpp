#include "JSystem/J3DGraphAnimator/J3DJointTree.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAttach.hpp"
#include "JSystem/J3DGraphAnimator/J3DShapeTable.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"

enum {
    kTypeEnd = 0x00,
    kTypeBeginChild = 0x01,
    kTypeEndChild = 0x02,
    kTypeJoint = 0x10,
    kTypeMaterial = 0x11,
    kTypeShape = 0x12,
};

void J3DJointTree::makeHierarchy(J3DJoint* pJoint, const J3DModelHierarchy** pHierarchy, J3DMaterialTable* pMaterialTable,
                                 J3DShapeTable* pShapeTable) {
    J3DJoint* curJoint = pJoint;

    while (true) {
        J3DJoint* newJoint = NULL;
        J3DMaterial* newMaterial = NULL;
        J3DShape* newShape = NULL;

        switch ((*pHierarchy)->mType) {
        case kTypeBeginChild:
            (*pHierarchy)++;
            makeHierarchy(curJoint, pHierarchy, pMaterialTable, pShapeTable);
            break;
        case kTypeEndChild:
            (*pHierarchy)++;
            return;
        case kTypeEnd:
            return;
        case kTypeJoint:
            newJoint = mJointNodePointer[((*pHierarchy)++)->mValue];
            break;
        case kTypeMaterial:
            newMaterial = pMaterialTable->getMaterialNodePointer(((*pHierarchy)++)->mValue);
            break;
        case kTypeShape:
            newShape = pShapeTable->getShapeNodePointer(((*pHierarchy)++)->mValue);
            break;
        }

        if (newJoint != NULL) {
            curJoint = newJoint;
            if (pJoint == NULL) {
                mRootNode = newJoint;
            } else {
                pJoint->appendChild(newJoint);
            }
        } else if (newMaterial != NULL && pJoint->getType() == 'NJNT') {
            pJoint->addMesh(newMaterial);
            newMaterial->setJoint(pJoint);
        } else if (newShape != NULL && pJoint->getType() == 'NJNT') {
            J3DMaterial* newMaterial = pJoint->getMesh();
            newMaterial->addShape(newShape);
            newShape->setMaterial(newMaterial);
        }
    }
}

