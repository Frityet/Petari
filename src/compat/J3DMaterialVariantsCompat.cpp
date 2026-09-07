#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

void J3DPatchedMaterial::initialize() {
    J3DMaterial::initialize();
}

void J3DPatchedMaterial::makeDisplayList() {
}

void J3DPatchedMaterial::makeSharedDisplayList() {
}

void J3DPatchedMaterial::load() {
    j3dSys.setMaterialMode(mMaterialMode);
    if (j3dSys.checkFlag(2)) {
        return;
    }
}

void J3DPatchedMaterial::loadSharedDL() {
    j3dSys.setMaterialMode(mMaterialMode);
    if (!j3dSys.checkFlag(0x02))
        mSharedDLObj->callDL();
}

void J3DPatchedMaterial::reset() {
}

void J3DPatchedMaterial::change() {
}

void J3DLockedMaterial::initialize() {
    J3DMaterial::initialize();
}

void J3DLockedMaterial::makeDisplayList() {
}

void J3DLockedMaterial::makeSharedDisplayList() {
}

void J3DLockedMaterial::load() {
    j3dSys.setMaterialMode(mMaterialMode);
    if (j3dSys.checkFlag(2)) {
        return;
    }
}

void J3DLockedMaterial::loadSharedDL() {
    j3dSys.setMaterialMode(mMaterialMode);
    if (!j3dSys.checkFlag(0x02))
        mSharedDLObj->callDL();
}

void J3DLockedMaterial::patch() {
}

void J3DLockedMaterial::diff(u32 diffFlags) {
}

void J3DLockedMaterial::calc(const Mtx param_0) {
}

void J3DLockedMaterial::reset() {
}

void J3DLockedMaterial::change() {
}
