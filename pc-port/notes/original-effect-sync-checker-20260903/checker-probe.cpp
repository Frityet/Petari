#include "Game/Effect/SyncBckEffectInfo.hpp"
#include "Game/Effect/SyncBckEffectChecker.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/System/ResourceInfo.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxCalc.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "resource/RarcArchive.hpp"
#include <aurora/aurora.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

namespace aurora { extern AuroraConfig g_config; }
namespace {
    using namespace smgpc::resource;
    using namespace smgpc::compat;
    using Bytes = std::vector<std::uint8_t>;
    void require(bool ok, const char* message) {
        if (!ok) { JkrHostAllocationScope host; throw std::runtime_error(message); }
    }
    // RARC/ANK1 fixture builder copied from OriginalResourceHolderTests; model
    // fixture copied from OriginalXanimePlayerTests. Both invoke actual SDK and
    // Game owners. No partially decoded archive is represented as a full model.
    void put16(Bytes& b, std::size_t p, unsigned v) { b.at(p) = v >> 8; b.at(p + 1) = v; }
    void put32(Bytes& b, std::size_t p, std::uint32_t v) {
        put16(b, p, v >> 16); put16(b, p + 2, v);
    }
    void tag(Bytes& b, std::size_t p, std::string_view s) { std::memcpy(b.data() + p, s.data(), s.size()); }
    struct File { std::string name; Bytes bytes; bool nested = false; };
    std::shared_ptr<RarcArchive> archive(std::vector<File> input) {
        // Three real directory records: root, nested, empty, and their dot links.
        // Payload IDs deliberately differ from file-table indexes.
        Bytes strings;
        auto str = [&](std::string_view s) {
            auto p = strings.size(); strings.insert(strings.end(), s.begin(), s.end()); strings.push_back(0); return p;
        };
        auto root = str("fixture_root"), nest = str("nested"), empty = str("empty"), dot = str("."), parent = str("..");
        std::vector<std::size_t> names;
        for (const auto& f : input) names.push_back(str(f.name));
        const std::size_t roots = std::ranges::count_if(input, [](const auto& f) { return !f.nested; });
        const std::size_t count = input.size() + 8, dirs = 0x40, files = dirs + 48;
        const auto string_offset = files + count * 20;
        const auto data_offset = (string_offset + strings.size() + 31) & ~std::size_t{31};
        Bytes data;
        std::vector<std::size_t> offsets;
        for (const auto& f : input) {
            while (data.size() % 32) data.push_back(0);
            offsets.push_back(data.size()); data.insert(data.end(), f.bytes.begin(), f.bytes.end());
        }
        Bytes out(data_offset + data.size());
        tag(out, 0, "RARC"); put32(out, 4, out.size()); put32(out, 8, 0x20);
        put32(out, 12, data_offset - 0x20); put32(out, 16, data.size());
        put32(out, 0x20, 3); put32(out, 0x24, dirs - 0x20);
        put32(out, 0x28, count); put32(out, 0x2c, files - 0x20);
        put32(out, 0x30, strings.size()); put32(out, 0x34, string_offset - 0x20); put16(out, 0x38, 200);
        auto directory = [&](unsigned index, unsigned name, unsigned n, unsigned first) {
            auto p = dirs + index * 16; tag(out, p, "ROOT"); put32(out, p + 4, name);
            put16(out, p + 8, RarcArchive::hash_name(reinterpret_cast<const char*>(strings.data() + name)));
            put16(out, p + 10, n); put32(out, p + 12, first);
        };
        directory(0, root, roots + 4, 0);
        directory(1, nest, input.size() - roots + 2, roots + 4);
        directory(2, empty, 2, input.size() + 6);
        auto entry = [&](unsigned index, unsigned id, unsigned name, unsigned flags, unsigned offset, unsigned size) {
            auto p = files + index * 20; put16(out, p, id);
            put16(out, p + 2, RarcArchive::hash_name(reinterpret_cast<const char*>(strings.data() + name)));
            put32(out, p + 4, (flags << 24) | name); put32(out, p + 8, offset); put32(out, p + 12, size);
        };
        unsigned index = 0;
        for (std::size_t i = 0; i < input.size(); ++i)
            if (!input[i].nested) entry(index++, 100 + i, names[i], 0x11, offsets[i], input[i].bytes.size());
        entry(index++, 0xffff, nest, 2, 1, 16); entry(index++, 0xffff, empty, 2, 2, 16);
        entry(index++, 0xffff, dot, 2, 0, 16); entry(index++, 0xffff, parent, 2, 0xffffffff, 16);
        for (std::size_t i = 0; i < input.size(); ++i)
            if (input[i].nested) entry(index++, 100 + i, names[i], 0x11, offsets[i], input[i].bytes.size());
        entry(index++, 0xffff, dot, 2, 1, 16); entry(index++, 0xffff, parent, 2, 0, 16);
        entry(index++, 0xffff, dot, 2, 2, 16); entry(index++, 0xffff, parent, 2, 0, 16);
        require(index == count, "fixture directory count");
        std::copy(strings.begin(), strings.end(), out.begin() + string_offset);
        std::copy(data.begin(), data.end(), out.begin() + data_offset);
        return std::make_shared<RarcArchive>(RarcArchive::from_bytes(std::move(out)));
    }
    Bytes file(std::string_view type, Bytes block) {
        Bytes b(0x20); tag(b, 0, "J3D1"); tag(b, 4, type); put32(b, 12, 1);
        b.insert(b.end(), block.begin(), block.end()); put32(b, 8, b.size()); return b;
    }
    Bytes transform(bool key) {
        Bytes b(0x24); tag(b, 0, key ? "ANK1" : "ANF1"); b[8] = 3; b[9] = 2;
        put16(b, 0xa, 7); put16(b, 0xc, 1);
        for (int p : {0xe, 0x10, 0x12}) put16(b, p, key ? 1 : 2);
        put32(b, 0x14, b.size()); const auto table = b.size(); b.resize(table + (key ? 54 : 36));
        for (int i = 0; i < 9; ++i) put16(b, table + i * (key ? 6 : 4), key ? 1 : 2);
        const auto f = [&](int field, float a, float c) {
            put32(b, field, b.size()); const auto p = b.size(); b.resize(p + (key ? 4 : 8));
            put32(b, p, std::bit_cast<std::uint32_t>(a)); if (!key) put32(b, p + 4, std::bit_cast<std::uint32_t>(c));
        };
        f(0x18, 2, 3); put32(b, 0x1c, b.size()); auto p = b.size(); b.resize(p + 4);
        put16(b, p, -3); put16(b, p + 2, 30); f(0x20, 4, 9); put32(b, 4, b.size());
        return file(key ? "bck1" : "bca1", std::move(b));
    }
    // Real joint-only model data and an actual J3DModel constructor. This does
    // not represent a partially decoded material/shape archive as a full BMD.
    struct Model {
        J3DModelData data;
        std::array<J3DJoint, 1> joints;
        std::array<J3DJoint*, 1> pointers{&joints[0]};
        std::array<u8, 1> draw_flags{};
        std::array<u16, 1> draw_indices{0};
        J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic> basic;
        std::unique_ptr<J3DModel> object;

