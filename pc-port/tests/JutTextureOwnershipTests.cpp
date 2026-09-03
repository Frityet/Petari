#include "compat/JutTextureAllocation.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "resource/Mem1ResourceHeap.hpp"
#include "render/RendererService.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <dolphin/gd.h>
#include <dolphin/os.h>
#include <array>
#include <source_location>
#include <string>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool condition, std::source_location location = std::source_location::current()) {
    if (!condition) throw std::runtime_error(std::string(location.file_name()) + ":" +
                                             std::to_string(location.line()) + ": condition failed");
}

int main() {
    smgpc::render::AuroraWindow window({.width=640,.height=456,.title="Owned JUTTexture MEM1 lifetime"});
    smgpc::render::AuroraRenderer renderer(window);
    OSInit();
    auto heap=smgpc::resource::Mem1ResourceHeap::create(4U*1024U*1024U);
    const auto available=heap->available_bytes();
    auto jkr=smgpc::compat::JkrHeapRuntime::create(2U*1024U*1024U);
    std::unique_ptr<JUTTexture> retained;
    std::shared_ptr<smgpc::compat::JkrAllocationDomain> retired_service_domain;
    std::size_t retained_capacity=0;
    (void)renderer.begin_frame();
    {
        smgpc::compat::JutTextureAllocationService provider(heap);
        retained=std::make_unique<JUTTexture>(8,8,GX_TF_RGB565);
        require(smgpc::compat::get_owned_jut_texture(retained->mTIMG)==retained.get());
        require(retained->getCaptureFlag());
        require(retained->mTIMG==retained->_3C);
        require(retained->mImage==reinterpret_cast<const u8*>(retained->mTIMG)+sizeof(ResTIMG));
        require((reinterpret_cast<std::uintptr_t>(retained->mTIMG)&31U)==0);
        require((reinterpret_cast<std::uintptr_t>(retained->mImage)&31U)==0);
        const auto physical=OSCachedToPhysical(retained->mImage);
        require(OSPhysicalToCached(physical)==retained->mImage);
        require(retained->mTIMG->mWidth==8 && retained->mTIMG->mHeight==8);
        require(retained->mTIMG->mFormat==GX_TF_RGB565 && retained->mTIMG->mMinType==GX_LINEAR);
        const auto allocated=heap->available_bytes();
        retained_capacity=allocated;
        {
            using namespace smgpc::compat;
            auto domain=JkrAllocationDomain::create(jkr,256U*1024U);
            auto& original=domain->heap();
            {
                JkrAllocationScope scope(domain);
                auto* texture=new JUTTexture(16,16,GX_TF_RGB565);
                texture->capture(0,0,GX_TF_RGB565,false,0);
                new JUTTexture(retained->mTIMG,0); // Borrowed image stays with retained.
            }
            require(heap->available_bytes()<allocated);
            original.freeAll();
            require(heap->available_bytes()==allocated);
            {
                JkrAllocationScope scope(domain);
                auto* texture=new JUTTexture(16,16,GX_TF_RGB565);
                delete texture;
            }
            original.freeAll(); require(heap->available_bytes()==allocated);
            std::size_t head_capacity;
            {
                JkrAllocationScope scope(domain);
                new JUTTexture(8,8,GX_TF_I8);
                head_capacity=heap->available_bytes();
                auto* tail=new(original.alloc(sizeof(JUTTexture),-int(alignof(JUTTexture)))) JUTTexture(16,16,GX_TF_RGB565);
                tail->capture(0,0,GX_TF_RGB565,false,0);
            }
            original.freeTail(); require(heap->available_bytes()==head_capacity);
            original.freeAll(); require(heap->available_bytes()==allocated);
            {
                JkrAllocationScope scope(domain);
                new JUTTexture(16,16,GX_TF_RGB565);
                bool failed=false;
                try { new JUTTexture(2048,2048,GX_TF_RGBA8); } catch(const std::bad_alloc&) { failed=true; }
                require(failed);
            }
            domain.reset(); require(heap->available_bytes()==allocated);
            std::cout << "JUTTexture: actual JKR bulk/tail/domain retirement, explicit delete, borrowed storage and failed construction pass\n";
        }

        {
            JUTTexture borrowed(retained->mTIMG,0);
            require(!borrowed.getCaptureFlag() && borrowed.mTIMG==retained->mTIMG && borrowed.mImage==retained->mImage);
            require(heap->available_bytes()==allocated);
        }
        require(heap->available_bytes()==allocated);
        alignas(32) std::array<u8,32> bytes{};
        GDLObj dl;
        auto* old=__GDCurrentDL;
        GDInitGDLObj(&dl,bytes.data(),bytes.size()); GDSetCurrent(&dl);
        GDSetTexImgPtr(GX_TEXMAP0,retained->mImage);
        GDSetCurrent(old);
        const auto word=(std::uint32_t(bytes[1])<<24)|(std::uint32_t(bytes[2])<<16)|(std::uint32_t(bytes[3])<<8)|bytes[4];
        require(bytes[0]==0x61 && word==(0x94000000U|(physical>>5)));

        // Preserve the original zero dimension; the native adapter must not
        // silently replace it with a one-pixel texture.
        { JUTTexture zero(0,8,GX_TF_I8); require(zero.mTIMG->mWidth==0 && zero.mTIMG->mHeight==8); }
        require(heap->available_bytes()==allocated);
        alignas(JUTTexture) std::byte object[sizeof(JUTTexture)];
        for(unsigned i=0;i<3;++i) {
            auto* texture=new(object) JUTTexture(16,16,GX_TF_RGB565);
            texture->capture(0,0,GX_TF_RGB565,false,0);
            texture->~JUTTexture();
            require(heap->available_bytes()==allocated);
        }
        struct ThrowingOwner {
            JUTTexture texture{16, 16, GX_TF_RGB565};
            ThrowingOwner() { throw std::runtime_error("test enclosing-constructor unwinding"); }
        };
        try { ThrowingOwner owner; } catch (const std::runtime_error&) {}
        require(heap->available_bytes()==allocated);
        bool capacity_rejected=false;
        try { JUTTexture too_large(2048, 2048, GX_TF_RGBA8); }
        catch (const std::bad_alloc&) { capacity_rejected=true; }
        require(capacity_rejected && heap->available_bytes()==allocated);
        retired_service_domain=smgpc::compat::JkrAllocationDomain::create(jkr,128U*1024U);
        {
            smgpc::compat::JkrAllocationScope scope(retired_service_domain);
            new JUTTexture(8,8,GX_TF_I8);
        }
        std::cout << "JUTTexture: enclosing-constructor unwind and failed allocation restore mapped capacity\n";
        std::cout << "JUTTexture: mapped owned/borrowed, GD address, dimensions, three capture/destruction/reuse cycles pass\n";
    }
    require(OSPhysicalToCached(OSCachedToPhysical(retained->mImage))==retained->mImage);
    bool rejected=false;
    try { JUTTexture missing(8,8,GX_TF_RGB565); } catch(const std::logic_error&) { rejected=true; }
    require(rejected);
    retired_service_domain.reset();
    require(heap->available_bytes()==retained_capacity);
    retained.reset();
    require(heap->available_bytes()==available);
    renderer.end_frame();
    std::cout << "JUTTexture: outstanding storage survives provider retirement, returns full MEM1 capacity on final destruction\n";
}
