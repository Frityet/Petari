#include "Game/Util/FileUtil.hpp"
#include <JSystem/JKernel/JKRArchive.hpp>
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DrawSyncManagerLifetime.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include <aurora/guest_thread.hpp>
#include "resource/BmgMessageArchive.hpp"
#include "resource/RarcArchive.hpp"
#include "Game/Screen/LayoutGroupCtrl.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "Game/Animation/LayoutAnmPlayer.hpp"
#include "layout/LayoutResourceResolver.hpp"
#include "layout/BrlytLayout.hpp"
#include <nw4r/lyt/group.h>
#include "Game/Screen/StarPointerBlur.hpp"
#include "Game/Screen/StarPointerController.hpp"
#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/Screen/StarPointerGuidance.hpp"
#include "Game/Screen/StarPointerLayout.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <aurora/exception.hpp>
#include <array>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool value, const char* message) {
    if (!value) aurora::throw_host_exception<std::runtime_error>(message);
}
void configure_draw_state() {
  constexpr float projection[4][4] = {
      {1.0F, 0.0F, 0.0F, 0.0F},
      {0.0F, 1.0F, 0.0F, 0.0F},
      {0.0F, 0.0F, -1.0F, -1.0F},
      {0.0F, 0.0F, 0.0F, 1.0F},
  };
  constexpr float identity[3][4] = {
      {1.0F, 0.0F, 0.0F, 0.0F},
      {0.0F, 1.0F, 0.0F, 0.0F},
      {0.0F, 0.0F, 1.0F, 0.0F},
  };

  GXSetProjection(projection, GX_ORTHOGRAPHIC);
  GXLoadPosMtxImm(identity, GX_PNMTX0);
  GXSetCurrentMtx(GX_PNMTX0);
  GXSetViewport(0.0F, 0.0F, static_cast<float>(MR::getFrameBufferWidth()), static_cast<float>(MR::getFrameBufferHeight()), 0.0F, 1.0F);
  GXSetScissor(0, 0, MR::getFrameBufferWidth(), MR::getFrameBufferHeight());
  GXSetCullMode(GX_CULL_NONE);
  GXSetClipMode(GX_CLIP_ENABLE);
  GXSetCoPlanar(GX_DISABLE);
  GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
  GXSetZCompLoc(GX_FALSE);
  GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_COPY);
  GXSetColorUpdate(GX_TRUE);
  GXSetAlphaUpdate(GX_TRUE);
  GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
  GXSetDither(GX_FALSE);
  GXSetDstAlpha(GX_FALSE, 0);
  GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

  GXSetNumChans(1);
  GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
  GXSetNumTexGens(0);
  GXSetNumIndStages(0);
  GXSetNumTevStages(1);
  GXSetTevDirect(GX_TEVSTAGE0);
  GXSetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
  GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
  GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
  GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
  GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
  GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
  GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

  GXClearVtxDesc();
  GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
  GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
  GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
  GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
}

void draw_fullscreen(float z, GXColor color) {
  GXBegin(GX_QUADS, GX_VTXFMT0, 4);
  GXPosition3f32(-1.0F, 1.0F, z);
  GXColor4u8(color.r, color.g, color.b, color.a);
  GXPosition3f32(1.0F, 1.0F, z);
  GXColor4u8(color.r, color.g, color.b, color.a);
  GXPosition3f32(1.0F, -1.0F, z);
  GXColor4u8(color.r, color.g, color.b, color.a);
  GXPosition3f32(-1.0F, -1.0F, z);
  GXColor4u8(color.r, color.g, color.b, color.a);
  GXEnd();
}

