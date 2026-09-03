#include "runtime/RuntimeContext.hpp"
#include "resource/Mem1ResourceHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/J3dCommandScope.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "Game/Player/J3DModelX.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeCore.hpp"
#include "ModelManagerOwner.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/System/DrawBufferGroup.hpp"
#include "Game/System/DrawBuffer.hpp"
#include <cstring>
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
        auto model_domain = holders->allocation_domain();
        std::unique_ptr<smgpc::compat::ModelManagerOwner> owner;
        std::unique_ptr<smgpc::compat::ModelManagerOwner> second_owner;
        const auto initial_actor_count = smgpc::compat::actor_runtime_state_count();
        auto actor = std::make_unique<LiveActor>("first original model");
        auto second_actor = std::make_unique<LiveActor>("second original model");
        require(smgpc::compat::actor_runtime_state_count() == initial_actor_count + 2, "both actual actors must register");
        DrawBufferGroup group;
        s32 first_index, second_index;
        struct SysRestore { J3DSys saved = j3dSys; ~SysRestore() { j3dSys = saved; } } restore;
        J3DModel* model;
        
        GDLObj* original_gd = __GDCurrentDL;
        {
            smgpc::compat::JkrAllocationScope heap(model_domain);
            smgpc::compat::J3dCommandScope commands;
            owner = std::make_unique<smgpc::compat::ModelManagerOwner>(*holders, model_domain, "Mario", "MarioAnime", false);
            auto& manager = owner->manager();
            model = manager.getJ3DModel();
            require(manager.mXanimePlayer && manager.mXanimePlayer->mModel == model, "manager must own the actual model's Xanime player");
            require(manager.mXanimePlayer->mModelData == model->getModelData(), "player must share actual model data");
            require(manager.mBtkPlayer && manager.mBtpPlayer && manager.mDisplayListMaker, "MarioAnime material players/display-list owner required");
            manager.startBck("Run", nullptr);
            require(dynamic_cast<J3DModelX*>(model), "original specialization did not create J3DModelX");
            require(model->getModelData() == holder->mModelResTable->getRes("Mario"), "model lost actual holder data identity");
            actor->mModelManager = &manager;
            second_owner = std::make_unique<smgpc::compat::ModelManagerOwner>(*holders, model_domain, "Mario", "MarioAnime", false);
            second_actor->mModelManager = &second_owner->manager();
            second_actor->mModelManager->startBck("Run", nullptr);
            second_actor->mModelManager->getJ3DModel()->mBaseTransformMtx[0][3] = 180;
            group.init(4);
            first_index = group.registerDrawBuffer(actor.get());
            second_index = group.registerDrawBuffer(second_actor.get());
            require(first_index == second_index && group.mExecutors.size() == 1, "same original resource names must share one executer");
            group.allocateActorListBuffer();
            require(group.mExecutors[first_index]->mMaxNumActors == 2, "registration phase must reserve both actors");
            group.active(actor.get(), first_index); group.active(second_actor.get(), second_index);
        }
        require(__GDCurrentDL == original_gd, "constructor scope failed to restore GD pointer");
        auto* specialized = static_cast<J3DModelX*>(model);
        for (unsigned i=0;i<16;++i)
            require(specialized->mDisplayLists[i] && specialized->mDisplayListSizes[i] && !(specialized->mDisplayListSizes[i] & 31U), "missing original ModelX display list");
        std::cout << "[modelx] original MR specialization constructed joints=" << model->getModelData()->getJointNum()
                  << " materials=" << model->getModelData()->getMaterialNum() << " shapes=" << model->getModelData()->getShapeNum()
                  << " display_lists=16; holder identity and GD restoration verified\n";
        
        auto* executer = group.mExecutors[first_index];
        auto* buffer = executer->mDrawBuffer;
        require(buffer->mModel == model, "original buffer must retain the first model as its prototype");
        auto* original_player = owner->manager().mXanimePlayer;
        auto* original_core = original_player->mCore;
        auto* surviving_model = second_owner->manager().getJ3DModel();
        auto* surviving_player = second_owner->manager().mXanimePlayer;
        auto* surviving_core = surviving_player->mCore;
        require(model != surviving_model && owner->manager().mXanimePlayer != surviving_player,
                "both managers must have independent model/player identities");
        require(original_core != surviving_core,
                "both managers must have independent original cores");
        auto packet_belongs_to = [](const J3DShapePacket* packet, const J3DModel* candidate) {
            for (u16 i = 0; i < candidate->mModelData->getShapeNum(); ++i)
                if (packet == &candidate->mShapePacket[i]) return true;
            return false;
        };
        auto verify_packets = [&](bool first_active) {
            require(executer->mNumActors == (first_active ? 2 : 1), "actual executer actor count changed incorrectly");
            require(buffer->mNumActors == executer->mNumActors, "buffer/executer counts must agree");
            require(buffer->mModel == model, "deactivation must not invent a replacement material prototype");
            for (s32 i = 0; i < buffer->mNumShapeDrawers; ++i) {
                auto* drawer = buffer->getShapeDrawerByIndex(i);
                bool original_material_packet = false;
                for (u16 material = 0; material < model->mModelData->getMaterialNum(); ++material)
                    original_material_packet |= drawer->mMatPacket == model->getMatPacket(material);
                require(original_material_packet, "drawer must still use the retained first model's material packet");
                require(drawer->mNumPackets == drawer->_8 * (first_active ? 2 : 1), "coalesced shape packet count mismatch");
                for (s32 packet = 0; packet < drawer->mNumPackets; ++packet) {
                    const auto* value = drawer->getShapePacket(packet);
                    require(packet_belongs_to(value, surviving_model) || (first_active && packet_belongs_to(value, model)),
                            "draw list retained a retired actor's shape packet");
                }
            }
        };
        verify_packets(true);
        unsigned frames_drawn = 0;
        for (unsigned frame=0;frame<5;++frame) {
            require(window.poll_events(), "window closed before lifetime fixture completed");
            if (frame == 2) {
                // Original drawers still borrow this manager's material packets.
                // The scene draw owner must retain it after its LiveActor retires.
                GXDrawDone();
                auto* retired_actor = actor.get();
                group.deactive(actor.get(), first_index);
                actor->mModelManager = nullptr;
                actor.reset();
                require(!smgpc::compat::has_actor_runtime_state(retired_actor) &&
                        !smgpc::compat::has_name_obj_runtime_state(retired_actor),
                        "first actor's native runtime and NameObj identities must retire");
                require(smgpc::compat::actor_runtime_state_count() == initial_actor_count + 1,
                        "exactly the surviving real actor must remain registered");
                require(group.mActiveExecutors.size() == 1 && executer->getActor(0) == second_actor.get(),
                        "source swap-removal must retain the second actor in the same executer");
                verify_packets(false);
                std::cout << "[lifetime] first actor destroyed; original material prototype retained; surviving packets only\n";
            }
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
                
                auto& manager = owner->manager();
                for (unsigned tick = 0; tick < 6; ++tick) {
                    if (actor) {
                        manager.update();
                        manager.calcAnim();
                    }
                    second_actor->mModelManager->update();
                    second_actor->mModelManager->calcAnim();
                    require(model->getModelData()->getJointNodePointer(0)->getMtxCalc() == nullptr, "original calc must clear its joint calculator");
                    for (auto* evaluated : {model, surviving_model})
                        for (unsigned joint=0; joint<evaluated->getModelData()->getJointNum(); ++joint)
                            for (unsigned row=0; row<3; ++row) for(unsigned col=0; col<4; ++col)
                                require(std::isfinite(evaluated->getAnmMtx(joint)[row][col]), "original manager produced nonfinite joint matrices");
                }
                const float survivor_frame = second_owner->manager().getBckCtrl()->getFrame();
                group.entry();
                require(second_owner->manager().mXanimePlayer == surviving_player && surviving_player->mCore == surviving_core &&
                        second_owner->manager().getJ3DModel() == surviving_model, "draw submission changed actual owner identities");
                require(second_owner->manager().getBckCtrl()->getFrame() == survivor_frame, "view entry must not advance animation");
                verify_packets(actor != nullptr);
                std::cout << "[manager] ticks=" << (frame+1)*6 << " survivor_bck_frame=" << survivor_frame
                          << " rate=" << second_owner->manager().getBckCtrl()->getRate() << " original pose is finite\n";
                group.drawOpa(); group.drawXlu();
            }
            renderer.end_frame();
            // Existing readback synchronization makes the completed frame's
            // renderer-worker statistics observable without sleeps or polling.
            u32 copy_width = 0, copy_height = 0, copy_stride = 0;
            require(AuroraReadDisplayCopyRGBA8(nullptr, 0, &copy_width, &copy_height, &copy_stride),
                    "completed original draw must have a real display copy");
            if (frame==1) renderer.request_screenshot_png(std::string(argv[1]) + ".two-actors.png");
            if (frame==4) renderer.request_screenshot_png(argv[1]);
            const auto* stats=aurora_get_stats();
            const u32 expected_draws = actor ? 22 : 11;
            std::cout << "[modelx] frame=" << frame << " original_calc_entry_draws=" << stats->drawCallCount << " expected=" << expected_draws << '\n';
            // Initial recording statistics include bootstrap work (36 observed).
            // Compare settled initial and survivor frames after that bootstrap.
            if (frame != 0)
                require(stats->drawCallCount == expected_draws, "real packet draw count must follow active actor removal");
            ++frames_drawn;
        }
        GXDrawDone();
        require(frames_drawn == 5 && actor == nullptr, "all initial and survivor frames must complete");
        group.deactive(second_actor.get(), second_index);
        require(group.mActiveExecutors.size() == 0, "deactivation must empty active executers");
        // Exercise the public pointers as borrowed mutable state: original
        // MarioAnimator replaces the manager player and shares lower/upper
        // transform ownership. Native deletion must use captured construction
        // identities, rather than following pointers at retirement time.
        owner->manager().mXanimePlayer = surviving_player;
        original_player->mCore = surviving_core;
        owner.reset();
        {
            smgpc::compat::JkrAllocationScope heap(model_domain);
            smgpc::compat::J3dCommandScope commands;
            auto& survivor = second_owner->manager();
            require(survivor.mXanimePlayer == surviving_player && surviving_player->mCore == surviving_core &&
                    survivor.getJ3DModel() == surviving_model, "prototype retirement must not destroy borrowed replacement identities");
            survivor.update();
            survivor.calcAnim();
            require(survivor.getBckCtrl()->getFrame() == 31.0F,
                    "actual surviving player and core must still calculate after prototype owner retirement");
        }
        std::cout << "[lifetime] captured original model/player/core retired; borrowed replacement still calculates at frame31\n";
        // The prototype manager remained retained through the last buffer use.
        second_actor->mModelManager = nullptr;
        second_actor.reset();
        require(smgpc::compat::actor_runtime_state_count() == initial_actor_count, "both actors must retire before model owners");
        second_owner.reset();
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
