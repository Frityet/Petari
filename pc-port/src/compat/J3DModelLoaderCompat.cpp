#include "compat/J3DModelLoaderCompat.hpp"
#include "resource/J3dModelResource.hpp"
#include "JSystem/J3DGraphLoader/J3DModelLoader.hpp"
#include "JSystem/J3DGraphLoader/J3DShapeFactory.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/JSupport/JSupport.hpp"

namespace smgpc::compat {
    namespace {
        // J3DModelLoader::setupBBoardInfo with its two retained inputs explicit.
        // No synthetic loader instance or alternate billboard algorithm.
        void setup_bboard_info(J3DModelData* mpModelData, const J3DShapeBlock* mpShapeBlock) {
            for (u16 i = 0; i < mpModelData->getJointNum(); i++) {
                J3DMaterial* mesh = mpModelData->getJointNodePointer(i)->getMesh();
                if (mesh != NULL) {
                    u16 shape_index = mesh->getShape()->getIndex();
                    u16* index_table = JSUConvertOffsetToPtr<u16>(mpShapeBlock, (uintptr_t)mpShapeBlock->mpIndexTable);
                    J3DShapeInitData* shape_init_data = JSUConvertOffsetToPtr<J3DShapeInitData>(mpShapeBlock, (uintptr_t)mpShapeBlock->mpShapeInitData);
                    J3DJoint* joint;
                    switch (shape_init_data[index_table[shape_index]].mShapeMtxType) {
                    case 0:
                        joint = mpModelData->getJointNodePointer(i);
                        joint->setMtxType(0);
                        break;
                    case 1:
                        joint = mpModelData->getJointNodePointer(i);
                        joint->setMtxType(1);
                        mpModelData->mbHasBillboard = true;
                        break;
                    case 2:
                        joint = mpModelData->getJointNodePointer(i);
                        joint->setMtxType(2);
                        mpModelData->mbHasBillboard = true;
                        break;
                    case 3:
                        joint = mpModelData->getJointNodePointer(i);
                        joint->setMtxType(0);
                        break;
                    default:
                        OSReport("WRONG SHAPE MATRIX TYPE (__FILE__)\n");
                        break;
                    }
                }
            }
        }
    }

    void finalize_j3d_model(J3DModelData& model, const J3DShapeBlock& shapes, bool binary_display_list) {
        J3DModelHierarchy const* hierarchy = model.getHierarchy();
        model.makeHierarchy(NULL, &hierarchy);
        model.getShapeTable()->sortVcdVatCmd();
        model.getJointTree().findImportantMtxIndex();
        setup_bboard_info(&model, &shapes);
        if (binary_display_list) {
            model.indexToPtr();
        } else if (model.getFlag() & 0x100) {
            for (u16 shape_no = 0; shape_no < model.getShapeNum(); shape_no++) {
                model.getShapeNodePointer(shape_no)->onFlag(0x200);
            }
        }
    }
}

J3DModelData* J3DModelLoaderDataBase::load(const void* data, u32 flags) {
    return smgpc::resource::load_registered_j3d_model(data, flags, false);
}

J3DModelData* J3DModelLoaderDataBase::loadBinaryDisplayList(const void* data, u32 flags) {
    return smgpc::resource::load_registered_j3d_model(data, flags, true);
}

J3DMaterialTable* J3DModelLoaderDataBase::loadMaterialTable(const void* data) {
    return smgpc::resource::load_registered_j3d_material_table(data);
}
