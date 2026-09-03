#include "render/RendererService.hpp"
#include "resource/J3dModelResource.hpp"
#include "resource/J3dAnimationResource.hpp"
#include "resource/RarcArchive.hpp"
#include "Game/Animation/XanimeCore.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/J3dCommandScope.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include <dolphin/os.h>
#include <aurora/aurora.h>
#include <aurora/gfx.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc != 3 && argc != 6) throw std::runtime_error("usage: probe model.bdl capture.png [animation.arc resource.bck frame]");
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) throw std::runtime_error("cannot open model");
        std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(file), {});
        smgpc::render::AuroraWindow window({.title="Original J3D model draw", .width=640, .height=456});
        smgpc::render::AuroraRenderer renderer(window);
        smgpc::resource::GameResourceRuntime process({24 * 1024 * 1024, 12 * 1024 * 1024, 8 * 1024 * 1024});
        auto domain = process.create_cohort();
        auto mem1 = process.mem1_heap();
        {
            const auto saved = j3dSys;
            smgpc::resource::J3dModelResource resource(bytes, domain, mem1);
            auto* data = resource.load(0x01200000U);
            if (!data) throw std::runtime_error("unsupported model");
            std::unique_ptr<smgpc::resource::J3dAnimationResource> animation_resource;
            std::unique_ptr<XanimeCore> animation_core;
            if (argc == 6) {
                auto archive = smgpc::resource::RarcArchive::from_file(argv[3]);
                animation_resource = std::make_unique<smgpc::resource::J3dAnimationResource>(archive.file_data_by_basename(argv[4]));
                smgpc::compat::JkrAllocationScope heap(domain);
                auto* animation = dynamic_cast<J3DAnmTransformKey*>(animation_resource->load());
                if (!animation || animation->field_0x1e < data->getJointNum()) throw std::runtime_error("BCK must contain every model joint");
                animation->setFrame(std::stof(argv[5]));
                animation_core.reset(new XanimeCore(1, data->getJointNum(), data->getFlag() & 15));
                animation_core->setBck(0, animation);
                data->getJointTree().setBasicMtxCalc(animation_core.get());
            }
            J3DModel* model;
            J3DDrawBuffer* opaque;
            J3DDrawBuffer* translucent;
            {
                smgpc::compat::JkrAllocationScope heap(domain);
                smgpc::compat::J3dCommandScope commands;
                model = new J3DModel(data, J3DMdlFlag_UseSharedDL | J3DMdlFlag_UseSingleDL, 1);
                opaque = new J3DDrawBuffer(32);
                translucent = new J3DDrawBuffer(32);
                translucent->setZSort();
                model->calc();
                model->calcMaterial();
                model->viewCalc();
                model->viewCalc();
            }
            j3dSys.mDrawBuffer[0] = opaque;
            j3dSys.mDrawBuffer[1] = translucent;
            for (int frame = 0; frame < 5 && window.poll_events(); ++frame) {
                (void)renderer.begin_frame();
                {
                    smgpc::compat::JkrAllocationScope heap(domain);
                    smgpc::compat::J3dCommandScope commands;
                    j3dSys.drawInit();
                    Mtx44 projection;
                    C_MTXPerspective(projection, 45.0F, 640.0F / 456.0F, 1.0F, 10000.0F);
                    GXSetProjection(projection, GX_PERSPECTIVE);
                    GXSetViewport(0, 0, 640, 456, 0, 1);
                    GXSetScissor(0, 0, 640, 456);
                    GXSetDispCopySrc(0, 0, 640, 456);
                    GXSetDispCopyDst(640, 456);
                    GXSetDispCopyYScale(1.0F);
                    Vec eye{0, 100, 600}, up{0, 1, 0}, target{0, 100, 0};
                    C_MTXLookAt(j3dSys.mViewMtx, &eye, &up, &target);
                    GXLightObj light;
                    GXInitLightPos(&light, 0, 500, 500);
                    GXInitLightColor(&light, GXColor{255,255,255,255});
                    GXInitLightAttn(&light, 1, 0, 0, 1, 0, 0);
                    GXLoadLightObjImm(&light, GX_LIGHT0);
                    opaque->frameInit();
                    translucent->frameInit();
                    model->calc();
                    model->calcMaterial();
                    model->viewCalc();
                    model->entry();
                    opaque->draw();
                    translucent->draw();
                }
                renderer.end_frame();
                if (frame == 4) renderer.request_screenshot_png(argv[2]);
                const auto* stats = aurora_get_stats();
                std::cerr << "[original-draw] frame=" << frame << " draws=" << stats->drawCallCount << '\n';
            }
            GXDrawDone();
            delete model;
            j3dSys = saved;
        }
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
