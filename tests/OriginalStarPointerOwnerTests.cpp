#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/StageSessionState.hpp"
#include "compat/StarPointerDepthOwnership.hpp"
#include "runtime/RuntimeContext.hpp"
#include "resource/BmgMessageArchive.hpp"
#include "resource/RarcArchive.hpp"
#include "Game/Screen/LayoutGroupCtrl.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Animation/LayoutAnmPlayer.hpp"
#include "layout/LayoutHost.hpp"
#include "layout/LayoutRuntime.hpp"
#include "layout/LayoutResourceResolver.hpp"
#include "layout/BrlytLayout.hpp"
#include <nw4r/lyt/group.h>
#include "Game/Screen/StarPointerBlur.hpp"
#include "Game/Screen/StarPointerController.hpp"
#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/Screen/StarPointerGuidance.hpp"
#include "Game/Screen/StarPointerLayout.hpp"
#include "Game/System/StarPointerOnOffController.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>
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
class Logger final : public smgpc::logging::ILogger {
    void write(std::FILE*, std::source_location, smgpc::logging::Level,
               smgpc::logging::Category, std::string_view) override {}
};
void group_resource_and_dispatch(StarPointerLayout& layout, smgpc::runtime::RuntimeContext& runtime) {
    auto* manager = layout.getLayoutManager();
    const auto* native = smgpc::layout::layout_runtime(&layout);
    const auto& archive = runtime.dvd().archive_for_path(*native->getArchivePath());
    const auto* entry = smgpc::layout::find_layout_brlyt(archive.entries(), native->getLayoutName());
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
void modes_and_resources(smgpc::runtime::RuntimeContext& runtime) {
    auto& owner = smgpc::compat::require_star_pointer_depth();
    auto& director = owner.director();
    auto& modes = owner.modes();
    aurora::wpad_service().set_connected(0, false);
    aurora::wpad_service().set_connected(1, false);
    aurora::wpad_service().begin_frame();
    owner.update();
    require(director.mStarPointerLayouts == nullptr, "actual layouts are created at the process layout boundary, after resources exist");
    smgpc::compat::StageSessionState outer("Game", "HeavensDoorGalaxy", 1, JMapIdInfo(0, 0));
    {
        smgpc::compat::StageSessionBinding outer_binding(outer);
        require(director.mStarPointerLayouts && director.mGuidance == owner.guidance() &&
                director.mControllers == &owner.controller(0) && director.mPeekZ->mInfos[1] == &owner.controller(1).mInfo,
                "the actual original director owns both controllers, layouts, guidance, and callback records");
        auto* guidance = owner.guidance();
        require(guidance->mSpineFrame1P && guidance->mSpineGuidance && guidance->mSpineFrame2P,
                "PointerGuidance.arc creates all three real original guidance state machines");
        for (int port = 0; port < 2; ++port) {
            const auto* layout = director.getStarPointerLayout(port);
            require(layout->mPadChannel == port && layout->mDirector == &director && layout->mCommandStream &&
                    layout->mNumber && layout->mBlur && layout->mBlur->mTexture,
                    "both complete original cursor resource graphs retain their original port and director");
            require(layout->mBlur->mTexture->getWidth() == 64 && layout->mBlur->mTexture->getHeight() == 64,
                    "both blur textures decode the actual 64x64 StarPointerBlur.arc/Blur.bti asset");
        }
        group_resource_and_dispatch(*director.getStarPointerLayout(0), runtime);
        require(modes.mMode == StarPointerMode_Game && modes.mModeCounter[StarPointerMode_Game] == 1 &&
                director.getStarPointerLayout(0)->mIsPointerValid && director.getStarPointerLayout(1)->mIsPointerValid &&
                director.isEnableStarPointerShootStarPiece(),
                "actual Base-to-Game entry enables original layouts and shooting");
        require(!MR::isStarPointerValid(0) && !MR::isStarPointerValid(1),
                "original public validity requires a connected controller as well as the actual valid layout");
        require(!MR::isExistStarPointerGuidance() && !MR::isExistStarPointerGuidanceFrame1P(),
                "initialized EndWait spines report no guidance or frame before requests");
        aurora::wpad_service().set_connected(0, true);
        aurora::wpad_service().begin_frame();
        owner.update();
        require(MR::getWPad(0)->mIsConnected && !MR::getWPad(1)->mIsConnected &&
                    MR::getWPad(0)->mReadInfo->getValidStatusCount() == 1 &&
                    MR::getWPad(1)->mReadInfo->getValidStatusCount() == 0,
                "SDK callback dispatch and original WPadHolder sampling publish the connected channel's actual record");
        require(MR::isStarPointerValid(0) && !MR::isStarPointerValid(1),
                "connecting a controller enables only its own valid original pointer query");
        const auto original_messages = smgpc::resource::BmgMessageArchive::from_message_archive(
            runtime.archive_mounts().retain("/MessageData/Message.arc")->source());
        const auto* expected_message = original_messages.find("System_Date000");
        require(expected_message && original_messages.find("System_Time002"), "the retained-pointer proof uses two actual authored messages");
        require(guidance->request1PGuidance("System_Date000", true), "actual connected and valid layout accepts original guidance request");
        const auto* text = guidance->mGuidanceMessage;
        (void)MR::getLayoutMessageDirect("System_Time002");
        require(text && text == MR::getLayoutMessageDirect("System_Date000") &&
                    std::equal(expected_message->raw_text.begin(), expected_message->raw_text.end(), text) &&
                    text[expected_message->raw_text.size()] == 0,
                "guidance borrows original MessageData wide storage including tag parameters across independent lookups");
        require(!MR::isExistStarPointerGuidance(), "a request alone does not fabricate visible guidance before the original spine advances");

        int yes_no_request;
        MR::startStarPointerModeChooseYesNo(&yes_no_request);
        require(modes.mMode == StarPointerMode_ChooseYesNo &&
                director.getStarPointerLayout(0)->mPointerKind == StarPointerKind_FingerPointer &&
                !director.isEnableStarPointerShootStarPiece(), "original priority selection changes actual pointer kind and shooting policy");
        {
            smgpc::compat::StageSessionState inner("Game", "EggStarGalaxy", 1, JMapIdInfo(0, 0));
            smgpc::compat::StageSessionBinding inner_binding(inner);
            require(modes.mModeCounter[StarPointerMode_Game] == 2 && modes.mMode == StarPointerMode_ChooseYesNo,
                    "a nested stage retains the outer high-priority request and adds only its own Game request");
        }
        require(modes.mModeCounter[StarPointerMode_Game] == 1 && modes.mMode == StarPointerMode_ChooseYesNo &&
                director.mIsUpdateTransHolder, "inner teardown removes its own request and restores outer camera-transform ownership");
        MR::endStarPointerMode(&yes_no_request);
        require(modes.mMode == StarPointerMode_Game && director.isEnableStarPointerShootStarPiece(),
                "popping the explicit higher-priority request resumes the original Game setup");
        modes.requestMode(nullptr, StarPointerMode_PauseMenu);
        modes.update();
        require(modes.mMode == StarPointerMode_PauseMenu && !modes.mRequested[StarPointerMode_PauseMenu],
                "one-frame mode requests are selected then cleared by original update");
        modes.update();
        require(modes.mMode == StarPointerMode_Game, "the next original update restores the retained Game request");

        std::array<int, 16> requests{};
        for (auto& request : requests) modes.incModeCounter(&request, StarPointerMode_ChooseYesNo);
        require(modes.mModeCounter[StarPointerMode_ChooseYesNo] == 15,
                "original 16-slot request capacity includes the existing stage request");
        for (int i = 14; i >= 0; --i) modes.popState(&requests[i]);
        require(modes.mModeCounter[StarPointerMode_ChooseYesNo] == 0 && modes.mMode == StarPointerMode_Game,
                "original reverse request compaction preserves the stage request");
        owner.controller(0).mOutScreenTime = 35;
        owner.controller(1).mOutScreenTime = 36;
        require(MR::isStarPointerInScreenAnyPort(nullptr), "original screen hysteresis includes frame 35");
        owner.controller(0).mOutScreenTime = 36;
        require(!MR::isStarPointerInScreenAnyPort(nullptr), "both actual controller counters beyond 35 exclude the pointer");
    }
    require(modes.mModeCounter[StarPointerMode_Game] == 0 && !director.mIsUpdateTransHolder &&
            !MR::isStarPointerValid(0) && !MR::isStarPointerValid(1),
            "last stage teardown resumes original Base mode while retaining process-owned resources");
}
}
int main() {
    try {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc, "SMGPC_REAL_DISC must name the real disc for original pointer owner validation");
        smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original pointer ownership"});
        smgpc::render::AuroraRenderer renderer(window);
        require(aurora_dvd_open(disc), "cannot open requested real disc");
        struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
        DVDInit();
        smgpc::resource::GameResourceRuntime process({96U << 20, 32U << 20, 4U << 20});
        Logger logger;
        const auto baseline_objects = smgpc::compat::name_obj_runtime_state_count();
        for (int cycle = 0; cycle < 2; ++cycle) {
            {
                smgpc::runtime::RuntimeContext runtime(logger, window, process);
                smgpc::runtime::SceneSchedulerBinding scheduler_binding(runtime.scheduler());
                (void)renderer.begin_frame();
                modes_and_resources(runtime);
                renderer.end_frame();
            }
            require(!smgpc::compat::try_star_pointer_depth() &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline_objects,
                    "process shutdown retires actual pointer descendants and publication before a second runtime");
        }
        std::cout << "Original pointer resources, guidance, mode priority/capacity, nested stages, and repeated ownership passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
