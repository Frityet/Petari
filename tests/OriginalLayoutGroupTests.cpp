#include "layout/BrlytLayout.hpp"
#include "layout/LayoutRuntime.hpp"
#include "layout/Nw4rLayoutRecords.hpp"
#include "layout/LytTexMap.hpp"
#include "Game/Screen/LayoutCoreUtil.hpp"
#include "Game/Screen/CustomTagProcessor.hpp"
#include <nw4r/lyt/picture.h>
#include <nw4r/lyt/window.h>
#include <nw4r/lyt/bounding.h>
#include <nw4r/lyt/material.h>
#include "resource/RarcArchive.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Screen/SubMeterLayout.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/System/Language.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include <JSystem/JKernel/JKRArchive.hpp>
#include "OriginalStageResourceProcessFixture.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <nw4r/lyt/group.h>
#include <nw4r/lyt/animation.h>
#include <aurora/exception.hpp>
#include <aurora/dvd.h>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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

Bytes locale_resource() {
    std::vector<Bytes> blocks{
        pane("Root", 0, 0), block("pas1", 8),
        pane("Current", 0, 0), block("pas1", 8), pane("Choice", 1, 0),
        pane("ChoiceKrKo", 2, 0), block("pas1", 8), pane("Inside", 0, 0), pane("InsideCnSi", 0, 0), block("pae1", 8),
        pane("ChoiceCnSi", 3, 0), pane("Other", 4, 0), block("pae1", 8),
        pane("Fallback", 0, 0), block("pas1", 8), pane("Shared", 0, 0),
        block("pas1", 8), pane("DeepCnSi", 0, 0), block("pae1", 8),
        pane("SharedCnSi", 0, 0), pane("Aux", 0, 0), block("pae1", 8),
        pane("Recurse", 0, 0), block("pas1", 8), pane("NestedBlock", 0, 0), block("pas1", 8),
        pane("ValueKrKo", 0, 0), pane("ValueCnSi", 0, 0), block("pae1", 8), block("pae1", 8),
        pane("XYZ", 0, 0), pane("UnknownAbCd", 0, 0), block("pae1", 8),
        group("RootGroup", {"Root"}), block("grs1", 8),
        group("BeforeRemoval", {"Choice", "ChoiceKrKo", "ChoiceCnSi", "Other"}), block("gre1", 8)};
    Bytes b(16); name(b, 0, "RLYT"); put16(b, 4, 0xFEFF); put16(b, 6, 8); put16(b, 12, 16); put16(b, 14, blocks.size());
    for (auto& entry : blocks) b.insert(b.end(), entry.begin(), entry.end()); put32(b, 8, b.size());
    return b;
}
Bytes size_animation(std::string_view target, float first_width, float last_width, float first_height, float last_height) {
    auto b = block("pai1", 140); put16(b, 8, 10); put16(b, 14, 1); put32(b, 16, 20); put32(b, 20, 24);
    name(b, 24, target); b[44] = 1; put32(b, 48, 28);
    name(b, 52, "RLPA"); b[56] = 2; put32(b, 60, 16); put32(b, 64, 52);
    for (int channel = 0; channel < 2; ++channel) {
        const size_t offset = 68 + channel * 36;
        put16(b, offset, 8 + channel); b[offset + 2] = 2; put16(b, offset + 4, 2); put32(b, offset + 8, 12);
        const float first = channel ? first_height : first_width, last = channel ? last_height : last_width;
        putf(b, offset + 12, 0); putf(b, offset + 16, first); putf(b, offset + 20, (last - first) / 10);
        putf(b, offset + 24, 10); putf(b, offset + 28, last); putf(b, offset + 32, (last - first) / 10);
    }
    Bytes result(16); name(result, 0, "RLAN"); put16(result, 4, 0xFEFF); put16(result, 6, 10); put16(result, 12, 16); put16(result, 14, 1);
    result.insert(result.end(), b.begin(), b.end()); put32(result, 8, result.size()); return result;
}
Bytes animated_archive(const Bytes& layout, const std::vector<std::pair<std::string, Bytes>>& animations) {
    constexpr size_t info = 0x20, dirs = 0x40, entries = 0x70;
    const size_t count = 3 + animations.size(), strings = entries + count * 20;
    std::string names; const auto add_name = [&](std::string_view value) {
        const auto offset = names.size(); names.append(value); names.push_back(0); return offset;
    };
    const auto blyt_name = add_name("blyt"), anim_name = add_name("anim"), layout_name = add_name("fixture.brlyt");
    std::vector<size_t> animation_names; for (const auto& [value, data] : animations) animation_names.push_back(add_name(value + ".brlan"));
    const size_t payload = (strings + names.size() + 31) & ~size_t(31);
    size_t data_size = layout.size(); for (const auto& [value, data] : animations) data_size += data.size();
    Bytes b(payload + data_size); name(b, 0, "RARC"); put32(b, 4, b.size()); put32(b, 8, 0x20);
    put32(b, 12, payload - info); put32(b, 16, data_size);
    put32(b, info, 3); put32(b, info + 4, dirs - info); put32(b, info + 8, count); put32(b, info + 12, entries - info);
    put32(b, info + 16, names.size()); put32(b, info + 20, strings - info); name(b, strings, names);
    name(b, dirs, "ROOT"); put16(b, dirs + 10, 2);
    name(b, dirs + 16, "BLYT"); put16(b, dirs + 26, 1); put32(b, dirs + 28, 2);
    name(b, dirs + 32, "ANIM"); put16(b, dirs + 42, animations.size()); put32(b, dirs + 44, 3);
    for (int directory = 0; directory < 2; ++directory) {
        const auto offset = entries + directory * 20;
        put16(b, offset, 0xFFFF); b[offset + 4] = 2; put16(b, offset + 6, directory ? anim_name : blyt_name);
        put32(b, offset + 8, directory + 1); put32(b, offset + 12, 16);
    }
    size_t cursor = 0;
    const auto append_file = [&](size_t index, size_t string_offset, const Bytes& data) {
        const auto offset = entries + index * 20; b[offset + 4] = 1; put16(b, offset + 6, string_offset);
        put32(b, offset + 8, cursor); put32(b, offset + 12, data.size());
        std::copy(data.begin(), data.end(), b.begin() + payload + cursor); cursor += data.size();
    };
    append_file(2, layout_name, layout);
    for (size_t i = 0; i < animations.size(); ++i) append_file(3 + i, animation_names[i], animations[i].second);
    return b;
}