void original_gpu_depth(StarPointerDirector& director) {
    const aurora::os::GuestThreadExecutionScope execution;
    auto* manager = DrawSyncManager::sInstance;
    auto* peek = director.mPeekZ;
    require(manager && manager->mTokenRanges[2].mCallback == peek &&
                manager->mTokenRanges[2].mStart == peek->mToken,
            "the original process manager borrows the exact pointer callback and token");
    configure_draw_state();
    for (int port = 0; port < 2; ++port) {
        auto& info = director.mControllers[port].mInfo;
        info.mPos.set(MR::getScreenWidth() * (port ? 0.6F : 0.4F), MR::getScreenHeight() * 0.5F);
        info.mDrawReady = false;
    }
    draw_fullscreen(-0.75F, GXColor{232, 24, 24, 255});
    MR::setStarPointerDrawSyncToken();
    smgpc::compat::quiesce_draw_sync();
    std::array<u32, 2> first{};
    for (int port = 0; port < 2; ++port) {
        const auto& info = director.mControllers[port].mInfo;
        require(info.mDrawReady && info.mZDepth > 0 && info.mZDepth < 0xffffff,
                "original PeekZ callback publishes each controller depth after GPU completion");
        first[port] = info.mZDepth;
        director.mControllers[port].mInfo.mDrawReady = false;
    }
    draw_fullscreen(-0.25F, GXColor{24, 232, 24, 255});
    MR::setStarPointerDrawSyncToken();
    smgpc::compat::quiesce_draw_sync();
    for (int port = 0; port < 2; ++port) {
        const auto& info = director.mControllers[port].mInfo;
        require(info.mDrawReady && info.mZDepth < first[port],
                "the next original token observes the later GPU depth through its original DPD pointer");
    }
    require(GXReadDrawSync() == peek->mToken && manager->mQueue.usedCount == 0 && manager->mFifo->getCount() == 0,
            "actual token, original queue acknowledgements and FIFO all complete together");
    std::array<f32, 7> projection{};
    std::array<f32, 6> viewport{};
    GXGetProjectionv(projection.data());
    GXGetViewportv(viewport.data());
    require(std::equal(projection.begin(), projection.end(), peek->mProjectionParameters) &&
                std::equal(viewport.begin(), viewport.end(), peek->mViewportParameters),
            "original submission records the current GX projection and viewport");
}

