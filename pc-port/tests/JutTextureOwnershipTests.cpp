#include "compat/JutTextureAllocation.hpp"
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
    std::unique_ptr<JUTTexture> retained;
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
        std::cout << "JUTTexture: enclosing-constructor unwind and failed allocation restore mapped capacity\n";
        std::cout << "JUTTexture: mapped owned/borrowed, GD address, dimensions, three capture/destruction/reuse cycles pass\n";
    }
    require(OSPhysicalToCached(OSCachedToPhysical(retained->mImage))==retained->mImage);
    bool rejected=false;
    try { JUTTexture missing(8,8,GX_TF_RGB565); } catch(const std::logic_error&) { rejected=true; }
    require(rejected);
    retained.reset();
    require(heap->available_bytes()==available);
    renderer.end_frame();
    std::cout << "JUTTexture: outstanding storage survives provider retirement, returns full MEM1 capacity on final destruction\n";
}