Bytes derived_resource() {
    auto material = block("mat1", 184);
    put16(material, 8, 1); put32(material, 12, 16); name(material, 16, "NativeMaterial");
    put16(material, 36, static_cast<u16>(-73)); put16(material, 38, 42);
    for (size_t i = 0; i < 16; ++i) material[60 + i] = static_cast<u8>(i + 17);
    put32(material, 76, (1U << 4) | (1U << 8) | (1U << 12) | (1U << 13) | (1U << 15) |
                         (1U << 18) | (1U << 23) | (1U << 24) | (1U << 25) | (1U << 27));
    for (size_t i = 0; i < 5; ++i) putf(material, 80 + i * 4, 0.25f + i);
    material[100] = GX_TG_MTX2x4; material[101] = GX_TG_TEX0; material[102] = GX_TEXMTX0;
    material[104] = material[105] = 1;
    put32(material, 108, 0x12345678); put32(material, 112, 0xE4E4E4E4);
    for (size_t i = 0; i < 5; ++i) putf(material, 116 + i * 4, -0.5f - i);
    for (size_t i = 0; i < 16; ++i) material[140 + i] = static_cast<u8>(i);
    material[156] = 0x77; material[158] = 31; material[159] = 63;
    material[160] = GX_BM_BLEND; material[161] = GX_BL_SRCALPHA; material[162] = GX_BL_INVSRCALPHA;
    material.resize(164); put32(material, 4, material.size());

    auto picture = pane("Picture", 0, 0); picture.resize(128); name(picture, 0, "pic1"); put32(picture, 4, picture.size());
    for (size_t i = 0; i < 4; ++i) put32(picture, 76 + i * 4, 0x10203040 + i);
    picture[94] = 1;
    for (size_t i = 0; i < 8; ++i) putf(picture, 96 + i * 4, i * 0.125f);
    auto window = pane("Window", 0, 0); window.resize(132); name(window, 0, "wnd1"); put32(window, 4, window.size());
    for (size_t i = 0; i < 4; ++i) putf(window, 76 + i * 4, i + 1);
    window[92] = 1; put32(window, 100, 124); put32(window, 124, 128); window[130] = 2;
    put32(window, 96, 104); for (size_t i = 0; i < 4; ++i) put32(window, 104 + i * 4, 0xFFEEDDCC);
    auto text = pane("TxtMessage", 0, 0); text.resize(130); name(text, 0, "txt1"); put32(text, 4, text.size());
    putf(text, 68, 96); putf(text, 72, 32); put16(text, 76, 128); put16(text, 78, 14); put32(text, 88, 116);
    put32(text, 92, 0x11223344); put32(text, 96, 0x55667788); putf(text, 100, 8); putf(text, 104, 16);
    const u16 tagged[] = {'A', 0x1a, 0x0800, 2, 0, 'B', 0};
    for (size_t i = 0; i < 7; ++i) put16(text, 116 + i * 2, tagged[i]);
    auto bounding = pane("Bounds", 0, 0); name(bounding, 0, "bnd1");
    std::vector<Bytes> blocks{material, pane("Root", 0, 0), block("pas1", 8), picture, window, text, bounding, block("pae1", 8)};
    Bytes b(16); name(b, 0, "RLYT"); put16(b, 4, 0xFEFF); put16(b, 6, 8); put16(b, 12, 16); put16(b, 14, blocks.size());
    for (auto& entry : blocks) b.insert(b.end(), entry.begin(), entry.end()); put32(b, 8, b.size());
    return b;
}
void materials_and_text(const std::filesystem::path& path) {
    for (int cycle = 0; cycle < 16; ++cycle) {
        smgpc::layout::LayoutRuntime runtime("Derived owner", "Fixture", 1, 0, path);
        auto& records = runtime.native_records();
        using namespace nw4r::lyt;
        auto* picture = nw4r::ut::DynamicCast<Picture*>(records.pane("Picture"));
        auto* window = nw4r::ut::DynamicCast<Window*>(records.pane("Window"));
        auto* box = nw4r::ut::DynamicCast<TextBox*>(records.pane("TxtMessage"));
        require(picture && window && box && nw4r::ut::DynamicCast<Bounding*>(records.pane("Bounds")),
                "all decoded pane kinds instantiate their actual SDK classes");
        auto* material = picture->GetMaterial();
        require(material && records.pane(nullptr)->FindMaterialByName("NativeMaterial", true) == material &&
                window->GetMaterial() != material && box->GetMaterial() != material &&
                window->GetFrameMaterial(0) != window->GetContentMaterial(),
                "original recursive lookup sees separate material instances owned by each SDK pane");
        require(material->GetTevColor(0).r == -73 && material->GetTevColor(0).g == 42 &&
                material->GetTexSRTCap() == 1 && material->GetTexSRTAry()[0].translate.x == 0.25f &&
                material->GetIndTexSRTAry()[0].scale.y == -4.5f && material->mTevKCols[0].r == 17 &&
                material->GetMatColAry()[0].a == 0x78,
                "original material constructor consumes signed colors, SRTs and byte channels in native order");
        require(picture->mTexCoordAry.GetSize() == 1 && picture->mTexCoordAry.GetArray()[0][3].y == 0.875f &&
                window->mContentInflation.l == 1 && window->mContentInflation.b == 4,
                "actual SDK geometry retains all authored texture coordinates and window inflation");
        require(box->mTextLen == 6 && box->mTextBuf[4] == 0 && box->mTextBuf[5] == 'B',
                "native text buffers retain embedded zero tag payload and subsequent authored units");
        LayoutCoreUtil::initTextBoxPane(box, nullptr, 64);
        require(box->mTextLen == 8 && std::wstring_view(box->mTextBuf, box->mTextLen) == L"XXXXXXXX",
                "native placeholder preserves original eight-X output when capacity clamps to nine");
        LayoutCoreUtil::setTextBoxMessage(box, L"AB\nCD");
        auto* processor = static_cast<CustomTagProcessor*>(box->mpTagProcessor);
        processor->initAlpha(0.8f, 0.9f, 0, 0);
        require(!processor->mAlphaCtrl.isEnd() && records.text_line_count("Root") == 2,
                "text measurement and reveal state use the current actual TextBox buffer");
        int frames = 0;
        while (!processor->mAlphaCtrl.isEnd() && frames < 32) { processor->mAlphaCtrl.update(); ++frames; }
        require(frames > 1 && frames < 32, "actual reveal controller completes only after frame progression");
        processor->initAlpha(0, 0, 0, 0);
        require(processor->mAlphaCtrl.isEnd(), "instant reveal uses the actual controller's disabled mode");

        std::weak_ptr<const HostTextureResourceState> retired;
        {
            Material owned;
            owned.ReserveGXMem(2, 0, 0, 0, false, 0, 0, false, false, false, false);
            owned.SetTextureNum(2);
            {
                TexMap texture;
                auto backing = std::make_shared<HostTextureResourceState>(); retired = backing;
                texture.SetHostResourceState(backing); owned.SetTexture(1, texture);
            }
            require(!retired.expired(), "actual material copy retains a texture's host backing");
            owned.SetTextureNum(1);
            require(retired.expired(), "shrinking actual material texture count releases removed backing");
            {
                TexMap texture; auto backing = std::make_shared<HostTextureResourceState>(); retired = backing;
                texture.SetHostResourceState(backing); owned.SetTexture(0, texture);
            }
            owned.ReserveGXMem(3, 0, 0, 0, false, 0, 0, false, false, false, false);
            require(retired.expired(), "actual material storage reallocation releases prior texture backing");
            owned.SetTextureNum(1);
            {
                TexMap texture; auto backing = std::make_shared<HostTextureResourceState>(); retired = backing;
                texture.SetHostResourceState(backing); owned.SetTexture(0, texture);
            }
        }
        require(retired.expired(), "actual material destruction releases its final texture backing");
    }
}
void host_heap_boundary(const std::filesystem::path& path) {
    auto domain = smgpc::compat::JkrAllocationDomain::create(smgpc::scene::current_scene_allocation_domain(), 64U << 10);
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
        auto& records = runtime.native_records();
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
    const auto derived = archive(derived_resource());
    { std::ofstream out(path, std::ios::binary); out.write(reinterpret_cast<const char*>(derived.data()), derived.size()); }
    materials_and_text(path);
}

