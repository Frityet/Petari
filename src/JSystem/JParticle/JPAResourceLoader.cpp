#include "resource/JpcResource.hpp"
#include <stdexcept>
#include "JSystem/JParticle/JPAResourceLoader.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JParticle/JPABaseShape.hpp"
#include "JSystem/JParticle/JPAChildShape.hpp"
#include "JSystem/JParticle/JPADynamicsBlock.hpp"
#include "JSystem/JParticle/JPAExTexShape.hpp"
#include "JSystem/JParticle/JPAExtraShape.hpp"
#include "JSystem/JParticle/JPAFieldBlock.hpp"
#include "JSystem/JParticle/JPAKeyBlock.hpp"
#include "JSystem/JParticle/JPAResource.hpp"
#include "JSystem/JParticle/JPAResourceManager.hpp"

JPAResourceLoader::JPAResourceLoader(u8 const* data, JPAResourceManager* mgr) {
    load_jpc(data, mgr);
}

void JPAResourceLoader::load_jpc(u8 const* data, JPAResourceManager* mgr) {
    if (mgr->mpResArr || mgr->mpTexArr)
        throw std::logic_error("JPAResourceLoader requires an empty manager");
    mgr->mNativeResource = smgpc::resource::resolve_jpc_source(data);
    const auto& decoded = *mgr->mNativeResource;
    JKRHeap* heap = mgr->mpHeap;
    mgr->mResMax = decoded.resources().size();
    mgr->mTexMax = decoded.textures().size();
    mgr->mpResArr = new (heap, 0) JPAResource*[mgr->mResMax];
    mgr->mpTexArr = new (heap, 0) JPATexture*[mgr->mTexMax];
    for (const auto& header : decoded.resources()) {
        JPAResource* res = new (heap, 0) JPAResource();
        res->mFieldBlockNum = header.field_count;
        res->mpFieldBlocks = res->mFieldBlockNum != 0 ? new (heap, 0) JPAFieldBlock*[res->mFieldBlockNum] : NULL;
        res->mKeyBlockNum = header.key_count;
        res->mpKeyBlocks = res->mKeyBlockNum != 0 ? new (heap, 0) JPAKeyBlock*[res->mKeyBlockNum] : NULL;
        res->mTDB1Num = header.texture_reference_count;
        res->mpTDB1 = NULL;
        res->mUsrIdx = header.user_index;
        u32 field_idx = 0;
        u32 key_idx = 0;
        for (const auto& block : header.blocks) {
            const u32 magic = block.tag;
            switch (magic) {
            case 'FLD1':
                res->mpFieldBlocks[field_idx] = new (heap, 0) JPAFieldBlock(block.bytes.data(), heap);
                field_idx++;
                break;
            case 'KFA1':
                res->mpKeyBlocks[key_idx] = new (heap, 0) JPAKeyBlock(block.bytes.data());
                key_idx++;
                break;
            case 'BEM1':
                res->mpDynamicsBlock = new (heap, 0) JPADynamicsBlock(block.bytes.data());
                break;
            case 'BSP1':
                res->mpBaseShape = new (heap, 0) JPABaseShape(block.bytes.data(), heap);
                break;
            case 'ESP1':
                res->mpExtraShape = new (heap, 0) JPAExtraShape(block.bytes.data());
                break;
            case 'SSP1':
                res->mpChildShape = new (heap, 0) JPAChildShape(block.bytes.data());
                break;
            case 'ETX1':
                res->mpExTexShape = new (heap, 0) JPAExTexShape(block.bytes.data());
                break;
            case 'TDB1':
                res->mpTDB1 = (const u16*)(block.bytes.data() + 8);
                break;
            }
        }
        res->init(heap);
        mgr->registRes(res);
    }
    for (const auto& block : decoded.textures()) {
        JPATexture* tex = new (heap, 0) JPATexture(block.bytes.data());
        mgr->registTex(tex);
    }
}