        Model() {
            joints[0].mJntNo = 0;
            joints[0].mTransformInfo.mTranslate = {7, 8, 9};
            auto& tree = data.getJointTree();
            tree.mJointNum = 1;
            tree.mJointNodePointer = pointers.data();
            tree.mRootNode = &joints[0];
            tree.mBasicMtxCalc = &basic;
            tree.mDrawMtxData.mEntryNum = 1;
            tree.mDrawMtxData.mDrawFullWgtMtxNum = 1;
            tree.mDrawMtxData.mDrawMtxFlag = draw_flags.data();
            tree.mDrawMtxData.mDrawMtxIndex = draw_indices.data();
            object = std::make_unique<J3DModel>(&data, 0, 1);
        }

        ~Model() {
            // The original destructor does not own arena allocations.
            auto* buffer = object->getMtxBuffer();
            for (auto bank = 0; bank < 2; ++bank) {
                ::operator delete[](buffer->mpDrawMtxArr[bank][0], 0x20);
                ::operator delete[](buffer->mpNrmMtxArr[bank][0], 0x20);
                delete[] buffer->mpDrawMtxArr[bank];
                delete[] buffer->mpNrmMtxArr[bank];
            }
            delete[] buffer->mpScaleFlagArr;
            delete[] buffer->mpAnmMtx;
            delete buffer;
        }
    };

