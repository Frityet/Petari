#include "Game/MapObj/DynamicCollisionObj.hpp"
#include "resource/KCollisionResource.hpp"
#include <aurora/allocation.hpp>
#include <memory>
#include <vector>
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util.hpp"

DynamicCollisionObj::DynamicCollisionObj(const char* pName) : LiveActor(pName) {
    _A4 = 0;
    _9C = 0;
    _11C = 0;
    _A8 = 0;
    _AC = 0;
}

void DynamicCollisionObj::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    MR::connectToScene(this, MR::MovementType_Enemy, MR::CalcAnimType_Enemy, -1, MR::DrawType_FlexibleSphere);

    mKCLFile = nullptr;
}

void DynamicCollisionObj::syncCollision() {
    updateCollisionHeader();
    updateTriangle();
    mParts->mServer->calcFarthestVertexDistance();
    mParts->updateBoundingSphereRange(mScale);
}

void DynamicCollisionObj::updateTriangle() {
    s32 nrmidxa, nrmidxb, nrmidxc;
    s32 prismidx;
    s32 normalidx = 0;

    for (u16 i = 0; i < _94; i++) {
        TVec3f Vertex1(mPositions[mIndices[i].mIndex[0]]);
        TVec3f Vertex2(mPositions[mIndices[i].mIndex[1]]);
        TVec3f Vertex3(mPositions[mIndices[i].mIndex[2]]);

        TVec3f a(Vertex3);
        a -= Vertex1;
        TVec3f SaveForLater(a);
        TVec3f b(Vertex2);
        b -= Vertex1;
        TVec3f c(Vertex3);
        c -= Vertex2;
        MR::normalizeOrZero(&a);
        MR::normalizeOrZero(&b);
        MR::normalizeOrZero(&c);
        TVec3f cross = b.cross(a);
        MR::normalizeOrZero(&cross);
        _9C[i] = cross;

        if (!mKCLFile) {
            continue;
        }

        TVec3f cross2 = (-a).cross(cross);
        TVec3f cross3 = b.cross(cross);
        TVec3f cross4 = c.cross(cross);

        MR::normalizeOrZero(&cross2);
        MR::normalizeOrZero(&cross3);
        MR::normalizeOrZero(&cross4);

        mKCLFile->mPos[i] = Vertex1;
        mKCLFile->mNorms[normalidx + 0] = cross;
        nrmidxa = normalidx + 1;
        mKCLFile->mNorms[nrmidxa] = cross2;
        nrmidxb = normalidx + 2;
        mKCLFile->mNorms[nrmidxb] = cross3;
        nrmidxc = normalidx + 3;
        mKCLFile->mNorms[nrmidxc] = cross4;

        prismidx = i + 1;
        KC_PrismData* prism = &mKCLFile->mPrisms[prismidx];
        MR::vecKillElement(SaveForLater, c, &SaveForLater);

        prism->mHeight = SaveForLater.length();
        prism->mPositionIndex = i;
        u16 nrmidx = normalidx;
        prism->mNormalIndex = nrmidx;
        nrmidx += 4;
        prism->mEdgeIndices[0] = nrmidxa;
        prism->mEdgeIndices[1] = nrmidxb;
        prism->mEdgeIndices[2] = nrmidxc;
        prism->mAttribute = prismidx;
        normalidx = nrmidx;
    }
}

void DynamicCollisionObj::updateCollisionHeader() {
    // Compute bounding box and extent
    TVec3f min, max;
    MR::createBoundingBox(this->mPositions, this->mPositionNum, &min, &max);
    TVec3f extent(max);
    extent -= min;

    // Initialize masks with extent
    u32 masks[3];
    masks[0] = (s32)extent.x;
    masks[1] = (s32)extent.y;
    masks[2] = (s32)extent.z;
    if (!masks[0]) {
        masks[0] = 1;
    }
    if (!masks[1]) {
        masks[1] = 1;
    }
    if (!masks[2]) {
        masks[2] = 1;
    }

    // Find highest number of bits to shift
    u32 max_entropy = 0;
    for (u32 component = 0; component < 3; component++) {
        u32 bits_sel = 0x80000000;
        u32 mask_sel = 0xFFFFFFFF;
        u32 val = masks[component];
        u32 i = 0;

        for (u32 bit = 0; bit < 32; bit++) {
            if ((val & bits_sel)) {
                masks[component] = ~mask_sel;
                if (max_entropy < i) {
                    max_entropy = i;
                }
                break;
            }

            bits_sel >>= 1;
            mask_sel >>= 1;
            i++;
        }
    }

    // Update header fields
    u32 bit_shift = 33 - max_entropy;
    u32 area_y_width_mask = masks[1];
    this->mKCLFile->mXMask = masks[0];
    u32 area_z_width_mask = masks[2];
    this->mKCLFile->mYMask = area_y_width_mask;
    this->mKCLFile->mZMask = area_z_width_mask;
    this->mKCLFile->mBlockWidthShift = bit_shift;
    this->mKCLFile->mMin = min;
}

namespace {
    struct GeneratedCollisionAllocation {
        KCLFile file{};
        std::unique_ptr<TVec3f[]> positions;
        std::unique_ptr<TVec3f[]> normals;
        std::unique_ptr<KC_PrismData[]> prisms;
        std::vector<u16> octree;
    };
}

void DynamicCollisionObj::createCollision() {
    const s32 v = _94;
    std::shared_ptr<GeneratedCollisionAllocation> allocation;
    {
        const aurora::allocation::HostAllocationScope host;
        allocation = std::make_shared<GeneratedCollisionAllocation>();
        allocation->positions = std::make_unique<TVec3f[]>(v);
        allocation->normals = std::make_unique<TVec3f[]>(v * 4);
        allocation->prisms = std::make_unique<KC_PrismData[]>(v + 1);
        allocation->octree.resize(v + 3);
    }
    mKCLFile = &allocation->file;
    mKCLFile->mPos = allocation->positions.get();
    mKCLFile->mNorms = allocation->normals.get();
    mKCLFile->mPrisms = allocation->prisms.get();
    auto* u16array = allocation->octree.data();
    mKCLFile->mOctree = u16array;

    u16array[0] = 0x8000;
    u16array[1] = 2;
    s32 count = v;
    for (s32 i = 0; i <= v; i++) {
        u16array[i + 2] = count--;
    }

    mKCLFile->mThickness = 40.f;
    mKCLFile->mBlockXShift = -1;
    mKCLFile->mBlockXYShift = -1;
    updateCollisionHeader();
    updateTriangle();

    std::shared_ptr<smgpc::resource::GeneratedKCollisionResource> resource;
    {
        const aurora::allocation::HostAllocationScope host;
        resource = std::make_shared<smgpc::resource::GeneratedKCollisionResource>(
            *mKCLFile, std::span(mKCLFile->mPos, v), std::span(mKCLFile->mNorms, v * 4),
            std::span(mKCLFile->mPrisms, v + 1), allocation->octree, allocation);
    }
    TPos3f posmtx;
    PSMTXTrans(posmtx, mPosition.x, mPosition.y, mPosition.z);
    const aurora::allocation::ClientAllocationScope client;
    auto parts = std::make_unique<CollisionParts>();
    parts->initFromGeneratedResource(std::move(resource), getSensor("body"), posmtx, 0);
    mParts = adoptCollisionParts(std::move(parts));
    MR::validateCollisionParts(mParts);
    mParts->mServer->calcFarthestVertexDistance();
    mParts->updateBoundingSphereRange(TVec3f(mScale));
}
