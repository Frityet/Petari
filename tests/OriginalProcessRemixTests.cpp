#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/AudioLib/AudRemixMgr.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "Game/Camera/CameraAnim.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/MapObj/Note.hpp"
#include "Game/MapObj/NoteFairy.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "resource/RemixSequenceResource.hpp"
#include "NativeHeapFixture.hpp"

#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <unistd.h>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void check_resource_boundary() {
    // Two groups, different note counts, and signed note/rest values. Feed the
    // converted resource into the unmodified original pointer-table builder.
    const std::vector<std::uint32_t> authored{
        2, 12, 64,
        1, 2, 0, 48, 7, 60, 100, 24, 0, 0xffffffff, 80, 48, 12,
        1, 1, 0, 9, 72, 120, 96, 4,
    };
    std::vector<std::uint8_t> bytes;
    for (auto word : authored)
        for (int shift : {24, 16, 8, 0}) bytes.push_back(word >> shift);
    auto native = smgpc::resource::decode_remix_sequence(bytes);
    auto heap = smgpc::test::create_native_root_heap(1024 * 1024);
    AudRemixMgr manager(heap.get());
    manager.setRemixSeqResource(native.data());
    require(manager.mGroupCount == 2, "Original manager reads both native groups");
    auto* first = manager.getRemixNoteGroupDataFromMelodyNo(0);
    auto* second = manager.getRemixNoteGroupDataFromMelodyNo(1);
    require(first && first->mTrackCount == 1 && first->mNoteCount == 2 && first->_C[1] == 48,
            "Original group retains its note timing table");
    require(first->mRemixTracks[0]._0 == 7 && first->mRemixTracks[0]._4[1]._0 == -1 &&
                first->mRemixTracks[0]._4[1]._4 == 80 && first->mRemixTracks[0]._4[1]._8 == 48 &&
                first->mRemixTracks[0]._4[1]._C == 12,
            "Instrument and every signed note field are native endian");
    require(second && second->mNoteCount == 1 && second->mRemixTracks[0]._0 == 9 &&
                second->mRemixTracks[0]._4[0]._0 == 72,
            "Original parser advances correctly into the next variable-sized group");
    AudRmxSeqNoteOnTimer timer;
    timer.setData(&first->mRemixTracks[0], &first->mRemixTracks[0]._4[1]);
    require(timer._0 == 12 && timer._4 == 60, "Original timer consumes converted duration and delay");

    for (std::size_t size = 0; size < bytes.size(); ++size) {
        bool rejected = false;
        try { smgpc::resource::decode_remix_sequence(std::span(bytes).first(size)); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Every incomplete resource is rejected before original pointer traversal");
    }
    bytes[0] = 0x80;
    bool rejected = false;
    try { smgpc::resource::decode_remix_sequence(bytes); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Signed count overflow is rejected before original allocation");
    std::fprintf(stderr, "[remix-probe] PASS original packed-resource consumer, timer and truncation checks\n");
}

#ifndef NDEBUG
struct Probe {
    bool checked = false;
    bool saw_action = false;
    std::vector<const NameObj*> arena_members;
    void after_frame(GameSystem& system, std::uint64_t frame) {
        const aurora::allocation::HostAllocationScope host;
        auto* controller = system.mSceneController;
        if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
            controller->getCurrentSceneForExecute() != controller->mScene ||
            !dynamic_cast<GameScene*>(controller->mScene)) return;
        auto* scene = static_cast<GameScene*>(controller->mScene);
        if (checked) {
            const auto* nerve = scene->mSpine->getCurrentNerve();
            if (nerve && std::string_view(typeid(*nerve).name()).find("GameSceneAction") != std::string_view::npos) {
                const auto* position = MR::getPlayerPos();
                require(position && std::isfinite(position->x) && std::isfinite(position->y) && std::isfinite(position->z),
                        "Original opening sequence reaches gameplay with a finite Mario position");
                if (!saw_action)
                    std::fprintf(stderr, "[remix-probe] PASS original opening reaches SceneAction at frame %llu\n",
                                 static_cast<unsigned long long>(frame));
                saw_action = true;
            }
            return;
        }
        auto* manager = AudWrap::getRemixMgr();
        require(manager && manager->mSoundObj && manager->mRemixSeq && manager->mGroupCount > 2,
                "Original process publishes the complete remix owner and archive");
        unsigned fairies = 0, notes = 0;
        for (auto* object : NameObj::snapshotNativeObjects()) {
            auto* storage = dynamic_cast<void*>(object);
            if (JKRHeap::findFromRoot(storage) && !JKRHeap::allocationHeap(storage)) arena_members.push_back(object);
            auto* fairy = dynamic_cast<NoteFairy*>(object);
            if (!fairy || fairy->mSong < 0) continue;
            const auto* group = manager->getRemixNoteGroupDataFromMelodyNo(fairy->mSong);
            require(group && group->mNoteCount > 0 && group->mTrackCount > 0,
                    "Authored stage melody resolves to a populated original group");
            require(fairy->mMelodyNoteNum == group->mNoteCount && fairy->mNoteArray &&
                        MR::getRemixMelodyNoteNum(fairy->mSong) == group->mNoteCount,
                    "Original NoteFairy derives its actor count from the authored audio resource");
            for (s32 note = 0; note < fairy->mMelodyNoteNum; ++note)
                require(fairy->mNoteArray[note] && NameObj::nativeGeneration(fairy->mNoteArray[note]),
                        "Every original musical note child is constructed");
            ++fairies;
            notes += fairy->mMelodyNoteNum;
        }
        require(fairies > 0, "Real Good Egg stage contains authored remix note actors");
        require(!arena_members.empty(), "Real stage exercises original actors embedded in arena allocations");
        void* camera = nullptr;
        s32 camera_size = 0;
        MR::getCurrentScenarioStartAnimCameraData(&camera, &camera_size);
        require(camera && camera_size > sizeof(CanmFileHeader) && CameraAnim::getAnimFrame(static_cast<u8*>(camera)) > 0,
                "Stage archive exposes the real native opening camera animation");
        std::fprintf(stderr, "[remix-probe] PASS ready at frame %llu: %d groups, %u fairies, %u notes; opening camera=%u frames\n",
                     static_cast<unsigned long long>(frame), manager->mGroupCount, fairies, notes,
                     CameraAnim::getAnimFrame(static_cast<u8*>(camera)));
        checked = true;
    }
};
#endif
}

int main() {
#ifdef NDEBUG
    return 1;
#else
    try {
        check_resource_boundary();
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        const char* source = std::getenv("SMGPC_TEST_SAVE_DIR");
        require(disc && *disc && source && *source, "Set SMGPC_REAL_DISC and SMGPC_TEST_SAVE_DIR");
        const auto save = std::filesystem::temp_directory_path() / ("petari-remix-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic save copy must be new");
        std::filesystem::copy(source, save, std::filesystem::copy_options::recursive);
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original remix stage diagnostic",
            .arguments = {"remix-probe", "--stage", "EggStarGalaxy", "--scenario", "1", "--save-slot", "1", "--max-frames", "1800"},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct Disc { ~Disc() { smgpc::app::close_disc_image(); } } disc_lifetime;
        Probe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<Probe*>(context)->after_frame(system, frame);
            },
        };
        const int result = smgpc::app::run_original_game(configuration, *logger, observer);
        require(result == 0 && probe.checked && probe.saw_action, "Original remix stage must reach gameplay and retire cleanly");
        for (auto* member : probe.arena_members)
            require(!NameObj::nativeGeneration(member), "Bulk retirement removes every embedded actor registration");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original remix: %s\n", error.what());
        return 1;
    }
#endif
}