    void select(XanimePlayer& player, const char* name) {
        require(player.changeSimpleBck(name), "actual named BCK selection");
        player._20->setEnd(10);
        player._20->setLoop(2);
    }

    void test_lifecycle(XanimePlayer& player) {
        select(player, "Loop");
        SyncBckEffectInfo info(&player, "Loop", 1, 2.0f, 4.0f, false);
        SyncBckEffectChecker checker(&player);
        require(checker._0 == &player && checker._4 == 0 && !checker._8 && !checker._C && !checker._10,
                "original checker constructor fields");
        checker.reset();
        require(checker._8 && checker._4 == 0, "explicit reset enables first-frame correction");
        unsigned created = 0, deleted = 0;
        for (unsigned current = 1; current <= 5; ++current) {
            player.updateBeforeMovement();
            player.updateAfterMovement();
            require(player._20->getFrame() == current, "actual player advances one frame");
            checker.updateBefore();
            require(checker._C == player.getCurrentBckName(), "checker reads actual player name");
            const bool one_time = checker.isCreate(&info, true);
            const bool continuous = checker.isCreate(&info, false);
            const bool remove = checker.isDelete(&info);
            require(one_time == (current == 3), "one-time emitter triggers crossing start frame");
            require(continuous, "continuous emitter remains eligible throughout matching BCK");
            require(remove == (current == 5), "end-frame crossing triggers delete");
            created += one_time; deleted += remove;
            checker.updateAfter();
            require(!checker._8 && checker._4 == current && checker._10 == checker._C,
                    "after phase records frame/name and clears reset");
            player.calcAnm(0);
        }
        require(created == 1 && deleted == 1, "one crossing each through five real player phases");

        // The original reset correction catches start+rate at the first
        // calculated frame, using its exact 0.001 tolerance.
        select(player, "Loop");
        player._20->setFrame(3);
        checker.reset(); checker.updateBefore();
        require(checker.isCreate(&info, true), "reset catches just-advanced start frame");
        checker.updateAfter();
        require(!checker.isCreate(&info, true), "reset correction is cleared after one phase");
        checker.reset(); player._20->setFrame(3.0005f); checker.updateBefore();
        require(checker.isCreate(&info, true), "original reset epsilon accepts nearby frame");
        player._20->setFrame(3.002f); checker.updateBefore();
        require(!checker.isCreate(&info, true), "reset epsilon does not widen crossing by whole frames");
    }

    void test_stopped_ranges(XanimePlayer& player) {
        struct Case { unsigned mode; float previous, current, target; bool expected; };
        constexpr Case cases[]{
            Case{0, 2, 4, 2, true}, {0, 2, 4, 3.5f, true}, {0, 2, 4, 4, false}, {0, 2, 4, 1.5f, false},
            {0, 4, 2, 2, true}, {0, 4, 2, 3.5f, true}, {0, 4, 2, 4, false}, {0, 4, 2, 1.5f, false},
            {0, 2, 2, 2, false}, {2, 8, 3, 8, true}, {2, 8, 3, 9.5f, true}, {2, 8, 3, 10, false},
            {2, 8, 3, 2, true}, {2, 8, 3, 2.5f, true}, {2, 8, 3, 3, false}, {2, 8, 3, 5, false},
            {4, 8, 3, 3, true}, {4, 8, 3, 7.5f, true}, {4, 8, 3, 8, false}, {4, 8, 3, 2, false},
            {0, -3, -1, -3, true}, {0, -3, -1, -1, false}, {255, 4, 2, 3, true},
        };
        select(player, "Loop");
        auto* ctrl = player._20;
        ctrl->setRate(0);
        for (const auto& item : cases) {
            SyncBckEffectChecker checker(&player);
            ctrl->setAttribute(static_cast<u8>(item.mode));
            ctrl->setFrame(item.previous); checker.updateAfter();
            ctrl->setFrame(item.current);
            require(checker.checkPassIfRate0(item.target) == item.expected, "literal stopped-frame boundary case");
            require(checker.checkPass(item.target) == item.expected, "zero rate dispatches actual stopped traversal");
        }
        SyncBckEffectChecker checker(&player);
        ctrl->setAttribute(0); ctrl->setFrame(2); checker.updateAfter();
        ctrl->setFrame(4); checker.updateBefore();
        require(checker._C == player.getCurrentBckName(), "zero-rate frame seek counts as animation movement");
        checker.updateAfter(); checker.updateBefore();
        require(checker._C == nullptr, "unchanged zero-rate frame has no active effect BCK");

        select(player, "Loop");
        ctrl = player._20; ctrl->setFrame(9); ctrl->setRate(2);
        require(checker.checkPass(9) && checker.checkPass(2) && !checker.checkPass(3),
                "nonzero forward wrap uses actual Xanime/J3D checkPass");
        ctrl->setAttribute(0); ctrl->setFrame(5); ctrl->setRate(-2);
        require(checker.checkPass(3) && checker.checkPass(4.5f) && !checker.checkPass(5),
                "nonzero reverse crossing uses original sampler interval");
    }