void fly_meter() {
    // The observer runs inside the actual Aurora frame after original scene initialization.
    {
        std::unique_ptr<SubMeterLayout> meter;
        {
            const smgpc::compat::JkrAllocationScope game(smgpc::scene::current_scene_allocation_domain());
            meter = std::make_unique<SubMeterLayout>("Original FlyMeter regression", "FlyMeter");
            meter->initWithoutIter();
        }
        auto* manager = meter->getLayoutManager();
        char archive_path[256];
        require(MR::makeLayoutArchiveFileNameFromPrefix(archive_path, sizeof(archive_path), "FlyMeter", false),
                "actual FlyMeter layout archive resolves through the original language and aspect selection");
        require(manager->mLayoutHolder && manager->mLayoutHolder->mArchive == MR::receiveArchive(archive_path) &&
                    manager->mLayoutHolder->mLayoutRes.isExistRes("FlyMeter.brlyt") &&
                    manager->mLayoutHolder->mAnimRes.isExistRes("Count.brlan") &&
                    manager->getPane("Count") && manager->getPaneCtrl("Count"),
                "actual SubMeterLayout borrows its mounted FlyMeter archive and authored Count animation/controller");

        MR::showLayout(meter.get());
        MR::startAnim(meter.get(), "Wait", 0);
        for (const auto ratio : {1.0F, 0.5F, 0.125F}) {
            {
                const auto domain = smgpc::scene::current_scene_allocation_domain();
                const smgpc::compat::JkrAllocationScope game(domain);
                const auto free_before = domain->heap().getFreeSize();
                meter->setLifeRatio(ratio);
                require(MR::getPaneAnimFrame(meter.get(), "Count", 0) == 128.0F * (1.0F - ratio) &&
                            MR::isPaneAnimStopped(meter.get(), "Count", 0),
                        "original life ratio selects and stops the authored Count animation frame");
                meter->draw();
                require(domain->heap().getFreeSize() == free_before,
                        "original layout animation and first texture/text draw keep all native work outside its Game heap");
            }
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
    require(MR::countMessageChar(nullptr) == 0 && MR::countMessageChar(L"") == 0 &&
                MR::countMessageChar(L"a\nb") == 3,
            "original reveal length counts ordinary newlines and accepts a null message");
    const wchar_t counted_tags[] = {L'a', 0x1a, 0x0603, 0, 0x1a, 0x0605, 0,
                                   0x1a, 0x060b, 0, L'\n', 0x1a, 0x0601, 1, L'z', 0};
    require(MR::countMessageChar(counted_tags) == 8,
            "original reveal length counts picture, player-name and race tags and stops at the page tag");
    wchar_t number_tag[] = {0x1a, 0x0a06, 0, 0, 0, 0};
    for (s32 number : {0, 9, 10, -10, 99999, std::numeric_limits<s32>::max(), std::numeric_limits<s32>::min()}) {
        const auto bits = static_cast<u32>(number);
        number_tag[3] = bits >> 16;
        number_tag[4] = bits & 0xffff;
        const auto expected = number == 0 || number == 9 ? 1 : number == 10 || number == -10 ? 2 : number == 99999 ? 5 : 10;
        require(MR::countMessageFigure(number) == expected && MR::countMessageChar(number_tag) == expected,
                "number tag length uses signed 32-bit magnitude including INT32_MIN, without counting a minus sign");
    }
    wchar_t string_tag[] = {0x1a, 0x0a07, 0, 0, 0, 0};
    const wchar_t* nested = counted_tags;
    std::memcpy(string_tag + 3, &nested, sizeof(nested));
    require(MR::countMessageChar(string_tag) == 8,
            "native string parameters retain a full-width borrowed pointer and recursively count its original tags");
    nested = nullptr;
    std::memcpy(string_tag + 3, &nested, sizeof(nested));
    require(MR::countMessageChar(string_tag) == 0, "a null string substitution adds no reveal characters");
    require(MR::getLanguageNum() == 12 && std::strcmp(MR::getLanguagePrefixByIndex(0), "JpJapanese") == 0 &&
                std::strcmp(MR::getLanguagePrefixByIndex(11), "KrKorean") == 0,
            "the original regional language table remains available without fabricating a current process language");
    require(MR::countMessageLine(L"") == 1 && MR::countMessageLine(L"a\nb\n") == 3, "original newline and empty-string counts");
    const wchar_t payload_newline[] = {L'a', 0x1a, 0x0800, 2, L'\n', L'b', L'\n', L'c', 0};
    require(MR::countMessageLine(payload_newline) == 2, "packed tag payload newline is skipped with actual big-endian UTF16-word semantics");
    const wchar_t page_break[] = {L'a', L'\n', 0x1a, 0x0601, 1, L'b', L'\n', L'c', 0};
    require(MR::countMessageLine(page_break) == 2, "group1/tag1 terminates this page's original line count");
    require(MR::getNextMessagePage(L"") == nullptr && MR::getNextMessagePage(L"a\nb\n") == nullptr &&
                MR::getNextMessagePage(payload_newline) == nullptr,
            "only a page-break tag advances pages; ordinary newlines and tag payload words do not");
    require(MR::getNextMessagePage(page_break) == page_break + 5 &&
                MR::getNextMessagePage(page_break + 5) == nullptr,
            "page lookup returns the original borrowed next-page pointer and stops at the actual string end");
    const wchar_t pages[] = {L'a', 0x1a, 0x0a00, 2, 0, L'\n', 0x1a, 0x0601, 1,
                            L'\n', L'\n', L'b', 0x1a, 0x0601, 1, L'c', 0};
    require(MR::getNextMessagePage(pages) == pages + 10 &&
                MR::getNextMessagePage(pages + 10) == pages + 15 &&
                MR::getNextMessagePage(pages + 15) == nullptr,
            "page scan skips embedded zero/newline tag payloads, skips exactly one optional newline and supports repeated pages");
    const wchar_t empty_last_page[] = {L'a', 0x1a, 0x0601, 1, L'\n', 0};
    require(MR::getNextMessagePage(empty_last_page) == empty_last_page + 5 &&
                MR::getNextMessagePage(empty_last_page + 5) == nullptr,
            "a final page-break returns the empty last-page pointer before a subsequent scan returns null");
    require(MR::getStringLengthWithMessageTag(L"") == 0 && MR::getStringLengthWithMessageTag(payload_newline) == 8 &&
                MR::getStringLengthWithMessageTag(page_break) == 2 && MR::getStringLengthWithMessageTag(pages) == 6,
            "original string length counts encoded tag words, skips embedded terminators and stops before next-page tags");
    wchar_t picture[] = {0, 0, 0, 0, 0x7777};
    require(MR::addPictureFontTag(picture, 'B') == picture + 3 && picture[4] == 0x7777 &&
                MR::getStringLengthWithMessageTag(picture) == 3 && MR::countMessageLine(picture) == 1 &&
                MR::getNextMessagePage(picture) == nullptr && !MR::isMessageEditorNextTag(picture),
            "original picture tags produce retained UTF16 words consumable by the shared message parser");
    require(MR::addPictureFontTag(picture, '0' + 0x10001) == picture + 3 && picture[2] == 1 && picture[4] == 0x7777 &&
                MR::addPictureFontCode(picture, 0x10002) == picture + 1 && picture[0] == 2 && picture[1] == 0,
            "picture tags and literal glyph codes preserve original 16-bit stores on wider native wchar_t hosts");
    const wchar_t wide[] = {L'A', 0xE9, 0x100, L'B', 0};
    char narrow[] = {0, 0, 0, 0x77};
    require(MR::convertUTF16ToASCII(narrow, wide, 4) == 2 && narrow[0] == 'A' &&
                static_cast<unsigned char>(narrow[1]) == 0xE9 && narrow[2] == 0 && narrow[3] == 0x77,
            "original wide-to-byte conversion retains low-byte characters and terminates before a nonzero high byte");
    const char slashless[] = "Message.arc";
    const char path[] = "/MessageData/Message.arc";
    require(MR::getBasename(slashless) == slashless && MR::getBasename(path) == path + 13,
            "original basename returns the input without a slash and the final component otherwise");
    nw4r::ut::Color color(0x12345678); require(color.r == 0x12 && color.g == 0x34 && color.b == 0x56 && color.a == 0x78 && u32(color) == 0x12345678,
        "SDK packed RRGGBBAA colors preserve byte channels on little-endian hosts");
}
}
int main() {
    return smgpc::test::run_stage_resource_process("original-layout-groups", [&] {
        tags(); unbound_panes(); records(); fly_meter();
        std::cout << "Original typed layout groups, transforms, tag lines and FlyMeter checks passed\n";
    });
}
