#include "layout/BrlytLayout.hpp"
#include "layout/LayoutRuntime.hpp"
#include "layout/Nw4rLayoutRecords.hpp"
#include "resource/RarcArchive.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Screen/SubMeterLayout.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "SceneExecutionFixture.hpp"
#include "layout/LayoutHost.hpp"
#include "runtime/RuntimeContext.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <nw4r/lyt/group.h>
#include <aurora/exception.hpp>
#include <aurora/dvd.h>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <unistd.h>

namespace {
using Bytes = std::vector<u8>;
void require(bool value, const char* message) { if (!value) aurora::throw_host_exception<std::runtime_error>(message); }
void put16(Bytes& b, size_t o, u16 v) { b.at(o) = v >> 8; b.at(o + 1) = v; }
void put32(Bytes& b, size_t o, u32 v) { put16(b, o, v >> 16); put16(b, o + 2, v); }
void putf(Bytes& b, size_t o, f32 v) { put32(b, o, std::bit_cast<u32>(v)); }
void name(Bytes& b, size_t o, std::string_view value) { std::copy(value.begin(), value.end(), b.begin() + o); }
Bytes block(std::string_view kind, size_t size) { Bytes b(size); name(b, 0, kind); put32(b, 4, size); return b; }
Bytes pane(std::string_view value, float x, float y) {
    auto b = block("pan1", 76); b[8] = 1; b[10] = 255; name(b, 12, value);
    putf(b, 36, x); putf(b, 40, y); putf(b, 60, 1); putf(b, 64, 1); putf(b, 68, 12); putf(b, 72, 14);
    return b;
}
Bytes group(std::string_view value, std::initializer_list<std::string_view> panes) {
    auto b = block("grp1", 28 + 16 * panes.size()); name(b, 8, value); put16(b, 24, panes.size());
    size_t o = 28; for (auto p : panes) { name(b, o, p); o += 16; } return b;
}
Bytes resource() {
    std::vector<Bytes> blocks{pane("Root", 10, 20), block("pas1", 8), pane("Child", 2, 3), pane("Sibling", 5, 7),
        block("pae1", 8), group("RootGroup", {"Root"}), block("grs1", 8),
        group("Ordered", {"Sibling", "Missing", "Child", "Sibling"}), block("grs1", 8),
        group("NestedIgnored", {"Child"}), block("gre1", 8), group("Second", {"Root"}), block("gre1", 8)};
    Bytes b(16); name(b, 0, "RLYT"); put16(b, 4, 0xFEFF); put16(b, 6, 8); put16(b, 12, 16); put16(b, 14, blocks.size());
    for (auto& block : blocks) b.insert(b.end(), block.begin(), block.end()); put32(b, 8, b.size()); return b;
}
Bytes archive(const Bytes& data) {
    // Two real RARC directory nodes: root -> blyt -> fixture.brlyt.
    constexpr size_t info = 0x20, dirs = 0x40, entries = 0x60, strings = 0x88, payload = 0xC0;
    Bytes b(payload + data.size()); name(b, 0, "RARC"); put32(b, 4, b.size()); put32(b, 8, 0x20);
    put32(b, 12, payload - 0x20); put32(b, 16, data.size());
    put32(b, info, 2); put32(b, info + 4, dirs - info); put32(b, info + 8, 2); put32(b, info + 12, entries - info);
    put32(b, info + 16, 24); put32(b, info + 20, strings - info);
    name(b, dirs, "ROOT"); put16(b, dirs + 10, 1); put32(b, dirs + 12, 0);
    name(b, dirs + 16, "BLYT"); put16(b, dirs + 26, 1); put32(b, dirs + 28, 1);
    name(b, strings, "blyt"); name(b, strings + 5, "fixture.brlyt");
    put16(b, entries, 0xFFFF); b[entries + 4] = 2; put32(b, entries + 8, 1); put32(b, entries + 12, 0x10);
    put16(b, entries + 20, 0); b[entries + 24] = 1; b[entries + 27] = 5; put32(b, entries + 32, data.size());
    std::copy(data.begin(), data.end(), b.begin() + payload); return b;
}
void host_heap_boundary(const std::filesystem::path& path) {
    auto heaps = smgpc::compat::JkrHeapRuntime::create(1U << 20);
    auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 64U << 10);
    const auto retired = std::weak_ptr(domain);
    auto archive_path = path;
    std::optional<smgpc::layout::LayoutRuntime> layout;
    std::vector<smgpc::layout::LayoutRuntime::DebugPaneState> published;
    {
        const smgpc::compat::JkrAllocationScope game(domain);
        const auto free_before = domain->heap().getFreeSize();
        layout.emplace("Native layout owner whose name exceeds string inline storage", "Fixture", 1, 0, std::move(archive_path));
        require(JKRHeap::findFromRoot(const_cast<char*>(layout->getName().data())) == nullptr,
                "layout constructor retains native strings outside the original caller heap");
        require(layout->hasPane("Child"), "native layout parser loads the real synthetic archive under a Game caller");
        layout->setPaneAlpha("Child", 0.5F);
        layout->setPaneVisibleRecursive("Root", true);
        layout->setPaneFollowPosition("Child", 2, TVec2f(4, 6));
        layout->setPaneRotation("Child", 10, 20, 30);
        published = layout->debugPanes();
        require(published.size() == 3 && JKRHeap::findFromRoot(published.data()) == nullptr,
                "native pane snapshots use host storage even when requested from original Game code");
        require(domain->heap().getFreeSize() == free_before,
                "native layout construction, parsing, metadata mutations and snapshots do not consume the caller Game heap");
        auto* original = new u8[16];
        require(JKRHeap::findFromRoot(original) == &domain->heap(),
                "layout host boundaries restore original caller allocation routing on return");
        delete[] original;
    }
    domain.reset();
    require(retired.expired(), "native layout metadata does not retain its caller Game heap");
    Mtx matrix;
    require(layout->copyPaneMatrix("Child", matrix) && published[1].name == "Child",
            "native layout state and returned snapshots survive caller Game heap retirement");
    layout->setPaneVisible("Sibling", false);
    require(!layout->isPaneVisible("Sibling"), "retained native pane maps remain mutable after caller retirement");
    layout.reset();
}
void records() {
    const auto path = std::filesystem::temp_directory_path() / ("smgpc-layout-group-" + std::to_string(getpid()) + ".arc");
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove(path); } } cleanup{path};
    const auto bytes = archive(resource());
    require(smgpc::resource::RarcArchive::from_bytes(bytes).contains("blyt/fixture.brlyt"), "synthetic archive uses actual RARC directory traversal");
    { std::ofstream out(path, std::ios::binary); out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); }
    for (int cycle = 0; cycle < 32; ++cycle) {
        smgpc::layout::LayoutRuntime runtime("Group fixture", "Fixture", 1, 0, path);
        smgpc::layout::Nw4rLayoutRecords records(runtime);
        auto* root = records.pane(nullptr); auto* child = records.pane("Child"); auto* sibling = records.pane("Sibling");
        require(records.pane_count() == 3 && root->mChildList.GetSize() == 2 && child->mpParent == root && sibling->mpParent == root,
                "actual intrusive Pane nodes preserve resource hierarchy and stable identity");
        const auto sibling_local = sibling->mMtx;
        const auto sibling_global = sibling->mGlbMtx;
        runtime.setPaneRotation("Sibling", 0, 0, 45);
        root->AnimateSelf(1);
        require(sibling->mRotate.z == 0,
                "AnimateSelf evaluates only its own pane, without publishing a sibling's pending animation properties");
        child->mTranslate.x = 9;
        sibling->AnimateSelf(1);
        require(sibling->mRotate.z == 45 && child->mTranslate.x == 9,
                "the next pane's local animation publishes without overwriting another pane's direct SDK edit");
        for (int row = 0; row < 3; ++row) for (int column = 0; column < 4; ++column) {
            require(sibling->mMtx.m[row][column] == sibling_local.m[row][column] &&
                    sibling->mGlbMtx.m[row][column] == sibling_global.m[row][column],
                    "AnimateSelf leaves local/global matrix propagation to the original later matrix phase");
        }
        records.synchronize();
        require(child->mTranslate.x == 9 && std::fabs(sibling->mMtx.m[0][0] - std::sqrt(0.5F)) < 0.0001F,
                "the matrix phase retains deferred direct edits and publishes the newly animated local transform");
        runtime.setPaneRotation("Sibling", 0, 0, 0);
        child->mTranslate.x = 2;
        records.synchronize();
        require(records.group_count() == 2 && records.group_index("Ordered") == 0 && records.group_index("Second") == 1 &&
                records.group_index("Missing") == 2 && !records.group("Missing") && !records.group("RootGroup") && !records.group("NestedIgnored"),
                "only immediate root groups exist; absent index is original one-past-end sentinel");
        auto& links = records.group("Ordered")->GetPaneList();
        require(links.GetSize() == 3, "absent group members are skipped and duplicate membership is retained");
        auto it = links.GetBeginIter(); require(it++->mTarget == sibling && it++->mTarget == child && it++->mTarget == sibling && it == links.GetEndIter(),
                "actual Group links preserve authored membership order including duplicates");
        require(!records.pane("Absent") && root->GetRuntimeTypeInfo() == &nw4r::lyt::Pane::typeInfo, "common pane has true SDK identity and no invented missing panes");
        int rejected = 0;
        try { root->SetName("Renamed"); } catch (const std::logic_error&) { ++rejected; }
        try { root->RemoveChild(child); } catch (const std::logic_error&) { ++rejected; }
        require(rejected == 2 && records.pane("Root") == root && child->mpParent == root,
                "unsupported published resource rename/reparent reject before detaching renderer identities");
        runtime.setScale(2, 3); runtime.setTrans(100, 200); records.synchronize();
        Mtx expected; require(runtime.copyPaneMatrix("Child", expected), "renderer exposes actual child transform");
        require(child->mGlbMtx.m[0][3] == expected[0][3] && child->mGlbMtx.m[1][3] == expected[1][3] && expected[0][3] == 114 && expected[1][3] == 229,
                "SDK global matrix includes the renderer's actor scale, root translation and parent composition");
        child->mTranslate.x = 8; child->mScale.x = 4; child->mRotate.z = 90; child->mAlpha = 127;
        records.synchronize(); require(runtime.copyPaneMatrix("Child", expected), "renderer still owns SDK-mutated child");
        require(std::fabs(expected[0][0]) < 0.0001F && std::fabs(expected[1][0] - 12) < 0.0001F && expected[0][3] == 126 && child->mAlpha == 127,
                "SDK writes update real rendered scale/rotation/translation and alpha");
        for (u32 alpha = 0; alpha <= 255; ++alpha) {
            child->mAlpha = static_cast<u8>(alpha);
            records.synchronize();
            require(child->mAlpha == alpha && child->mGlbAlpha == alpha,
                    "every SDK alpha byte reaches the shared renderer without normalized-float conversion");
        }
        runtime.setPaneRotation("Child", 0, 0, 45); records.synchronize();
        require(child->mRotate.z == 45, "host animation/property changes publish back to the same SDK pane");

        runtime.setScale(1, 1); runtime.setTrans(0, 0);
        root->mTranslate.z = 7; root->mRotate.y = 90;
        child->mTranslate.x = 2; child->mTranslate.y = 3; child->mTranslate.z = 5;
        child->mScale.x = child->mScale.y = 1;
        child->mRotate.x = 90; child->mRotate.y = child->mRotate.z = 0;
        records.synchronize();
        require(runtime.copyPaneMatrix("Child", expected), "3D child matrix remains available");
        const float composed[3][4] = {{0, 1, 0, 15}, {0, 0, -1, 23}, {-1, 0, 0, 5}};
        for (auto row = 0; row < 3; ++row) for (auto column = 0; column < 4; ++column) {
            require(std::fabs(expected[row][column] - composed[row][column]) < 0.0001F,
                    "parent Y rotation composes child X rotation and Z translation without flattening");
            require(expected[row][column] == child->mGlbMtx.m[row][column],
                    "SDK and renderer publish all twelve components of the same global matrix");
        }
        runtime.setPaneFollowPosition("Child", 2, TVec2f(4, 6)); records.synchronize();
        require(std::fabs(child->mGlbMtx.m[0][3] - 15) < 0.0001F &&
                std::fabs(child->mGlbMtx.m[1][3] - 26) < 0.0001F &&
                std::fabs(child->mGlbMtx.m[2][3] - 3) < 0.0001F,
                "local follow replacement uses the invertible 3D matrix even when its XY projection is singular");
        runtime.clearPaneFollowPositions();
        runtime.setPaneFollowPosition("Child", 3, TVec2f(4, 6)); records.synchronize();
        require(std::fabs(child->mGlbMtx.m[0][3] - 19) < 0.0001F &&
                std::fabs(child->mGlbMtx.m[1][3] - 23) < 0.0001F &&
                std::fabs(child->mGlbMtx.m[2][3] - 5) < 0.0001F,
                "original local-offset follow adds transformed XY while retaining global Z");
    }
    host_heap_boundary(path);
}

