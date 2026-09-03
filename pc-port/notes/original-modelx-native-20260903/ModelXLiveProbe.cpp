#include "runtime/RuntimeContext.hpp"
#include "resource/Mem1ResourceHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/J3dCommandScope.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "Game/Player/J3DModelX.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/JUtility/JUTVideo.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <aurora/gfx.h>
#include <dolphin/gd.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

static void require(bool valid, const char* message) {
    if (!valid) {
        smgpc::compat::JkrHostAllocationScope host;
        throw std::runtime_error(message);
    }
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: live screenshot.png; SMGPC_REAL_DISC must name RMGK01");
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && aurora_dvd_open(disc), "cannot open real disc");
        struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
        DVDInit();
        auto logger = smgpc::logging::create_default_logger();
        smgpc::render::AuroraWindow window({.width=640, .height=456, .title="Original Mario J3DModelX ownership"});
        smgpc::render::AuroraRenderer renderer(window);
        smgpc::resource::GameResourceRuntime process({192U * 1024U * 1024U, 64U * 1024U * 1024U, 16U * 1024U * 1024U});
        const auto original_mem1_capacity = process.mem1_heap()->available_bytes();
        {
        smgpc::runtime::RuntimeContext runtime(*logger, window, process);
        smgpc::render::ScopedAuroraRendererContext renderer_context(renderer);
        auto* holders = smgpc::compat::ResourceHolderService::active();
        require(holders && JUTVideo::getManager() && MR::getScreenResTIMG(), "real screen and resource owners required");
        auto* holder = holders->create_and_add("Mario.arc");
        auto lease = holders->retain(*holder);
        auto model_domain = process.create_cohort();
        struct SysRestore { J3DSys saved = j3dSys; ~SysRestore() { j3dSys = saved; } } restore;
        J3DModel* model;
        J3DDrawBuffer* opaque;
        J3DDrawBuffer* translucent;
        GDLObj* original_gd = __GDCurrentDL;
        {
            smgpc::compat::JkrAllocationScope heap(model_domain);
            smgpc::compat::J3dCommandScope commands;
            model = MR::newJ3DModel(holder, "Mario", static_cast<J3DMdlFlag>(J3DMdlFlag_UseSharedDL | J3DMdlFlag_UseSingleDL));
            require(dynamic_cast<J3DModelX*>(model), "original specialization did not create J3DModelX");
            require(model->getModelData() == holder->mModelResTable->getRes("Mario"), "model lost actual holder data identity");
            opaque = new J3DDrawBuffer(32);
            translucent = new J3DDrawBuffer(32);
            translucent->setZSort();
        }
        require(__GDCurrentDL == original_gd, "constructor scope failed to restore GD pointer");
        auto* specialized = static_cast<J3DModelX*>(model);
        for (unsigned i=0;i<16;++i)
            require(specialized->mDisplayLists[i] && specialized->mDisplayListSizes[i] && !(specialized->mDisplayListSizes[i] & 31U), "missing original ModelX display list");
        std::cout << "[modelx] original MR specialization constructed joints=" << model->getModelData()->getJointNum()
                  << " materials=" << model->getModelData()->getMaterialNum() << " shapes=" << model->getModelData()->getShapeNum()
                  << " display_lists=16; holder identity and GD restoration verified\n";
        j3dSys.mDrawBuffer[0] = opaque;
        j3dSys.mDrawBuffer[1] = translucent;
        for (unsigned frame=0;frame<5 && window.poll_events();++frame) {
            (void)renderer.begin_frame();
            {
                smgpc::compat::JkrAllocationScope heap(model_domain);
                smgpc::compat::J3dCommandScope commands;
                j3dSys.drawInit();
                Mtx44 projection;
                C_MTXPerspective(projection,45.0F,640.0F/456.0F,1.0F,10000.0F);
                GXSetProjection(projection,GX_PERSPECTIVE);
                GXSetViewport(0,0,640,456,0,1); GXSetScissor(0,0,640,456);
                GXSetDispCopySrc(0,0,640,456); GXSetDispCopyDst(640,456); GXSetDispCopyYScale(1.0F);
                Vec eye{0,100,600}, up{0,1,0}, target{0,100,0};
                C_MTXLookAt(j3dSys.mViewMtx,&eye,&up,&target);
                GXLightObj light;
                GXInitLightPos(&light,0,500,500); GXInitLightColor(&light,GXColor{255,255,255,255});
                GXInitLightAttn(&light,1,0,0,1,0,0); GXLoadLightObjImm(&light,GX_LIGHT0);
                opaque->frameInit(); translucent->frameInit();
                model->calc(); model->calcMaterial(); model->viewCalc(); model->entry();
                opaque->draw(); translucent->draw();
            }
            renderer.end_frame();
            if (frame==4) renderer.request_screenshot_png(argv[1]);
            const auto* stats=aurora_get_stats();
            std::cout << "[modelx] frame=" << frame << " original_calc_entry_draws=" << stats->drawCallCount << '\n';
        }
        GXDrawDone();
        delete model;
        // Original draw buffers contain raw table storage and retire with the
        // retained scene heap; no retail destructor provider exists here.
        std::cout << "[modelx] actual model retired before holder lease and original allocation domain\n";
        }
        require(process.mem1_heap()->available_bytes() == original_mem1_capacity, "runtime owners did not release all mapped resources before process retirement");
        std::cout << "[modelx] runtime and all textures retired before GX; full MEM1 capacity restored\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr << "[modelx] FAILED: " << error.what() << '\n';
        return 1;
    }
}