void group_resource_and_dispatch(StarPointerLayout& layout) {
    auto* manager = layout.getLayoutManager();
    const auto& archive = manager->mLayoutHolder->nativeResourceSource();
    const auto* entry = smgpc::layout::find_layout_brlyt(archive.entries(), manager->mLayoutName);
    require(entry, "actual pointer archive contains its named BRLYT");
    const auto resource = smgpc::layout::parse_brlyt_layout(archive.file_data(*entry));
    u32 catalog_index = 0;
    for (const auto& group : resource.groups) {
        if (group.root_group || group.nest_level != 1) continue;
        auto* actual = manager->getGroup(group.name.c_str());
        require(actual && manager->getIndexOfGroupCtrl(group.name.c_str()) == catalog_index++, "actual SDK groups preserve original resource catalog order");
        auto& list = actual->GetPaneList(); auto iter = list.GetBeginIter();
        for (auto pane_index : group.pane_indices) {
            require(iter != list.GetEndIter() && iter++->mTarget == manager->getPane(resource.panes[pane_index].name.c_str()),
                    "actual SDK group members are the exact live manager Pane identities");
        }
        require(iter == list.GetEndIter(), "actual group member capacity agrees with resource names and absence filtering");
    }
    require(!manager->getGroup("MissingGroup") && manager->getIndexOfGroupCtrl("MissingGroup") == catalog_index,
            "manager preserves original missing group result and one-past-end index");
    bool rejected = false;
    try { manager->createAndAddGroupCtrl("MissingGroup", 1); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected, "absent group creation fails at actual resource ownership boundary");
    auto* first = manager->createAndAddGroupCtrl("GroupRing", 1);
    auto* second = manager->createAndAddGroupCtrl("GroupRing", 1);
    require(first != second && first->mGroup == second->mGroup && first->getPaneNum() > 0 &&
            !first->getPane(first->getPaneNum()) && first->mAnmPlayerArray[0]->isStop(),
            "original duplicate controllers share actual group metadata and own fresh stopped players");
    std::vector<int> calls;
    struct Probe final : LayoutAnmPlayer {
        Probe(LayoutManager* m, std::vector<int>& c, int id) : LayoutAnmPlayer(m), calls(c), id(id) {}
        void movement() override { calls.push_back(id); }
        void reflectFrame() override { calls.push_back(id + 10); }
        std::vector<int>& calls; int id;
    } a(manager, calls, 1), b(manager, calls, 2);
    struct Restore {
        LayoutGroupCtrl* a; LayoutGroupCtrl* b; LayoutAnmPlayer* old_a; LayoutAnmPlayer* old_b;
        ~Restore() { a->mAnmPlayerArray[0] = old_a; b->mAnmPlayerArray[0] = old_b; }
    } restore{first, second, first->mAnmPlayerArray[0], second->mAnmPlayerArray[0]};
    first->mAnmPlayerArray[0] = &a; second->mAnmPlayerArray[0] = &b;
    manager->movement(); require(calls == std::vector<int>{2}, "only latest registered group occupies the movement catalog slot");
    calls.clear(); manager->calcAnim();
    std::vector<int> expected;
    for (const auto& pane : resource.panes) {
        u32 count = 0;
        for (u32 i = 0; i < first->getPaneNum(); ++i) if (first->getPane(i) == manager->getPane(pane.name.c_str())) ++count;
        expected.insert(expected.end(), count, 12); expected.insert(expected.end(), count, 11);
    }
    require(calls == expected, "per-pane animation traversal preserves reverse registration links, duplicate membership and depth-first resource order");
}
void original_resources() {
    auto* director = MR::getGameSystemObjHolder()->mStarPointerDirector;
    require(director && director->mStarPointerLayouts && director->mGuidance && director->mControllers &&
                director->mPeekZ->mInfos[0] == &director->mControllers[0].mInfo &&
                director->mPeekZ->mInfos[1] == &director->mControllers[1].mInfo,
            "the actual GameSystem director owns both controllers, layouts, guidance and callback records");
    auto* guidance = director->mGuidance;
    require(guidance->mSpineFrame1P && guidance->mSpineGuidance && guidance->mSpineFrame2P,
            "PointerGuidance.arc creates all three original guidance state machines");
    for (int port = 0; port < 2; ++port) {
        const auto* layout = director->getStarPointerLayout(port);
        require(layout->mPadChannel == port && layout->mDirector == director && layout->mCommandStream &&
                    layout->mNumber && layout->mBlur && layout->mBlur->mTexture,
                "both original cursor resource graphs retain their port and director");
        require(layout->mBlur->mTexture->getWidth() == 64 && layout->mBlur->mTexture->getHeight() == 64,
                "both blur textures decode the actual 64x64 StarPointerBlur.arc/Blur.bti asset");
    }
    group_resource_and_dispatch(*director->getStarPointerLayout(0));
    original_gpu_depth(*director);

    const auto original_messages = smgpc::resource::BmgMessageArchive::from_message_archive(
        MR::receiveArchive("/MessageData/Message.arc")->source());
    const auto* expected = original_messages.find("System_Date000");
    require(expected && original_messages.find("System_Time002"), "the pointer message proof uses actual authored messages");
    const auto* text = MR::getLayoutMessageDirect("System_Date000");
    (void)MR::getLayoutMessageDirect("System_Time002");
    require(text && text == MR::getLayoutMessageDirect("System_Date000") &&
                std::equal(expected->raw_text.begin(), expected->raw_text.end(), text) &&
                text[expected->raw_text.size()] == 0,
            "original MessageData wide storage retains tag parameters across independent lookups");

    const std::array<s32, 2> previous{
        director->mControllers[0].mOutScreenTime, director->mControllers[1].mOutScreenTime};
    director->mControllers[0].mOutScreenTime = 35;
    director->mControllers[1].mOutScreenTime = 36;
    require(MR::isStarPointerInScreenAnyPort(nullptr), "original screen hysteresis includes frame 35");
    director->mControllers[0].mOutScreenTime = 36;
    require(!MR::isStarPointerInScreenAnyPort(nullptr), "both actual controller counters beyond 35 exclude the pointer");
    for (int port = 0; port < 2; ++port) director->mControllers[port].mOutScreenTime = previous[port];
}
}

int main() {
    const auto baseline_objects = smgpc::compat::name_obj_runtime_state_count();
    const int result = smgpc::test::run_stage_resource_process("original-star-pointer-owner", original_resources);
    if (result != 0) return result;
    try {
        require(!DrawSyncManager::sInstance && smgpc::compat::name_obj_runtime_state_count() == baseline_objects,
                "original process shutdown retires pointer descendants and GPU callbacks");
        std::cout << "Original pointer resources, GPU callbacks, layout groups and process ownership passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