    void test_names_and_stop(XanimePlayer& player) {
        select(player, "Loop");
        SyncBckEffectInfo info(&player, "Loop", 1, 0, -1, false);
        SyncBckEffectChecker checker(&player);
        checker.updateBefore(); checker.updateAfter();
        const char* previous = checker._10;
        select(player, "Once"); checker.updateBefore();
        require(checker._C != previous && checker.isDelete(&info), "animation name transition retires previous effect");
        require(!checker.isCreate(&info, true) && !checker.isCreate(&info, false), "unregistered BCK creates neither kind");
        checker.updateAfter();
        require(!checker.isDelete(&info), "unchanged unregistered name does not repeat transition delete");

        select(player, "Loop"); checker.updateBefore(); checker.updateAfter();
        player._20->setRate(0); checker.updateBefore();
        require(checker._C == nullptr && checker.isDelete(&info), "stopped animation deletes once when continuation disabled");
        info.mContinueBckEnd = true;
        require(!checker.isDelete(&info), "continuation retains matching stopped BCK without authored end");
        require(!player.changeSimpleBck("Missing"), "real missing BCK lookup reports absence");
        checker.updateBefore();
        require(!checker.isDelete(&info), "continuation with no current BCK preserves original false result");

        select(player, "Once"); player._20->setEnd(1);
        player.updateAfterMovement(); checker.updateBefore();
        require(player.isTerminate() && checker._C == nullptr, "actual stopped frame state clears checker current name");
        require(checker.isDelete(&info), "unregistered terminated BCK permits continuation cleanup");
        player.calcAnm(0);

        // The original checker samples termination from the active bank even
        // when a distinct next frame controller is selected for a transition.
        auto* selected = player._20;
        const auto active = player._54;
        auto& pending = player._24[1 - active];
        pending.init(10); pending.setRate(1); pending.setFrame(3);
        player._20 = &pending;
        player._24[active].mState = 1; pending.mState = 0;
        checker.updateBefore(); require(checker._C == nullptr, "termination uses active bank, not pending controller");
        player._24[active].mState = 0; pending.mState = 1;
        checker.updateBefore(); require(checker._C == player.getCurrentBckName(), "rate/frame use selected controller independently");
        player._20 = selected;
        checker.updateAfter(); const char* name = checker._C; const char* last = checker._10;
        checker.reset();
        require(checker._8 && checker._4 == 0 && checker._C == name && checker._10 == last,
                "reset preserves name identities while clearing only frame history");
    }

    void run(GameResourceRuntime& process) {
        auto loop = transform(true); loop[0x28] = 2;
        auto once = transform(true); once[0x28] = 0;
        auto source = archive({{"Loop.bck", loop}, {"Once.bck", once}});
        auto domain = process.create_cohort();
        ResourceArchiveOwner owner(source, "Checker.arc", domain, process.mem1_heap());
        JkrAllocationScope scope(domain);
        Model model;
        XanimeResourceTable table(&owner.holder());
        XanimePlayer player(model.object.get(), &table);
        test_lifecycle(player); { JkrHostAllocationScope host; std::cout << "PASS original player phases, creation/deletion and reset correction\n"; }
        test_stopped_ranges(player); { JkrHostAllocationScope host; std::cout << "PASS 23 stopped-frame boundaries and original moving-frame dispatch\n"; }
        test_names_and_stop(player); { JkrHostAllocationScope host; std::cout << "PASS animation transitions, stop/continue behavior and bank identity\n"; }
    }
}
int main() {
    try {
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        GameResourceRuntime process;
        run(process);
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