class Logger final : public smgpc::logging::ILogger {
    void write(std::FILE*, std::source_location, smgpc::logging::Level,
               smgpc::logging::Category, std::string_view) override {}
};
void fly_meter() {
    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    if (!disc) return;
    smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original 3D layout regression"});
    smgpc::render::AuroraRenderer renderer(window);
    require(aurora_dvd_open(disc), "real FlyMeter regression requires the supplied disc");
    struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
    DVDInit();
    smgpc::resource::GameResourceRuntime resources({96U << 20, 32U << 20, 4U << 20});
    Logger logger;
    smgpc::runtime::RuntimeContext runtime(logger, window, resources);
    smgpc::runtime::SceneSchedulerBinding scheduler_binding(runtime.scheduler());
    {
        smgpc::test::SceneExecutionFixture execution(runtime.scheduler(),
            smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 8U << 20));
        std::unique_ptr<SubMeterLayout> meter;
        {
            const smgpc::compat::JkrAllocationScope game(smgpc::scene::current_scene_allocation_domain());
            meter = std::make_unique<SubMeterLayout>("Original FlyMeter regression", "FlyMeter");
            meter->initWithoutIter();
        }
        auto* layout = smgpc::layout::layout_runtime(meter.get());
        require(layout && layout->getArchivePath() && meter->getLayoutManager()->getPane("Count"),
                "actual SubMeterLayout initializes its real FlyMeter archive and Count controller");
#ifndef NDEBUG
        auto three_dimensional = 0U;
        for (const auto& state : layout->debugPanes()) {
            const auto* pane = meter->getLayoutManager()->getPane(state.name.c_str());
            require(pane, "every actual FlyMeter pane has its SDK record");
            if (pane->mRotate.x != 0 || pane->mRotate.y != 0 || pane->mTranslate.z != 0) ++three_dimensional;
            for (const auto& row : pane->mGlbMtx.m) for (const auto value : row)
                require(std::isfinite(value), "actual FlyMeter 3D matrices remain finite");
        }
        require(three_dimensional > 0, "the actual archive exercises authored out-of-plane pane transforms");
#endif
        MR::showLayout(meter.get());
        MR::startAnim(meter.get(), "Wait", 0);
        for (const auto ratio : {1.0F, 0.5F, 0.125F}) {
            (void)renderer.begin_frame();
            {
                const smgpc::render::ScopedAuroraRendererContext context(renderer);
                const auto domain = smgpc::scene::current_scene_allocation_domain();
                const smgpc::compat::JkrAllocationScope game(domain);
                const auto free_before = domain->heap().getFreeSize();
                meter->setLifeRatio(ratio);
                meter->draw();
                require(domain->heap().getFreeSize() == free_before,
                        "original layout animation and first texture/text draw keep all native work outside its Game heap");
            }
            renderer.end_frame();
        }
    }
    std::cout << "Actual FlyMeter SubMeterLayout initialization and three rendered life ratios passed\n";
}
void unbound_panes() {
    nw4r::lyt::res::Pane resource{};
    resource.scale.x = resource.scale.y = 1; resource.alpha = 255;
    nw4r::lyt::Pane parent(&resource), child(&resource);
    parent.mbUserAllocated = child.mbUserAllocated = true;
    parent.SetName("Parent"); child.SetName("Child"); parent.AppendChild(&child);
    require(parent.FindPaneByName("Child", true) == &child && child.mpParent == &parent, "unbound real SDK panes retain original name/hierarchy operations");
    parent.RemoveChild(&child); require(!child.mpParent && !parent.mChildList.GetSize(), "unbound original SDK pane removal clears the real list and parent");
}
void tags() {
    require(MR::countMessageLine(L"") == 1 && MR::countMessageLine(L"a\nb\n") == 3, "original newline and empty-string counts");
    const wchar_t payload_newline[] = {L'a', 0x1a, 0x0800, 2, L'\n', L'b', L'\n', L'c', 0};
    require(MR::countMessageLine(payload_newline) == 2, "packed tag payload newline is skipped with actual big-endian UTF16-word semantics");
    const wchar_t page_break[] = {L'a', L'\n', 0x1a, 0x0601, 1, L'b', L'\n', L'c', 0};
    require(MR::countMessageLine(page_break) == 2, "group1/tag1 terminates this page's original line count");
    nw4r::ut::Color color(0x12345678); require(color.r == 0x12 && color.g == 0x34 && color.b == 0x56 && color.a == 0x78 && u32(color) == 0x12345678,
        "SDK packed RRGGBBAA colors preserve byte channels on little-endian hosts");
}
}
int main() {
    try { tags(); unbound_panes(); records(); fly_meter(); std::cout << "Original typed layout groups, transforms, tag lines and repeated teardown passed\n"; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
